#pragma once

#include <chrono>

namespace util
{
    class Timer final
    {
    private:
        std::chrono::steady_clock::time_point _start{};
        std::chrono::steady_clock::time_point _stop{};

    public:
        void start() noexcept
        {
            _start = std::chrono::steady_clock::now();
        }

        void stop() noexcept
        {
            _stop = std::chrono::steady_clock::now();
        }

        double elapsed_seconds() const noexcept
        {
            return std::chrono::duration<double>(_stop - _start).count();
        }
    };
}