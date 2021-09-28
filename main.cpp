#include "lazydaw.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace LazyDAW;
    using namespace std::string_literals;

    ComputationGraph g;

    AudioSample a;

    a.audio.reserve(16);

    for(auto i = 0; i < 16; ++i)
        a.audio.push_back(i+8);
    
    g.add_interior_node();

    auto interior_node = g.peek_inner(0);

    interior_node->set(1,1,[](auto const &input, auto &output) -> std::vector<error> {
        assert(input.size() == output.size() == 1);

        std::vector<error> errors;

        for(auto i = 0; i < input.size(); ++i) {
            auto input_payload = input[i].maybe_value->data.value();

            auto visitor = [errors](auto&& arg) mutable -> VertexPayload {

                using namespace LazyDAW;
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, AudioSample>) {

                    auto input_amplitudes = arg.audio;

                    AudioSample output_amplitudes;
                    output_amplitudes.audio.reserve(input_amplitudes.size());
                
                    for(auto amplitude : input_amplitudes) {
                        output_amplitudes.audio.push_back(2*amplitude);
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

    g.link_node({&(g.peek_source()), 0}, {&interior_node, 0});
    g.link_node({&interior_node, 0}, {&(g.peek_sink()), 0});


    VertexPayload p = {a};

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
