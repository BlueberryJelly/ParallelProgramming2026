#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <cuda_runtime.h>

#include "bench_utils.hpp"
#include "i_multiplier.hpp"
#include "json_utils.hpp"
#include "matrix.hpp"
#include "cuda_strategy.cuh"
#include "strategy_creator.hpp"

namespace
{
    struct CliOptions
    {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        int repeats = 1;
        int block_x = 16;
        int block_y = 16;
    };

    CliOptions parse_cli(int argc, char **argv)
    {
        if (argc < 3)
        {
            throw std::invalid_argument(
                "Использование: lab_04 <input.json> <output.json> [repeats=1] [block_x=16] [block_y=16]");
        }

        CliOptions options;
        options.input_path = argv[1];
        options.output_path = argv[2];
        if (argc > 3)
        {
            options.repeats = bench::parse_positive(argv[3], "Число повторов");
        }
        if (argc > 4)
        {
            options.block_x = bench::parse_positive(argv[4], "Размер блока по X");
        }
        if (argc > 5)
        {
            options.block_y = bench::parse_positive(argv[5], "Размер блока по Y");
        }

        return options;
    }

    struct DeviceInfo
    {
        std::string name;
        int compute_major = 0;
        int compute_minor = 0;
        int multiprocessors = 0;
        int max_threads_per_block = 0;
    };

    void check_cuda(cudaError_t status, const char *what)
    {
        if (status != cudaSuccess)
        {
            throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(status));
        }
    }

    DeviceInfo query_device()
    {
        int device = 0;
        check_cuda(cudaGetDevice(&device), "cudaGetDevice");

        cudaDeviceProp props{};
        check_cuda(cudaGetDeviceProperties(&props, device), "cudaGetDeviceProperties");

        DeviceInfo info;
        info.name = props.name;
        info.compute_major = props.major;
        info.compute_minor = props.minor;
        info.multiprocessors = props.multiProcessorCount;
        info.max_threads_per_block = props.maxThreadsPerBlock;
        return info;
    }
}

int main(int argc, char **argv)
{
    try
    {
        const CliOptions options = parse_cli(argc, argv);
        const DeviceInfo device = query_device();

        const std::string input_json = json_utils::read_text_file(options.input_path);
        const matrix::Matrix a = json_utils::json_to_matrix(input_json, "matrix_a");
        const matrix::Matrix b = json_utils::json_to_matrix(input_json, "matrix_b");

        matrix::Matrix::check_multiplicable(a, b);

        const auto multiplier = matrix_ops::create_multiplier("cuda", options.block_x, options.block_y);
        const bench::Measurement measurement =
            bench::measure(options.repeats, [&] { return multiplier->multiply(a, b); });
        const matrix::Matrix &result = measurement.result;

        const std::uint64_t n = a.get_rows();
        const std::uint64_t m = a.get_columns();
        const std::uint64_t p = b.get_columns();
        const std::uint64_t flops = 2ULL * n * m * p;
        const std::uint64_t memory_bytes = (a.size() + b.size() + result.size()) * sizeof(double);
        const std::uint64_t grid_x = (p + static_cast<std::uint64_t>(options.block_x) - 1) /
                                      static_cast<std::uint64_t>(options.block_x);
        const std::uint64_t grid_y = (n + static_cast<std::uint64_t>(options.block_y) - 1) /
                                      static_cast<std::uint64_t>(options.block_y);

        std::ostringstream oss;
        oss << "{\n";
        oss << json_utils::matrix_to_json_string("result", result) << ",\n";
        oss << "  \"rows_a\": " << n << ",\n";
        oss << "  \"columns_a\": " << m << ",\n";
        oss << "  \"columns_b\": " << p << ",\n";
        oss << "  \"strategy\": \"" << multiplier->name() << "\",\n";
        oss << "  \"repeats\": " << options.repeats << ",\n";
        oss << "  \"block_x\": " << options.block_x << ",\n";
        oss << "  \"block_y\": " << options.block_y << ",\n";
        oss << "  \"threads_per_block\": " << (options.block_x * options.block_y) << ",\n";
        oss << "  \"grid_x\": " << grid_x << ",\n";
        oss << "  \"grid_y\": " << grid_y << ",\n";
        oss << "  \"gpu_name\": \"" << device.name << "\",\n";
        oss << "  \"gpu_compute_capability\": \"" << device.compute_major << "." << device.compute_minor << "\",\n";
        oss << "  \"gpu_multiprocessors\": " << device.multiprocessors << ",\n";
        oss << "  \"gpu_max_threads_per_block\": " << device.max_threads_per_block << ",\n";
        oss << "  \"flops\": " << flops << ",\n";
        oss << "  \"memory_bytes\": " << memory_bytes << ",\n";
        oss << "  \"elapsed_seconds\": " << measurement.elapsed_seconds << "\n";
        oss << "}\n";

        json_utils::write_text_file(options.output_path, oss.str());

        std::cout << "OK: n=" << n
                  << " strategy=" << multiplier->name()
                  << " block=(" << options.block_x << "x" << options.block_y << ")"
                  << " grid=(" << grid_x << "x" << grid_y << ")"
                  << " gpu=" << device.name
                  << " time=" << measurement.elapsed_seconds << "s"
                  << " flops=" << flops << "\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Ошибка: " << error.what() << std::endl;
        return 1;
    }
}