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
    std::vector<std::string> header, data;
    std::string filename;
    std::ifstream stream;
    char separator;

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

        while (!stream.eof())
        {
            header.push_back(readCell());

            char ch = stream.get();

            if (ch == '\n')
                break;
        }

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

        while (!stream.eof())
        {
            if (i >= data.size())
                throw std::domain_error("Data length exceed header length");

            data[i] = readCell();
            i++;

            char ch = stream.get();

            if (ch == '\n')
                break;
        }

        if (i == 0 || (i == 1 && data[0] == ""))
            return false;

        if (i < data.size())
            throw std::domain_error("Data size below header size " + std::to_string(i));

        return true;
    }

    std::string readCell()
    {
        std::string result;

        char ch = stream.get();

        if (ch == std::char_traits<char>::eof())
            return "";

        bool startWithQuote = ch == '"';
        bool insideQuote = startWithQuote;

        if (!insideQuote)
            result.push_back(ch);

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
                
                result.push_back(ch);
            }
            else if (ch == '"')
            {
                if (!insideQuote)
                    throw std::domain_error("Field not enclosed with double quoted cannot have double qoute");

                if (!stream.eof() && stream.peek() == '"')
                {
                    ch = stream.get();
                    result.push_back(ch);
                }
                else
                {
                    insideQuote = false;
                }
            }
            else
                result.push_back(ch);
        }

        if (startWithQuote && insideQuote)
            throw std::domain_error("Quoted field not closed");

        return result;
    }
};