#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bench_utils.hpp"
#include "i_multiplier.hpp"
#include "json_utils.hpp"
#include "matrix.hpp"
#include "sequential_strategy.hpp"
#include "strategy_creator.hpp"

namespace
{
    struct CliOptions
    {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        int repeats = 1;
    };

    CliOptions parse_cli(int argc, char **argv)
    {
        if (argc < 3)
        {
            throw std::invalid_argument(
                "Использование: lab_01 <input.json> <output.json> [repeats=1]");
        }

        CliOptions options;
        options.input_path = argv[1];
        options.output_path = argv[2];
        if (argc > 3)
        {
            options.repeats = bench::parse_positive(argv[3], "Число повторов");
        }

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

        const auto multiplier = matrix_ops::create_multiplier("sequential");
        const bench::Measurement measurement =
            bench::measure(options.repeats, [&] { return multiplier->multiply(a, b); });
        const matrix::Matrix &result = measurement.result;

        const std::uint64_t n = a.get_rows();
        const std::uint64_t m = a.get_columns();
        const std::uint64_t p = b.get_columns();
        const std::uint64_t flops = 2ULL * n * m * p;
        const std::uint64_t memory_bytes = (a.size() + b.size() + result.size()) * sizeof(double);

        std::ostringstream oss;
        oss << "{\n";
        oss << json_utils::matrix_to_json_string("result", result) << ",\n";
        oss << "  \"rows_a\": " << n << ",\n";
        oss << "  \"columns_a\": " << m << ",\n";
        oss << "  \"columns_b\": " << p << ",\n";
        oss << "  \"strategy\": \"" << multiplier->name() << "\",\n";
        oss << "  \"repeats\": " << options.repeats << ",\n";
        oss << "  \"flops\": " << flops << ",\n";
        oss << "  \"memory_bytes\": " << memory_bytes << ",\n";
        oss << "  \"elapsed_seconds\": " << measurement.elapsed_seconds << "\n";
        oss << "}\n";

        json_utils::write_text_file(options.output_path, oss.str());

        std::cout << "OK: n=" << n
                  << " strategy=" << multiplier->name()
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
