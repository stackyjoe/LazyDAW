#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <utility>

namespace LazyDAW {

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
            if(data.has_value() && std::holds_alternative<T>(data.value()))
                return std::addressof(std::get<T>(data.value()));
            return nullptr;
        }

        template <class T>
        T const * get() const noexcept {
            if(data.has_value() && std::holds_alternative<T>(data.value()))
                return std::addressof(std::get<T>(data.value()));
            return nullptr;
        }

    };




}
