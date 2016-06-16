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

#ifndef AST_CODE_SNIPPET_H_
#define AST_CODE_SNIPPET_H_

#include "expression.h"
#include "visitor.h"
#include <string>

namespace ast
{
struct CodeSnippet final : public Expression
{
    std::string code;
    CodeSnippet(Location location, std::string code)
        : Expression(std::move(location)), code(std::move(code))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitCodeSnippet(this);
    }
    virtual bool defaultNeedsCaching() override
    {
        return true;
    }
};
}

#endif /* AST_CODE_SNIPPET_H_ */
