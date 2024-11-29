#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <fstream>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cctype>
#include <algorithm>
// #include <thread>
// #include <mutex>
#include <condition_variable>
#include <Windows.h>

constexpr size_t sCSVReaderIOBuffSize = 256 * 1024;

struct CSVReaderIOWinFMAP
{
    HANDLE hFile, hMap;
    const char *buffer;
    size_t bufferIndex = 0;

    void open(const std::string &filename)
    {
        hFile = CreateFileA(filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if (!hFile)
        {
            throw std::domain_error("Error during CreateFileA. Code: " + std::to_string(GetLastError()));
        }

        hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

        if (!hMap)
        {
            throw std::domain_error("Error during CreateFileMappingA. Code: " + std::to_string(GetLastError()));
        }

        buffer = reinterpret_cast<const char *>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));

        if (!buffer)
        {
            throw std::domain_error("Error during MapViewOfFile. Code: " + std::to_string(GetLastError()));
        }
    }

    ~CSVReaderIOWinFMAP()
    {
        if (buffer)
        {
            UnmapViewOfFile(reinterpret_cast<LPCVOID>(buffer));
        }

        if (hMap)
        {
            CloseHandle(hMap);
        }

        if (hFile)
        {
            CloseHandle(hFile);
        }
    }

    char get()
    {
        if (eof())
        {
            return std::char_traits<char>::eof();
        }
        return buffer[bufferIndex];
    }

    void next()
    {
        if (!eof())
            bufferIndex++;
    }

    bool eof()
    {
        return buffer[bufferIndex] == '\0';
    }
};

// struct CSVReaderIOBuffAsync
// {
//     size_t bufferIndex = 0;
//     bool isEOf = false;
//     size_t bufferSize = 0;
//     std::vector<char> buffer = std::vector<char>(sCSVReaderIOBuffSize);

//     size_t nextBufferSize = 0;
//     std::vector<char> nextBuffer = std::vector<char>(sCSVReaderIOBuffSize);

//     std::thread worker;
//     bool needRead = true;
//     bool terminated = false;
//     std::mutex mutex;
//     std::condition_variable cv;

//     ~CSVReaderIOBuffAsync()
//     {
//         {
//             std::lock_guard lk(mutex);
//             terminated = true;
//             cv.notify_one();
//         }
//         worker.join();
//     }

//     void open(const std::string &filename)
//     {
//         worker = std::thread([&]()
//                              {
//             std::ifstream stream;

//             stream.open(filename);

//             if (!stream)
//                 throw std::domain_error("File not found"); 
            

//             while (true) {
//                 // std::cout << "Wait Action" << std::endl;
//                 std::unique_lock lk(mutex); 
//                 cv.wait(lk, [&]() {
//                     return needRead || terminated;
//                 });

//                 if (terminated) return;

//                 stream.read(nextBuffer.data(), sCSVReaderIOBuffSize);
//                 int readed = stream.gcount();
//                 nextBufferSize = readed;

//                 needRead = false;
//                 lk.unlock();
//                 cv.notify_one();

//                 if (readed == 0) {
//                     break;
//                 }
//             } });

//         next();
//     }

//     char get()
//     {
//         if (isEOf || bufferIndex == bufferSize)
//         {
//             isEOf = true;
//             return std::char_traits<char>::eof();
//         }
//         return buffer[bufferIndex];
//     }

//     void next()
//     {
//         bufferIndex++;
//         if (bufferIndex >= bufferSize && !isEOf)
//         {
//             {
//                 std::unique_lock lk(mutex);
//                 cv.wait(lk, [&]()
//                         { return !needRead; });

//                 std::swap(nextBuffer, buffer);
//                 std::swap(nextBufferSize, bufferSize);
//                 needRead = true;
//             }

//             cv.notify_one();
//             bufferIndex = 0;

//             if (bufferSize == 0)
//             {
//                 isEOf = true;
//             }
//         }
//     }

//     bool eof()
//     {
//         return isEOf;
//     }
// };

struct CSVReaderIOBuffSync
{
    std::ifstream stream;
    size_t bufferIndex = 0;
    size_t bufferSize = 0;
    std::vector<char> buffer = std::vector<char>(sCSVReaderIOBuffSize);
    bool isEOf = false;

