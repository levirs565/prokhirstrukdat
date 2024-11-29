#pragma once
#include <string>
#include "Winapi.hpp"

namespace Utils
{
    struct StringView {
        char* begin;
        size_t size;

        StringView() {
            begin = NULL;
            size = 0;
        }

        StringView(char* begin, size_t size) {
            this->begin = begin;
            this->size = size;
        }
    };

    std::wstring stringviewToWstring(const StringView &view)
    {   if (view.size == 0) return L"";
        const size_t bufferSize = MultiByteToWideChar(CP_UTF8, 0, view.begin, static_cast<int>(view.size), NULL, 0);

        if (bufferSize <= 0) throw Winapi::Error("MultiByteToWideChar fail.");

        std::wstring buffer(bufferSize, 0);
        MultiByteToWideChar(CP_UTF8, 0, view.begin, static_cast<int>(view.size), const_cast<wchar_t*>(buffer.data()), bufferSize);
        return buffer;
    }
}