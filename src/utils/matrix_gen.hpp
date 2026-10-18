#pragma once

#include <cstdint>

#include "matrix.hpp"

namespace matrix_gen
{
    class Lcg64
    {
    private:
        std::uint64_t _state;

    public:
        explicit Lcg64(std::uint64_t seed) : _state(seed) {}

        std::uint64_t next_u64()
        {
            _state = _state * 6364136223846793005ULL + 1442695040888963407ULL;
            return _state;
        }

        double next_uniform(double lo, double hi)
        {
            const std::uint64_t bits = next_u64() >> 11;
            const double unit = static_cast<double>(bits) * (1.0 / 9007199254740992.0);
            return lo + (hi - lo) * unit;
        }
    };

    inline matrix::Matrix generate(std::uint32_t n, std::uint64_t seed,
                                    double lo = -10.0, double hi = 10.0)
    {
        matrix::Matrix result(n, n);
        Lcg64 rng(seed);
        double *data = result.data();
        const std::size_t count = static_cast<std::size_t>(n) * n;
        for (std::size_t i = 0; i < count; ++i)
        {
            data[i] = rng.next_uniform(lo, hi);
        }
        return result;
    }
}
