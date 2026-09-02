export module utils:in_out_data;

import std;

export class SquareMatrix final
{
private:
    std::size_t _res = 0;
    std::vector<int> _matrix;

public:
    SquareMatrix() = default;

    SquareMatrix(std::size_t res, std::vector<int> matrix) 
        : _res(res), _matrix(std::move(matrix)) {}

    SquareMatrix operator*(const SquareMatrix& other) const
    {
        if (_res != other._res)
        {
            throw std::logic_error("Матрицы имеют разные размерности.");
        }

        std::vector<int> result(_res * _res, 0);

        for (std::size_t i = 0; i < _res; ++i) 
        {
            for (std::size_t j = 0; j < _res; ++j) 
            {
                int sum = 0;
                for (std::size_t k = 0; k < _res; ++k) 
                {
                    sum += _matrix[i * _res + k] * other._matrix[k * _res + j];
                }
                result[i * _res + j] = sum;
            }
        }

        return SquareMatrix(_res, std::move(result));
    }

    std::size_t get_size() const
    {
        return _res;
    }
};