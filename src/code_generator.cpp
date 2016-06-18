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
#include "ast/code.h"
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
    std::string headerFileName;
    std::string headerFileNameFromSourceFile;
    std::string sourceFileName;
    const std::size_t indentSize = 4;
    const std::size_t tabSize = 0;
    const ast::Nonterminal *nonterminal = nullptr;
    enum class State
    {
        DeclareLocals,
        ParseAndEvaluateFunction,
    };
    State state = State::ParseAndEvaluateFunction;
    bool needsIsRequiredForSuccess = false;
    CPlusPlus11(std::ostream &finalSourceFile,
                std::ostream &finalHeaderFile,
                std::string headerFileName,
                std::string headerFileNameFromSourceFile,
                std::string sourceFileName)
        : finalSourceFile(finalSourceFile),
          finalHeaderFile(finalHeaderFile),
          headerFileName(std::move(headerFileName)),
          headerFileNameFromSourceFile(std::move(headerFileNameFromSourceFile)),
          sourceFileName(std::move(sourceFileName))
    {
    }
    static std::string escapeChar(char32_t ch)
    {
        return ast::DumpVisitor::escapeCharacter(ch);
    }
    static std::string escapeCharForCharacterClass(char32_t ch)
    {
        if(ch == '-' || ch == '^' || ch == ']')
            return std::string("\\") + static_cast<char>(ch);
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
    static std::string getCharacterClassMatchFailMessage(const ast::CharacterClass *characterClass)
    {
        std::ostringstream ss;
        if(!characterClass->inverted)
            ss << "missing ";
        if(characterClass->characterRanges.matchesClassifier(
               ast::CharacterClass::CharacterRanges::DecimalDigitClassifier()))
        {
            ss << "decimal digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::OctalDigitClassifier()))
        {
            ss << "octal digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::HexDigitClassifier()))
        {
            ss << "hexadecimal digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LowercaseHexDigitClassifier()))
        {
            ss << "lowercase hexadecimal digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::UppercaseHexDigitClassifier()))
        {
            ss << "uppercase hexadecimal digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterClassifier()))
        {
            ss << "letter";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LowercaseLetterClassifier()))
        {
            ss << "lowercase letter";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::UppercaseLetterClassifier()))
        {
            ss << "uppercase letter";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterOrDigitClassifier()))
        {
            ss << "letter or digit";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::UppercaseLetterOrUnderlineClassifier()))
        {
            ss << "uppercase letter or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LowercaseLetterOrUnderlineClassifier()))
        {
            ss << "lowercase letter or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterOrUnderlineClassifier()))
        {
            ss << "letter or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::DigitOrUnderlineClassifier()))
        {
            ss << "digit or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterDigitOrUnderlineClassifier()))
        {
            ss << "letter, digit, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::
                        UppercaseLetterDollarOrUnderlineClassifier()))
        {
            ss << "uppercase letter, $, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::
                        LowercaseLetterDollarOrUnderlineClassifier()))
        {
            ss << "lowercase letter, $, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterDollarOrUnderlineClassifier()))
        {
            ss << "letter, $, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LetterDigitDollarOrUnderlineClassifier()))
        {
            ss << "letter, digit, $, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::DigitDollarOrUnderlineClassifier()))
        {
            ss << "digit, $, or _";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::SpaceOrTabClassifier()))
        {
            ss << "space or tab";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::SpaceTabOrLineEndingClassifier()))
        {
            ss << "space, tab, or line ending";
        }
        else if(characterClass->characterRanges.matchesClassifier(
                    ast::CharacterClass::CharacterRanges::LineEndingClassifier()))
        {
            ss << "line ending";
        }
        else
        {
            std::uint_fast32_t totalCharCount = 0;
            const std::size_t firstCharsSize = 5;
            char32_t firstChars[firstCharsSize];
            std::size_t firstCharsUsed = 0;
            for(const auto &range : characterClass->characterRanges.ranges)
            {
                assert(!range.empty());
                totalCharCount += range.max - range.min + 1;
                for(std::uint_fast32_t i = range.min; i <= range.max; i++)
                {
                    if(firstCharsUsed < firstCharsSize)
                    {
                        firstChars[firstCharsUsed++] = i;
                    }
                }
            }
            if(totalCharCount == 1)
            {
                assert(firstCharsUsed == 1);
                ss << getCharName(firstChars[0]);
            }
            else if(totalCharCount == 2)
            {
                assert(firstCharsUsed == 2);
                ss << getCharName(firstChars[0]) << " or " << getCharName(firstChars[1]);
            }
            else if(totalCharCount > 1 && totalCharCount <= firstCharsSize)
            {
                assert(firstCharsUsed == totalCharCount);
                ss << getCharName(firstChars[0]);
                for(std::size_t i = 1; i < totalCharCount; i++)
                {
                    ss << ", ";
                    if(i + 1 == totalCharCount)
                        ss << "or ";
                    ss << getCharName(firstChars[i]);
                }
            }
            else
            {
                ss << "[";
                for(const auto &range : characterClass->characterRanges.ranges)
                {
                    if(range.min == range.max)
                    {
                        ss << escapeCharForCharacterClass(range.min);
                    }
                    else
                    {
                        ss << escapeCharForCharacterClass(range.min);
                        ss << '-';
                        ss << escapeCharForCharacterClass(range.max);
                    }
                }
                ss << "]";
            }
        }
        if(characterClass->inverted)
            ss << " not allowed here";
        return ss.str();
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
    static std::string translateName(const char *prefix, std::string name, const char *suffix)
    {
        assert(!name.empty());
        name[0] = std::toupper(name[0]);
        return prefix + std::move(name) + suffix;
    }
    static std::string makeResultVariableName(std::string name)
    {
        return translateName("result", std::move(name), "");
    }
    static std::string makeParseFunctionName(std::string name)
    {
        return translateName("parse", std::move(name), "");
    }
    static std::string makeInternalParseFunctionName(std::string name)
    {
        return translateName("internalParse", std::move(name), "");
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
    void reindent(std::ostream &os, const std::string &source, const std::string &fileName) const
    {
        bool isAtStartOfLine = true;
        std::size_t indentDepth = 0;
        std::size_t startIndentDepth = 0;
        std::size_t savedIndentDepth = 0;
        std::size_t escapedCountLeft = 0;
        std::size_t lineNumber = 1;
        for(std::size_t i = 0; i < source.size(); i++)
        {
            char ch = source[i];
            if(escapedCountLeft > 0)
            {
                if(ch == '\n')
                {
                    lineNumber++;
                }
                os << ch;
                escapedCountLeft--;
                continue;
            }
            assert(ch != '\r');
            assert(ch != '\t');
            assert(ch != '\f');
            assert(ch != '\0');
            if(ch == '\n')
            {
                lineNumber++;
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
                    case 's':
                        savedIndentDepth = startIndentDepth;
                        continue;
                    case 'l':
                    {
                        std::ostringstream ss;
                        lineNumber++;
                        ss << "#line " << lineNumber << " \"" << escapeString(fileName) << "\"\n";
                        i++;
                        assert(i < source.size() && source[i] == '\n');
                        os << ss.str();
                        continue;
                    }
                    case '0':
                        startIndentDepth = 0;
                        indentDepth = 0;
                        continue;
                    case 'r':
                        startIndentDepth = savedIndentDepth;
                        indentDepth = savedIndentDepth;
                        continue;
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'D':
                    case 'E':
                    case 'F':
                    {
                        assert(escapedCountLeft == 0);
                        if(source[i] >= '0' && source[i] <= '9')
                            escapedCountLeft = source[i] - '0';
                        else
                            escapedCountLeft = source[i] - 'A' + 10;
                        i++;
                        assert(i < source.size());
                        while((source[i] >= '0' && source[i] <= '9')
                              || (source[i] >= 'A' && source[i] <= 'F'))
                        {
                            escapedCountLeft *= 0x10;
                            if(source[i] >= '0' && source[i] <= '9')
                                escapedCountLeft += source[i] - '0';
                            else
                                escapedCountLeft += source[i] - 'A' + 10;
                            i++;
                            assert(i < source.size());
                        }
                        assert(source[i] == ';');
                        continue;
                    }
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
        assert(escapedCountLeft == 0);
    }
    std::string makeEscape(std::size_t size)
    {
        if(size == 0)
            return "";
        std::ostringstream ss;
        ss << std::uppercase;
        ss << std::hex;
        ss << "@" << size << ";";
        return ss.str();
    }
    void writeCode(std::ostream &os, std::string code, Location location)
    {
        code.insert(0, location.getColumn() - 1, ' ');
        os << R"(@s@0#line )" << location.getLine() << R"( ")"
           << escapeString(location.source->fileName) << R"("
)" << makeEscape(code.size()) << code << R"(
@l
@r)";
    }
    virtual void generateCode(const ast::Grammar *grammar) override
    {
        auto guardMacroName = getGuardMacroName();
        sourceFile << R"(// automatically generated from )" << grammar->location.source->fileName
                   << R"(
)";
        headerFile << R"(// automatically generated from )" << grammar->location.source->fileName
                   << R"(
)";
        for(auto topLevelCodeSnippet : grammar->topLevelCodeSnippets)
        {
            if(topLevelCodeSnippet->kind == ast::TopLevelCodeSnippet::Kind::License)
            {
                writeCode(sourceFile, topLevelCodeSnippet->code, topLevelCodeSnippet->location);
                writeCode(headerFile, topLevelCodeSnippet->code, topLevelCodeSnippet->location);
            }
        }
        sourceFile << R"(#include ")" << headerFileNameFromSourceFile << R"("

namespace parser
{
)";
        headerFile << R"(#ifndef )" << guardMacroName << R"(
#define )" << guardMacroName << R"(

#include <utility>
#include <cstddef>
#include <string>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <list>
#include <cassert>
)";
        for(auto topLevelCodeSnippet : grammar->topLevelCodeSnippets)
        {
            if(topLevelCodeSnippet->kind == ast::TopLevelCodeSnippet::Kind::Header)
            {
                writeCode(headerFile, topLevelCodeSnippet->code, topLevelCodeSnippet->location);
            }
        }
        headerFile << R"(
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
        std::size_t endLocation;
        bool isSuccess;
        constexpr RuleResult() noexcept : location(std::string::npos),
        ``````````````````````````````````endLocation(0),
        ``````````````````````````````````isSuccess(false)
        {
        }
        constexpr RuleResult(std::size_t location, std::size_t endLocation, bool success) noexcept
            : location(location),
            ``endLocation(endLocation),
            ``isSuccess(success)
        {
        }
        constexpr bool empty() const
        {
            return location == std::string::npos;
        }
        constexpr bool success() const
        {
            return !empty() && isSuccess;
        }
        constexpr bool fail() const
        {
            return !empty() && !isSuccess;
        }
    };
    struct Results final
    {
@+@+)";
        for(const ast::Nonterminal *nonterminal : grammar->nonterminals)
        {
            if(nonterminal->settings.caching)
                headerFile << "RuleResult " << makeResultVariableName(nonterminal->name) << ";\n";
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
    };

private:
    std::vector<Results *> resultsPointers;
    std::list<ResultsChunk> resultsChunks;
    Results eofResults;
    const std::shared_ptr<const char32_t> source;
    const std::size_t sourceSize;
    std::size_t errorLocation = 0;
    std::size_t errorInputEndLocation = 0;
    const char *errorMessage = "no error";

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
    RuleResult makeFail(std::size_t location,
    ````````````````````std::size_t inputEndLocation,
    ````````````````````const char *message,
    ````````````````````bool isRequiredForSuccess)
    {
        if(isRequiredForSuccess && errorInputEndLocation <= inputEndLocation)
        {
            errorLocation = location;
            errorInputEndLocation = inputEndLocation;
            errorMessage = message;
        }
        return RuleResult(location, inputEndLocation, false);
    }
    RuleResult makeFail(std::size_t inputEndLocation,
    ````````````````````const char *message,
    ````````````````````bool isRequiredForSuccess)
    {
        return makeFail(inputEndLocation, inputEndLocation, message, isRequiredForSuccess);
    }
    static RuleResult makeSuccess(std::size_t location, std::size_t inputEndLocation)
    {
        assert(location != std::string::npos);
        return RuleResult(location, inputEndLocation, true);
    }
    static RuleResult makeSuccess(std::size_t inputEndLocation)
    {
        assert(inputEndLocation != std::string::npos);
        return RuleResult(inputEndLocation, inputEndLocation, true);
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
            headerFile << nonterminal->type->code << " " << makeParseFunctionName(nonterminal->name)
                       << "();\n";
        }
        headerFile << R"(@-
private:
)";
        for(auto topLevelCodeSnippet : grammar->topLevelCodeSnippets)
        {
            if(topLevelCodeSnippet->kind == ast::TopLevelCodeSnippet::Kind::Source)
            {
                writeCode(sourceFile, topLevelCodeSnippet->code, topLevelCodeSnippet->location);
            }
        }
        sourceFile << R"(
Parser::Parser(std::shared_ptr<const char32_t> source, std::size_t sourceSize)
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
            headerFile << "    " << nonterminal->type->code << " "
                       << makeInternalParseFunctionName(nonterminal->name)
                       << "(std::size_t startLocation, RuleResult &ruleResult, bool "
                          "isRequiredForSuccess);\n";
            sourceFile << R"(
)" << nonterminal->type->code << R"( Parser::)" << makeParseFunctionName(nonterminal->name) << R"(()
{
    RuleResult result;
    )" << (nonterminal->type->isVoid ? "" : "auto retval = ")
                       << makeInternalParseFunctionName(nonterminal->name) << R"((0, result, true);
    assert(!result.empty());
    if(result.fail())
        throw ParseError(errorLocation, errorMessage);
)";
            if(!nonterminal->type->isVoid)
            {
                sourceFile << R"(    return retval;
)";
            }
            sourceFile << R"(}

)";
            sourceFile
                << nonterminal->type->code << R"( Parser::)"
                << makeInternalParseFunctionName(nonterminal->name)
                << R"((std::size_t startLocation__, RuleResult &ruleResultOut__, bool isRequiredForSuccess__)
{
@+)";
            if(!nonterminal->type->isVoid)
            {
                sourceFile << nonterminal->type->code << R"( returnValue__{};
)";
            }
            this->nonterminal = nonterminal;
            needsIsRequiredForSuccess = false;
            state = State::DeclareLocals;
            nonterminal->expression->visit(*this);
            if(nonterminal->settings.caching)
            {
                needsIsRequiredForSuccess = true;
                sourceFile << R"(auto &ruleResult__ = this->getResults(startLocation__).)"
                           << makeResultVariableName(nonterminal->name) << R"(;
if(!ruleResult__.empty() && (ruleResult__.fail() || !isRequiredForSuccess__))
{
    ruleResultOut__ = ruleResult__;
)";
                if(nonterminal->type->isVoid)
                {
                    sourceFile << R"(    return;
}
)";
                }
                else
                {
                    sourceFile << R"(    return returnValue__;
}
)";
                }
            }
            else
            {
                sourceFile << R"(Parser::RuleResult ruleResult__;
)";
            }
            this->nonterminal = nonterminal;
            state = State::ParseAndEvaluateFunction;
            nonterminal->expression->visit(*this);
            if(!needsIsRequiredForSuccess)
            {
                sourceFile << R"(static_cast<void>(isRequiredForSuccess__);
)";
            }
            if(nonterminal->type->name == "char")
            {
                if(auto characterClass =
                       dynamic_cast<ast::CharacterClass *>(nonterminal->expression))
                {
                    if(characterClass->variableName.empty())
                    {
                        sourceFile << R"(if(ruleResultOut__.success())
    returnValue__ = this->source.get()[startLocation__];
)";
                    }
                }
            }
            sourceFile << R"(ruleResultOut__ = ruleResult__;
)";
            if(!nonterminal->type->isVoid)
            {
                sourceFile << R"(return returnValue__;
)";
            }
            sourceFile << R"(@-}
)";
        }
        headerFile << R"(};
}

