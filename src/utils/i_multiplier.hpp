#pragma once

#include <string>

#include "matrix.hpp"

namespace matrix_ops
{
    class IMatrixMultiplier
    {
    public:
        virtual ~IMatrixMultiplier() = default;

        [[nodiscard]]
        virtual matrix::Matrix multiply(const matrix::Matrix &a, const matrix::Matrix &b) const = 0;

        [[nodiscard]]
        virtual std::string name() const = 0;
    };
}