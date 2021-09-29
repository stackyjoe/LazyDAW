#include "lazydaw.hpp"
#include "computationgraph.hpp"


#include <cassert>
#include <iostream>

int main() {
    using namespace LazyDAW;
    using namespace std::string_literals;

    ComputationGraph<AudioRepresentation> g;

    AudioSample a;

    a.audio.reserve(16);

    for(auto i = 0; i < 16; ++i)
        a.audio.push_back(i);
    
    g.add_interior_node();
    g.add_interior_node();

    auto & first_node = g.peek_inner(0);
    auto & second_node = g.peek_inner(1);

    first_node.set(1,2,[](auto const &input, auto &output) -> std::vector<error> {
        std::vector<error> errors;

        // Given the previous assert we actually know the size here, but I wrote up a
        //     'fully general' example to see what one actually has to do to set up a 
        //     node with the current API. Clearly it must be made simpler for the user.
        for(auto i = 0; i < output.size(); ++i) {
            auto input_payload = input[0].maybe_value->data.value();

            auto visitor = [&errors, i](auto&& arg) mutable -> AudioRepresentation {

                using namespace LazyDAW;
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, AudioSample>) {

                    auto input_amplitudes = arg.audio;

                    AudioSample output_amplitudes;
                    output_amplitudes.audio.reserve(input_amplitudes.size());
                
                    for(auto amplitude : input_amplitudes) {
                        output_amplitudes.audio.push_back((i+2)*amplitude);
                    }

                    return {output_amplitudes};
                }
                else if constexpr (std::is_same_v<T, FourierCoefficients>) {
                    errors.push_back("Memory got mashed but it didn't cause a Segmentation Fault."s);
                    return {std::nullopt};
                }
                else 
                    errors.push_back("Bad visitor access."s);
                    return {std::nullopt};
            }; // end of capturing lambda named visitor
            
            auto output_payload = std::visit(visitor, input_payload);

            output[i].value = std::move(output_payload);
        }

        return errors;
    });

    second_node.set(2,1, [](auto const &input, auto &output){
                std::vector<error> errors;

                using namespace LazyDAW;

                assert(input[0].maybe_value->template get<AudioSample>()->audio.size() == input[1].maybe_value->template get<AudioSample>()->audio.size());

                size_t length = input[0].maybe_value->template get<AudioSample>()->audio.size();

                AudioSample output_amplitudes;
                output_amplitudes.audio.reserve(length);

                for(auto j = 0; j < length; ++j)
                    output_amplitudes.audio.push_back(0);
                
                for(auto i = 0; i < input.size(); ++i) {
                    auto input_payload = input[i].maybe_value->template get<AudioSample>();
    
                    auto const & input_amplitudes = input_payload->audio;
                
                    for(auto j = 0; j < length; ++j) {
                        output_amplitudes.audio[j]+=input_amplitudes[j];
                    }

                }
                output[0].value = {std::move(output_amplitudes)};

                return errors;
    });


    g.link_node({&(g.peek_source()), 0}, {&first_node, 0});
    g.link_node({&first_node, 0},{&second_node, 0});
    g.link_node({&first_node, 1},{&second_node, 1});
    g.link_node({&second_node, 0}, {&(g.peek_sink()), 0});

    AudioRepresentation p = {a};

    auto output = g.compute(p);

    auto errors = output.errors;

    for(auto err : errors)
        std::cout << err.message << "\n";

    auto real_output = output.result.get<AudioSample>()->audio;

    for(auto amplitude : real_output) {
        std::cout << amplitude << " ";
    }
    
    std::cout << std::endl;


    return EXIT_SUCCESS;
}
