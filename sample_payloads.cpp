auto test1 = [](auto const &input, auto &output) -> std::vector<error> {
        std::vector<error> errors;

        for(auto i = 0; i < output.size(); ++i) {
            auto input_amplitudes = *input[0].maybe_value->template get<AudioSample>();

            AudioSample output_amplitudes;
            output_amplitudes.discrete_amplitudes.reserve(input_amplitudes.size());
                
            for(auto amplitude : input_amplitudes) {
                output_amplitudes.discrete_amplitudes.push_back((i+2)*amplitude);
            }
            
            output[i].value = {std::move(output_amplitudes)};
        }

        return errors;
    };


auto test2 =    [](auto const &input, auto &output){
                std::vector<error> errors;

                assert(input[0].maybe_value->template get<AudioSample>()->size() == input[1].maybe_value->template get<AudioSample>()->size());

                size_t length = input[0].maybe_value->template get<AudioSample>()->size();

                AudioSample output_amplitudes;
                output_amplitudes.zero_out(length);
                
                for(auto i = 0; i < input.size(); ++i) {
                    auto input_samples = input[i].maybe_value->template get<AudioSample>();
    
                    auto const & input_amplitudes = input_samples->discrete_amplitudes;
                
                    // output_amplitudes += input_amplitudes[j];

                    for(auto j = 0; j < length; ++j) {
                        output_amplitudes[j]+=input_amplitudes[j];
                    }
                }
                output[0].value = {std::move(output_amplitudes)};

                return errors;
    };