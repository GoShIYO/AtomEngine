#pragma once
#include <cstddef>
#include <functional>

namespace AtomEngine
{
    template<typename T>
    inline void HashCombine(std::size_t& seed, const T& v)
    {
        seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<typename T, typename... Ts>
    inline void HashCombine(std::size_t& seed, const T& v, const Ts&... rest)
    {
        HashCombine(seed, v);
        HashCombine(seed, rest...);
    }
}


