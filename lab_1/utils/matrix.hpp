#pragma once

#include <cstddef>
#include <vector>
#include <stdexcept>
#include <format>

namespace matrix
{
    class Matrix final
    {
    private:
        std::uint32_t _rows = 0;
        std::uint32_t _columns = 0;
        std::vector<double> _matrix;

    public:
        Matrix() = default;

        Matrix(const std::uint32_t rows, const std::uint32_t columns)
            : _rows(rows), _columns(columns), _matrix(rows * columns) {}

        double operator()(const std::uint32_t row, const std::uint32_t column) const
        {
            if (row >= _rows || column >= _columns)
            {
                throw std::out_of_range("Выход за пределы матрицы.");
            }

            return _matrix[_columns * row + column];
        }

        double &operator()(const std::uint32_t row, const std::uint32_t column)
        {
            if (row >= _rows || column >= _columns)
            {
                throw std::out_of_range("Выход за пределы матрицы.");
            }

            return _matrix[_columns * row + column];
        }

        std::uint32_t get_rows() const { return _rows; }

        std::uint32_t get_columns() const { return _columns; }

    private:
        static void validate_multiplication_sizes(const Matrix &a, const Matrix &b)
        {
            if (a.get_columns() != b.get_rows())
            {
                throw std::logic_error(std::format("Ошибка умножения, {} != {}", a.get_columns(), b.get_rows()));
            }
        }

        static double row_x_column(const Matrix &a, const std::uint32_t a_i,
                                   const Matrix &b, const std::uint32_t b_j)
        {
            if (a_i >= a.get_rows() || b_j >= b.get_columns())
            {
                throw std::out_of_range("Выход за пределы матрицы.");
            }

            double result = 0;
            for (std::uint32_t k = 0; k < a.get_columns(); ++k)
            {
                result += a(a_i, k) * b(k, b_j);
            }

            return result;
        }

    public:
        Matrix operator*(const Matrix &other) const
        {
            validate_multiplication_sizes(*this, other);

            Matrix result(_rows, other.get_columns());

            for (std::uint32_t row = 0; row < _rows; ++row)
            {
                for (std::uint32_t column = 0; column < other.get_columns(); ++column)
                {
                    result(row, column) = row_x_column(*this, row, other, column);
                }
            }

            return result;
        }
    };
}