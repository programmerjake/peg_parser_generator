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

#ifndef INTERVAL_H_
#define INTERVAL_H_

#include <type_traits>

template <typename T>
struct InclusiveInterval final
{
    static_assert(std::is_integral<T>::value, "type must be integral");
    static_assert(!std::is_const<T>::value, "type must not be const");
    static_assert(!std::is_volatile<T>::value, "type must not be volatile");
    typedef T value_type;
    T min;
    T max;
    constexpr InclusiveInterval() : min(1), max(0)
    {
    }
    constexpr explicit InclusiveInterval(T value) : min(value), max(value)
    {
    }
    constexpr explicit InclusiveInterval(T min, T max) : min(min), max(max)
    {
    }
    constexpr bool overlaps(const InclusiveInterval &rt) const
    {
        return min <= rt.max && max >= rt.min;
    }
    constexpr bool contains(const InclusiveInterval &rt) const
    {
        return rt.min >= min && rt.max <= max;
    }
    constexpr bool empty() const
    {
        return min > max;
    }
    constexpr bool includes(T value) const
    {
        return value >= min && value <= max;
    }
    struct SetOperationResults final
    {
        InclusiveInterval ranges[2];
        std::size_t rangeCount;
        typedef InclusiveInterval *iterator;
        typedef const InclusiveInterval *const_iterator;
        iterator begin()
        {
            return ranges;
        }
        constexpr iterator begin() const
        {
            return ranges;
        }
        constexpr iterator cbegin() const
        {
            return ranges;
        }
        iterator end()
        {
            return ranges + rangeCount;
        }
        constexpr iterator end() const
        {
            return ranges + rangeCount;
        }
        constexpr iterator cend() const
        {
            return ranges + rangeCount;
        }
        constexpr std::size_t size() const
        {
            return rangeCount;
        }
        constexpr bool empty() const
        {
            return rangeCount == 0;
        }
        constexpr SetOperationResults() : ranges(), rangeCount(0)
        {
        }
        constexpr explicit SetOperationResults(const InclusiveInterval &range)
            : ranges{range}, rangeCount(1)
        {
        }
        constexpr SetOperationResults(const InclusiveInterval &range0,
                                      const InclusiveInterval &range1)
            : ranges{range0, range1}, rangeCount(2)
        {
        }
    };
    constexpr bool operator==(const InclusiveInterval &rt) const
    {
        return min == rt.min && max == rt.max;
    }
    constexpr bool operator!=(const InclusiveInterval &rt) const
    {
        return min != rt.min || max != rt.max;
    }
    constexpr SetOperationResults operator-(const InclusiveInterval &rt) const
    {
        return min < rt.min ?
                   (max > rt.max ? SetOperationResults(InclusiveInterval(min, rt.min - 1),
                                                       InclusiveInterval(rt.max + 1, max)) :
                                   SetOperationResults(InclusiveInterval(min, rt.min - 1))) :
                   (max > rt.max ? SetOperationResults(InclusiveInterval(rt.max + 1, max)) :
                                   SetOperationResults());
    }
    constexpr SetOperationResults operator&(const InclusiveInterval &rt) const
    {
        return overlaps(rt) ? SetOperationResults(InclusiveInterval(min > rt.min ? min : rt.min,
                                                                    max < rt.max ? max : rt.max)) :
                              SetOperationResults();
    }
    constexpr SetOperationResults operator^(const InclusiveInterval &rt) const
    {
        return min == rt.min ?
                   (max == rt.max ? SetOperationResults() : max < rt.max ?
                                    SetOperationResults(InclusiveInterval(max + 1, rt.max)) :
                                    SetOperationResults(InclusiveInterval(rt.max + 1, max))) :
                   min < rt.min ?
                   (max == rt.max ?
                        SetOperationResults(InclusiveInterval(min, rt.min - 1)) :
                        max < rt.max ? SetOperationResults(InclusiveInterval(min, rt.min - 1),
                                                           InclusiveInterval(max + 1, rt.max)) :
                                       SetOperationResults(InclusiveInterval(min, rt.min - 1),
                                                           InclusiveInterval(rt.max + 1, max))) :
                   (max == rt.max ?
                        SetOperationResults(InclusiveInterval(rt.min, min - 1)) :
                        max < rt.max ? SetOperationResults(InclusiveInterval(rt.min, min - 1),
                                                           InclusiveInterval(max + 1, rt.max)) :
                                       SetOperationResults(InclusiveInterval(rt.min, min - 1),
                                                           InclusiveInterval(rt.max + 1, max)));
    }
    constexpr SetOperationResults operator|(const InclusiveInterval &rt) const
    {
        return overlaps(rt) ?
                   SetOperationResults(InclusiveInterval(min < rt.min ? min : rt.min,
                                                         max > rt.max ? max : rt.max)) :
                   min < rt.min ? SetOperationResults(*this, rt) : SetOperationResults(rt, *this);
    }
    struct MinLess final
    {
        constexpr bool operator()(const InclusiveInterval &a, const InclusiveInterval &b) const
        {
            return a.min < b.min;
        }
    };
    struct MinGreater final
    {
        constexpr bool operator()(const InclusiveInterval &a, const InclusiveInterval &b) const
        {
            return a.min > b.min;
        }
    };
    struct MaxLess final
    {
        constexpr bool operator()(const InclusiveInterval &a, const InclusiveInterval &b) const
        {
            return a.max < b.max;
        }
    };
    struct MaxGreater final
    {
        constexpr bool operator()(const InclusiveInterval &a, const InclusiveInterval &b) const
        {
            return a.max > b.max;
        }
    };
};

#endif /* INTERVAL_H_ */
