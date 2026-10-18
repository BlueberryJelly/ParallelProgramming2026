#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "matrix.hpp"

namespace json_compat
{
    namespace detail
    {
        inline bool is_whitespace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        inline bool is_number_start(char c)
        {
            return c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9');
        }
    }

    inline matrix::Matrix json_to_matrix(const std::string &json_string, const std::string &key)
    {
        const std::string search_key = "\"" + key + "\"";
        const std::size_t key_position = json_string.find(search_key);
        if (key_position == std::string::npos)
        {
            throw std::runtime_error("Ключ не найден в JSON: " + key);
        }

        std::size_t position = json_string.find('[', key_position);
        if (position == std::string::npos)
        {
            throw std::runtime_error("Не найден массив для ключа: " + key);
        }

        const char *const begin = json_string.c_str();
        const std::size_t length = json_string.size();

        std::vector<double> flat;
        std::uint32_t rows = 0;
        std::uint32_t columns = 0;
        std::uint32_t current_column = 0;
        int depth = 0;

        while (position < length)
        {
            const char c = begin[position];

            if (detail::is_whitespace(c) || c == ',')
            {
                ++position;
            }
            else if (c == '[')
            {
                ++depth;
                if (depth == 2)
                {
                    current_column = 0;
                }
                ++position;
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
                        std::ostringstream message;
                        message << "Матрица не прямоугольная: строка " << rows << " содержит "
                                << current_column << " элементов, ожидалось " << columns;
                        throw std::runtime_error(message.str());
                    }
                    ++rows;
                }
                ++position;
                if (depth == 0)
                {
                    break;
                }
            }
            else if (detail::is_number_start(c))
            {
                char *end = 0;
                const double value = std::strtod(begin + position, &end);
                if (end == begin + position)
                {
                    std::ostringstream message;
                    message << "Ошибка парсинга числа в JSON на позиции " << position;
                    throw std::runtime_error(message.str());
                }
                flat.push_back(value);
                ++current_column;
                position = static_cast<std::size_t>(end - begin);
            }
            else
            {
                std::ostringstream message;
                message << "Неожиданный символ в JSON на позиции " << position << ": '" << c << "'";
                throw std::runtime_error(message.str());
            }
        }

        if (rows == 0 || columns == 0)
        {
            throw std::runtime_error("Пустая или некорректная матрица для ключа: " + key);
        }

        matrix::Matrix result(rows, columns);
        double *out = result.data();
        for (std::size_t i = 0; i < flat.size(); ++i)
        {
            out[i] = flat[i];
        }
        return result;
    }

    inline std::string matrix_to_json_string(const std::string &key, const matrix::Matrix &m)
    {
        std::string out;
        out.reserve(static_cast<std::size_t>(m.get_rows()) * m.get_columns() * 26 + 64);
        out += "  \"" + key + "\": [\n";

        char buffer[32];
        for (std::uint32_t row = 0; row < m.get_rows(); ++row)
        {
            out += "    [";
            for (std::uint32_t column = 0; column < m.get_columns(); ++column)
            {
                std::snprintf(buffer, sizeof(buffer), "%.17g", m(row, column));
                out += buffer;
                if (column + 1 < m.get_columns())
                {
                    out += ", ";
                }
            }
            out += "]";
            if (row + 1 < m.get_rows())
            {
                out += ",";
            }
            out += "\n";
        }
        out += "  ]";
        return out;
    }

    inline std::string read_text_file(const std::string &path)
    {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Не удалось открыть файл для чтения: " + path);
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    inline void write_text_file(const std::string &path, const std::string &content)
    {
        std::ofstream file(path.c_str(), std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Не удалось открыть файл для записи: " + path);
        }
        file << content;
    }
}
