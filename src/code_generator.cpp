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
#include "code_generator.h"
#include "ast/visitor.h"
#include "ast/dump_visitor.h"
#include "ast/nonterminal.h"
#include "source.h"
#include "location.h"
#include <sstream>
#include <cassert>
#include <cctype>

struct CodeGenerator::CPlusPlus11 final : public CodeGenerator, public ast::Visitor
{
    std::ostream &finalSourceFile;
    std::ostream &finalHeaderFile;
    std::ostringstream sourceFile;
    std::ostringstream headerFile;
    std::string headerFileNameFromSourceFile;
    const std::size_t indentSize = 4;
    const std::size_t tabSize = 0;
    CPlusPlus11(std::ostream &finalSourceFile,
                std::ostream &finalHeaderFile,
                std::string headerFileNameFromSourceFile)
        : finalSourceFile(finalSourceFile),
          finalHeaderFile(finalHeaderFile),
          headerFileNameFromSourceFile(std::move(headerFileNameFromSourceFile))
    {
    }
    static std::string translateName(std::string name, std::size_t partNumber = std::string::npos)
    {
        assert(!name.empty());
        std::ostringstream ss;
        if(partNumber == std::string::npos)
        {
            ss << "parseRule";
        }
        else
        {
            ss << "rulePart" << partNumber;
        }
        name[0] = std::toupper(name[0]);
        ss << name;
        return ss.str();
    }
    std::string getGuardMacroName() const
    {
        assert(!headerFileNameFromSourceFile.empty());
        std::string retval;
        retval.reserve(3 + headerFileNameFromSourceFile.size());
        std::size_t i = 0;
        if(!std::isalpha(headerFileNameFromSourceFile[0]))
        {
            i++;
            retval = "HEADER_";
        }
        for(; i < headerFileNameFromSourceFile.size(); i++)
        {
            char ch = headerFileNameFromSourceFile[i];
            if(std::isalnum(ch))
                retval += std::toupper(ch);
            else
                retval += '_';
        }
        retval += '_';
        return retval;
    }
    void writeIndent(std::ostream &os, std::size_t depth) const
    {
        if(tabSize > 0)
        {
            while(depth >= tabSize)
            {
                os << '\t';
                depth -= tabSize;
            }
        }
        while(depth-- > 0)
            os << ' ';
    }
    void reindent(std::ostream &os, const std::string &source) const
    {
        bool isAtStartOfLine = true;
        std::size_t indentDepth = 0;
        std::size_t startIndentDepth = 0;
        for(std::size_t i = 0; i < source.size(); i++)
        {
            char ch = source[i];
            assert(ch != '\r');
            assert(ch != '\t');
            assert(ch != '\f');
            assert(ch != '\0');
            if(ch == '\n')
            {
                os << ch;
                isAtStartOfLine = true;
                indentDepth = startIndentDepth;
            }
            else if(isAtStartOfLine)
            {
                switch(ch)
                {
                case '`':
                    indentDepth++;
                    continue;
                case ' ':
                {
                    i++;
                    assert(i < source.size());
                    assert(source[i] == ' ');
                    i++;
                    assert(i < source.size());
                    assert(source[i] == ' ');
                    i++;
                    assert(i < source.size());
                    assert(source[i] == ' ');
                    indentDepth += indentSize;
                    continue;
                }
                case '@':
                {
                    i++;
                    assert(i < source.size());
                    switch(source[i])
                    {
                    case '+':
                        indentDepth += indentSize;
                        startIndentDepth += indentSize;
                        continue;
                    case '-':
                        assert(indentDepth >= indentSize);
                        assert(startIndentDepth >= indentSize);
                        indentDepth -= indentSize;
                        startIndentDepth -= indentSize;
                        continue;
                    case '_':
                        assert(startIndentDepth >= indentSize);
                        startIndentDepth -= indentSize;
                        continue;
                    }
                    assert(false);
                    continue;
                }
                default:
                    isAtStartOfLine = false;
                    writeIndent(os, indentDepth);
                    os << ch;
                    break;
                }
            }
            else
            {
                os << ch;
            }
        }
    }
    virtual void generateCode(const ast::Grammar *grammar) override
    {
        auto guardMacroName = getGuardMacroName();
        sourceFile << R"(// automatically generated from )" << grammar->location.source->fileName
                   << R"(
#include ")" << headerFileNameFromSourceFile << R"("

namespace parser
{
)";
        headerFile << R"(// automatically generated from )" << grammar->location.source->fileName
                   << R"(
#ifndef )" << guardMacroName << R"(
#define )" << guardMacroName << R"(

#include <utility>
#include <cstddef>
#include <string>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <list>

namespace parser
{
class Parser final
{
    Parser(const Parser &) = delete;
    Parser &operator=(const Parser &) = delete;

private:
    struct RuleResult final
    {
        std::size_t location;
        std::size_t inputStartLocation;
        std::size_t inputEndLocation;
        const char *message;
        constexpr RuleResult() noexcept : location(std::string::npos),
        ``````````````````````````````````inputStartLocation(std::string::npos),
        ``````````````````````````````````inputEndLocation(std::string::npos),
        ``````````````````````````````````message(nullptr)
        {
        }
        constexpr RuleResult(std::size_t location,
        `````````````````````std::size_t inputStartLocation,
        `````````````````````std::size_t inputEndLocation,
        `````````````````````const char *message) noexcept : location(location),
        `````````````````````````````````````````````````````inputStartLocation(inputStartLocation),
        `````````````````````````````````````````````````````inputEndLocation(inputEndLocation),
        `````````````````````````````````````````````````````message(message)
        {
        }
        constexpr bool empty() const
        {
            return inputStartLocation == std::string::npos;
        }
        constexpr bool success() const
        {
            return !empty() && message == nullptr;
        }
        constexpr bool fail() const
        {
            return !empty() && message != nullptr;
        }
    };
    struct Results final
    {
@+@+)";
        for(const ast::Nonterminal *nonterminal : grammar->nonterminals)
        {
            headerFile << "RuleResult " << translateName(nonterminal->name) << ";\n";
        }
        headerFile << R"(@_@-};
    struct ResultsChunk final
    {
        static constexpr std::size_t allocated = 0x100;
        Results values[allocated];
        std::size_t used = 0;
    };

public:
    struct ParseError : public std::runtime_error
    {
        std::size_t location;
        const char *message;
        static std::string makeWhatString(std::size_t location, const char *message)
        {
            std::ostringstream ss;
            ss << "error at " << location << ": " << message;
            return ss.str();
        }
        ParseError(std::size_t location, const char *message)
            : runtime_error(makeWhatString(location, message)), location(location), message(message)
        {
        }
        ParseError(const RuleResult &ruleResult)
            : ParseError(ruleResult.location, ruleResult.message)
        {
        }
    };

private:
    std::vector<Results *> resultsPointers;
    std::list<ResultsChunk> resultsChunks;
    Results eofResults;
    const std::shared_ptr<const char32_t> source;
    const std::size_t sourceSize;

private:
    Results &getResults(std::size_t position)
    {
        if(position >= sourceSize)
            return eofResults;
        Results *&resultsPointer = resultsPointers[position];
        if(!resultsPointer)
        {
            if(resultsChunks.empty() || resultsChunks.back().used >= ResultsChunk::allocated)
            {
                resultsChunks.emplace_back();
            }
            resultsPointer = &resultsChunks.back().values[resultsChunks.back().used++];
        }
        return *resultsPointer;
    }
    static std::pair<std::shared_ptr<const char32_t>, std::size_t> makeSource(std::u32string source);
    static std::pair<std::shared_ptr<const char32_t>, std::size_t> makeSource(const std::string &source);

public:
    Parser(std::pair<std::shared_ptr<const char32_t>, std::size_t> source) : Parser(std::move(std::get<0>(source)), std::get<1>(source))
    {
    }
    Parser(std::shared_ptr<const char32_t> source, std::size_t sourceSize);
    Parser(std::u32string source);
    Parser(const char *source, std::size_t sourceSize);
    Parser(const std::string &source) : Parser(source.data(), source.size())
    {
    }
)";
        sourceFile << R"(Parser::Parser(std::shared_ptr<const char32_t> source, std::size_t sourceSize)
    : resultsPointers(sourceSize, nullptr),
    ``resultsChunks(),
    ``eofResults(),
    ``source(std::move(source)),
    ``sourceSize(sourceSize)
{
}

Parser::Parser(std::u32string source) : Parser(makeSource(std::move(source)))
{
}

Parser::Parser(const std::string &source) : Parser(makeSource(std::move(source)))
{
}

std::pair<std::shared_ptr<const char32_t>, std::size_t> Parser::makeSource(std::u32string source)
{
    auto sourceSize = source.size();
    auto pSource = std::make_shared<std::u32string>(std::move(source));
    return std::make_pair(std::shared_ptr<const char32_t>(pSource, pSource->data()), sourceSize);
}

std::pair<std::shared_ptr<const char32_t>, std::size_t> Parser::makeSource(const char *source, std::size_t sourceSize)
{
    std::u32string retval;
    retval.reserve(source.size());
    std::size_t position = 0;
    while(position < sourceSize)
    {
        unsigned long byte1 = source[position];
        if(byte1 < 0x80)
        {
            retval += static_cast<char32_t>(byte1);
        }
        else
        {
            #error finish implementing utf-8 decoding
        }
    }
    return makeSource(std::move(retval));
}
)";
#warning finish implementing makeSource(const char *source, std::size_t sourceSize)
        for(const ast::Nonterminal *nonterminal : grammar->nonterminals)
        {
        }
        headerFile << R"(};
}

#endif /* )" << guardMacroName << R"( */
)";
        sourceFile << R"(}
)";
        reindent(finalHeaderFile, headerFile.str());
        reindent(finalSourceFile, sourceFile.str());
