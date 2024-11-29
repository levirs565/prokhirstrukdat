#pragma once
#include <string>
#include <locale>
#include <codecvt>

namespace Utils
{
    struct StringView {
        char* begin;
        size_t size;

        StringView() {
            begin = nullptr;
            size = 0;
        }

        StringView(char* begin, size_t size) {
            this->begin = begin;
            this->size = size;
        }
    };

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

    std::wstring stringviewToWstring(const StringView &view)
    {
        return converter.from_bytes(view.begin, view.begin + view.size);
    }
}