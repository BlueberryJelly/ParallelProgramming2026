#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <charconv>

#include "matrix.hpp"

namespace json_utils
{
    class MatrixParser
    {
    private:
        std::string_view _json_string;
        std::size_t _position = 0;

        constexpr bool is_whitespace(char c) const noexcept
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        constexpr bool is_digit(char c) const noexcept
        {
            return c >= '0' && c <= '9';
        }

        constexpr bool is_exponent(char c) const noexcept
        {
            return c == 'e' || c == 'E';
        }

        void skip_whitespace() noexcept
        {
            while (_position < _json_string.size() && is_whitespace(_json_string[_position]))
            {
                ++_position;
            }
        }

        double parse_double()
        {
            double value = 0.0;
            std::from_chars_result result = std::from_chars(
                _json_string.data() + _position,
                _json_string.data() + _json_string.size(),
                value);

            if (result.ec != std::errc{})
            {
                throw std::runtime_error("Ошибка парсинга числа в JSON");
            }

            _position = result.ptr - _json_string.data();
            return value;
        }

        std::size_t find_key(std::string_view key) const
        {
            std::string search_key;
            search_key.reserve(key.size() + 2);
            search_key += '"';
            search_key += key;
            search_key += '"';

            return _json_string.find(search_key);
        }

        std::size_t find_array_start(std::size_t key_position) const
        {
            std::size_t pos = _json_string.find('[', key_position);
            if (pos == std::string_view::npos)
            {
                throw std::runtime_error("Массив не найден для ключа");
            }
            return pos;
        }

    public:
        explicit MatrixParser(std::string_view json_string)
            : _json_string(json_string), _position(0) {}

        matrix::Matrix parse_matrix(std::string_view key)
        {
            std::size_t key_position = find_key(key);
            if (key_position == std::string_view::npos)
            {
                throw std::runtime_error("Ключ не найден: " + std::string(key));
            }

            _position = find_array_start(key_position);

            std::vector<double> flat;
            std::uint32_t rows = 0;
            std::uint32_t columns = 0;
            std::uint32_t current_column = 0;
            int depth = 0;

            while (_position < _json_string.size())
            {
                skip_whitespace();
                if (_position >= _json_string.size())
                {
                    break;
                }

                char c = _json_string[_position];

                if (c == '[')
                {
                    ++depth;
                    if (depth == 2)
                    {
                        current_column = 0;
                    }
                    ++_position;
                }
                else if (c == ']')
                {
                    --depth;
                    if (depth == 1)
                    {
                        if (rows == 0)
                        {
                            columns = current_column;
                        }
                        ++rows;
                    }
                    if (depth == 0)
                    {
                        break;
                    }
                    ++_position;
                }
                else if (c == ',')
                {
                    ++_position;
                }
                else if (c == '-' || c == '.' || is_digit(c))
                {
                    double value = parse_double();
                    flat.push_back(value);
                    ++current_column;
                }
                else
                {
                    throw std::runtime_error("Неожиданный символ в JSON: " + std::string(1, c));
                }
            }

            if (rows == 0 || columns == 0)
            {
                throw std::runtime_error("Пустая матрица");
            }

            matrix::Matrix result(rows, columns);
            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns; ++column)
                {
                    result(row, column) = flat[row * columns + column];
                }
            }

            return result;
        }

        static std::string matrix_to_json(std::string_view key, const matrix::Matrix &m)
        {
            std::ostringstream oss;
            oss << "  \"" << key << "\": [\n";

            for (std::uint32_t row = 0; row < m.get_rows(); ++row)
            {
                oss << "    [";
                for (std::uint32_t column = 0; column < m.get_columns(); ++column)
                {
                    oss << m(row, column);
                    if (column + 1 < m.get_columns())
                    {
                        oss << ", ";
                    }
                }
                oss << "]";
                if (row + 1 < m.get_rows())
                {
                    oss << ",";
                }
                oss << "\n";
            }
            oss << "  ]";

            return oss.str();
        }
    };

    inline matrix::Matrix json_to_matrix(std::string_view json_string, std::string_view key)
    {
        MatrixParser parser(json_string);
        return parser.parse_matrix(key);
    }

    inline std::string matrix_to_json_string(std::string_view key, const matrix::Matrix &m)
    {
        return MatrixParser::matrix_to_json(key, m);
    }
}