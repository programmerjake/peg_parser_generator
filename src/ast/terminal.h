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
#include <cstdint>

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
        template <typename Classifier>
        bool matchesClassifier(Classifier &&classifier) const
        {
            std::uint_fast32_t characterCount = 0;
            for(const auto &range : ranges)
            {
                // check max separately in case max is the maximum value for this type
                for(std::uint_fast32_t ch = range.min; ch < range.max; ch++)
                {
                    if(!classifier.matches(static_cast<char32_t>(ch)))
                        return false;
                    characterCount++;
                }
                // check max separately in case max is the maximum value for this type
                if(!classifier.matches(static_cast<char32_t>(range.max)))
                    return false;
                characterCount++;
            }
            if(characterCount == classifier.totalCharacterCount)
                return true;
            return false;
        }
        template <typename Classifier>
        bool containsClassifier(Classifier &&classifier) const
        {
            for(const auto &range : ranges)
            {
                // check max separately in case max is the maximum value for this type
                for(std::uint_fast32_t ch = range.min; ch < range.max; ch++)
                {
                    if(!classifier.matches(static_cast<char32_t>(ch)))
                        return false;
                }
                // check max separately in case max is the maximum value for this type
                if(!classifier.matches(static_cast<char32_t>(range.max)))
                    return false;
            }
            return true;
        }
        template <typename Classifier>
        bool excludesClassifier(Classifier &&classifier) const
        {
            for(const auto &range : ranges)
            {
                // check max separately in case max is the maximum value for this type
                for(std::uint_fast32_t ch = range.min; ch < range.max; ch++)
                {
                    if(classifier.matches(static_cast<char32_t>(ch)))
                        return false;
                }
                // check max separately in case max is the maximum value for this type
                if(classifier.matches(static_cast<char32_t>(range.max)))
                    return false;
            }
            return true;
        }
        struct OctalDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 8;
            static constexpr bool matches(char32_t ch)
            {
                return ch >= '0' && ch <= '7';
            }
        };
        struct DecimalDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10;
            static constexpr bool matches(char32_t ch)
            {
                return ch >= '0' && ch <= '9';
            }
        };
        struct UppercaseHexDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10 + 6;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
            }
        };
        struct LowercaseHexDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10 + 6;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
            }
        };
        struct HexDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10 + 6 + 6;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')
                       || (ch >= 'A' && ch <= 'F');
            }
        };
        struct UppercaseLetterClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26;
            static constexpr bool matches(char32_t ch)
            {
                return ch >= 'A' && ch <= 'Z';
            }
        };
        struct LowercaseLetterClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26;
            static constexpr bool matches(char32_t ch)
            {
                return ch >= 'a' && ch <= 'z';
            }
        };
        struct LetterClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
            }
        };
        struct LetterOrDigitClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26 + 10;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                       || (ch >= '0' && ch <= '9');
            }
        };
        struct DigitOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= '0' && ch <= '9') || ch == '_';
            }
        };
        struct UppercaseLetterOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'A' && ch <= 'Z') || ch == '_';
            }
        };
        struct LowercaseLetterOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || ch == '_';
            }
        };
        struct LetterOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
            }
        };
        struct LetterDigitOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26 + 10 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                       || (ch >= '0' && ch <= '9') || ch == '_';
            }
        };
        struct DigitDollarOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 10 + 1 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= '0' && ch <= '9') || ch == '$' || ch == '_';
            }
        };
        struct UppercaseLetterDollarOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 1 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '$';
            }
        };
        struct LowercaseLetterDollarOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 1 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '$';
            }
        };
        struct LetterDollarOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26 + 1 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_'
                       || ch == '$';
            }
        };
        struct LetterDigitDollarOrUnderlineClassifier final
        {
            static constexpr unsigned totalCharacterCount = 26 + 26 + 10 + 1 + 1;
            static constexpr bool matches(char32_t ch)
            {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                       || (ch >= '0' && ch <= '9') || ch == '_' || ch == '$';
            }
        };
        struct SpaceOrTabClassifier final
        {
            static constexpr unsigned totalCharacterCount = 2;
            static constexpr bool matches(char32_t ch)
            {
                return ch == ' ' || ch == '\t';
            }
        };
        struct SpaceTabOrLineEndingClassifier final
        {
            static constexpr unsigned totalCharacterCount = 4;
            static constexpr bool matches(char32_t ch)
            {
                return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
            }
        };
        struct LineEndingClassifier final
        {
            static constexpr unsigned totalCharacterCount = 2;
            static constexpr bool matches(char32_t ch)
            {
                return ch == '\r' || ch == '\n';
            }
        };
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
