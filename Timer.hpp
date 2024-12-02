#pragma once

#include <chrono>
#include <sstream>
#include <cmath>

struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> _start, _end;
    std::chrono::duration<float> duration;

    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }

    void end() {
        _end = std::chrono::high_resolution_clock::now();

        duration = _end - _start;
    }

    float durationMs() {
        return duration.count() * 1000.0f;
    }

    std::wstring durationStr() {
        float sf = duration.count();
        uint64_t ms = uint64_t(sf * 1000.0f) % 1000;
        uint64_t s = uint64_t(std::floor(sf)) % 60;
        uint64_t m = uint64_t(std::floor(sf / 60.0f));

        std::wstringstream stream;
        if (m > 0) {
            stream << m << L" minute ";
        }

        stream << s << L" s " << ms << L" ms";

        return stream.str();
    }
};