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

#ifndef AST_SEQUENCE_H_
#define AST_SEQUENCE_H_

#include "expression.h"
#include "visitor.h"
#include <utility>

namespace ast
{
struct Sequence final : public Expression
{
    Expression *first;
    Expression *second;
    Sequence(Location location, Expression *first, Expression *second)
        : Expression(std::move(location)), first(first), second(second)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitSequence(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return true;
    }
    virtual bool hasLeftRecursion() override
    {
        return first->hasLeftRecursion()
               || (first->canAcceptEmptyString() && second->hasLeftRecursion());
    }
    virtual bool canAcceptEmptyString() override
    {
        return first->canAcceptEmptyString() && second->canAcceptEmptyString();
    }
};
}

#endif /* AST_SEQUENCE_H_ */
