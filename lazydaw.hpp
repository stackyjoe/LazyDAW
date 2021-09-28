#pragma once

#include <cassert>

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>


#include <iostream>

namespace LazyDAW{

    class ComputationGraph;
    class VertexWithEdgeData;
    
    class VertexPayload;

    struct AudioSample {
        std::vector<int16_t> audio;
    };

    struct FourierCoefficients {

    };

    struct VertexPayload {
        std::optional<std::variant<AudioSample,FourierCoefficients>> data;

        template <class T>
        T * get() noexcept {
            if(data.has_value() && std::holds_alternative<T>(data.value()))
                return std::addressof(std::get<T>(data.value()));
            return nullptr;
        }
    };

    struct error {
        error(std::string message) : message(message) { }
        std::string message;
    };

    struct input_index {
		VertexWithEdgeData * vertex;
		size_t slot;
	};

	struct output_index {
	    VertexWithEdgeData * vertex;
		size_t slot;
	};

	struct linked_input {
	    VertexPayload const * maybe_value;
	};

    struct linked_output {
        mutable VertexPayload value;
        input_index target_node;
    };

    struct VertexWithEdgeData {

        using node_t = VertexWithEdgeData;
        
	    using payload_t = VertexPayload;
        


        std::vector<linked_input> input_slots;
        mutable std::vector<linked_output> output_slots;

        using computation_t = std::vector<error>(decltype(input_slots) const &, decltype(output_slots) & );

        mutable std::optional<std::function<computation_t>> maybe_function;

        public:
        VertexWithEdgeData() {
            input_slots.push_back(linked_input());
            output_slots.push_back(linked_output());

            maybe_function.emplace([](auto const & inputs, auto & outputs) -> std::vector<error> {
                assert(outputs.size() == inputs.size());

                for(auto i = 0; i < inputs.size(); ++i)
                    outputs[i].value = *(inputs[i].maybe_value);

                return {};
                });
        }

        VertexWithEdgeData(size_t input_node_count, size_t output_node_count, std::function<computation_t> function)
            : input_slots(),
            output_slots(),
           maybe_function(function) {
            input_slots.reserve(input_node_count);
            output_slots.reserve(output_node_count);

            for(auto i = 0; i < input_node_count; ++i)
                input_slots.push_back(linked_input());

            for(auto i = 0; i < output_node_count; ++i)
                output_slots.push_back(linked_output());
        }

        VertexWithEdgeData(VertexWithEdgeData const &) = delete;
        VertexWithEdgeData &operator=(VertexWithEdgeData const &) = delete;

        VertexWithEdgeData(VertexWithEdgeData &&) = default;
        VertexWithEdgeData &operator=(VertexWithEdgeData &&) = default;

        linked_output & peek_output(size_t i) {
            return output_slots.at(i);
        }

        linked_input & peek_input(size_t i) {
            return input_slots.at(i);
        }

	    bool is_ready_to_compute() const noexcept {
            bool is_ready = true;
		    
            for(auto const &vertex : input_slots) {
                is_ready &= vertex.maybe_value->data.has_value();
            }

            is_ready &= maybe_function.has_value();

            return is_ready;
	    }

        void set(size_t inputs, size_t outputs, std::function<computation_t> &&function) {
            input_slots.clear();
            output_slots.clear();
            maybe_function.emplace(function);

            input_slots.reserve(inputs);
            output_slots.reserve(outputs);

            for(auto i = 0; i < inputs; ++i)
                input_slots.push_back(linked_input());
            for(auto i = 0; i < outputs; ++i)
                output_slots.push_back(linked_output());
        }

        error compute() const noexcept {
            using namespace std::string_literals;
            if(!is_ready_to_compute())
                return "Computation::VertexWithEdgeData::compute() called while vertex was not ready to compute."s;
            else {
                auto errs = (*maybe_function)(input_slots, output_slots);
                return ""s;
            }
        }

    };

    class ComputationGraph {
        using node_t = VertexWithEdgeData;

        std::vector<node_t> interior_nodes;

        mutable node_t source;
        mutable node_t sink;

        auto compute_graph() const noexcept {
            std::set<node_t const *> this_depth;
            std::set<node_t const *> next_depth;
            std::vector<error> errors;

            this_depth.insert(&source);

            std::vector<node_t *> children;

            while((this_depth.find(&sink) == this_depth.end()) && !this_depth.empty()) {
                
                for( node_t const * node : this_depth ) {
                    auto maybe_err = node->compute();

                    if(!maybe_err.message.empty())
                        errors.push_back(std::move(maybe_err));

                    for(auto & slot : node->output_slots) {
                        using namespace std::string_literals;
           
                        if(slot.target_node.vertex == nullptr)
                            errors.push_back("A node passed a linked_output with a null pointer."s);
                        else
                            children.push_back(slot.target_node.vertex);
                    }
                }

                for(auto node_that_received_input : children) {
                    if(node_that_received_input->is_ready_to_compute())
                        next_depth.insert(node_that_received_input);
                }

                std::swap(this_depth, next_depth);
                children.clear();
                next_depth.clear();
            }

            if(sink.is_ready_to_compute()) {
                auto maybe_err = sink.compute();
                if(!maybe_err.message.empty())
                    errors.push_back(std::move(maybe_err));
            }
            else {
                using namespace std::string_literals;

                errors.push_back("No nodes, including sink node, are ready to compute."s);
            }

            return errors;
        }

        public:
        ComputationGraph() = default;

        node_t & peek_source() noexcept { return source;}
        node_t & peek_sink() noexcept { return sink; }
        
        node_t * peek_inner(size_t i) noexcept {
            return i > interior_nodes.size() ? nullptr : std::addressof(interior_nodes[i]);
        }

        std::vector<node_t>& peek_interior() noexcept { return interior_nodes; }


        struct computation_result {
            std::vector<error> errors;
            VertexPayload result;
        };

        size_t add_interior_node() {
            auto index = interior_nodes.size();

            interior_nodes.push_back(node_t());

            return index;
        }

        linked_input & to_slot(input_index i) {
            return i.vertex->input_slots.at(i.slot);
        }

        linked_output & to_slot(output_index o) {
            return o.vertex->output_slots.at(o.slot);

        }

        computation_result compute(VertexPayload const & input) const noexcept {
            source.input_slots[0].maybe_value = std::addressof(input);
            
            auto errors = compute_graph();
            auto optional_value = sink.output_slots[0].value;

            return {std::move(errors), std::move(optional_value)};
        }

        bool link_node(output_index output, input_index input) noexcept {
            try {
                // Set the input's pointer to the optional VertexPayload
    	        to_slot(input).maybe_value = &(to_slot(output).value);

                // Set the target node of the slot in the output node to the input node to be linked.
                to_slot(output).target_node = input;

                return true;
            }
            catch(std::exception const & e) {
                return false;
            }
        }
    };
}