#endif /* )" << guardMacroName << R"( */
)";
        sourceFile << R"(}
)";
        reindent(finalHeaderFile, headerFile.str(), headerFileName);
        reindent(finalSourceFile, sourceFile.str(), sourceFileName);
    }
    virtual void visitEmpty(ast::Empty *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            break;
        case State::ParseAndEvaluateFunction:
            sourceFile << R"(ruleResult__ = this->makeSuccess(startLocation__);
    )";
            break;
        }
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
        switch(state)
        {
        case State::DeclareLocals:
            if(!node->variableName.empty())
            {
                sourceFile << node->value->type->code << R"( )" << node->variableName << R"({};
)";
            }
            break;
        case State::ParseAndEvaluateFunction:
            sourceFile << R"(ruleResult__ = Parser::RuleResult();
)";
            if(!node->variableName.empty())
            {
                sourceFile << node->variableName << R"( = )";
            }
            needsIsRequiredForSuccess = true;
            sourceFile << R"(this->)" << makeInternalParseFunctionName(node->value->name)
                       << R"((startLocation__, ruleResult__, isRequiredForSuccess__);
assert(!ruleResult__.empty());
)";
            break;
        }
    }
    virtual void visitOrderedChoice(ast::OrderedChoice *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->first->visit(*this);
            node->second->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            node->first->visit(*this);
            sourceFile << R"(if(ruleResult__.fail())
{
    Parser::RuleResult lastRuleResult__ = ruleResult__;
@+)";
            node->second->visit(*this);
            sourceFile << R"(@_if(ruleResult__.success())
    {
        if(lastRuleResult__.endLocation >= ruleResult__.endLocation)
        {
            ruleResult__.endLocation = lastRuleResult__.endLocation;
        }
    }
}
)";
            break;
        }
    }
    virtual void visitFollowedByPredicate(ast::FollowedByPredicate *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->expression->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            node->expression->visit(*this);
            sourceFile << R"(if(ruleResult__.success())
    ruleResult__.location = startLocation__;
)";
            break;
        }
    }
    virtual void visitNotFollowedByPredicate(ast::NotFollowedByPredicate *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->expression->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            needsIsRequiredForSuccess = true;
            sourceFile << R"(isRequiredForSuccess__ = !isRequiredForSuccess__;
)";
            node->expression->visit(*this);
            sourceFile << R"(isRequiredForSuccess__ = !isRequiredForSuccess__;
