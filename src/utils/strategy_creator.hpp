#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "matrix.hpp"
#include "i_multiplier.hpp"
#include "sequential_strategy.hpp"
#include "openmp_strategy.hpp"
#include "cuda_strategy.cuh"

namespace matrix_ops
{
    inline std::unique_ptr<IMatrixMultiplier> create_multiplier(
        const std::string &strategy,
        [[maybe_unused]] int threads = 1,
        [[maybe_unused]] int block_y = 1)
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
#ifdef __CUDACC__
        if (strategy == "cuda")
        {
            return std::make_unique<CudaMultiplier>(static_cast<unsigned int>(threads),
                                                      static_cast<unsigned int>(block_y));
        }
#endif

        throw std::invalid_argument(
            "Неизвестная стратегия умножения: '" + strategy + "' (ожидалось sequential, openmp или cuda)");
    }
}