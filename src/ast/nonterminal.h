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
#include <vector>
#include <cassert>

namespace ast
{
struct TemplateArgumentType;

struct TemplateArgumentTypeValue final : public Node
{
    const std::string name;
    const std::string code;
    TemplateArgumentType *const type;
    TemplateArgumentTypeValue(Location location,
                              std::string name,
                              std::string code,
                              TemplateArgumentType *type)
        : Node(std::move(location)), name(std::move(name)), code(std::move(code)), type(type)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTemplateArgumentTypeValue(this);
    }
};

struct TemplateArgumentType final : public Node
{
    const std::string name;
    const std::string code;
    std::vector<TemplateArgumentTypeValue *> values;
    TemplateArgumentType(Location location,
                         std::string name,
                         std::string code,
                         std::vector<TemplateArgumentTypeValue *> values)
        : Node(std::move(location)),
          name(std::move(name)),
          code(std::move(code)),
          values(std::move(values))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTemplateArgumentType(this);
    }
};

struct TemplateArgument : public Node
{
    TemplateArgumentType *type;
    virtual std::string getName() const = 0;
    virtual std::string getCode() const = 0;
    TemplateArgument(Location location, TemplateArgumentType *type)
        : Node(std::move(location)), type(type)
    {
        assert(type);
    }
};

struct TemplateArgumentConstant final : public TemplateArgument
{
    TemplateArgumentTypeValue *value;
    virtual std::string getName() const override
    {
        return value->name;
    }
    virtual std::string getCode() const override
    {
        return value->code;
    }
    TemplateArgumentConstant(Location location,
                             TemplateArgumentType *type,
                             TemplateArgumentTypeValue *value)
        : TemplateArgument(std::move(location), type), value(value)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTemplateArgumentConstant(this);
    }
};

struct TemplateVariableDeclaration final : public Node
{
    TemplateArgumentType *type;
    const std::string name;
    TemplateVariableDeclaration(Location location, TemplateArgumentType *type, std::string name)
        : Node(std::move(location)), type(type), name(std::move(name))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTemplateVariableDeclaration(this);
    }
};

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
    std::vector<TemplateVariableDeclaration *> templateArguments;
    Nonterminal(Location location,
                std::string name,
                Expression *expression,
                Type *type,
                Settings settings,
                std::vector<TemplateVariableDeclaration *> templateArguments)
        : Node(std::move(location)),
          name(std::move(name)),
          expression(expression),
          type(type),
          settings(std::move(settings)),
          templateArguments(std::move(templateArguments))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitNonterminal(this);
    }
};

struct TemplateArgumentVariableReference final : public TemplateArgument
{
    TemplateVariableDeclaration *declaration;
    virtual std::string getName() const override
    {
        assert(declaration);
        return declaration->name;
    }
    virtual std::string getCode() const override
    {
        return getName();
    }
    TemplateArgumentVariableReference(Location location,
                                      TemplateArgumentType *type,
                                      TemplateVariableDeclaration *declaration)
        : TemplateArgument(std::move(location), type), declaration(declaration)
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitTemplateArgumentVariableReference(this);
    }
};

struct NonterminalExpression final : public Expression
{
    Nonterminal *value;
    Nonterminal *containingNonterminal;
    std::string variableName;
    std::vector<TemplateArgument *> templateArguments;
    NonterminalExpression(Location location,
                          Nonterminal *value,
                          Nonterminal *containingNonterminal,
                          std::string variableName,
                          std::vector<TemplateArgument *> templateArguments)
        : Expression(std::move(location)),
          value(value),
          containingNonterminal(containingNonterminal),
          variableName(std::move(variableName)),
          templateArguments(std::move(templateArguments))
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