#warning finish
    }
    virtual void visitEmpty(ast::Empty *node) override
    {
#warning finish
    }
    virtual void visitGrammar(ast::Grammar *node) override
    {
#warning finish
    }
    virtual void visitNonterminal(ast::Nonterminal *node) override
    {
#warning finish
    }
    virtual void visitNonterminalExpression(ast::NonterminalExpression *node) override
    {
#warning finish
    }
    virtual void visitOrderedChoice(ast::OrderedChoice *node) override
    {
#warning finish
    }
    virtual void visitFollowedByPredicate(ast::FollowedByPredicate *node) override
    {
#warning finish
    }
    virtual void visitNotFollowedByPredicate(ast::NotFollowedByPredicate *node) override
    {
#warning finish
    }
    virtual void visitCustomPredicate(ast::CustomPredicate *node) override
    {
#warning finish
    }
    virtual void visitGreedyRepetition(ast::GreedyRepetition *node) override
    {
#warning finish
    }
    virtual void visitGreedyPositiveRepetition(ast::GreedyPositiveRepetition *node) override
    {
#warning finish
    }
    virtual void visitOptionalExpression(ast::OptionalExpression *node) override
    {
#warning finish
    }
    virtual void visitSequence(ast::Sequence *node) override
    {
#warning finish
    }
    virtual void visitTerminal(ast::Terminal *node) override
    {
#warning finish
    }
    virtual void visitCharacterClass(ast::CharacterClass *node) override
    {
#warning finish
    }
    virtual void visitEOFTerminal(ast::EOFTerminal *node) override
    {
#warning finish
    }
};

std::unique_ptr<CodeGenerator> CodeGenerator::makeCPlusPlus11(
    std::ostream &sourceFile, std::ostream &headerFile, std::string headerFileNameFromSourceFile)
{
    return std::unique_ptr<CodeGenerator>(
        new CPlusPlus11(sourceFile, headerFile, headerFileNameFromSourceFile));
}
