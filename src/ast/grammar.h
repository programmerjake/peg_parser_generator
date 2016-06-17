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

#ifndef AST_GRAMMAR_H_
#define AST_GRAMMAR_H_

#include "nonterminal.h"
#include "node.h"
#include "visitor.h"
#include <vector>

namespace ast
{
struct TopLevelCodeSnippet;
struct Grammar final : public Node
{
    std::vector<TopLevelCodeSnippet *> topLevelCodeSnippets;
    std::vector<Nonterminal *> nonterminals;
    Grammar(Location location,
            std::vector<TopLevelCodeSnippet *> topLevelCodeSnippets,
            std::vector<Nonterminal *> nonterminals)
        : Node(std::move(location)),
          topLevelCodeSnippets(std::move(topLevelCodeSnippets)),
          nonterminals(std::move(nonterminals))
    {
    }
    virtual void visit(Visitor &visitor) override
    {
        visitor.visitGrammar(this);
    }
};
}

#endif /* AST_GRAMMAR_H_ */
