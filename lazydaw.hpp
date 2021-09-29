#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <type_traits>
#include <utility>

namespace LazyDAW {


    template<class T, class ... Args>
    inline bool holds_alternative( std::variant<Args...> const & v ) {
        if constexpr ( ! std::disjunction_v<std::is_same<T,Args> ...>) {
            return false;
        }

        return std::holds_alternative<T>(v);
    }

    template<class T, class ... Args>
    inline T * get_if( std::variant<Args...> &v ) {
        if constexpr ( !std::disjunction_v<std::is_same<T,Args> ...>) {
            return nullptr;
        }

        return std::get_if<T>(&v);
    }

    template<class T, class ... Args>
    inline T const * get_if( std::variant<Args...> const &v ) {
        if constexpr ( ! std::disjunction_v<std::is_same<T,Args> ...>) {
            return nullptr;
        }

        return std::get_if<T>(&v);
    }

    struct AudioSample {
        std::vector<int16_t> audio;
    };

    struct FourierCoefficients {

    };

    struct AudioRepresentation {
        std::optional<std::variant<AudioSample,FourierCoefficients>> data;

        bool has_value() const noexcept {
            return data.has_value();
        }

        template <class T>
        T * get() noexcept {
            if(data.has_value())
                return get_if<T>(data.value());
            return nullptr;
        }

        template <class T>
        T const * get() const noexcept {
            if(data.has_value())
                return get_if<T>(data.value());
            return nullptr;
        }

    };




}
