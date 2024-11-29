#pragma once
#include <string>
#include <locale>
#include <codecvt>

namespace Utils
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

    std::wstring stringviewToWstring(const std::string_view &view)
    {
        return converter.from_bytes(view.data(), view.data() + view.size());
    }
}