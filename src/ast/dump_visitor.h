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

#ifndef AST_DUMP_VISITOR_H_
#define AST_DUMP_VISITOR_H_

#include "visitor.h"
#include <iosfwd>
#include <string>

namespace ast
{
class DumpVisitor final : public Visitor
{
private:
    std::size_t indentDepth;
    std::ostream &os;
    std::string indentString;

private:
    void indent();

public:
    explicit DumpVisitor(std::ostream &os) : DumpVisitor(os, "    ")
    {
    }
    DumpVisitor(std::ostream &os, std::string indentString)
        : indentDepth(0), os(os), indentString(std::move(indentString))
    {
    }
    static std::string escapeCharacter(char32_t ch);
    virtual void visitEmpty(Empty *node) override;
    virtual void visitGrammar(Grammar *node) override;
    virtual void visitNonterminal(Nonterminal *node) override;
    virtual void visitNonterminalExpression(NonterminalExpression *node) override;
    virtual void visitOrderedChoice(OrderedChoice *node) override;
    virtual void visitFollowedByPredicate(FollowedByPredicate *node) override;
    virtual void visitNotFollowedByPredicate(NotFollowedByPredicate *node) override;
    virtual void visitCustomPredicate(CustomPredicate *node) override;
    virtual void visitGreedyRepetition(GreedyRepetition *node) override;
    virtual void visitGreedyPositiveRepetition(GreedyPositiveRepetition *node) override;
    virtual void visitOptionalExpression(OptionalExpression *node) override;
    virtual void visitSequence(Sequence *node) override;
    virtual void visitTerminal(Terminal *node) override;
    virtual void visitCharacterClass(CharacterClass *node) override;
    virtual void visitEOFTerminal(EOFTerminal *node) override;
};
}

#endif /* AST_DUMP_VISITOR_H_ */
