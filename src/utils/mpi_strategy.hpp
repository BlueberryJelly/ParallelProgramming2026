#pragma once

#include <climits>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <mpi.h>

#include "i_multiplier.hpp"
#include "matrix.hpp"

namespace matrix_ops
{
    class MPIMultiplier final : public IMatrixMultiplier
    {
    private:
        MPI_Comm _comm;
        int _root;
        mutable double _last_compute_seconds;

        static int to_int_count(std::uint64_t value)
        {
            if (value > static_cast<std::uint64_t>(INT_MAX))
            {
                throw std::overflow_error("Размер блока данных превышает INT_MAX элементов (ограничение MPI)");
            }
            return static_cast<int>(value);
        }

    public:
        explicit MPIMultiplier(MPI_Comm comm = MPI_COMM_WORLD, int root = 0)
            : _comm(comm), _root(root), _last_compute_seconds(0.0)
        {
        }

        double last_compute_seconds() const
        {
            return _last_compute_seconds;
        }

        matrix::Matrix multiply(const matrix::Matrix &a, const matrix::Matrix &b) const override
        {
            int rank = 0;
            int size = 1;
            MPI_Comm_rank(_comm, &rank);
            MPI_Comm_size(_comm, &size);

            unsigned long dims[3] = {0, 0, 0};
            if (rank == _root)
            {
                matrix::Matrix::check_multiplicable(a, b);
                dims[0] = a.get_rows();
                dims[1] = a.get_columns();
                dims[2] = b.get_columns();
            }
            MPI_Bcast(dims, 3, MPI_UNSIGNED_LONG, _root, _comm);

            const std::uint32_t n = static_cast<std::uint32_t>(dims[0]);
            const std::uint32_t m = static_cast<std::uint32_t>(dims[1]);
            const std::uint32_t p = static_cast<std::uint32_t>(dims[2]);

            std::vector<int> row_counts(static_cast<std::size_t>(size));
            std::vector<int> a_counts(static_cast<std::size_t>(size));
            std::vector<int> a_displs(static_cast<std::size_t>(size));
            std::vector<int> c_counts(static_cast<std::size_t>(size));
            std::vector<int> c_displs(static_cast<std::size_t>(size));
            {
                std::uint64_t row_offset = 0;
                for (int r = 0; r < size; ++r)
                {
                    const std::uint32_t rows = n / static_cast<std::uint32_t>(size) +
                                               (static_cast<std::uint32_t>(r) < n % static_cast<std::uint32_t>(size) ? 1u : 0u);
                    row_counts[r] = static_cast<int>(rows);
                    a_counts[r] = to_int_count(static_cast<std::uint64_t>(rows) * m);
                    a_displs[r] = to_int_count(row_offset * m);
                    c_counts[r] = to_int_count(static_cast<std::uint64_t>(rows) * p);
                    c_displs[r] = to_int_count(row_offset * p);
                    row_offset += rows;
                }
            }
            const std::uint32_t local_rows = static_cast<std::uint32_t>(row_counts[rank]);

            matrix::Matrix b_local;
            const double *b_data = 0;
            if (rank == _root)
            {
                b_data = b.data();
                MPI_Bcast(const_cast<double *>(b_data), to_int_count(static_cast<std::uint64_t>(m) * p),
                          MPI_DOUBLE, _root, _comm);
            }
            else
            {
                b_local = matrix::Matrix(m, p);
                b_data = b_local.data();
                MPI_Bcast(b_local.data(), to_int_count(static_cast<std::uint64_t>(m) * p),
                          MPI_DOUBLE, _root, _comm);
            }

            matrix::Matrix a_local(local_rows, m);
            MPI_Scatterv(rank == _root ? const_cast<double *>(a.data()) : 0,
                         &a_counts[0], &a_displs[0], MPI_DOUBLE,
                         a_local.data(), a_counts[rank], MPI_DOUBLE, _root, _comm);

            const double compute_start = MPI_Wtime();
            matrix::Matrix c_local(local_rows, p);
            const double *a_ptr = a_local.data();
            double *c_ptr = c_local.data();
            for (std::uint32_t i = 0; i < local_rows; ++i)
            {
                double *c_row = c_ptr + static_cast<std::size_t>(i) * p;
                for (std::uint32_t k = 0; k < m; ++k)
                {
                    const double a_ik = a_ptr[static_cast<std::size_t>(i) * m + k];
                    const double *b_row = b_data + static_cast<std::size_t>(k) * p;
                    for (std::uint32_t j = 0; j < p; ++j)
                    {
                        c_row[j] += a_ik * b_row[j];
                    }
                }
            }
            _last_compute_seconds = MPI_Wtime() - compute_start;

            matrix::Matrix result;
            if (rank == _root)
            {
                result = matrix::Matrix(n, p);
            }
            MPI_Gatherv(c_local.data(), c_counts[rank], MPI_DOUBLE,
                        rank == _root ? result.data() : 0, &c_counts[0], &c_displs[0], MPI_DOUBLE,
                        _root, _comm);

            return result;
        }

        std::string name() const override
        {
            return "mpi";
        }
    };
}
