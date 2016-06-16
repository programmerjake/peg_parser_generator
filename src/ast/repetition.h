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

#ifndef AST_REPETITION_H_
#define AST_REPETITION_H_

#include "expression.h"
#include "visitor.h"
#include <utility>

namespace ast
{
struct GreedyRepetition final : public Expression
{
    Expression *expression;
    GreedyRepetition(Location location, Expression *expression)
        : Expression(std::move(location)), expression(expression)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitGreedyRepetition(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return true;
    }
};

struct GreedyPositiveRepetition final : public Expression
{
    Expression *expression;
    GreedyPositiveRepetition(Location location, Expression *expression)
        : Expression(std::move(location)), expression(expression)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitGreedyPositiveRepetition(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return true;
    }
};

struct OptionalExpression final : public Expression
{
    Expression *expression;
    OptionalExpression(Location location, Expression *expression)
        : Expression(std::move(location)), expression(expression)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitOptionalExpression(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return expression->defaultNeedsCaching();
    }
};
}

#endif /* AST_REPETITION_H_ */
