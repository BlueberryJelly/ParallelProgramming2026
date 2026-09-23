#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "matrix.hpp"
#include "i_multiplier.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace matrix_ops
{
#ifdef _OPENMP
    class OpenMPMultiplier final : public IMatrixMultiplier
    {
    private:
        int _threads;

    public:
        explicit OpenMPMultiplier(int threads) : _threads(threads)
        {
            if (_threads < 1)
            {
                throw std::invalid_argument("Число потоков должно быть >= 1");
            }
        }

        [[nodiscard]]
        matrix::Matrix multiply(const matrix::Matrix &a, const matrix::Matrix &b) const override
        {
            matrix::Matrix::check_multiplicable(a, b);

            const std::int64_t n = a.get_rows();
            const std::uint32_t m = a.get_columns();
            const std::uint32_t p = b.get_columns();

            matrix::Matrix result(static_cast<std::uint32_t>(n), p);

#pragma omp parallel for num_threads(_threads) schedule(static)
            for (std::int64_t i = 0; i < n; ++i)
            {
                const auto row = static_cast<std::uint32_t>(i);
                for (std::uint32_t k = 0; k < m; ++k)
                {
                    const double a_ik = a(row, k);
                    for (std::uint32_t j = 0; j < p; ++j)
                    {
                        result(row, j) += a_ik * b(k, j);
                    }
                }
            }

            return result;
        }

        [[nodiscard]]
        std::string name() const override
        {
            return "openmp";
        }
    };
#endif
}