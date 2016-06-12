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

#ifndef INTERVAL_SET_H_
#define INTERVAL_SET_H_

#include "interval.h"
#include <set>

template <typename T>
class IntervalSet final
{
private:
    typedef std::set<InclusiveInterval<T>, typename InclusiveInterval<T>::MaxLess> IntervalsType;

public:
    typedef typename IntervalsType::const_iterator iterator;
    typedef typename IntervalsType::const_iterator const_iterator;
    typedef typename IntervalsType::const_reverse_iterator reverse_iterator;
    typedef typename IntervalsType::const_reverse_iterator const_reverse_iterator;

private:
    IntervalsType intervals;

public:
    IntervalSet() : intervals()
    {
    }
    iterator begin()
    {
        return intervals.begin();
    }
    const_iterator begin() const
    {
        return intervals.begin();
    }
    const_iterator cbegin() const
    {
        return intervals.begin();
    }
    iterator end()
    {
        return intervals.end();
    }
    const_iterator end() const
    {
        return intervals.end();
    }
    const_iterator cend() const
    {
        return intervals.end();
    }
    reverse_iterator rbegin()
    {
        return intervals.rbegin();
    }
    const_reverse_iterator rbegin() const
    {
        return intervals.rbegin();
    }
    const_reverse_iterator crbegin() const
    {
        return intervals.rbegin();
    }
    reverse_iterator rend()
    {
        return intervals.rend();
    }
    const_reverse_iterator rend() const
    {
        return intervals.rend();
    }
    const_reverse_iterator crend() const
    {
        return intervals.rend();
    }
    std::size_t size() const
    {
        return intervals.size();
    }
    bool empty() const
    {
        return intervals.empty();
    }
    bool includes(const T &value) const
    {
        auto iter = intervals.lower_bound(InclusiveInterval<T>(value));
        if(iter == intervals.end())
            return false;
        return iter->includes(value);
    }
    bool contains(const InclusiveInterval<T> &value) const
    {
        if(value.empty())
            return false;
        auto iter = intervals.lower_bound(value);
        if(iter == intervals.end())
            return false;
        return iter->contains(value);
    }
    bool overlaps(const InclusiveInterval<T> &value) const
    {
        if(value.empty())
            return false;
        auto beginIter = intervals.lower_bound(InclusiveInterval<T>(value.min));
        auto endIter = intervals.upper_bound(InclusiveInterval<T>(value.max));
        for(auto iter = beginIter; iter != endIter; ++iter)
        {
            if(iter->overlaps(value))
                return true;
        }
        return false;
    }
    bool operator ==(const IntervalSet &rt) const
    {
        return intervals == rt.intervals;
    }
    bool operator !=(const IntervalSet &rt) const
    {
        return intervals != rt.intervals;
    }
    IntervalSet &operator -=(const InclusiveInterval<T> &value) const
    {
        if(value.empty())
            return *this;
        auto beginIter = intervals.lower_bound(InclusiveInterval<T>(value.min));
        auto endIter = intervals.upper_bound(InclusiveInterval<T>(value.max));
        for(auto iter = beginIter; iter != endIter;)
        {
            typename InclusiveInterval<T>::SetOperationResults results = *iter - value;
            if(results.size() == 1 && *results.begin() == *iter)
            {
                ++iter;
                continue;
            }
            iter = intervals.erase(iter);
            for(auto v : results)
                intervals.insert(v);
        }
    }
};

#endif /* INTERVAL_SET_H_ */
