#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <omp.h>

#include "json_utils.hpp"
#include "matrix.hpp"
#include "i_multiplier.hpp"
#include "openmp_strategy.hpp"
#include "strategy_creator.hpp"
#include "timer.hpp"

namespace
{
    struct CliOptions
    {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        std::string strategy = "openmp";
        int threads = 1;
        int repeats = 1;
    };

    int parse_positive(const char *value, const char *what)
    {
        const int parsed = std::stoi(value);
        if (parsed < 1)
        {
            throw std::invalid_argument(std::string(what) + " должно быть >= 1");
        }
        return parsed;
    }

    CliOptions parse_cli(int argc, char **argv)
    {
        if (argc < 3)
        {
            throw std::invalid_argument(
                "Использование: lab_02 <input.json> <output.json> [threads=1] [repeats=1]");
        }

        CliOptions options;
        options.input_path = argv[1];
        options.output_path = argv[2];
        if (argc > 3)
        {
            options.threads = parse_positive(argv[3], "Число потоков");
        }
        if (argc > 4)
        {
            options.repeats = parse_positive(argv[4], "Число повторов");
        }

        return options;
    }

    double median(std::vector<double> values)
    {
        std::sort(values.begin(), values.end());
        const std::size_t mid = values.size() / 2;
        return values.size() % 2 == 1 ? values[mid] : (values[mid - 1] + values[mid]) / 2.0;
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

        omp_set_dynamic(0);
        const auto multiplier = matrix_ops::create_multiplier(options.strategy, options.threads);

        matrix::Matrix result;
        std::vector<double> times;
        times.reserve(static_cast<std::size_t>(options.repeats));
        util::Timer timer;
        for (int r = 0; r < options.repeats; ++r)
        {
            timer.start();
            result = multiplier->multiply(a, b);
            timer.stop();
            times.push_back(timer.elapsed_seconds());
        }
        const double elapsed = median(times);
        const double elapsed_min = *std::min_element(times.begin(), times.end());

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
        oss << "  \"available_procs\": " << omp_get_num_procs() << ",\n";
        oss << "  \"repeats\": " << options.repeats << ",\n";
        oss << "  \"flops\": " << flops << ",\n";
        oss << "  \"memory_bytes\": " << memory_bytes << ",\n";
        oss << "  \"elapsed_min_seconds\": " << elapsed_min << ",\n";
        oss << "  \"elapsed_seconds\": " << elapsed << "\n";
        oss << "}\n";

        json_utils::write_text_file(options.output_path, oss.str());

        std::cout << "OK: n=" << n
                  << " strategy=" << multiplier->name()
                  << " threads=" << options.threads
                  << " procs=" << omp_get_num_procs()
                  << " time=" << elapsed << "s"
                  << " flops=" << flops << "\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Ошибка: " << error.what() << std::endl;
        return 1;
    }
}
