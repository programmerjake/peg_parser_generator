/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "source.h"
#include "error.h"
#include "location.h"
#include "arena.h"
#include <cstdio>
#include <memory>
#include <algorithm>

std::vector<std::size_t> Source::calculateLineStartingPositions(const std::string &text)
{
    std::size_t lineStartingPositionCount = 0;
    for(std::size_t i = 0; i < text.size(); i++)
    {
        if(text[i] == '\r')
        {
            lineStartingPositionCount++;
            if(i + 1 < text.size() && text[i + 1] == '\n')
                i++;
            continue;
        }
        if(text[i] == '\n')
            lineStartingPositionCount++;
    }
    std::vector<std::size_t> lineStartingPositions;
    lineStartingPositions.reserve(lineStartingPositionCount);
    for(std::size_t i = 0; i < text.size(); i++)
    {
        if(text[i] == '\r')
        {
            if(i + 1 < text.size() && text[i + 1] == '\n')
                i++;
            lineStartingPositions.push_back(i + 1);
            continue;
        }
        if(text[i] == '\n')
            lineStartingPositions.push_back(i + 1);
    }
    return lineStartingPositions;
}

std::string Source::getLocationString(std::size_t position) const
{
    std::ostringstream ss;
    writeLocation(ss, position);
    return ss.str();
}

void Source::writeLocation(std::ostream &os, std::size_t position) const
{
    auto lineAndColumn = translateLocation(position);
    os << fileName;
    os << ":";
    os << lineAndColumn.line;
    os << ":";
    os << lineAndColumn.column;
}

Source::LineAndColumn Source::translateLocation(std::size_t position) const noexcept
{
    std::size_t line =
        1 + lineStartingPositions.size()
        + (lineStartingPositions.rbegin() - std::lower_bound(lineStartingPositions.rbegin(),
                                                             lineStartingPositions.rend(),
                                                             position,
                                                             std::greater<std::size_t>()));
    std::size_t column = line <= 1 ? position + 1 : position - lineStartingPositions[line - 2] + 1;
    return LineAndColumn(line, column);
}

const Source *Source::load(Arena &arena, ErrorHandler &errorHandler, std::string fileName)
{
    struct FileDeleter final
    {
        void operator()(std::FILE *f) const
        {
            if(f && f != stdin)
                std::fclose(f);
        }
    };
    std::unique_ptr<FILE, FileDeleter> file(fileName == "-" ? stdin :
                                                              std::fopen(fileName.c_str(), "rb"));
    if(!file)
    {
        errorHandler(ErrorLevel::FatalError,
                     Location(arena.make<Source>(std::move(fileName), ""), 0),
                     "can't open file");
        return nullptr;
    }
    std::string content;
    std::size_t contentLength = 0;
    constexpr std::size_t minChunkSize = 0x1000;
    while(true)
    {
        content.resize(contentLength + minChunkSize);
        content.resize(content.capacity());
        contentLength += std::fread(static_cast<void *>(&content[contentLength]),
                                    sizeof(content[0]),
                                    content.size() - contentLength,
                                    file.get());
        if(std::ferror(file.get()))
        {
            errorHandler(ErrorLevel::FatalError,
                         Location(arena.make<Source>(std::move(fileName), ""), 0),
                         "file read error");
            return nullptr;
        }
        if(std::feof(file.get()))
            break;
    }
    file.reset();
    content.resize(contentLength);
    return arena.make<Source>(std::move(fileName), std::move(content));
}
