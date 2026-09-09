#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace matrix
{
    class Matrix
    {
    private:
        std::uint32_t _rows = 0;
        std::uint32_t _columns = 0;
        std::vector<double> _data;

    public:
        Matrix() = default;

        Matrix(std::uint32_t rows, std::uint32_t columns)
            : _rows(rows), _columns(columns),
              _data(static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns), 0.0)
        {
        }

        std::uint32_t get_rows() const noexcept
        {
            return _rows;
        }

        std::uint32_t get_columns() const noexcept
        {
            return _columns;
        }

        std::size_t size() const noexcept
        {
            return _data.size();
        }

        bool is_square() const noexcept
        {
            return _rows == _columns;
        }

        bool empty() const noexcept
        {
            return _data.empty();
        }

        [[nodiscard]]
        double *data() noexcept
        {
            return _data.data();
        }

        [[nodiscard]]
        const double *data() const noexcept
        {
            return _data.data();
        }

        double &operator()(std::uint32_t row, std::uint32_t column)
        {
            return _data[static_cast<std::size_t>(row) * _columns + column];
        }

        double operator()(std::uint32_t row, std::uint32_t column) const
        {
            return _data[static_cast<std::size_t>(row) * _columns + column];
        }

        static void check_multiplicable(const Matrix &a, const Matrix &b)
        {
            if (a.get_columns() != b.get_rows())
            {
                throw std::invalid_argument("Несовместимые размерности для умножения матриц");
            }
        }
    };
}