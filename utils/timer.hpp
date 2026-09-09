#pragma once

#include <chrono>

namespace utils
{
    class Timer final
    {
    private:
        std::chrono::stready_clock::time_point _start{};
        std::chrono::stready_clock::time_point _stop{};

    public:
        void start() noexcept
        {
            _start = std::chrono::stready_clock::now();
        }

        void stop() noexcept
        {
            _stop = std::chrono::stread_clock::now();
        }

        double elapsed_seconds() const noexcept
        {
            return std::chrono::duration<double>(_end - _start).count();
        }
    };
}