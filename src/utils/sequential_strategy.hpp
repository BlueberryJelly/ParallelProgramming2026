#pragma once

#include <cstdint>
#include <string>

#include "matrix.hpp"
#include "i_multiplier.hpp"

namespace matrix_ops
{
class SequentialMultiplier final : public IMatrixMultiplier
    {
    public:
        [[nodiscard]]
        matrix::Matrix multiply(const matrix::Matrix &a, const matrix::Matrix &b) const override
        {
            matrix::Matrix::check_multiplicable(a, b);

            const std::uint32_t n = a.get_rows();
            const std::uint32_t m = a.get_columns();
            const std::uint32_t p = b.get_columns();

            matrix::Matrix result(n, p);
            for (std::uint32_t i = 0; i < n; ++i)
            {
                for (std::uint32_t k = 0; k < m; ++k)
                {
                    const double a_ik = a(i, k);
                    for (std::uint32_t j = 0; j < p; ++j)
                    {
                        result(i, j) += a_ik * b(k, j);
                    }
                }
            }

            return result;
        }

        [[nodiscard]]
        std::string name() const override
        {
            return "sequential";
        }
    };
}