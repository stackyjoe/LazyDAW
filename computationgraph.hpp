#pragma once

#include <cassert>

#include <concepts>
#include <functional>
#include <optional>
//#include <set>
#include <string>
#include <utility>
#include <vector>

#include "set.hpp"


namespace LazyDAW {
    namespace {
        auto lambda = []() mutable -> void {};
    }   

    template<class T>
    concept OptionalVariantLike = requires(T t) {
        {t.has_value()} -> std::same_as<bool>;

        // I want to demand that the template parameter for the concept has a template member function get,
        // which takes a template parameter S, no arguments, and returns a pointer to S. This is not possible
        // with concepts, and since virtual template member functions are not part of the language, it cannot
        // be done with inheritance either. This is a crude hack requiring that the template get function exists
        // for a type that the user cannot name. I suspect it is a weaker requirement, but also that one needs
        // something like "adversarial input" to get a false positive.
        { t.template get<decltype(lambda)>()} -> std::same_as<decltype(lambda)*>;
    };

    template<OptionalVariantLike Payload>
    class ComputationGraph;

    template<OptionalVariantLike Payload>
    class VertexWithEdgeData;


    struct error {
        error(std::string message) : message(message) { }
        std::string message;
    };

    template<class Payload>
    struct input_index {
		VertexWithEdgeData<Payload> * vertex;
		size_t slot;
	};

    template<class Payload>
	struct output_index {
	    VertexWithEdgeData<Payload> * vertex;
		size_t slot;
	};

    template<class Payload>
	struct linked_input {
	    Payload const * maybe_value;
	};

    template<class Payload>
    struct linked_output {
        mutable Payload value;
        input_index<Payload> target_node;
    };

    template<OptionalVariantLike Payload>
    struct VertexWithEdgeData {

        using node_t = VertexWithEdgeData;

        std::vector<linked_input<Payload>> input_slots;
        mutable std::vector<linked_output<Payload>> output_slots;

        using computation_t = std::vector<error>(decltype(input_slots) const &, decltype(output_slots) & );

        mutable std::optional<std::function<computation_t>> maybe_function;

        public:
        VertexWithEdgeData() {
            input_slots.push_back(linked_input<Payload>());
            output_slots.push_back(linked_output<Payload>());

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
                input_slots.push_back(linked_input<Payload>());

            for(auto i = 0; i < output_node_count; ++i)
                output_slots.push_back(linked_output<Payload>());
        }

        VertexWithEdgeData(VertexWithEdgeData const &) = delete;
        VertexWithEdgeData &operator=(VertexWithEdgeData const &) = delete;

        VertexWithEdgeData(VertexWithEdgeData &&) = default;
        VertexWithEdgeData &operator=(VertexWithEdgeData &&) = default;

        linked_output<Payload> & peek_output(size_t i) {
            return output_slots.at(i);
        }

        linked_input<Payload> & peek_input(size_t i) {
            return input_slots.at(i);
        }

	    bool is_ready_to_compute() const noexcept {
            bool is_ready = true;
		    
            for(auto const &vertex : input_slots) {
                is_ready &= vertex.maybe_value->has_value();
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
                input_slots.push_back(linked_input<Payload>());
            for(auto i = 0; i < outputs; ++i)
                output_slots.push_back(linked_output<Payload>());
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

    template<OptionalVariantLike Payload>
    class ComputationGraph {
        using node_t = VertexWithEdgeData<Payload>;

        std::vector<node_t> interior_nodes;

        mutable node_t source;
        mutable node_t sink;

        auto compute_graph() const noexcept {
            jl::containers::set<node_t const *> this_depth;
            jl::containers::set<node_t const *> next_depth;
            std::vector<error> errors;

            this_depth.insert(&source);

            std::vector<node_t const *> children;

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

                std::sort(children.begin(), children.end());
                std::unique(children.begin(), children.end());
                std::erase_if(children, [](auto node_that_received_input){return !node_that_received_input->is_ready_to_compute();});
                next_depth.insert_ordered_unique(std::move(children));

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
        
        node_t & peek_inner(size_t i) {
            return interior_nodes.at(i);
        }

        std::vector<node_t> & peek_interior() noexcept { return interior_nodes; }


        struct computation_result {
            std::vector<error> errors;
            Payload result;
        };

        size_t add_interior_node() {
            auto index = interior_nodes.size();

            interior_nodes.push_back(node_t());

            return index;
        }

        linked_input<Payload> & to_slot(input_index<Payload> i) {
            return i.vertex->input_slots.at(i.slot);
        }

        linked_output<Payload> & to_slot(output_index<Payload> o) {
            return o.vertex->output_slots.at(o.slot);

        }

        computation_result compute(Payload const & input) const noexcept {
            source.input_slots[0].maybe_value = std::addressof(input);
            
            auto errors = compute_graph();
            auto optional_value = sink.output_slots[0].value;

            return {std::move(errors), std::move(optional_value)};
        }

        bool link_node(output_index<Payload> output, input_index<Payload> input) noexcept {
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