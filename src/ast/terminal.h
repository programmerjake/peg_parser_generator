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
