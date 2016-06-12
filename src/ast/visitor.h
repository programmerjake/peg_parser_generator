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

#ifndef AST_VISITOR_H_
#define AST_VISITOR_H_

namespace ast
{
struct Empty;
struct Grammar;
struct Nonterminal;
struct NonterminalExpression;
struct OrderedChoice;
struct FollowedByPredicate;
struct NotFollowedByPredicate;
struct CustomPredicate;
struct GreedyRepetition;
struct GreedyPositiveRepetition;
struct OptionalExpression;
struct Sequence;
struct Terminal;
struct CharacterClass;
struct EOFTerminal;
struct Visitor
{
    virtual ~Visitor() = default;
    virtual void visitEmpty(Empty *node) = 0;
    virtual void visitGrammar(Grammar *node) = 0;
    virtual void visitNonterminal(Nonterminal *node) = 0;
    virtual void visitNonterminalExpression(NonterminalExpression *node) = 0;
    virtual void visitOrderedChoice(OrderedChoice *node) = 0;
    virtual void visitFollowedByPredicate(FollowedByPredicate *node) = 0;
    virtual void visitNotFollowedByPredicate(NotFollowedByPredicate *node) = 0;
    virtual void visitCustomPredicate(CustomPredicate *node) = 0;
    virtual void visitGreedyRepetition(GreedyRepetition *node) = 0;
    virtual void visitGreedyPositiveRepetition(GreedyPositiveRepetition *node) = 0;
    virtual void visitOptionalExpression(OptionalExpression *node) = 0;
    virtual void visitSequence(Sequence *node) = 0;
    virtual void visitTerminal(Terminal *node) = 0;
    virtual void visitCharacterClass(CharacterClass *node) = 0;
    virtual void visitEOFTerminal(EOFTerminal *node) = 0;
};
}

#endif /* AST_VISITOR_H_ */
