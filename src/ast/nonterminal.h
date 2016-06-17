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

#ifndef AST_NONTERMINAL_H_
#define AST_NONTERMINAL_H_

#include "expression.h"
#include "node.h"
#include "visitor.h"
#include "type.h"
#include <utility>
#include <string>

namespace ast
{
struct Nonterminal final : public Node
{
    std::string name;
    Expression *expression;
    Type *type;
    struct Settings final
    {
        bool caching = true;
        bool hasLeftRecursion = true;
        bool canAcceptEmptyString = true;
    };
    Settings settings;
    Nonterminal(
        Location location, std::string name, Expression *expression, Type *type, Settings settings)
        : Node(std::move(location)),
          name(std::move(name)),
          expression(expression),
          type(type),
          settings(std::move(settings))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitNonterminal(this);
    }
};

struct NonterminalExpression final : public Expression
{
    Nonterminal *value;
    std::string variableName;
    NonterminalExpression(Location location, Nonterminal *value, std::string variableName)
        : Expression(std::move(location)), value(value), variableName(std::move(variableName))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitNonterminalExpression(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return false;
    }
    virtual bool hasLeftRecursion() override
    {
        return value->settings.hasLeftRecursion;
    }
    virtual bool canAcceptEmptyString() override
    {
        return value->settings.canAcceptEmptyString;
    }
};
}

#endif /* AST_NONTERMINAL_H_ */
