#include "lazydaw.hpp"
#include "computationgraph.hpp"

#include <cassert>
#include <cstring>
#include <cstddef>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <numeric>

int main() {
    using namespace LazyDAW;
    using namespace std::string_literals;

    ComputationGraph<AudioRepresentation> g;
    
    g.add_interior_node();
    g.add_interior_node();
    g.add_interior_node();

    g.peek_inner(0).set(1,1, [](auto const &input, auto &output) -> std::vector<error> {
        auto const & optional_input = *input[0].maybe_value;

        if(optional_input.has_value())
            output[0].value = AudioRepresentation(std::move(NaiveDiscreteFourierTransform(*optional_input.template get<AudioSample>())));
        return {};
    });
    g.peek_inner(1).set(1,1, [](auto const &input, auto &output) -> std::vector<error> {
        constexpr auto cutoff_freq = 10000.;

        size_t i = 0;

        auto const * input_samples = input[0].maybe_value->template get<FourierCoefficients>();

        output[0].value = FourierCoefficients();

        auto  * output_samples = output[0].value.template get<FourierCoefficients>();

        output_samples->zero_out(input_samples->size());

        while(i < std::min(cutoff_freq, static_cast<double>(input_samples->size()))) {
            output_samples->data()[i] = 0;
            ++i;
        }

        while(i < std::min(input_samples->size(), output_samples->size())) {
            output_samples->data()[i] = static_cast<int16_t>((*(input_samples->data() +i)).real());
            ++i;
        }
        
        return {};
    });
    g.peek_inner(2).set(1,1,[](auto const &input, auto & output) -> std::vector<error> {
        auto const & optional_payload = *input[0].maybe_value;

        if(optional_payload.has_value())
            output[0].value = AudioRepresentation(std::move(NaiveDiscreteInverseFourierTransform(*optional_payload.template get<FourierCoefficients>())));
        else
            throw std::runtime_error("No value");
        return {};
    });

    g.link_node({&(g.peek_source()), 0}, {&g.peek_inner(0), 0});
    g.link_node({&g.peek_inner(0), 0},{&g.peek_inner(1), 0});
    g.link_node({&g.peek_inner(1), 0},{&g.peek_inner(2), 0});
    g.link_node({&g.peek_inner(2), 0}, {&(g.peek_sink()), 0});

    std::ifstream input("input.wav", std::ios::binary | std::ios::in);

    std::vector<std::byte> raw_input(44,std::byte(0));

    input.read(reinterpret_cast<char*>(raw_input.data()),44);

    std::vector<std::byte> raw_size = {std::byte(0), std::byte(0), std::byte(0), std::byte(0)};

    size_t i = 0;
    for(; i < 4; ++i) {
            raw_size[i]=raw_input.data()[i+4];
    }

    int32_t * truncated_size = reinterpret_cast<int32_t *>(raw_size.data());

    std::cout << "Expect wav size of " << *truncated_size << std::endl;

    raw_input.resize(std::max(*truncated_size,45));
    input.read(reinterpret_cast<char *>(&raw_input[44]),std::max(45, ((*truncated_size)-44)));
    
    AudioRepresentation p;
    AudioSample wav;

    wav.zero_out(*truncated_size-44);

    std::memcpy(&*wav.begin(),
                &raw_input[44],
               *truncated_size-44);

    input.close();

    p = { std::move(wav) };

    std::cout << p.template get<AudioSample>()->size() << std::endl;

    auto raw_output = g.compute(std::move(p));

    auto const &errors = raw_output.errors;

    if(!errors.empty()) {
    for(auto err : errors)
        std::cout << err.message << "\n";
        return EXIT_FAILURE;
    }

    auto & real_output = raw_output.result.template get<AudioSample>()->discrete_amplitudes;

    std::ofstream output("output.wav", std::ios::binary | std::ios::out);

    output.write(reinterpret_cast<char const *>(&raw_input[0]),44);

    output.write(reinterpret_cast<char*>(real_output.data()), real_output.size());

    std::cout << "Success." << std::endl;

    return EXIT_SUCCESS;
}
