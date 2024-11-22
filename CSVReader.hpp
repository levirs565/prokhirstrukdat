#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cctype>

inline std::string ReadCell(std::ifstream &stream)
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
        if (!insideQuote && (stream.peek() == ',' || stream.peek() == '\n'))
            break;

        ch = stream.get();

        if (ch == '\r')
            continue;

        if (startWithQuote && !insideQuote)
            throw std::domain_error("Illegal character after double quote");

        if (ch == '"')
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

inline void ReadRow(std::ifstream &stream, std::vector<std::string> &res)
{
    while (!stream.eof())
    {
        res.push_back(ReadCell(stream));

        char ch = stream.get();

        if (ch == '\n')
            break;
    }
}

inline void ReadCSV(
    const std::string &filename,
    const std::function<void(std::map<std::string, std::string>)> onLine
)
{
    std::ifstream stream(filename);

    if (!stream)
        throw std::domain_error("File not found");

    std::vector<std::string> headerCells;

    ReadRow(stream, headerCells);

    std::vector<std::string> cells;
    std::map<std::string, std::string> obj;

    while (!stream.eof())
    {
        ReadRow(stream, cells);
        
        if (cells.size() != headerCells.size())
            throw std::domain_error("Row cell count mismatch header cell count");

        for (size_t i = 0; i < cells.size(); i++) {
            obj[headerCells[i]] = cells[i];
        }

        onLine(obj);

        cells.clear();
        obj.clear();
    }
}