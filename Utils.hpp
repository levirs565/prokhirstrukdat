#pragma once
#include <string>
#include <cwctype>
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

    // Urutan karakter AaBbCcDd
    int CompareWStringHalfInsensitive(const std::wstring& a, const std::wstring& b) {
        size_t mx = std::max(a.size(), b.size());

        for (size_t i = 0; i < mx; i++) {
            if (a[i] == b[i]) continue;

            wchar_t ai = std::towlower(a[i]);
            wchar_t bi = std::towlower(b[i]);

            if (ai == bi) {
                return b[i] == bi ? -1 : 1;
            }

            return ai < bi ? -1 : 1;
        }

        if (a.size() == b.size()) return 0;

        return a.size() < b.size() ? -1 : 1;
    }

    void CopyToClipboard(const std::wstring& data) {
        size_t sz = sizeof(wchar_t) * (data.length() + 1);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
        if (hMem == 0) throw Winapi::Error("Error at GlobalAlloc");

        LPVOID hLock = GlobalLock(hMem);
        memcpy(hLock, data.c_str(), sz);
        GlobalUnlock(hLock);

        if (!OpenClipboard(0)) throw Winapi::Error("Error at OpenClipboard");

        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
}