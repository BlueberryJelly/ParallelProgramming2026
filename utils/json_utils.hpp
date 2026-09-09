#pragma once

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "matrix.hpp"

namespace json_utils
{
    namespace detail
    {
        constexpr bool is_whitespace(char c) noexcept
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        constexpr bool is_digit(char c) noexcept
        {
            return c >= '0' && c <= '9';
        }

        constexpr bool is_number_start(char c) noexcept
        {
            return c == '-' || c == '+' || c == '.' || is_digit(c);
        }
    }

    class MatrixReader
    {
    private:
        std::string_view _json_string;
        std::size_t _position = 0;

        void skip_whitespace() noexcept
        {
            while (_position < _json_string.size() && detail::is_whitespace(_json_string[_position]))
            {
                ++_position;
            }
        }

        double parse_double()
        {
            double value = 0.0;
            const auto result = std::from_chars(
                _json_string.data() + _position,
                _json_string.data() + _json_string.size(),
                value);

            if (result.ec != std::errc{})
            {
                throw std::runtime_error("Ошибка парсинга числа в JSON на позиции " + std::to_string(_position));
            }

            _position = static_cast<std::size_t>(result.ptr - _json_string.data());
            return value;
        }

        [[nodiscard]]
        std::size_t find_key(std::string_view key) const
        {
            std::string search_key;
            search_key.reserve(key.size() + 2);
            search_key += '"';
            search_key += key;
            search_key += '"';

            const std::size_t position = _json_string.find(search_key);
            if (position == std::string_view::npos)
            {
                throw std::runtime_error("Ключ не найден в JSON: " + std::string(key));
            }
            return position;
        }

        [[nodiscard]]
        std::size_t find_array_start(std::size_t key_position) const
        {
            const std::size_t position = _json_string.find('[', key_position);
            if (position == std::string_view::npos)
            {
                throw std::runtime_error("Не найден массив для ключа (позиция " + std::to_string(key_position) + ")");
            }
            return position;
        }

    public:
        explicit MatrixReader(std::string_view json_string)
            : _json_string(json_string) {}

        matrix::Matrix parse_matrix(std::string_view key)
        {
            _position = find_array_start(find_key(key));

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

                const char c = _json_string[_position];

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
                    if (depth < 0)
                    {
                        throw std::runtime_error("Некорректный JSON: лишняя закрывающая скобка ']'");
                    }

                    if (depth == 1)
                    {
                        if (rows == 0)
                        {
                            columns = current_column;
                        }
                        else if (current_column != columns)
                        {
                            throw std::runtime_error(
                                "Матрица не прямоугольная: строка " + std::to_string(rows) +
                                " содержит " + std::to_string(current_column) +
                                " элементов, ожидалось " + std::to_string(columns));
                        }
                        ++rows;
                    }

                    ++_position;
                    if (depth == 0)
                    {
                        break;
                    }
                }
                else if (c == ',')
                {
                    ++_position;
                }
                else if (detail::is_number_start(c))
                {
                    flat.push_back(parse_double());
                    ++current_column;
                }
                else
                {
                    throw std::runtime_error(
                        "Неожиданный символ в JSON на позиции " + std::to_string(_position) +
                        ": '" + std::string(1, c) + "'");
                }
            }

            if (rows == 0 || columns == 0)
            {
                throw std::runtime_error("Пустая или некорректная матрица для ключа: " + std::string(key));
            }

            matrix::Matrix result(rows, columns);
            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns; ++column)
                {
                    result(row, column) = flat[static_cast<std::size_t>(row) * columns + column];
                }
            }

            return result;
        }
    };

    inline std::string matrix_to_json_string(std::string_view key, const matrix::Matrix &m)
    {
        std::ostringstream oss;
        oss << std::setprecision(17);
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

    inline matrix::Matrix json_to_matrix(std::string_view json_string, std::string_view key)
    {
        MatrixReader reader(json_string);
        return reader.parse_matrix(key);
    }

    inline std::string read_text_file(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Не удалось открыть файл для чтения: " + path.string());
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    inline void write_text_file(const std::filesystem::path &path, std::string_view content)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Не удалось открыть файл для записи: " + path.string());
        }
        file << content;
    }
}