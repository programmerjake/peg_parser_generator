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

#ifndef LOCATION_H_
#define LOCATION_H_

#include <iosfwd>
#include <cstddef>

struct Source;

struct Location final
{
    const Source *source;
    std::size_t position;
    Location(const Source *source, std::size_t position) : source(source), position(position)
    {
    }
    Location() : source(nullptr), position(0)
    {
    }
    explicit operator bool() const
    {
        return source;
    }
    bool operator!() const
    {
        return !source;
    }
    void write(std::ostream &os) const;
    friend std::ostream &operator <<(std::ostream &os, const Location &location)
    {
        location.write(os);
        return os;
    }
    std::size_t getLine() const;
    std::size_t getColumn() const;
};

#endif /* LOCATION_H_ */
