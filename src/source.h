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

#ifndef SOURCE_H_
#define SOURCE_H_

#include <string>
#include <vector>
#include <iosfwd>

struct ErrorHandler;
struct Arena;

struct Source final
{
    std::string fileName;
    std::string contents;
    std::vector<std::size_t> lineStartingPositions;
    Source(std::string fileName, std::string contents)
        : fileName(std::move(fileName)),
          contents(std::move(contents)),
          lineStartingPositions(calculateLineStartingPositions(this->contents))
    {
    }
    static std::vector<std::size_t> calculateLineStartingPositions(const std::string &text);
    std::string getLocationString(std::size_t position) const;
    void writeLocation(std::ostream &os, std::size_t position) const;
    struct LineAndColumn final
    {
        std::size_t line;
        std::size_t column;
        LineAndColumn(std::size_t line, std::size_t column) : line(line), column(column)
        {
        }
    };
    LineAndColumn translateLocation(std::size_t position) const noexcept;
    static const Source *load(Arena &arena, ErrorHandler &errorHandler, std::string fileName);
};

#endif /* SOURCE_H_ */
