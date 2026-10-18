#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <mpi.h>

#include "json_utils_compat.hpp"
#include "matrix.hpp"
#include "matrix_gen.hpp"
#include "mpi_strategy.hpp"

namespace
{
    const int ROOT = 0;

    struct CliOptions
    {
        std::string input_path;
        std::string output_path;
        int repeats;
        int size;
        unsigned long seed;
        bool write_result;

        CliOptions() : output_path("result.json"), repeats(3), size(0), seed(42), write_result(false) {}
    };

    int require_positive(const char *value, const char *what)
    {
        const int parsed = std::atoi(value);
        if (parsed < 1)
        {
            throw std::invalid_argument(std::string(what) + " должно быть >= 1");
        }
        return parsed;
    }

    CliOptions parse_cli(int argc, char **argv)
    {
        CliOptions options;

        const bool flag_mode = argc > 1 && std::strncmp(argv[1], "--", 2) == 0;

        if (flag_mode)
        {
            for (int i = 1; i < argc; ++i)
            {
                const std::string arg = argv[i];
                if (arg == "--size" && i + 1 < argc)
                {
                    options.size = require_positive(argv[++i], "Размер матрицы");
                }
                else if (arg == "--seed" && i + 1 < argc)
                {
                    options.seed = std::strtoul(argv[++i], 0, 10);
                }
                else if (arg == "--repeats" && i + 1 < argc)
                {
                    options.repeats = require_positive(argv[++i], "Число повторов");
                }
                else if (arg == "--output" && i + 1 < argc)
                {
                    options.output_path = argv[++i];
                }
                else if (arg == "--input" && i + 1 < argc)
                {
                    options.input_path = argv[++i];
                }
                else if (arg == "--with-result")
                {
                    options.write_result = true;
                }
                else
                {
                    throw std::invalid_argument("Неизвестный или неполный аргумент: " + arg);
                }
            }
            if (options.size == 0 && options.input_path.empty())
            {
                throw std::invalid_argument(
                    "Нужно указать либо --size <N>, либо --input <input.json>");
            }
        }
        else
        {
            if (argc < 3)
            {
                throw std::invalid_argument(
                    "Использование: mpirun -np <p> lab_03 --size <N> [--seed S] [--repeats R] [--output out.json]\n"
                    "           либо: mpirun -np <p> lab_03 <input.json> <output.json> [repeats]");
            }
            options.input_path = argv[1];
            options.output_path = argv[2];
            options.repeats = argc > 3 ? require_positive(argv[3], "Число повторов") : 1;
            options.write_result = true;
        }

        return options;
    }

    double median(std::vector<double> values)
    {
        std::sort(values.begin(), values.end());
        const std::size_t mid = values.size() / 2;
        return values.size() % 2 == 1 ? values[mid] : (values[mid - 1] + values[mid]) / 2.0;
    }

    std::vector<std::string> gather_node_names(int rank, int size)
    {
        char name[MPI_MAX_PROCESSOR_NAME];
        std::memset(name, 0, sizeof(name));
        int length = 0;
        MPI_Get_processor_name(name, &length);

        std::vector<char> all;
        if (rank == ROOT)
        {
            all.resize(static_cast<std::size_t>(size) * MPI_MAX_PROCESSOR_NAME);
        }
        MPI_Gather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                   rank == ROOT ? &all[0] : 0, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, ROOT, MPI_COMM_WORLD);

