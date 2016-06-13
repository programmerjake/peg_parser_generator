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
#include "ast/empty.h"
#include "ast/expression.h"
#include "ast/ordered_choice.h"
#include "ast/predicate.h"
#include "ast/repetition.h"
#include "ast/sequence.h"
#include "ast/terminal.h"
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
    std::size_t nextPartNumber = 0;
    std::size_t currentPartNumber = 0;
    const ast::Nonterminal *nonterminal = nullptr;
    CPlusPlus11(std::ostream &finalSourceFile,
                std::ostream &finalHeaderFile,
                std::string headerFileNameFromSourceFile)
        : finalSourceFile(finalSourceFile),
          finalHeaderFile(finalHeaderFile),
          headerFileNameFromSourceFile(std::move(headerFileNameFromSourceFile))
    {
    }
    static std::string escapeChar(char32_t ch)
    {
        return ast::DumpVisitor::escapeCharacter(ch);
    }
    static std::string getCharName(char32_t ch)
    {
        switch(ch)
        {
        case '\n':
            return "end of line ('\\n')";
        case '\r':
            return "end of line ('\\r')";
        case '\t':
            return "tab (\\t)";
        case ' ':
            return "space (' ')";
        default:
            if(ch <= 0x20 || ch >= 0x7F)
            {
                std::ostringstream ss;
                ss << "character with code " << static_cast<unsigned long>(ch) << " (0x" << std::hex
                   << std::uppercase << static_cast<unsigned long>(ch) << ")";
                return ss.str();
            }
            return std::string(1, static_cast<char>(ch));
        }
    }
    static std::string escapeString(const std::string &str)
    {
        std::string retval;
        retval.reserve(str.size() * 5 / 4);
        for(unsigned char ch : str)
        {
            retval += escapeChar(ch);
        }
        return retval;
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
            return inputEndLocation == std::string::npos;
        }
        constexpr bool success() const
        {
            return !empty() && message == nullptr;
        }
        constexpr bool fail() const
        {
            return !empty() && message != nullptr;
        }
        static constexpr RuleResult makeSuccess(std::size_t inputStartLocation,
        ````````````````````````````````````````std::size_t inputEndLocation)
        {
            return RuleResult(inputEndLocation, inputStartLocation, inputEndLocation, nullptr);
        }
        static constexpr RuleResult makeSuccess(std::size_t location,
        ````````````````````````````````````````std::size_t inputStartLocation,
        ````````````````````````````````````````std::size_t inputEndLocation)
        {
            return RuleResult(location, inputStartLocation, inputEndLocation, nullptr);
        }
        static constexpr RuleResult makeFail(std::size_t inputStartLocation,
        `````````````````````````````````````std::size_t inputEndLocation,
        `````````````````````````````````````const char *message)
        {
            return RuleResult(inputStartLocation, inputStartLocation, inputEndLocation, message);
        }
        static constexpr RuleResult makeFail(std::size_t location,
        `````````````````````````````````````std::size_t inputStartLocation,
        `````````````````````````````````````std::size_t inputEndLocation,
        `````````````````````````````````````const char *message)
        {
            return RuleResult(location, inputStartLocation, inputEndLocation, message);
        }
        constexpr RuleResult makeInverse(const char *failureMessage) const
        {
            return fail() ? makeSuccess(inputStartLocation, inputStartLocation, inputEndLocation) :
            ````````````````makeFail(inputStartLocation, inputEndLocation, failureMessage);
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
    static std::pair<std::shared_ptr<const char32_t>, std::size_t> makeSource(
        std::u32string source);
    static std::pair<std::shared_ptr<const char32_t>, std::size_t> makeSource(
        const char *source, std::size_t sourceSize);

public:
    Parser(std::pair<std::shared_ptr<const char32_t>, std::size_t> source)
        : Parser(std::move(std::get<0>(source)), std::get<1>(source))
    {
    }
    Parser(std::shared_ptr<const char32_t> source, std::size_t sourceSize);
    Parser(std::u32string source);
    Parser(const char *source, std::size_t sourceSize);
    Parser(const char32_t *source, std::size_t sourceSize);
    Parser(const std::string &source) : Parser(source.data(), source.size())
    {
    }

public:
@+)";
        for(const ast::Nonterminal *nonterminal : grammar->nonterminals)
        {
            headerFile << "void " << translateName(nonterminal->name) << "();\n";
        }
        headerFile << R"(@-
private:
)";
        sourceFile
            << R"(Parser::Parser(std::shared_ptr<const char32_t> source, std::size_t sourceSize)
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

Parser::Parser(const char *source, std::size_t sourceSize) : Parser(makeSource(source, sourceSize))
{
}

Parser::Parser(const char32_t *source, std::size_t sourceSize)
    : Parser(makeSource(std::u32string(source, sourceSize)))
{
}

std::pair<std::shared_ptr<const char32_t>, std::size_t> Parser::makeSource(std::u32string source)
{
    auto sourceSize = source.size();
    auto pSource = std::make_shared<std::u32string>(std::move(source));
    return std::make_pair(std::shared_ptr<const char32_t>(pSource, pSource->data()), sourceSize);
}

std::pair<std::shared_ptr<const char32_t>, std::size_t> Parser::makeSource(const char *source,
```````````````````````````````````````````````````````````````````````````std::size_t sourceSize)
{
    std::u32string retval;
    retval.reserve(sourceSize);
    std::size_t position = 0;
    const char32_t replacementChar = U'\uFFFD';
    while(position < sourceSize)
    {
        unsigned long byte1 = source[position++];
        if(byte1 < 0x80)
        {
            retval += static_cast<char32_t>(byte1);
            continue;
        }
        if(position >= sourceSize || byte1 < 0xC0 || (source[position] & 0xC0) != 0x80)
        {
            retval += replacementChar;
            continue;
        }
        bool invalid = byte1 < 0xC2 || byte1 > 0xF4;
        unsigned long byte2 = source[position++];
        if(byte1 < 0xE0)
        {
            if(invalid)
                retval += replacementChar;
            else
                retval += static_cast<char32_t>(((byte1 & 0x1F) << 6) | (byte2 & 0x3F));
            continue;
        }
        if(position >= sourceSize || (source[position] & 0xC0) != 0x80)
        {
            retval += replacementChar;
            continue;
        }
        unsigned long byte3 = source[position++];
        if(byte1 < 0xF0)
        {
            if(byte1 == 0xE0 && byte2 < 0xA0)
                invalid = true;
            if(invalid)
                retval += replacementChar;
            else
                retval += static_cast<char32_t>(((byte1 & 0xF) << 12) | ((byte2 & 0x3F) << 6)
                                                | (byte3 & 0x3F));
            continue;
        }
        if(position >= sourceSize || (source[position] & 0xC0) != 0x80)
        {
            retval += replacementChar;
            continue;
        }
        unsigned long byte4 = source[position++];
        if(byte1 == 0xF0 && byte2 < 0x90)
            invalid = true;
        if(byte1 == 0xF4 && byte2 > 0x8F)
            invalid = true;
        if(byte1 > 0xF4)
            invalid = true;
        if(invalid)
            retval += replacementChar;
        else
            retval += static_cast<char32_t>(((byte1 & 0x7) << 18) | ((byte2 & 0x3F) << 12)
                                            | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F));
    }
    return makeSource(std::move(retval));
}
)";
        for(const ast::Nonterminal *nonterminal : grammar->nonterminals)
        {
            nextPartNumber = 0;
            currentPartNumber = nextPartNumber++;
            sourceFile << R"(
void Parser::)" << translateName(nonterminal->name) << R"(()
{
    auto result = )" << translateName(nonterminal->name, currentPartNumber) << R"((0);
    if(result.fail())
        throw ParseError(result);
}
)";
            headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                       << "(std::size_t startLocation);\n";
            sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                       << R"((std::size_t startLocation)
{
    RuleResult &retval = getResults(startLocation).)" << translateName(nonterminal->name) << R"(;
    if(!retval.empty())
        return retval;
    retval = )" << translateName(nonterminal->name, nextPartNumber) << R"((startLocation);
    return retval;
}
)";
            currentPartNumber = nextPartNumber++;
            this->nonterminal = nonterminal;
            nonterminal->expression->visit(*this);
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
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    return RuleResult::makeSuccess(startLocation, startLocation);
}
)";
    }
    virtual void visitGrammar(ast::Grammar *node) override
    {
        assert(false);
    }
    virtual void visitNonterminal(ast::Nonterminal *node) override
    {
        assert(false);
    }
    virtual void visitNonterminalExpression(ast::NonterminalExpression *node) override
    {
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    return )" << translateName(node->value->name, 0) << R"((startLocation);
}
)";
    }
    virtual void visitOrderedChoice(ast::OrderedChoice *node) override
    {
        auto firstPartNumber = nextPartNumber++;
        auto secondPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto firstResult = )" << translateName(nonterminal->name, firstPartNumber)
                   << R"((startLocation);
    if(firstResult.success())
        return firstResult;
    auto secondResult = )" << translateName(nonterminal->name, secondPartNumber)
                   << R"((startLocation);
    if(secondResult.success())
        return secondResult;
    if(firstResult.inputEndLocation >= secondResult.inputEndLocation)
        return firstResult;
    return secondResult;
}
)";
        currentPartNumber = firstPartNumber;
        node->first->visit(*this);
        currentPartNumber = secondPartNumber;
        node->second->visit(*this);
    }
    virtual void visitFollowedByPredicate(ast::FollowedByPredicate *node) override
    {
        auto expressionPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto expressionResult = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((startLocation);
    if(expressionResult.fail())
        return expressionResult;
    expressionResult.location = startLocation;
    return expressionResult;
}
)";
        currentPartNumber = expressionPartNumber;
        node->expression->visit(*this);
    }
    virtual void visitNotFollowedByPredicate(ast::NotFollowedByPredicate *node) override
    {
        auto expressionPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto expressionResult = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((startLocation);
    return expressionResult.makeInverse("not allowed here");
}
)";
        currentPartNumber = expressionPartNumber;
        node->expression->visit(*this);
    }
    virtual void visitCustomPredicate(ast::CustomPredicate *node) override
    {
#warning finish
    }
    virtual void visitGreedyRepetition(ast::GreedyRepetition *node) override
    {
        auto expressionPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto retval = RuleResult::makeSuccess(startLocation, startLocation);
    while(true)
    {
        auto expressionResult = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((retval.location);
        if(expressionResult.fail())
            break;
        retval = RuleResult::makeSuccess(
            expressionResult.location, startLocation, expressionResult.inputEndLocation);
    }
    return retval;
}
)";
        currentPartNumber = expressionPartNumber;
        node->expression->visit(*this);
    }
    virtual void visitGreedyPositiveRepetition(ast::GreedyPositiveRepetition *node) override
    {
        auto expressionPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto retval = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((startLocation);
    if(retval.fail())
        return retval;
    while(true)
    {
        auto expressionResult = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((retval.location);
        if(expressionResult.fail())
            break;
        retval = RuleResult::makeSuccess(
            expressionResult.location, startLocation, expressionResult.inputEndLocation);
    }
    return retval;
}
)";
        currentPartNumber = expressionPartNumber;
        node->expression->visit(*this);
    }
    virtual void visitOptionalExpression(ast::OptionalExpression *node) override
    {
        auto expressionPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto retval = )" << translateName(nonterminal->name, expressionPartNumber)
                   << R"((startLocation);
    if(retval.fail())
        return RuleResult::makeSuccess(startLocation, startLocation);
    return retval;
}
)";
        currentPartNumber = expressionPartNumber;
        node->expression->visit(*this);
    }
    virtual void visitSequence(ast::Sequence *node) override
    {
        auto firstPartNumber = nextPartNumber++;
        auto secondPartNumber = nextPartNumber++;
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    auto firstResult = )" << translateName(nonterminal->name, firstPartNumber)
                   << R"((startLocation);
    if(firstResult.fail())
        return firstResult;
    return )" << translateName(nonterminal->name, secondPartNumber) << R"((firstResult.location);
}
)";
        currentPartNumber = firstPartNumber;
        node->first->visit(*this);
        currentPartNumber = secondPartNumber;
        node->second->visit(*this);
    }
    virtual void visitTerminal(ast::Terminal *node) override
    {
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    if(startLocation >= sourceSize)
    {
        return RuleResult::makeFail(startLocation, startLocation, "missing )"
                   << escapeString(getCharName(node->value)) << R"(");
    }
    if(source[startLocation] == U')" << escapeChar(node->value) << R"(')
    {
        return RuleResult::makeSuccess(startLocation + 1, startLocation, startLocation + 1);
    }
    return RuleResult::makeFail(startLocation, startLocation, "missing )"
                   << escapeString(getCharName(node->value)) << R"(");
}
)";
    }
    virtual void visitCharacterClass(ast::CharacterClass *node) override
    {
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    if(startLocation >= sourceSize)
    {
        return RuleResult::makeFail(startLocation, startLocation, "missing character");
    }
    bool matches = false;
)";
        auto elseString = "";
        for(const auto &range : node->characterRanges.ranges)
        {
            if(range.min == range.max)
            {
                sourceFile << R"(    )" << elseString << R"(if(source[startLocation] == U')"
                           << escapeChar(range.min) << R"(')
    {
        matches = true;
    }
)";
            }
            else
            {
                sourceFile << R"(    )" << elseString << R"(if(source[startLocation] >= U')"
                           << escapeChar(range.min) << R"(' && source[startLocation] <= U')"
                           << escapeChar(range.max) << R"(')
    {
        matches = true;
    }
)";
            }
            elseString = "else ";
        }
        if(node->inverted)
        {
            sourceFile << R"(    if(!matches))";
        }
        else
        {
            sourceFile << R"(    if(matches))";
        }
        sourceFile << R"(
        return RuleResult::makeSuccess(startLocation + 1, startLocation, startLocation + 1);
    return RuleResult::makeFail(
        startLocation, startLocation, startLocation + 1, "missing character");
}
)";
    }
    virtual void visitEOFTerminal(ast::EOFTerminal *node) override
    {
        headerFile << "    RuleResult " << translateName(nonterminal->name, currentPartNumber)
                   << "(std::size_t startLocation);\n";
        sourceFile << R"(
Parser::RuleResult Parser::)" << translateName(nonterminal->name, currentPartNumber)
                   << R"((std::size_t startLocation)
{
    if(startLocation >= sourceSize)
    {
        return RuleResult::makeSuccess(startLocation, startLocation);
    }
    return RuleResult::makeFail(startLocation, startLocation, "expected end of file");
}
)";
    }
};

std::unique_ptr<CodeGenerator> CodeGenerator::makeCPlusPlus11(
    std::ostream &sourceFile, std::ostream &headerFile, std::string headerFileNameFromSourceFile)
{
    return std::unique_ptr<CodeGenerator>(
        new CPlusPlus11(sourceFile, headerFile, headerFileNameFromSourceFile));
}
