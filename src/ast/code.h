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

#ifndef AST_CODE_H_
#define AST_CODE_H_

#include "expression.h"
#include "visitor.h"
#include <string>

namespace ast
{
struct ExpressionCodeSnippet final : public Expression
{
    std::string code;
    struct Substitution final
    {
        enum class Kind
        {
            ReturnValue,
            PredicateReturnValue,
        };
        Kind kind;
        std::size_t position;
        Substitution(Kind kind, std::size_t position) : kind(kind), position(position)
        {
        }
        Substitution() : kind(), position()
        {
        }
    };
    std::vector<Substitution> substitutions;
    ExpressionCodeSnippet(Location location,
                          std::string code,
                          std::vector<Substitution> substitutions)
        : Expression(std::move(location)),
          code(std::move(code)),
          substitutions(std::move(substitutions))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitExpressionCodeSnippet(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return true;
    }
    virtual bool hasLeftRecursion() override
    {
        return false;
    }
    virtual bool canAcceptEmptyString() override
    {
        return true;
    }
};
struct TopLevelCodeSnippet final : public Node
{
    enum class Kind
    {
        License,
        Header,
        Source,
    };
    Kind kind;
    std::string code;
    TopLevelCodeSnippet(Location location, Kind kind, std::string code)
        : Node(std::move(location)), kind(kind), code(std::move(code))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTopLevelCodeSnippet(this);
    }
};
}

#endif /* AST_CODE_H_ */