        std::vector<std::string> names;
        if (rank == ROOT)
        {
            std::set<std::string> unique;
            for (int r = 0; r < size; ++r)
            {
                unique.insert(std::string(&all[static_cast<std::size_t>(r) * MPI_MAX_PROCESSOR_NAME]));
            }
            names.assign(unique.begin(), unique.end());
        }
        return names;
    }

    int run(int argc, char **argv)
    {
        int rank = 0;
        int size = 1;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        const CliOptions options = parse_cli(argc, argv);

        matrix::Matrix a;
        matrix::Matrix b;
        if (rank == ROOT)
        {
            if (options.size > 0)
            {
                const std::uint32_t n = static_cast<std::uint32_t>(options.size);
                a = matrix_gen::generate(n, options.seed);
                b = matrix_gen::generate(n, options.seed + 1);
            }
            else
            {
                const std::string input_json = json_compat::read_text_file(options.input_path);
                a = json_compat::json_to_matrix(input_json, "matrix_a");
                b = json_compat::json_to_matrix(input_json, "matrix_b");
            }
            matrix::Matrix::check_multiplicable(a, b);
        }

        const std::vector<std::string> node_names = gather_node_names(rank, size);

        const matrix_ops::MPIMultiplier multiplier(MPI_COMM_WORLD, ROOT);

        std::vector<double> elapsed;
        std::vector<double> compute;
        matrix::Matrix result;
        for (int r = 0; r < options.repeats; ++r)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            const double start = MPI_Wtime();
            result = multiplier.multiply(a, b);
            const double local_elapsed = MPI_Wtime() - start;
            const double local_compute = multiplier.last_compute_seconds();

            double max_elapsed = 0.0;
            double max_compute = 0.0;
            MPI_Reduce(&local_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, ROOT, MPI_COMM_WORLD);
            MPI_Reduce(&local_compute, &max_compute, 1, MPI_DOUBLE, MPI_MAX, ROOT, MPI_COMM_WORLD);
            elapsed.push_back(max_elapsed);
            compute.push_back(max_compute);
        }

        if (rank != ROOT)
        {
            return 0;
        }
        
        long double checksum = 0.0L;
        long double checksum_sq = 0.0L;
        {
            const double *data = result.data();
            const std::size_t count = result.size();
            for (std::size_t i = 0; i < count; ++i)
            {
                checksum += data[i];
                checksum_sq += static_cast<long double>(data[i]) * data[i];
            }
        }

        const std::uint64_t n = a.get_rows();
        const std::uint64_t m = a.get_columns();
        const std::uint64_t p = b.get_columns();
        const std::uint64_t flops = 2ULL * n * m * p;
        const std::uint64_t memory_bytes = (a.size() + b.size() + result.size()) * sizeof(double);
        const double elapsed_seconds = median(elapsed);
        const double elapsed_min_seconds = *std::min_element(elapsed.begin(), elapsed.end());
        const double compute_seconds = median(compute);

        int mpi_major = 0;
        int mpi_minor = 0;
        MPI_Get_version(&mpi_major, &mpi_minor);

        std::ostringstream oss;
        oss.precision(17);
        oss << "{\n";
        if (options.write_result)
        {
            oss << json_compat::matrix_to_json_string("result", result) << ",\n";
        }
        oss << "  \"rows_a\": " << n << ",\n";
        oss << "  \"columns_a\": " << m << ",\n";
        oss << "  \"columns_b\": " << p << ",\n";
        oss << "  \"strategy\": \"" << multiplier.name() << "\",\n";
        oss << "  \"size\": " << n << ",\n";
        oss << "  \"seed\": " << options.seed << ",\n";
        oss << "  \"generated\": " << (options.size > 0 ? "true" : "false") << ",\n";
        oss << "  \"repeats\": " << options.repeats << ",\n";
        oss << "  \"processes\": " << size << ",\n";
        oss << "  \"nodes\": " << node_names.size() << ",\n";
        oss << "  \"node_names\": [";
        for (std::size_t i = 0; i < node_names.size(); ++i)
        {
            oss << (i ? ", " : "") << "\"" << node_names[i] << "\"";
        }
        oss << "],\n";
        oss << "  \"mpi_version\": \"" << mpi_major << "." << mpi_minor << "\",\n";
        oss << "  \"flops\": " << flops << ",\n";
        oss << "  \"memory_bytes\": " << memory_bytes << ",\n";
        oss << "  \"checksum\": " << static_cast<double>(checksum) << ",\n";
        oss << "  \"checksum_sq\": " << static_cast<double>(checksum_sq) << ",\n";
        oss << "  \"time_seconds\": " << elapsed_seconds << ",\n";
        oss << "  \"elapsed_seconds\": " << elapsed_seconds << ",\n";
        oss << "  \"elapsed_min_seconds\": " << elapsed_min_seconds << ",\n";
        oss << "  \"compute_seconds\": " << compute_seconds << "\n";
        oss << "}\n";

        json_compat::write_text_file(options.output_path, oss.str());

        std::cout << "OK: n=" << n
                  << " strategy=" << multiplier.name()
                  << " processes=" << size
                  << " nodes=" << node_names.size()
                  << " time=" << elapsed_seconds << "s"
                  << " compute=" << compute_seconds << "s"
                  << " flops=" << flops << std::endl;
        return 0;
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int status = 0;
    try
    {
        status = run(argc, argv);
    }
    catch (const std::exception &error)
    {
        int rank = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        std::cerr << "Ошибка (процесс " << rank << "): " << error.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return status;
}
