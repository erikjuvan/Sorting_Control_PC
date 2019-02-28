#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// All these operator overloads are to support enum bitwise operations but if it is in the same file as an SFML file it causes errors
// because of bitwise | operator
/*
template <class T>
inline T operator~(T a) { return (T) ~(int)a; }
template <class T>
inline T operator|(T a, T b) { return (T)((int)a | (int)b); }
template <class T>
inline T operator&(T a, T b) { return (T)((int)a & (int)b); }
template <class T>
inline T operator^(T a, T b) { return (T)((int)a ^ (int)b); }
template <class T>
inline T& operator|=(T& a, T b) { return (T&)((int&)a |= (int)b); }
template <class T>
inline T& operator&=(T& a, T b) { return (T&)((int&)a &= (int)b); }
template <class T>
inline T& operator^=(T& a, T b) { return (T&)((int&)a ^= (int)b); }
*/

template <typename T>
struct Statistics {
    std::vector<T> buffer;

    // min is special since when uninitialized no value makes real sense
    std::optional<T> min;

    T max;
    T avg, prev_avg;
    T stdev, S;
    T last;
    T cnt;
    T sum;

    Statistics(size_t history_size = 1000)
    {
        buffer.reserve(history_size);
        Clear();
    }

    void Clear()
    {
        buffer.clear();
        min = std::nullopt;
        max = avg = prev_avg = stdev = S = last = cnt = sum = static_cast<T>(0);
    }

    void Update(T val)
    {
        last = val;
        sum += val;
        cnt++;
        if (!min)
            min = val;
        if (val < min)
            min = val;
        if (val > max)
            max = val;

        // Std dev
        prev_avg = avg;
        avg      = sum / cnt;
        S        = S + (val - avg) * (val - prev_avg);
        stdev    = static_cast<T>(std::sqrt(S / cnt));

        buffer.push_back(val);
    }
};

namespace Help
{

#define TIME_IT(x)                                                                                                 \
    {                                                                                                              \
        auto s1 = std::chrono::high_resolution_clock::now();                                                       \
        x;                                                                                                         \
        auto s2  = std::chrono::high_resolution_clock::now();                                                      \
        auto str = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(s2 - s1).count()) + " us"; \
        std::cout << str << std::endl;                                                                             \
    }

#define TIME_IT_VERBOSE(x)                                                                                                                  \
    {                                                                                                                                       \
        auto s1 = std::chrono::high_resolution_clock::now();                                                                                \
        x;                                                                                                                                  \
        auto s2  = std::chrono::high_resolution_clock::now();                                                                               \
        auto str = std::string(#x) + ": " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(s2 - s1).count()) + " us"; \
        std::cout << str << std::endl;                                                                                                      \
    }

inline uint64_t          rdtsc();
std::vector<std::string> TokenizeString(std::string const& str, std::string const& delims);

} // namespace Help