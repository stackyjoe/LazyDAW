#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>


namespace jl {
    
    struct AddressGetter {
        // TY to D. Wright, B. Bi, and L. Szonolki for this one
        template<class T>
        T * operator()(T &t) {
            return std::addressof<T>(t);
        }

    };

    inline constexpr auto address_of = AddressGetter{};

    namespace containers {



        template<class T>
        struct set {
        private:
            std::vector<T> dynamic_array;

            using iterator = typename std::vector<T>::iterator;
            using const_iterator = typename std::vector<T>::const_iterator;

        public:
            iterator begin() noexcept {
                return dynamic_array.begin();
            }

            iterator begin() const noexcept {
                return dynamic_array.begin();
            }

            iterator end() noexcept {
                return dynamic_array.end();
            }

            iterator end() const noexcept {
                return dynamic_array.end();
            }

            iterator find(T const &t) noexcept {
                return std::lower_bound(dynamic_array.begin(), dynamic_array.end(), t);
            }

            iterator find(T &&t) noexcept {
                return std::lower_bound(dynamic_array.begin(), dynamic_array.end(), t);
            }

            const_iterator find(T const &t) const noexcept {
                return std::lower_bound(dynamic_array.begin(), dynamic_array.end(), t);
            }

            const_iterator find(T &&t) const noexcept {
                return std::lower_bound(dynamic_array.begin(), dynamic_array.end(), t);
            }

            bool contains(T const &t) const noexcept {
                return dynamic_array.end() == std::lower_bound(dynamic_array.begin(), dynamic_array.end(), t);
            }

            void reserve(size_t capacity) {
                dynamic_array.reserve(capacity);
            }

            template<class Predicate, class Operation = AddressGetter>
            auto all_satisfying(Predicate p,
                                Operation o = address_of)
                                noexcept(noexcept(p(std::declval<T const &>())) && noexcept(o(std::declval<T &>())))
                                -> std::vector<decltype(o(std::declval<T&>()))> {
                // Envisioned use
                // auto widgets = all_satisfying([](T const &t) -> {
                //     return t.is_red() & t.is_sphere();
                // });
                // widgets is a vector with pointers to all spherical red objects in the set.

                std::vector<decltype(o(std::declval<T&>()))> desiderata;

                desiderata.reserve(dynamic_array.size());

                for(auto & value : dynamic_array) {
                    if(p(value)) {
                        desiderata.push_back(o(value));
                    }
                }

                return desiderata;
            }

            void insert(T t) {
                auto it = find(t);
                if( (it == dynamic_array.end()) || (*it < t)) {
                    dynamic_array.insert(it, t);
                }
            }

            void insert_ordered_unique(std::vector<T> &&ordered_unique_element_vector) {
                auto old_size = dynamic_array.size();

                dynamic_array.reserve(dynamic_array.size() + ordered_unique_element_vector.size());
                dynamic_array.insert(dynamic_array.end(),
                    std::make_move_iterator(ordered_unique_element_vector.begin()),
                    std::make_move_iterator(ordered_unique_element_vector.end()));
                std::inplace_merge(dynamic_array.begin(), dynamic_array.begin()+old_size, dynamic_array.end());
            }

            void insert(std::vector<T> && unordered_or_repeat_element_vector) {
                std::sort(unordered_or_repeat_element_vector.begin(), unordered_or_repeat_element_vector.end());
                std::unique(unordered_or_repeat_element_vector.begin(), unordered_or_repeat_element_vector.end());

                insert_ordered_unique(std::move(unordered_or_repeat_element_vector));
            }

            bool empty() const noexcept {
                return dynamic_array.empty();
            }

            void clear() noexcept(noexcept(std::declval<T>().~T())) {
                dynamic_array.clear();
            }

        };
    }
}
