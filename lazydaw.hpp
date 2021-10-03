#pragma once

#include <complex>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>
#include <type_traits>
#include <utility>

#include <iostream>


namespace LazyDAW {


    template<class T, class ... Args>
    inline bool holds_alternative( std::variant<Args...> const & v ) {
        if constexpr ( ! std::disjunction_v<std::is_same<T,Args> ...>) {
            return false;
        }

        return std::holds_alternative<T>(v);
    }

    template<class T, class ... Args>
    inline T * get_if( std::variant<Args...> &v ) noexcept {
        if constexpr ( !std::disjunction_v<std::is_same<T,Args> ...>) {
            return nullptr;
        }

        return std::get_if<T>(&v);
    }

    template<class T, class ... Args>
    inline T const * get_if( std::variant<Args...> const &v ) noexcept {
        if constexpr ( ! std::disjunction_v<std::is_same<T,Args> ...>) {
            return nullptr;
        }

        return std::get_if<T>(&v);
    }

        template<class T, class ... Args>
    inline T * try_get_if( std::variant<Args...> &v ) {
        auto * p = get_if(v);
        if(p == nullptr)
            throw std::runtime_error("Tried to access contents of variant not held by variant at run time.");
        return p;
    }

    template<class T, class ... Args>
    inline T const * try_get_if( std::variant<Args...> const &v ) {
        auto * p = get_if(v);
        if(p == nullptr)
            throw std::runtime_error("Tried to access contents of variant not held by variant at run time.");
        return p;
    }


    struct AudioSample {
        using iterator = std::vector<int16_t>::iterator;
        using const_iterator = std::vector<int16_t>::const_iterator;

        std::vector<int16_t> discrete_amplitudes;

        iterator begin() noexcept {
            return discrete_amplitudes.begin();
        }

        const_iterator begin() const noexcept {
            return discrete_amplitudes.begin();
        }

        iterator end() noexcept {
            return discrete_amplitudes.end();
        }

        const_iterator end() const noexcept {
            return discrete_amplitudes.end();
        }

        size_t size() const noexcept {
            return discrete_amplitudes.size();
        }

        int16_t * data() noexcept {
            return discrete_amplitudes.data();
        }
        
        int16_t const * data() const noexcept {
            return discrete_amplitudes.data();
        }

        void zero_out(size_t desired_length) {
         discrete_amplitudes.clear();
         discrete_amplitudes.reserve(desired_length);
            for(auto i = 0; i < desired_length; ++i)
             discrete_amplitudes.push_back(0);
        }

        void reserve(size_t desired_capacity) {
            discrete_amplitudes.reserve(desired_capacity);
        }

        int16_t * get() noexcept {
            return discrete_amplitudes.data();
        }

        int16_t const * get() const noexcept {
            return discrete_amplitudes.data();
        }

        int16_t & operator[](size_t i) noexcept {
            return discrete_amplitudes[i];
        }
        
        int16_t const & operator[](size_t i) const noexcept {
            return discrete_amplitudes[i];
        }
    };

    struct FourierCoefficients {
        using complex = std::complex<double>;
        using iterator = std::vector<complex>::iterator;
        using const_iterator = std::vector<complex>::const_iterator;

        std::vector<std::complex<double>> discrete_frequency_components;

        iterator begin() noexcept {
            return discrete_frequency_components.begin();
        }

        const_iterator begin() const noexcept {
            return discrete_frequency_components.begin();
        }

        iterator end() noexcept {
            return discrete_frequency_components.end();
        }

        const_iterator end() const noexcept {
            return discrete_frequency_components.end();
        }

        size_t size() const noexcept {
            return discrete_frequency_components.size();
        }

        void zero_out(size_t desired_length) {
         discrete_frequency_components.clear();
         discrete_frequency_components.reserve(desired_length);
            for(auto i = 0; i < desired_length; ++i)
             discrete_frequency_components.push_back(0);
        }

        void reserve(size_t desired_capacity) {
            discrete_frequency_components.reserve(desired_capacity);
        }

        complex * data() noexcept {
            return discrete_frequency_components.data();
        }

        complex const * data() const noexcept {
            return discrete_frequency_components.data();
        }

        complex & operator[](size_t i) noexcept {
            return discrete_frequency_components[i];
        }
        
        complex const & operator[](size_t i) const noexcept {
            return discrete_frequency_components[i];
        }
    };

    FourierCoefficients NaiveDiscreteFourierTransform(AudioSample const & amplitudes) {
        using namespace std::complex_literals;
        FourierCoefficients f;
        size_t sequence_length = amplitudes.size();

        f.zero_out(sequence_length);

        std::cout << "Attempting to do discrete fourier transform with " << sequence_length << " elements.\n";

        for(auto i = 0; i < sequence_length; ++i) {

            for(auto j = 0; j < sequence_length; ++j) {
                f[i] += static_cast<double>(amplitudes[j]) * std::exp(2.*std::numbers::pi_v<double>*static_cast<double>(i)*(0.-1.i)*static_cast<double>(j)/static_cast<double>(sequence_length));
            }
        }

        return f;
    }

     AudioSample NaiveDiscreteInverseFourierTransform(FourierCoefficients const & frequencies) {
        using namespace std::complex_literals;
        // First will compute the inverse fourier transform as complex numbers
        // Then cast the real part to an int16_t.
        
        FourierCoefficients f;
        size_t sequence_length = frequencies.size();

        f.zero_out(sequence_length);

        std::cout << "Attempting to do discrete inverse fourier transform with " << sequence_length << " elements.\n";


        for(auto i = 0; i < sequence_length; ++i) {
            for(auto j = 0; j < sequence_length; ++j) {
                f[i] += 1./static_cast<double>(sequence_length) * static_cast<double>(frequencies[j].real()) * std::exp(2.*std::numbers::pi_v<double>*static_cast<double>(i)*(0.-1.i)*static_cast<double>(j)/static_cast<double>(sequence_length));
            }

        }

        AudioSample approx;

        approx.zero_out(sequence_length);

        for(auto i = 0; i < sequence_length; ++i) {
            approx[i] = static_cast<int16_t>(f[i].real());
        }

        return approx;
    }



    struct AudioRepresentation {
        std::optional<std::variant<AudioSample,FourierCoefficients>> data;

        template<class T>
        AudioRepresentation(T t)
        : data() {
            data.emplace(std::move(t));
        }

        AudioRepresentation() : data({}) { }

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
