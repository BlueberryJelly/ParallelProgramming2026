#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "matrix.hpp"
#include "i_multiplier.hpp"
#include "sequential_strategy.hpp"
#include "openmp_strategy.hpp"

namespace matrix_ops
{
inline std::unique_ptr<IMatrixMultiplier> create_multiplier(const std::string &strategy, [[maybe_unused]] int threads = 1)
    {
        if (strategy == "sequential")
        {
            return std::make_unique<SequentialMultiplier>();
        }
#ifdef _OPENMP
        if (strategy == "openmp")
        {
            return std::make_unique<OpenMPMultiplier>(threads);
        }
#endif

        throw std::invalid_argument("Неизвестная стратегия умножения: '" + strategy + "' (ожидалось sequential или openmp)");
    }
}