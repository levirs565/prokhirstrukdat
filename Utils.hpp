#pragma once
#include <string>
#include <cwctype>
#include "Winapi.hpp"
#include <chrono>

namespace Utils
{
    struct StringView
    {
        char *begin;
        size_t size;

        StringView()
        {
            begin = NULL;
            size = 0;
        }

        StringView(char *begin, size_t size)
        {
            this->begin = begin;
            this->size = size;
        }
    };

    std::wstring stringviewToWstring(const StringView &view)
    {
        if (view.size == 0)
            return L"";
        const size_t bufferSize = MultiByteToWideChar(CP_UTF8, 0, view.begin, static_cast<int>(view.size), NULL, 0);

        if (bufferSize <= 0)
            throw Winapi::Error("MultiByteToWideChar fail.");

        std::wstring buffer(bufferSize, 0);
        MultiByteToWideChar(CP_UTF8, 0, view.begin, static_cast<int>(view.size), const_cast<wchar_t *>(buffer.data()), bufferSize);
        return buffer;
    }

    // Urutan karakter AaBbCcDd
    int CompareWStringHalfInsensitive(const std::wstring &a, const std::wstring &b)
    {
        size_t mx = std::max(a.size(), b.size());

        for (size_t i = 0; i < mx; i++)
        {
            if (a[i] == b[i])
                continue;

            wchar_t ai = std::towlower(a[i]);
            wchar_t bi = std::towlower(b[i]);

            if (ai == bi)
            {
                return b[i] == bi ? -1 : 1;
            }

            return ai < bi ? -1 : 1;
        }

        if (a.size() == b.size())
            return 0;

        return a.size() < b.size() ? -1 : 1;
    }

    void CopyToClipboard(const std::wstring &data)
    {
        size_t sz = sizeof(wchar_t) * (data.length() + 1);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
        if (hMem == 0)
            throw Winapi::Error("Error at GlobalAlloc");

        LPVOID hLock = GlobalLock(hMem);
        memcpy(hLock, data.c_str(), sz);
        GlobalUnlock(hLock);

        if (!OpenClipboard(0))
            throw Winapi::Error("Error at OpenClipboard");

        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }

    std::wstring GetExecutableDirectory()
    {
        std::wstring buffer(4096, 0);
        DWORD lastIndex = GetModuleFileNameW(nullptr, const_cast<wchar_t*>(buffer.data()), 4096);

        while (buffer[lastIndex] != L'/' && buffer[lastIndex] != L'\\')
            lastIndex--;

        buffer.resize(lastIndex + 1);
        return buffer;
    }

    std::wstring SystemTimeToDateStr(const SYSTEMTIME& in) {
        return std::to_wstring(in.wDay) + L"/" + std::to_wstring(in.wMonth) + L"/" + std::to_wstring(in.wYear);
    }

    SYSTEMTIME DateStrToSystemTime(const std::wstring& in) {
        size_t firstSlash = in.find_first_of(L'/');
        if (firstSlash == std::wstring::npos) throw std::domain_error("first slash not found");
        if (firstSlash + 1 == in.size()) throw std::domain_error("first slash in end");
        size_t secondSlash = in.find_first_of(L'/', firstSlash + 1);
        if (secondSlash == std::wstring::npos) throw std::domain_error("second slash not found");
        if (secondSlash + 1 == in.size()) throw std::domain_error("second slash in end");

        SYSTEMTIME res{0};
        res.wDay = std::stoi(in.substr(0, firstSlash));
        res.wMonth = std::stoi(in.substr(firstSlash + 1, secondSlash - firstSlash - 1));
        res.wYear = std::stoi(in.substr(secondSlash + 1));
        return res;
    }

    std::chrono::nanoseconds GetSystemDateDifferenceNanos(const SYSTEMTIME a, const SYSTEMTIME& b) {
        FILETIME fa, fb;
        if (!SystemTimeToFileTime(&a, &fa)) throw Winapi::Error("Convert first systemtime fail");
        if (!SystemTimeToFileTime(&b, &fb)) throw Winapi::Error("Convert second systemtime fail");

        ULARGE_INTEGER ua, ub;

        ua.LowPart = fa.dwLowDateTime;
        ua.HighPart = fa.dwHighDateTime;
    
        ub.LowPart = fb.dwLowDateTime;
        ub.HighPart = fb.dwHighDateTime;

        LONGLONG diff = static_cast<LONGLONG>(ua.QuadPart) - static_cast<LONGLONG>(ub.QuadPart);
        diff *= 100;
        return std::chrono::nanoseconds(std::abs(diff));
    }

    int GetSystemDateDifferenceDays(const SYSTEMTIME& a, const SYSTEMTIME& b) {
        return std::chrono::duration_cast<std::chrono::hours>(GetSystemDateDifferenceNanos(a, b)).count() / 24;
    }
}