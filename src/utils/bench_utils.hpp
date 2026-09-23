#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "matrix.hpp"
#include "timer.hpp"

namespace bench
{
    inline int parse_positive(const char *value, const char *what)
    {
        const int parsed = std::stoi(value);
        if (parsed < 1)
        {
            throw std::invalid_argument(std::string(what) + " должно быть >= 1");
        }
        return parsed;
    }

    struct Measurement
    {
        matrix::Matrix result;
        double elapsed_seconds = 0.0;
    };

    template <typename Multiply>
    Measurement measure(int repeats, Multiply &&multiply)
    {
        Measurement measurement;
        std::vector<double> times;
        times.reserve(static_cast<std::size_t>(repeats));

        util::Timer timer;
        for (int r = 0; r < repeats; ++r)
        {
            timer.start();
            measurement.result = multiply();
            timer.stop();
            times.push_back(timer.elapsed_seconds());
        }

        std::sort(times.begin(), times.end());
        const std::size_t mid = times.size() / 2;
        measurement.elapsed_seconds = times.size() % 2 == 1 ? times[mid] : (times[mid - 1] + times[mid]) / 2.0;
        return measurement;
    }
}