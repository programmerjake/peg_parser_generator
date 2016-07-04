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
#include "dump_visitor.h"
#include "empty.h"
#include "grammar.h"
#include "nonterminal.h"
#include "ordered_choice.h"
#include "predicate.h"
#include "repetition.h"
#include "sequence.h"
#include "terminal.h"
#include "code.h"
#include <sstream>
#include <cassert>

namespace ast
{
void DumpVisitor::indent()
{
    for(std::size_t i = 0; i < indentDepth; i++)
        os << indentString;
}

std::string DumpVisitor::escapeCharacter(char32_t ch)
{
    switch(ch)
    {
    case '\\':
    case '\'':
    case '\"':
    case '?':
        return std::string("\\") + static_cast<char>(ch);
    case '\n':
        return "\\n";
    case '\r':
        return "\\r";
    case '\t':
        return "\\t";
    case '\0':
        return "\\000";
    default:
        if(ch < 0x20 || ch >= 0x7F)
        {
            std::ostringstream ss;
            ss.fill('0');
            if(ch >= 0x80)
            {
                ss << std::uppercase << std::hex;
                if(ch > 0xFFFFU)
                {
                    ss << "\\U";
                    ss.width(8);
                    ss << ch;
                }
                else
                {
                    ss << "\\u";
                    ss.width(4);
                    ss << ch;
                }
            }
            else
            {
                ss << std::oct;
                ss << "\\";
                ss.width(3);
                ss << ch;
            }
            return ss.str();
        }
        return std::string(1, static_cast<char>(ch));
    }
}

void DumpVisitor::visitEmpty(Empty *node)
{
    assert(node);
    indent();
    os << "Empty" << std::endl;
}

void DumpVisitor::visitGrammar(Grammar *node)
{
    indent();
    os << "Grammar outputNamespace = \"";
    auto seperator = "";
    for(const auto &v : node->outputNamespace)
    {
        os << seperator << v;
        seperator = "::";
    }
    os << "\"" << std::endl;
    indentDepth++;
    for(auto topLevelCodeSnippet : node->topLevelCodeSnippets)
    {
        topLevelCodeSnippet->visit(*this);
    }
    for(auto nonterminal : node->nonterminals)
    {
        nonterminal->visit(*this);
    }
    indentDepth--;
}

void DumpVisitor::visitNonterminal(Nonterminal *node)
{
    indent();
    os << "Nonterminal name = \"" << node->name
       << "\" caching = " << (node->settings.caching ? "true" : "false") << std::endl;
    indentDepth++;
    for(auto templateArgument : node->templateArguments)
    {
        templateArgument->visit(*this);
    }
    node->type->visit(*this);
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitNonterminalExpression(NonterminalExpression *node)
{
    indent();
    os << "NonterminalExpression Nonterminal->name = \"" << node->value->name
       << "\" variableName = \"" << node->variableName << "\"" << std::endl;
    indentDepth++;
    for(auto templateArgument : node->templateArguments)
    {
        templateArgument->visit(*this);
    }
    indentDepth--;
}

void DumpVisitor::visitOrderedChoice(OrderedChoice *node)
{
    indent();
    os << "OrderedChoice" << std::endl;
    indentDepth++;
    node->first->visit(*this);
    node->second->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitFollowedByPredicate(FollowedByPredicate *node)
{
    indent();
    os << "FollowedByPredicate" << std::endl;
    indentDepth++;
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitNotFollowedByPredicate(NotFollowedByPredicate *node)
{
    indent();
    os << "NotFollowedByPredicate" << std::endl;
    indentDepth++;
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitCustomPredicate(CustomPredicate *node)
{
    indent();
    os << "CustomPredicate" << std::endl;
    indentDepth++;
    node->codeSnippet->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitGreedyRepetition(GreedyRepetition *node)
{
    indent();
    os << "GreedyRepetition" << std::endl;
    indentDepth++;
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitGreedyPositiveRepetition(GreedyPositiveRepetition *node)
{
    indent();
    os << "GreedyPositiveRepetition" << std::endl;
    indentDepth++;
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitOptionalExpression(OptionalExpression *node)
{
    indent();
    os << "OptionalExpression" << std::endl;
    indentDepth++;
    node->expression->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitSequence(Sequence *node)
{
    indent();
    os << "Sequence" << std::endl;
    indentDepth++;
    node->first->visit(*this);
    node->second->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitTerminal(Terminal *node)
{
    indent();
    os << "Terminal value = '" << escapeCharacter(node->value) << "' ("
       << static_cast<unsigned long>(node->value) << ")" << std::endl;
}

void DumpVisitor::visitCharacterClass(CharacterClass *node)
{
    indent();
    os << "CharacterClass inverted = " << (node->inverted ? "true" : "false")
       << " variableName = \"" << node->variableName << "\"" << std::endl;
    indentDepth++;
    for(const auto &range : node->characterRanges.ranges)
    {
        indent();
        if(range.min == range.max)
        {
            os << "'" << escapeCharacter(range.min) << "' ("
               << static_cast<unsigned long>(range.min) << ")" << std::endl;
        }
        else
        {
            os << "'" << escapeCharacter(range.min) << "' ("
               << static_cast<unsigned long>(range.min) << ") to '" << escapeCharacter(range.max)
               << "' (" << static_cast<unsigned long>(range.max) << ")" << std::endl;
        }
    }
    indentDepth--;
}

void DumpVisitor::visitEOFTerminal(EOFTerminal *node)
{
    indent();
    os << "EOFTerminal" << std::endl;
}

void DumpVisitor::visitExpressionCodeSnippet(ExpressionCodeSnippet *node)
{
    indent();
    os << "CodeSnippet code = '";
    for(auto ch : node->code)
    {
        os << escapeCharacter(ch);
    }
    os << "'" << std::endl;
}

void DumpVisitor::visitTopLevelCodeSnippet(TopLevelCodeSnippet *node)
{
    indent();
    os << "TopLevelCodeSnippet kind = ";
    switch(node->kind)
    {
    case TopLevelCodeSnippet::Kind::License:
        os << "License";
        break;
    case TopLevelCodeSnippet::Kind::Header:
        os << "Header";
        break;
    case TopLevelCodeSnippet::Kind::Source:
        os << "Source";
        break;
    case TopLevelCodeSnippet::Kind::Class:
        os << "Class";
        break;
    }
    os << " code = '";
    for(auto ch : node->code)
    {
        os << escapeCharacter(ch);
    }
    os << "'" << std::endl;
}

void DumpVisitor::visitType(Type *node)
{
    indent();
    os << "Type name = " << node->name << " code = '";
    for(auto ch : node->code)
    {
        os << escapeCharacter(ch);
    }
    os << "'" << std::endl;
}

void DumpVisitor::visitTemplateArgumentType(TemplateArgumentType *node)
{
    indent();
    os << "TemplateArgumentType name = " << node->name << " code = " << node->code << std::endl;
    indentDepth++;
    for(auto value : node->values)
    {
        value->visit(*this);
    }
    indentDepth--;
}

void DumpVisitor::visitTemplateArgumentTypeValue(TemplateArgumentTypeValue *node)
{
    indent();
    os << "TemplateArgumentTypeValue type->name = " << node->type->name << " name = " << node->name
       << " code = " << node->code << std::endl;
}

void DumpVisitor::visitTemplateArgumentConstant(TemplateArgumentConstant *node)
{
    indent();
    os << "TemplateArgumentConstant type->name = " << node->type->name << std::endl;
    indentDepth++;
    node->value->visit(*this);
    indentDepth--;
}

void DumpVisitor::visitTemplateVariableDeclaration(TemplateVariableDeclaration *node)
{
    indent();
    os << "TemplateVariableDeclaration type->name = " << node->type->name
       << " name = " << node->name << std::endl;
}

void DumpVisitor::visitTemplateArgumentVariableReference(TemplateArgumentVariableReference *node)
{
    indent();
    os << "TemplateArgumentVariableReference" << std::endl;
    indentDepth++;
    node->declaration->visit(*this);
    indentDepth--;
}
}