if(ruleResult__.success())
    ruleResult__ = this->makeFail(startLocation__, "not allowed here", isRequiredForSuccess__);
else
    ruleResult__ = this->makeSuccess(startLocation__);
)";
            break;
        }
    }
    virtual void visitCustomPredicate(ast::CustomPredicate *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->codeSnippet->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            sourceFile << R"({
    const char *predicateReturnValue__ = nullptr;
@+)";
            node->codeSnippet->visit(*this);
            needsIsRequiredForSuccess = true;
            sourceFile << R"(@_if(predicateReturnValue__ != nullptr)
        ruleResult__ = this->makeFail(startLocation__, predicateReturnValue__, isRequiredForSuccess__);
}
)";
            break;
        }
    }
    virtual void visitGreedyRepetition(ast::GreedyRepetition *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->expression->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            sourceFile << R"(ruleResult__ = this->makeSuccess(startLocation__);
{
    auto savedStartLocation__ = startLocation__;
    auto &savedRuleResult__ = ruleResult__;
    while(true)
    {
        Parser::RuleResult ruleResult__;
        startLocation__ = savedRuleResult__.location;
@+@+)";
            node->expression->visit(*this);
            sourceFile << R"(@_@_if(ruleResult__.fail() || ruleResult__.location == startLocation__)
        {
            savedRuleResult__ = this->makeSuccess(savedRuleResult__.location, ruleResult__.endLocation);
            startLocation__ = savedStartLocation__;
            break;
        }
        savedRuleResult__ = this->makeSuccess(ruleResult__.location, ruleResult__.endLocation);
    }
}
)";
            break;
        }
    }
    virtual void visitGreedyPositiveRepetition(ast::GreedyPositiveRepetition *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->expression->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            node->expression->visit(*this);
            sourceFile << R"(if(ruleResult__.success())
{
    auto savedStartLocation__ = startLocation__;
    auto &savedRuleResult__ = ruleResult__;
    while(true)
    {
        Parser::RuleResult ruleResult__;
        startLocation__ = savedRuleResult__.location;
@+@+)";
            node->expression->visit(*this);
            sourceFile << R"(@_@_if(ruleResult__.fail() || ruleResult__.location == startLocation__)
        {
            savedRuleResult__ = this->makeSuccess(savedRuleResult__.location, ruleResult__.endLocation);
            startLocation__ = savedStartLocation__;
            break;
        }
        savedRuleResult__ = this->makeSuccess(ruleResult__.location, ruleResult__.endLocation);
    }
}
)";
            break;
        }
    }
    virtual void visitOptionalExpression(ast::OptionalExpression *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->expression->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            node->expression->visit(*this);
            sourceFile << R"(if(ruleResult__.fail())
    ruleResult__ = this->makeSuccess(startLocation__);
)";
            break;
        }
    }
    virtual void visitSequence(ast::Sequence *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            node->first->visit(*this);
            node->second->visit(*this);
            break;
        case State::ParseAndEvaluateFunction:
            node->first->visit(*this);
            sourceFile << R"(if(ruleResult__.success())
{
    auto savedStartLocation__ = startLocation__;
    startLocation__ = ruleResult__.location;
@+)";
            node->second->visit(*this);
            sourceFile << R"(@_startLocation__ = savedStartLocation__;
}
)";
            break;
        }
    }
    virtual void visitTerminal(ast::Terminal *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            break;
        case State::ParseAndEvaluateFunction:
            needsIsRequiredForSuccess = true;
            sourceFile << R"(if(startLocation__ >= this->sourceSize)
{
    ruleResult__ = this->makeFail(startLocation__, "missing )"
                       << escapeString(getCharName(node->value)) << R"(", isRequiredForSuccess__);
}
else if(this->source.get()[startLocation__] == U')" << escapeChar(node->value) << R"(')
{
    ruleResult__ = this->makeSuccess(startLocation__ + 1, startLocation__ + 1);
}
else
{
    ruleResult__ = this->makeFail(startLocation__, startLocation__ + 1, "missing )"
                       << escapeString(getCharName(node->value)) << R"(", isRequiredForSuccess__);
}
)";
            break;
        }
    }
    virtual void visitCharacterClass(ast::CharacterClass *node) override
    {
        std::string matchFailMessage = getCharacterClassMatchFailMessage(node);
        switch(state)
        {
        case State::DeclareLocals:
            if(!node->variableName.empty())
            {
                sourceFile << R"(char32_t )" << node->variableName << R"({};
)";
            }
            break;
        case State::ParseAndEvaluateFunction:
        {
            needsIsRequiredForSuccess = true;
            sourceFile << R"(if(startLocation__ >= this->sourceSize)
{
    ruleResult__ = this->makeFail(startLocation__, "unexpected end of input", isRequiredForSuccess__);
}
else
{
    bool matches = false;
)";
            auto elseString = "";
            for(const auto &range : node->characterRanges.ranges)
            {
                if(range.min == range.max)
                {
                    sourceFile << R"(    )" << elseString
                               << R"(if(this->source.get()[startLocation__] == U')"
                               << escapeChar(range.min) << R"(')
    {
        matches = true;
    }
)";
                }
                else
                {
                    sourceFile << R"(    )" << elseString
                               << R"(if(this->source.get()[startLocation__] >= U')"
                               << escapeChar(range.min)
                               << R"(' && this->source.get()[startLocation__] <= U')"
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
    {
        ruleResult__ = this->makeSuccess(startLocation__ + 1, startLocation__ + 1);
)";
            if(state == State::ParseAndEvaluateFunction && !node->variableName.empty())
            {
                sourceFile << R"(        )" << node->variableName
                           << R"( = this->source.get()[startLocation__];
)";
            }
            sourceFile << R"(    }
    else
    {
        ruleResult__ = this->makeFail(startLocation__, startLocation__ + 1, ")"
                       << escapeString(matchFailMessage) << R"(", isRequiredForSuccess__);
    }
}
)";
            break;
        }
        }
    }
    virtual void visitEOFTerminal(ast::EOFTerminal *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            break;
        case State::ParseAndEvaluateFunction:
            needsIsRequiredForSuccess = true;
            sourceFile << R"(if(startLocation__ >= this->sourceSize)
{
    ruleResult__ = this->makeSuccess(startLocation__);
}
else
{
    ruleResult__ = this->makeFail(startLocation__, startLocation__, "expected end of file", isRequiredForSuccess__);
}
)";
            break;
        }
    }
    virtual void visitExpressionCodeSnippet(ast::ExpressionCodeSnippet *node) override
    {
        switch(state)
        {
        case State::DeclareLocals:
            break;
        case State::ParseAndEvaluateFunction:
        {
            std::string code = node->code;
            for(auto iter = node->substitutions.rbegin(); iter != node->substitutions.rend();
                ++iter)
            {
                auto substitution = *iter;
                switch(substitution.kind)
                {
                case ast::ExpressionCodeSnippet::Substitution::Kind::ReturnValue:
                    code.insert(substitution.position, "returnValue__");
                    continue;
                case ast::ExpressionCodeSnippet::Substitution::Kind::PredicateReturnValue:
                    code.insert(substitution.position, "predicateReturnValue__");
                    continue;
                }
                assert(false);
            }
            sourceFile << R"({
)";
            writeCode(sourceFile, std::move(code), node->location);
            sourceFile << R"(}
ruleResult__ = this->makeSuccess(startLocation__);
)";
            break;
        }
        }
    }
    virtual void visitTopLevelCodeSnippet(ast::TopLevelCodeSnippet *node) override
    {
    }
    virtual void visitType(ast::Type *node) override
    {
    }
};

std::unique_ptr<CodeGenerator> CodeGenerator::makeCPlusPlus11(
    std::ostream &sourceFile,
    std::ostream &headerFile,
    std::string headerFileName,
    std::string headerFileNameFromSourceFile,
    std::string sourceFileName)
{
    return std::unique_ptr<CodeGenerator>(new CPlusPlus11(sourceFile,
                                                          headerFile,
                                                          std::move(headerFileName),
                                                          std::move(headerFileNameFromSourceFile),
                                                          std::move(sourceFileName)));
}
