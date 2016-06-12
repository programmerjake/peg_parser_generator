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

#ifndef AST_TERMINAL_H_
#define AST_TERMINAL_H_

#include "expression.h"
#include "visitor.h"
#include <utility>
#include <vector>
#include <algorithm>

namespace ast
{
struct Terminal final : public Expression
{
    char32_t value;
    Terminal(Location location, char32_t value) : Expression(std::move(location)), value(value)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTerminal(this);
    }
};

struct CharacterClass final : public Expression
{
    struct CharacterRange final
    {
        char32_t min, max;
        constexpr CharacterRange(char32_t min, char32_t max) : min(min), max(max)
        {
        }
        explicit constexpr CharacterRange(char32_t value) : min(value), max(value)
        {
        }
        constexpr CharacterRange() : min(U'1'), max(U'0')
        {
        }
        constexpr bool empty() const
        {
            return max < min;
        }
        constexpr bool overlaps(const CharacterRange &rt) const
        {
            return min <= rt.max && max >= rt.min;
        }
    };
    struct CharacterRanges final
    {
        std::vector<CharacterRange> ranges;
        std::size_t getSearchStartIndex(char32_t value) const noexcept
        {
            return std::lower_bound(ranges.begin(),
                                    ranges.end(),
                                    value,
                                    [](const CharacterRange &a, char32_t b)
                                    {
                                        return a.min < b;
                                    }) - ranges.begin();
        }
        bool contains(char32_t value) const noexcept
        {
            std::size_t searchStartIndex = getSearchStartIndex(value);
            if(searchStartIndex >= ranges.size())
                return false;
            return value >= ranges[searchStartIndex].min && value <= ranges[searchStartIndex].max;
        }
        bool overlaps(const CharacterRange &range) const noexcept
        {
            if(range.empty())
                return false;
            std::size_t searchStartIndex = getSearchStartIndex(range.min);
            if(searchStartIndex >= ranges.size())
                return false;
            return ranges[searchStartIndex].overlaps(range);
        }
        bool insert(const CharacterRange &range)
        {
            if(range.empty())
                return false;
            std::size_t searchStartIndex = getSearchStartIndex(range.min);
            if(searchStartIndex < ranges.size() && ranges[searchStartIndex].overlaps(range))
                return false;
            ranges.insert(ranges.begin() + searchStartIndex, range);
            return true;
        }
    };
    CharacterRanges characterRanges;
    bool inverted;
    CharacterClass(Location location, CharacterRanges characterRanges, bool inverted)
        : Expression(std::move(location)),
          characterRanges(std::move(characterRanges)),
          inverted(inverted)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitCharacterClass(this);
    }
};

struct EOFTerminal final : public Expression
{
    using Expression::Expression;
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitEOFTerminal(this);
    }
};
}

#endif /* AST_TERMINAL_H_ */
