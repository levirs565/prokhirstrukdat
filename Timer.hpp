#pragma once

#include <chrono>
#include <sstream>
#include <cmath>

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> _start, _end;
    std::chrono::nanoseconds duration;

    void start()
    {
        _start = std::chrono::high_resolution_clock::now();
    }

    void end()
    {
        _end = std::chrono::high_resolution_clock::now();

        duration = _end - _start;
    }

    std::wstring durationStr()
    {
        std::chrono::nanoseconds dur = duration;
        std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(dur);
        dur -= m;
        std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(dur);
        dur -= s;
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
        dur -= ms;

        std::wstringstream stream;
        if (m.count() == 0 && s.count() == 0)
        {
            std::chrono::microseconds micros = std::chrono::duration_cast<std::chrono::microseconds>(dur);
            dur -= micros;
            std::chrono::nanoseconds nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);

            stream << ms.count() << L" ms " << micros.count() << L" \x03BCs " << nanos.count() << " ns";
        }
        else
        {
            if (m.count() > 0)
            {
                stream << m.count() << L" minute ";
            }

            stream << s.count() << L" s " << ms.count() << L" ms";
        }

        return stream.str();
    }
};