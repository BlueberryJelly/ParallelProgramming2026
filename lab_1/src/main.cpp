#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "json_utils.hpp"
#include "matrix.hpp"
#include "multiplier.hpp"
#include "timer.hpp"

namespace
{
    struct CliOptions
    {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        std::string strategy = "sequential";
        std::uint32_t threads = 1;
    };

    CliOptions parse_cli(int argc, char **argv)
    {
        if (argc < 3)
        {
            throw std::invalid_argument(
                "Использование: matrix_mul <input.json> <output.json>");
        }

        CliOptions options;
        options.input_path = argv[1];
        options.output_path = argv[2];

        return options;
    }
}

int main(int argc, char **argv)
{
    try
    {
        const CliOptions options = parse_cli(argc, argv);

        const std::string input_json = json_utils::read_text_file(options.input_path);
        const matrix::Matrix a = json_utils::json_to_matrix(input_json, "matrix_a");
        const matrix::Matrix b = json_utils::json_to_matrix(input_json, "matrix_b");

        matrix::Matrix::check_multiplicable(a, b);

        const auto multiplier = matrix_ops::create_multiplier(options.strategy, options.threads);

        util::Timer timer;
        timer.start();
        const matrix::Matrix result = multiplier->multiply(a, b);
        timer.stop();

        const std::uint64_t n = a.get_rows();
        const std::uint64_t m = a.get_columns();
        const std::uint64_t p = b.get_columns();
        const std::uint64_t flops = 2ULL * n * m * p; // n*m*p умножений + n*m*p сложений
        const std::uint64_t memory_bytes = (a.size() + b.size() + result.size()) * sizeof(double);

        std::ostringstream oss;
        oss << "{\n";
        oss << json_utils::matrix_to_json_string("result", result) << ",\n";
        oss << "  \"rows_a\": " << n << ",\n";
        oss << "  \"columns_a\": " << m << ",\n";
        oss << "  \"columns_b\": " << p << ",\n";
        oss << "  \"strategy\": \"" << multiplier->name() << "\",\n";
        oss << "  \"threads\": " << options.threads << ",\n";
        oss << "  \"flops\": " << flops << ",\n";
        oss << "  \"memory_bytes\": " << memory_bytes << ",\n";
        oss << "  \"elapsed_seconds\": " << timer.elapsed_seconds() << "\n";
        oss << "}\n";

        json_utils::write_text_file(options.output_path, oss.str());

        std::cout << "OK: n=" << n
                  << " strategy=" << multiplier->name()
                  << " time=" << timer.elapsed_seconds() << "s"
                  << " flops=" << flops << "\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Ошибка: " << error.what() << std::endl;
        return 1;
    }
}