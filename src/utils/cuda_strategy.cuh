#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "matrix.hpp"
#include "i_multiplier.hpp"

#ifdef __CUDACC__

#include <cuda_runtime.h>

namespace matrix_ops
{
    namespace cuda_detail
    {
        inline void check(cudaError_t status, const char *what)
        {
            if (status != cudaSuccess)
            {
                throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(status));
            }
        }

        __global__ void multiply_kernel(const double *a, const double *b, double *c,
                                         std::uint32_t n, std::uint32_t m, std::uint32_t p)
        {
            const std::uint32_t col = blockIdx.x * blockDim.x + threadIdx.x;
            const std::uint32_t row = blockIdx.y * blockDim.y + threadIdx.y;

            if (row < n && col < p)
            {
                double acc = 0.0;
                for (std::uint32_t k = 0; k < m; ++k)
                {
                    acc += a[static_cast<std::size_t>(row) * m + k] *
                           b[static_cast<std::size_t>(k) * p + col];
                }
                c[static_cast<std::size_t>(row) * p + col] = acc;
            }
        }

        class DeviceBuffer
        {
        private:
            double *_ptr = nullptr;

        public:
            explicit DeviceBuffer(std::size_t bytes)
            {
                check(cudaMalloc(reinterpret_cast<void **>(&_ptr), bytes), "cudaMalloc");
            }

            DeviceBuffer(const DeviceBuffer &) = delete;
            DeviceBuffer &operator=(const DeviceBuffer &) = delete;

            ~DeviceBuffer()
            {
                if (_ptr != nullptr)
                {
                    cudaFree(_ptr);
                }
            }

            [[nodiscard]] double *get() const noexcept
            {
                return _ptr;
            }
        };
    }

    class CudaMultiplier final : public IMatrixMultiplier
    {
    private:
        unsigned int _block_x;
        unsigned int _block_y;

    public:
        CudaMultiplier(unsigned int block_x, unsigned int block_y)
            : _block_x(block_x), _block_y(block_y)
        {
            if (_block_x == 0 || _block_y == 0)
            {
                throw std::invalid_argument("Размеры блока CUDA должны быть >= 1");
            }
            if (static_cast<std::uint64_t>(_block_x) * static_cast<std::uint64_t>(_block_y) > 1024ULL)
            {
                throw std::invalid_argument(
                    "blockDim.x * blockDim.y не должно превышать 1024 (аппаратное ограничение CUDA)");
            }
        }

        [[nodiscard]]
        matrix::Matrix multiply(const matrix::Matrix &a, const matrix::Matrix &b) const override
        {
            matrix::Matrix::check_multiplicable(a, b);

            const std::uint32_t n = a.get_rows();
            const std::uint32_t m = a.get_columns();
            const std::uint32_t p = b.get_columns();

            matrix::Matrix result(n, p);

            const std::size_t bytes_a = static_cast<std::size_t>(n) * m * sizeof(double);
            const std::size_t bytes_b = static_cast<std::size_t>(m) * p * sizeof(double);
            const std::size_t bytes_c = static_cast<std::size_t>(n) * p * sizeof(double);

            cuda_detail::DeviceBuffer d_a(bytes_a);
            cuda_detail::DeviceBuffer d_b(bytes_b);
            cuda_detail::DeviceBuffer d_c(bytes_c);

            cuda_detail::check(cudaMemcpy(d_a.get(), a.data(), bytes_a, cudaMemcpyHostToDevice),
                                "cudaMemcpy(matrix_a, H2D)");
            cuda_detail::check(cudaMemcpy(d_b.get(), b.data(), bytes_b, cudaMemcpyHostToDevice),
                                "cudaMemcpy(matrix_b, H2D)");

            const dim3 block(_block_x, _block_y);
            const dim3 grid((p + block.x - 1) / block.x, (n + block.y - 1) / block.y);

            cuda_detail::multiply_kernel<<<grid, block>>>(d_a.get(), d_b.get(), d_c.get(), n, m, p);
            cuda_detail::check(cudaGetLastError(), "запуск ядра multiply_kernel");
            cuda_detail::check(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

            cuda_detail::check(cudaMemcpy(result.data(), d_c.get(), bytes_c, cudaMemcpyDeviceToHost),
                                "cudaMemcpy(result, D2H)");

            return result;
        }

        [[nodiscard]]
        std::string name() const override
        {
            return "cuda";
        }

        [[nodiscard]] unsigned int block_x() const noexcept
        {
            return _block_x;
        }

        [[nodiscard]] unsigned int block_y() const noexcept
        {
            return _block_y;
        }
    };
}

#endif