#pragma once

#include <chrono>

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
};