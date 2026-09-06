export module utils:in_out_matrix;

import std;

/*
std::expected<Matrix, std::string> read_matrix(std::string_view path)
{
    std::ifstream in(std::filesystem::path(path));
    if (!in)
    {
        return std::unexpected<std::string>(
            "Не удалось открыть файл для чтения" + std::string(path));
    }

    std::size_t n = 0;
    if (!(in >> n))
    {
        std::unexpected<std::string>("Не удалось прочитать размерность матрицы");
    }

    Matrix a(n)
}
*/