    void open(const std::string &filename)
    {
        stream.open(filename);

        if (!stream)
            throw std::domain_error("File not found");

        fillBuffer();
    }

    void fillBuffer()
    {
        bufferIndex = 0;
        stream.read(buffer.data(), sCSVReaderIOBuffSize);
        bufferSize = stream.gcount();

        if (bufferSize == 0)
            isEOf = true;
    }

    char get()
    {
        if (isEOf || bufferIndex == bufferSize)
        {
            isEOf = true;
            return std::char_traits<char>::eof();
        }
        return buffer[bufferIndex];
    }

    void next()
    {
        bufferIndex++;
        if (bufferIndex == bufferSize)
        {
            fillBuffer();
        }
    }

    bool eof()
    {
        return isEOf;
    }
};

struct CSVReaderIOSync
{
    std::ifstream stream;

    void open(const std::string &filename)
    {
        stream.open(filename);

        if (!stream)
            throw std::domain_error("File not found");
    }

    char get()
    {
        return stream.peek();
    }

    void next()
    {
        stream.get();
    }

    bool eof()
    {
        return stream.eof();
    }
};

template <typename IO>
struct CSVReader
{
    std::vector<std::string> header;
    std::vector<std::vector<char>> dataBuffer;
    std::vector<std::string_view> data;
    std::string filename;
    IO io;

    char separator;

    const size_t defaultBufferSize = 8 * 1024;

    CSVReader(const std::string &filename, char separator)
    {
        this->filename = filename;
        this->separator = separator;
    }

    void startRead()
    {
        io.open(filename);

        std::vector<char> buffer(defaultBufferSize);
        size_t filledSize;

        while (!io.eof())
        {
            readCell(buffer, filledSize);
            header.push_back(std::string(buffer.data(), filledSize));

            char ch = io.get();
            io.next();

            if (ch == '\n')
                break;
        }

        dataBuffer = std::vector<std::vector<char>>(header.size(), std::vector<char>(defaultBufferSize));
        data.resize(header.size());
    }

    int findHeaderIndex(const std::string &name)
    {
        auto it = std::find(header.begin(), header.end(), name);

        if (it == header.end())
            return -1;

        return it - header.begin();
    }

    bool readData()
    {
        size_t i = 0;
        size_t filledSize;

        while (!io.eof())
        {
            if (i >= header.size())
                throw std::domain_error("Data length exceed header length");

            readCell(dataBuffer[i], filledSize);
            data[i] = std::string_view(dataBuffer[i].data(), filledSize);
            i++;

            char ch = io.get();
            io.next();

            if (ch == '\n')
                break;
        }

        if (i == 0 || (i == 1 && data[0].size() == 0))
            return false;

        if (i < data.size())
            throw std::domain_error("Data size below header size " + std::to_string(i));

        return true;
    }

    void readCell(std::vector<char> &buffer, size_t &size)
    {
        size = 0;
        char ch = io.get();
        io.next();

        if (ch == std::char_traits<char>::eof())
            return;

        bool startWithQuote = ch == '"';
        bool insideQuote = startWithQuote;

        if (!insideQuote)
            buffer.at(size++) = ch;

        while (io.get() != std::char_traits<char>::eof())
        {
            if (!insideQuote && (io.get() == separator || io.get() == '\n'))
                break;

            ch = io.get();
            io.next();

            if (ch == '\r')
                continue;

            if (startWithQuote && !insideQuote)
                throw std::domain_error("Illegal character after double quote " + std::string{ch});

            if (ch == '\\')
            {
                ch = io.get();
                io.next();

                if (ch == std::char_traits<char>::eof())
                    throw std::domain_error("Espacing in end of file is illegal");

                buffer.at(size++) = ch;
            }
            else if (ch == '"')
            {
                if (!insideQuote)
                    throw std::domain_error("Field not enclosed with double quoted cannot have double qoute");

                if (!io.eof() && io.get() == '"')
                {
                    ch = io.get();
                    io.next();
                    buffer.at(size++) = ch;
                }
                else
                {
                    insideQuote = false;
                }
            }
            else
                buffer.at(size++) = ch;
        }

        if (startWithQuote && insideQuote)
            throw std::domain_error("Quoted field not closed");
    }
};