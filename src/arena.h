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

#ifndef ARENA_H_
#define ARENA_H_

#include <utility>

class Arena final
{
    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

private:
    struct Node final
    {
        void *ptr = nullptr;
        void (*deleter)(void *ptr) = nullptr;
    };
    struct Chunk final
    {
        static constexpr std::size_t allocatedCount = 0x100 - 1;
        Node nodes[allocatedCount];
        std::size_t usedCount = 0;
        Chunk *next = nullptr;
    };
private:
    Chunk *head;
    Chunk *tail;
private:
    Node &appendNode()
    {
        if(tail->usedCount >= Chunk::allocatedCount)
        {
            tail->next = new Chunk;
            tail = tail->next;
        }
        return tail->nodes[tail->usedCount++];
    }

public:
    Arena() : head(new Chunk), tail(head)
    {
    }
    Arena(Arena &&rt) : head(new Chunk), tail(head)
    {
        swap(rt);
    }
    ~Arena()
    {
        while(head)
        {
            Chunk *chunk = head;
            head = chunk->next;
            for(std::size_t i = 0; i < chunk->usedCount; i++)
            {
                chunk->nodes[i].deleter(chunk->nodes[i].ptr);
            }
            delete chunk;
        }
    }
    Arena &operator=(Arena rt)
    {
        swap(rt);
        return *this;
    }
    void swap(Arena &rt)
    {
        auto tempHead = head;
        auto tempTail = tail;
        head = rt.head;
        tail = rt.tail;
        rt.head = tempHead;
        rt.tail = tempTail;
    }
    template <typename T, typename... Args>
    T *make(Args &&... args)
    {
        Node &node = appendNode();
        node.deleter = [](void *ptr)
        {
            delete static_cast<T *>(ptr);
        };
        try
        {
            node.ptr = new T(std::forward<Args>(args)...);
        }
        catch(...)
        {
            tail->usedCount--;
            throw;
        }
        return static_cast<T *>(node.ptr);
    }
};

#endif /* ARENA_H_ */
