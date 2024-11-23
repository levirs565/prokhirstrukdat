#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cctype>
#include <algorithm>

struct CSVReader
{
    std::vector<std::string> header;
    std::vector<std::vector<char>> dataBuffer;
    std::vector<std::string_view> data;
    std::string filename;
    std::ifstream stream;
    char separator;

    const size_t defaultBufferSize = 8 * 1024;

    CSVReader(const std::string &filename, char separator)
    {
        this->filename = filename;
        this->separator = separator;
    }

    void startRead()
    {
        stream.open(filename);

        if (!stream)
            throw std::domain_error("File not found");
        
        std::vector<char> buffer(defaultBufferSize);
        size_t filledSize;

        while (!stream.eof())
        {
            readCell(buffer, filledSize);
            header.push_back(std::string(buffer.data(), filledSize));

            char ch = stream.get();

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

        while (!stream.eof())
        {
            if (i >= header.size())
                throw std::domain_error("Data length exceed header length");

            readCell(dataBuffer[i], filledSize);
            data[i] = std::string_view(dataBuffer[i].data(), filledSize);
            i++;

            char ch = stream.get();

            if (ch == '\n')
                break;
        }

        if (i == 0 || (i == 1 && data[0].size() == 0))
            return false;

        if (i < data.size())
            throw std::domain_error("Data size below header size " + std::to_string(i));

        return true;
    }

    void readCell(std::vector<char>& buffer, size_t& size)
    {
        size = 0;
        char ch = stream.get();

        if (ch == std::char_traits<char>::eof())
            return;

        bool startWithQuote = ch == '"';
        bool insideQuote = startWithQuote;

        if (!insideQuote)
            buffer.at(size++) = ch;

        while (stream.peek() != std::char_traits<char>::eof())
        {
            if (!insideQuote && (stream.peek() == separator || stream.peek() == '\n'))
                break;

            ch = stream.get();

            if (ch == '\r')
                continue;

            if (startWithQuote && !insideQuote)
                throw std::domain_error("Illegal character after double quote " + std::string{ch});

            if (ch == '\\')
            {
                ch = stream.get();

                if (ch == std::char_traits<char>::eof())
                    throw std::domain_error("Espacing in end of file is illegal");
                
                buffer.at(size++) = ch;
            }
            else if (ch == '"')
            {
                if (!insideQuote)
                    throw std::domain_error("Field not enclosed with double quoted cannot have double qoute");

                if (!stream.eof() && stream.peek() == '"')
                {
                    ch = stream.get();
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