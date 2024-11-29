#pragma once
#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <CommCtrl.h>
#include <stdexcept>
#include <sstream>

namespace Winapi
{
    // std::string _getErrorString(const std::string& what) {
    //     std::stringstream stream;
    //     stream << what <<  ". Error code : " << GetLastError();
    //     return stream.str();
    // }

    struct Error : std::runtime_error
    {

        Error(const std::string& what) :  std::runtime_error(what + ". Error code : " + std::to_string(GetLastError()))
        {
        }
    };
}