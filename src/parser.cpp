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

#include "parser.h"
#include "error.h"
#include "location.h"
#include "ast/grammar.h"
#include "ast/nonterminal.h"
#include "ast/empty.h"
#include "ast/expression.h"
#include "ast/ordered_choice.h"
#include "ast/predicate.h"
#include "ast/repetition.h"
#include "ast/sequence.h"
#include "ast/terminal.h"
#include "ast/type.h"
#include "ast/code.h"
#include "arena.h"
#include "source.h"
#include <cassert>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

struct Parser final
{
    static constexpr int eof = -1;
    static int getDigitValue(int ch, int base = 36)
    {
        int retval;
        if(ch >= '0' && ch <= '9')
            retval = ch - '0';
        else if(ch >= 'a' && ch <= 'z')
            retval = ch - 'a' + 10;
        else if(ch >= 'A' && ch <= 'Z')
            retval = ch - 'A' + 10;
        else
            return -1;
        if(retval >= base)
            return -1;
        return retval;
    }
    struct Token final
    {
        enum class Type
        {
            EndOfFile,
            Semicolon,
            Colon,
            ColonColon,
            QMark,
            Plus,
            EMark,
            Star,
            FSlash,
            Equal,
            LParen,
            RParen,
            Amp,
            String,
            Identifier,
            EOFKeyword,
            TypedefKeyword,
            CodeKeyword,
            CharacterClass,
            CodeSnippet,
        };
        Location location;
        Type type;
        std::string value;
        std::vector<ast::ExpressionCodeSnippet::Substitution> substitutions;
        Token() : location(), type(Type::EndOfFile), value(), substitutions()
        {
        }
        Token(Location location, Type type, std::string value)
            : location(std::move(location)), type(type), value(std::move(value)), substitutions()
        {
        }
        Token(Location location,
              Type type,
              std::string value,
              std::vector<ast::ExpressionCodeSnippet::Substitution> substitutions)
            : location(std::move(location)),
              type(type),
              value(std::move(value)),
              substitutions(std::move(substitutions))
        {
        }
    };
    struct Tokenizer final
    {
        Location currentLocation;
        int peek;
        Tokenizer(const Source *source)
            : currentLocation(source, 0),
              peek(source->contents.empty() ? eof : static_cast<unsigned char>(source->contents[0]))
        {
        }
        int get()
        {
            int retval = peek;
            currentLocation.position++;
            if(currentLocation.position < currentLocation.source->contents.size())
            {
                peek = static_cast<unsigned char>(
                    currentLocation.source->contents[currentLocation.position]);
            }
            else
                peek = eof;
            return retval;
        }
        static bool isIdentifierStart(int ch)
        {
            if(ch >= 'a' && ch <= 'z')
                return true;
            if(ch >= 'A' && ch <= 'Z')
                return true;
            return false;
        }
        static bool isIdentifierContinue(int ch)
        {
            if(isIdentifierStart(ch))
                return true;
            if(ch >= '0' && ch <= '9')
                return true;
            if(ch == '_')
                return true;
            return false;
        }
        Token parseToken(ErrorHandler &errorHandler)
        {
            while(peek != eof
                  && (peek == ' ' || peek == '\t' || peek == '\r' || peek == '\n' || peek == '/'))
            {
                if(peek == '/')
                {
                    auto slashLocation = currentLocation;
                    get();
                    if(peek == '/')
                    {
                        while(peek != eof && peek != '\r' && peek != '\n')
                            get();
                    }
                    else if(peek == '*')
                    {
                        get();
                        while(true)
                        {
                            if(peek == '*')
                            {
                                while(peek == '*')
                                    get();
                                if(peek == '/')
                                {
                                    get();
                                    break;
                                }
                            }
                            else if(peek == eof)
                            {
                                errorHandler(
                                    ErrorLevel::FatalError, slashLocation, "missing closing */");
                                break;
                            }
                            else
                            {
                                get();
                            }
                        }
                    }
                    else
                    {
                        return Token(slashLocation, Token::Type::FSlash, "");
                    }
                }
                else
                {
                    get();
                }
            }
            auto tokenLocation = currentLocation;
            if(peek == eof)
            {
                return Token(std::move(tokenLocation), Token::Type::EndOfFile, "");
            }
            if(isIdentifierStart(peek))
            {
                std::string value;
                while(isIdentifierContinue(peek))
                {
                    value += static_cast<char>(get());
                }
                Token::Type type = Token::Type::Identifier;
                if(value == "EOF")
                {
                    type = Token::Type::EOFKeyword;
                }
                else if(value == "typedef")
                {
                    type = Token::Type::TypedefKeyword;
                }
                else if(value == "code")
                {
                    type = Token::Type::CodeKeyword;
                }
                return Token(std::move(tokenLocation), type, std::move(value));
            }
            switch(peek)
            {
            case '\"':
            {
                get();
                std::string value;
                while(peek != eof && peek != '\"' && peek != '\r' && peek != '\n')
                {
                    if(peek == '\\')
                    {
                        value += static_cast<char>(get());
                        if(peek == eof || peek == '\r' || peek == '\n')
                        {
                            errorHandler(
                                ErrorLevel::FatalError, tokenLocation, "missing closing \"");
                        }
                        else
                        {
                            value += static_cast<char>(get());
                        }
                    }
                    else
                    {
                        value += static_cast<char>(get());
                    }
                }
                if(peek != '\"')
                {
                    errorHandler(ErrorLevel::FatalError, tokenLocation, "missing closing \"");
                }
                else
                {
                    get();
                }
                return Token(std::move(tokenLocation), Token::Type::String, std::move(value));
            }
            case '[':
            {
                get();
                tokenLocation = currentLocation;
                std::string value;
                while(peek != eof && peek != ']' && peek != '\r' && peek != '\n')
                {
                    if(peek == '\\')
                    {
                        value += static_cast<char>(get());
                        if(peek == eof || peek == '\r' || peek == '\n')
                        {
                            errorHandler(
                                ErrorLevel::FatalError, tokenLocation, "missing closing ]");
                        }
                        else
                        {
                            value += static_cast<char>(get());
                        }
                    }
                    else
                    {
                        value += static_cast<char>(get());
                    }
                }
                if(peek != ']')
                {
                    errorHandler(ErrorLevel::FatalError, tokenLocation, "missing closing ]");
                }
                else
                {
                    get();
                }
                return Token(
                    std::move(tokenLocation), Token::Type::CharacterClass, std::move(value));
            }
            case '{':
            {
                enum class IncludeState
                {
                    StartOfLine,
                    GotPound,
                    GotInclude,
                    Other,
                };
                auto includeState = IncludeState::StartOfLine;
                get();
                tokenLocation = currentLocation;
                std::string value;
                std::size_t nestLevel = 1;
                std::vector<ast::ExpressionCodeSnippet::Substitution> substitutions;
                while(peek != eof)
                {
                    while(peek == ' ' || peek == '\t')
                    {
                        value += static_cast<char>(get());
                    }
                    if(peek == '#' && includeState == IncludeState::StartOfLine)
                    {
                        value += static_cast<char>(get());
                        includeState = IncludeState::GotPound;
                        continue;
                    }
                    if(peek == 'i' && includeState == IncludeState::GotPound)
                    {
                        includeState = IncludeState::Other;
                        value += static_cast<char>(get());
                        if(peek != 'n')
                            continue;
                        value += static_cast<char>(get());
                        if(peek != 'c')
                            continue;
                        value += static_cast<char>(get());
                        if(peek != 'l')
                            continue;
                        value += static_cast<char>(get());
                        if(peek != 'u')
                            continue;
                        value += static_cast<char>(get());
                        if(peek != 'd')
                            continue;
                        value += static_cast<char>(get());
                        if(peek != 'e')
                            continue;
                        value += static_cast<char>(get());
                        includeState = IncludeState::GotInclude;
                        continue;
                    }
                    if((peek == '<' || peek == '\"') && includeState == IncludeState::GotInclude)
                    {
                        includeState = IncludeState::Other;
                        auto closingCharacter = peek == '<' ? '>' : peek;
                        value += static_cast<char>(get());
                        while(peek != closingCharacter && peek != eof && peek != '\r'
                              && peek != '\n')
                        {
                            value += static_cast<char>(get());
                        }
                        if(peek != closingCharacter)
                        {
                            errorHandler(ErrorLevel::FatalError,
                                         tokenLocation,
                                         std::string("#include missing closing ")
                                             + static_cast<char>(closingCharacter));
                            return Token();
                        }
                        value += static_cast<char>(get());
                    }
                    if(peek == 'R')
                    {
                        includeState = IncludeState::Other;
                        value += static_cast<char>(get());
                        if(peek != '\"')
                            continue;
                        value += static_cast<char>(get());
                        std::string seperator;
                        while(peek != '(' && peek != eof && peek != '\"' && peek != ')'
                              && peek != ' '
                              && peek != '\t'
                              && peek != '\r'
                              && peek != '\n'
                              && peek != '\\')
                        {
                            seperator += static_cast<char>(get());
                        }
                        value += seperator;
                        if(peek != '(')
                        {
                            errorHandler(ErrorLevel::FatalError,
                                         tokenLocation,
                                         "C++11 raw string literal missing opening (");
                            return Token();
                        }
                        value += static_cast<char>(get());
                        seperator = ")" + std::move(seperator) + "\"";
                        while(value.compare(
                                  value.size() - seperator.size(), seperator.size(), seperator)
                              != 0)
                        {
                            if(peek == eof)
                            {
                                errorHandler(
                                    ErrorLevel::FatalError,
                                    tokenLocation,
                                    "C++11 raw string literal missing closing " + seperator);
                                return Token();
                            }
                            if(peek == '\r')
                            {
                                get();
                                if(peek == '\n')
                                    get();
                                value += '\n';
                            }
                            else
                            {
                                value += static_cast<char>(get());
                            }
                        }
                        continue;
                    }
                    if(peek == '\'' || peek == '\"')
                    {
                        includeState = IncludeState::Other;
                        auto seperatorCharacter = peek;
                        value += static_cast<char>(get());
                        while(peek != seperatorCharacter && peek != eof && peek != '\r'
                              && peek != '\n')
                        {
                            if(peek == '\\')
                            {
                                value += static_cast<char>(get());
                                if(peek == eof)
                                    break;
                                value += static_cast<char>(get());
                            }
                            else
                            {
                                value += static_cast<char>(get());
                            }
                        }
                        if(peek != seperatorCharacter)
                        {
                            errorHandler(ErrorLevel::FatalError,
                                         tokenLocation,
                                         std::string("string literal missing closing ")
                                             + static_cast<char>(seperatorCharacter));
                            return Token();
                        }
                        value += static_cast<char>(get());
                        continue;
                    }
                    if(peek == '{')
                    {
                        includeState = IncludeState::Other;
                        nestLevel++;
                        value += static_cast<char>(get());
                        continue;
                    }
                    if(peek == '}')
                    {
                        includeState = IncludeState::Other;
                        if(--nestLevel == 0)
                            break;
                        value += static_cast<char>(get());
                        continue;
                    }
                    if(peek == '$')
                    {
                        includeState = IncludeState::Other;
                        get();
                        if(peek == '$')
                        {
                            substitutions.emplace_back(
                                ast::ExpressionCodeSnippet::Substitution::Kind::ReturnValue,
                                value.size());
                            get();
                        }
                        else
                        {
                            errorHandler(ErrorLevel::Warning,
                                         tokenLocation,
                                         "unrecognized code substitution");
                            value += '$';
                        }
                        continue;
                    }
                    if(peek == '/')
                    {
                        value += static_cast<char>(get());
                        if(peek == '/')
                        {
                            while(peek != eof && peek != '\r' && peek != '\n')
                            {
                                value += static_cast<char>(get());
                            }
                            continue;
                        }
                        if(peek == '*')
                        {
                            value += static_cast<char>(get());
                            while(peek != eof)
                            {
                                if(peek == '*')
                                {
                                    while(peek == '*')
                                    {
                                        value += static_cast<char>(get());
                                    }
                                    if(peek == '/')
                                    {
                                        break;
                                    }
                                    continue;
                                }
                                if(peek == '\r')
                                {
                                    get();
                                    if(peek == '\n')
                                        get();
                                    value += '\n';
                                }
                                else
                                {
                                    value += static_cast<char>(get());
                                }
                            }
                            if(peek != '/')
                            {
                                errorHandler(ErrorLevel::FatalError,
                                             tokenLocation,
                                             "comment missing closing */");
                                return Token();
                            }
                            value += static_cast<char>(get());
                            continue;
                        }
                        includeState = IncludeState::Other;
                    }
                    if(peek == '\r')
                    {
                        includeState = IncludeState::StartOfLine;
                        get();
                        if(peek == '\n')
                            get();
                        value += '\n';
                    }
                    else if(peek == '\n')
                    {
                        includeState = IncludeState::StartOfLine;
                        value += static_cast<char>(get());
                    }
                    else
                    {
                        includeState = IncludeState::Other;
                        value += static_cast<char>(get());
                    }
                }
                if(peek != '}')
                {
                    errorHandler(ErrorLevel::FatalError, tokenLocation, "missing closing }");
                }
                else
                {
                    get();
                }
                return Token(std::move(tokenLocation),
                             Token::Type::CodeSnippet,
                             std::move(value),
                             std::move(substitutions));
            }
            case ';':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::Semicolon, "");
            }
            case ':':
            {
                get();
                if(peek == ':')
                {
                    get();
                    return Token(std::move(tokenLocation), Token::Type::ColonColon, "");
                }
                return Token(std::move(tokenLocation), Token::Type::Colon, "");
            }
            case '?':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::QMark, "");
            }
            case '+':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::Plus, "");
            }
            case '!':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::EMark, "");
            }
            case '*':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::Star, "");
            }
            case '/':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::FSlash, "");
            }
            case '=':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::Equal, "");
            }
            case '(':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::LParen, "");
            }
            case ')':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::RParen, "");
            }
            case '&':
            {
                get();
                return Token(std::move(tokenLocation), Token::Type::Amp, "");
            }
            default:
                errorHandler(ErrorLevel::FatalError, tokenLocation, "invalid character");
                return Token(std::move(tokenLocation), Token::Type::EndOfFile, "");
            }
        }
    };
    std::unordered_map<std::string, ast::Nonterminal *> nonterminalTable;
    std::unordered_map<std::string, ast::Type *> typeTable;
    std::unordered_set<std::string> variableNames;
    Tokenizer tokenizer;
    Token token;
    Arena &arena;
    ErrorHandler &errorHandler;
    ast::Type *voidType;
    ast::Type *charType;
    std::vector<ast::NonterminalExpression *> nonterminalReferences;
    Parser(Arena &arena, ErrorHandler &errorHandler, const Source *source)
        : tokenizer(source),
          token(tokenizer.parseToken(errorHandler)),
          arena(arena),
          errorHandler(errorHandler),
          voidType(),
          charType()
    {
        voidType = createBuiltinType("void", "void", true);
        charType = createBuiltinType("char", "char32_t");
    }
    ast::Nonterminal *getNonterminal()
    {
        assert(token.type == Token::Type::Identifier);
        ast::Nonterminal *&retval = nonterminalTable[token.value];
        if(!retval)
            retval = arena.make<ast::Nonterminal>(
                token.location, token.value, nullptr, nullptr, ast::Nonterminal::Settings());
        return retval;
    }
    ast::Type *getType()
    {
        assert(token.type == Token::Type::Identifier);
        auto iter = typeTable.find(token.value);
        if(iter == typeTable.end())
        {
            errorHandler(ErrorLevel::Error, token.location, "undefined type");
            return nullptr;
        }
        return std::get<1>(*iter);
    }
    ast::Type *makeType(std::string code)
    {
        assert(token.type == Token::Type::Identifier);
        ast::Type *&retval = typeTable[token.value];
        if(retval)
        {
            errorHandler(ErrorLevel::Error, token.location, "already defined type");
        }
        retval = arena.make<ast::Type>(token.location, std::move(code), token.value);
        return retval;
    }
    ast::Type *createBuiltinType(std::string name, std::string code, bool isVoid = false)
    {
        auto *&type = typeTable[name];
        assert(!type);
        type = arena.make<ast::Type>(Location(tokenizer.currentLocation.source, 0),
                                     std::move(code),
                                     std::move(name),
                                     isVoid);
        return type;
    }
    void next()
    {
        token = tokenizer.parseToken(errorHandler);
    }
    enum class CharacterLocation
    {
        String,
        CharacterClass
    };
    char32_t parseCharacterValue(std::size_t &position, CharacterLocation characterLocation)
    {
        int peek;
        if(position >= token.value.size())
            peek = eof;
        else
            peek = token.value[position];
        auto get = [&]() -> int
        {
            int retval = peek;
            position++;
            if(position >= token.value.size())
                peek = eof;
            else
                peek = token.value[position];
            return retval;
        };
        if(peek == '\\')
        {
            get();
            switch(peek)
            {
            case 'f':
                get();
                return '\f';
            case 'n':
                get();
                return '\n';
            case 'r':
                get();
                return '\r';
            case 't':
                get();
                return '\t';
            case ']':
            case '-':
                if(characterLocation != CharacterLocation::CharacterClass)
                {
                    errorHandler(
                        ErrorLevel::FatalError,
                        Location(token.location.source, token.location.position + position),
                        "invalid escape sequence");
                }
                return get();
            case '\\':
            case '\'':
            case '\"':
                return get();
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                char32_t retval = 0;
                for(int i = 0; i < 3; i++)
                {
                    int digit = getDigitValue(peek, 0x10);
                    if(digit < 0)
                    {
                        break;
                    }
                    get();
                    retval = retval * 0x10 + digit;
                }
                return retval;
            }
            case 'x':
            {
                get();
                if(getDigitValue(peek, 0x10) < 0)
                {
                    errorHandler(
                        ErrorLevel::FatalError,
                        Location(token.location.source, token.location.position + position),
                        "invalid escape sequence");
                }
                char32_t retval = 0;
                while(true)
                {
                    int digit = getDigitValue(peek, 0x10);
                    if(digit < 0)
                    {
                        break;
                    }
                    get();
                    retval = retval * 0x10 + digit;
                    if(retval >= 0x10FFFFUL)
                    {
                        errorHandler(
                            ErrorLevel::FatalError,
                            Location(token.location.source, token.location.position + position),
                            "invalid escape sequence");
                    }
                }
                return retval;
            }
            case 'u':
            {
                get();
                char32_t retval = 0;
                for(int i = 0; i < 4; i++)
                {
                    int digit = getDigitValue(peek, 0x10);
                    if(digit < 0)
                    {
                        errorHandler(
                            ErrorLevel::FatalError,
                            Location(token.location.source, token.location.position + position),
                            "invalid escape sequence");
                    }
                    get();
                    retval = retval * 0x10 + digit;
                }
                return retval;
            }
            case 'U':
            {
                get();
                char32_t retval = 0;
                for(int i = 0; i < 8; i++)
                {
                    int digit = getDigitValue(peek, 0x10);
                    if(digit < 0)
                    {
                        errorHandler(
                            ErrorLevel::FatalError,
                            Location(token.location.source, token.location.position + position),
                            "invalid escape sequence");
                    }
                    get();
                    retval = retval * 0x10 + digit;
                }
                if(retval >= 0x10FFFFUL)
                {
                    errorHandler(
                        ErrorLevel::FatalError,
                        Location(token.location.source, token.location.position + position),
                        "invalid escape sequence");
                }
                return retval;
            }
            default:
                errorHandler(ErrorLevel::FatalError,
                             Location(token.location.source, token.location.position + position),
                             "invalid escape sequence");
                return get();
            }
        }
        if((peek >= 0x7F || peek < 0x20) && peek != '\t')
        {
            errorHandler(ErrorLevel::FatalError,
                         Location(token.location.source, token.location.position + position),
                         "invalid character");
        }
        return get();
    }
    void parseCharacterClass(ast::CharacterClass::CharacterRanges &characterRanges, bool &inverted)
    {
        std::size_t position = 0;
        int peek = token.value.empty() ? eof : token.value[0];
        auto get = [&]() -> int
        {
            int retval = peek;
            position++;
            if(position >= token.value.size())
                peek = eof;
            else
                peek = token.value[position];
            return retval;
        };
        auto getCharacterValue = [&]() -> char32_t
        {
            auto retval = parseCharacterValue(position, CharacterLocation::CharacterClass);
            if(position >= token.value.size())
                peek = eof;
            else
                peek = token.value[position];
            return retval;
        };
        if(peek == '^')
        {
            get();
            inverted = true;
        }
        while(peek != eof)
        {
            auto rangeLocation =
                Location(token.location.source, token.location.position + position);
            ast::CharacterClass::CharacterRange range(getCharacterValue());
            if(peek == '-')
            {
                rangeLocation = Location(token.location.source, token.location.position + position);
                get();
                if(peek == eof)
                {
                    if(!characterRanges.insert(range))
                    {
                        errorHandler(ErrorLevel::FatalError,
                                     rangeLocation,
                                     "invalid character: overlaps other entries");
                    }
                    if(!characterRanges.insert(ast::CharacterClass::CharacterRange(U'-')))
                    {
                        errorHandler(ErrorLevel::FatalError,
                                     rangeLocation,
                                     "invalid character: overlaps other entries");
                    }
                    break;
                }
                range.max = getCharacterValue();
                if(range.empty())
                {
                    errorHandler(ErrorLevel::FatalError,
                                 rangeLocation,
                                 "invalid character range: start character has a larger value than "
                                 "end character");
                    continue;
                }
                if(!characterRanges.insert(range))
                {
                    errorHandler(ErrorLevel::FatalError,
                                 rangeLocation,
                                 "invalid character range: overlaps other entries");
                }
            }
            else if(!characterRanges.insert(range))
            {
                errorHandler(ErrorLevel::FatalError,
                             rangeLocation,
                             "invalid character: overlaps other entries");
            }
        }
    }
    template <bool codeAllowed>
    ast::Expression *parsePrimaryExpression()
    {
        switch(token.type)
        {
        case Token::Type::LParen:
        {
            next();
            if(token.type == Token::Type::RParen)
            {
                auto retval = arena.make<ast::Empty>(token.location);
                next();
                return retval;
            }
            auto retval = parseExpression<codeAllowed>();
            if(token.type != Token::Type::RParen)
            {
                errorHandler(ErrorLevel::FatalError, token.location, "missing )");
                return nullptr;
            }
            next();
            return retval;
        }
        case Token::Type::Identifier:
        {
            auto nonterminal = getNonterminal();
            auto retval = arena.make<ast::NonterminalExpression>(token.location, nonterminal, "");
            nonterminalReferences.push_back(retval);
            next();
            if(token.type == Token::Type::Colon)
            {
                next();
                if(token.type != Token::Type::Identifier)
                {
                    errorHandler(ErrorLevel::FatalError, token.location, "missing variable name");
                }
                else
                {
                    if(!codeAllowed)
                    {
                        errorHandler(
                            ErrorLevel::Error, token.location, "variable not allowed inside !");
                    }
                    if(!std::get<1>(variableNames.insert(token.value)))
                    {
                        errorHandler(ErrorLevel::Error, token.location, "duplicate variable name");
                    }
                    retval->variableName = token.value;
                    next();
                }
            }
            return retval;
        }
        case Token::Type::EOFKeyword:
        {
            auto retval = arena.make<ast::EOFTerminal>(token.location);
            next();
            return retval;
        }
        case Token::Type::String:
        {
            ast::Expression *retval = nullptr;
            std::size_t position = 0;
            while(position < token.value.size())
            {
                auto ch = parseCharacterValue(position, CharacterLocation::String);
                auto terminal = arena.make<ast::Terminal>(token.location, ch);
                if(retval)
                {
                    retval = arena.make<ast::Sequence>(token.location, retval, terminal);
                }
                else
                {
                    retval = terminal;
                }
            }
            if(retval == nullptr)
            {
                retval = arena.make<ast::Empty>(token.location);
            }
            next();
            return retval;
        }
        case Token::Type::CharacterClass:
        {
            ast::CharacterClass::CharacterRanges characterRanges;
            bool inverted = false;
            parseCharacterClass(characterRanges, inverted);
            auto retval = arena.make<ast::CharacterClass>(
                token.location, std::move(characterRanges), inverted, "");
            next();
            if(token.type == Token::Type::Colon)
            {
                next();
                if(token.type != Token::Type::Identifier)
                {
                    errorHandler(ErrorLevel::FatalError, token.location, "missing variable name");
                }
                else
                {
                    if(!codeAllowed)
                    {
                        errorHandler(
                            ErrorLevel::Error, token.location, "variable not allowed inside !");
                    }
                    if(!std::get<1>(variableNames.insert(token.value)))
                    {
                        errorHandler(ErrorLevel::Error, token.location, "duplicate variable name");
                    }
                    retval->variableName = token.value;
                    next();
                }
            }
            return retval;
        }
        case Token::Type::Amp:
        {
            auto ampLocation = token.location;
            next();
            auto expression = parsePrimaryExpression<codeAllowed>();
            return arena.make<ast::FollowedByPredicate>(ampLocation, expression);
        }
        case Token::Type::EMark:
        {
            auto emarkLocation = token.location;
            next();
            auto expression = parsePrimaryExpression<false>();
            return arena.make<ast::NotFollowedByPredicate>(emarkLocation, expression);
        }
        case Token::Type::CodeSnippet:
        {
            auto retval = arena.make<ast::ExpressionCodeSnippet>(
                token.location, token.value, token.substitutions);
            if(!codeAllowed)
            {
                errorHandler(ErrorLevel::Error, token.location, "code not allowed inside !");
            }
            next();
            return retval;
        }
        default:
            errorHandler(ErrorLevel::FatalError, token.location, "missing expression");
            return nullptr;
        }
    }
    template <bool codeAllowed>
    ast::Expression *parseRepeatOptionalExpression()
    {
        ast::Expression *retval = parsePrimaryExpression<codeAllowed>();
        while(true)
        {
            if(token.type == Token::Type::QMark)
            {
                retval = arena.make<ast::OptionalExpression>(token.location, retval);
                next();
            }
            else if(token.type == Token::Type::Star)
            {
                retval = arena.make<ast::GreedyRepetition>(token.location, retval);
                next();
            }
            else if(token.type == Token::Type::Plus)
            {
                retval = arena.make<ast::GreedyPositiveRepetition>(token.location, retval);
                next();
            }
            else
            {
                break;
            }
        }
        return retval;
    }
    template <bool codeAllowed>
    ast::Expression *parseSequenceExpression()
    {
        ast::Expression *retval = parseRepeatOptionalExpression<codeAllowed>();
        while(true)
        {
            bool done = false;
            switch(token.type)
            {
            case Token::Type::EndOfFile:
            case Token::Type::Semicolon:
            case Token::Type::Colon:
            case Token::Type::ColonColon:
            case Token::Type::FSlash:
            case Token::Type::Equal:
            case Token::Type::RParen:
            case Token::Type::TypedefKeyword:
            case Token::Type::CodeKeyword:
                done = true;
                break;
            case Token::Type::QMark:
            case Token::Type::Plus:
            case Token::Type::EMark:
            case Token::Type::Star:
            case Token::Type::LParen:
            case Token::Type::Amp:
            case Token::Type::String:
            case Token::Type::Identifier:
            case Token::Type::EOFKeyword:
            case Token::Type::CharacterClass:
            case Token::Type::CodeSnippet:
                break;
            }
            if(done)
                break;
            auto sequenceLocation = token.location;
            ast::Expression *rhs = parseRepeatOptionalExpression<codeAllowed>();
            retval = arena.make<ast::Sequence>(std::move(sequenceLocation), retval, rhs);
        }
        return retval;
    }
    template <bool codeAllowed>
    ast::Expression *parseExpression()
    {
        ast::Expression *retval = parseSequenceExpression<codeAllowed>();
        while(token.type == Token::Type::FSlash)
        {
            auto slashLocation = token.location;
            next();
            ast::Expression *rhs = parseSequenceExpression<codeAllowed>();
            retval = arena.make<ast::OrderedChoice>(std::move(slashLocation), retval, rhs);
        }
        return retval;
    }
    ast::Nonterminal *parseRule()
    {
        variableNames.clear();
        variableNames.insert("$$");
        if(token.type != Token::Type::Identifier)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing rule name");
            return nullptr;
        }
        auto retval = getNonterminal();
        if(retval->expression)
        {
            errorHandler(ErrorLevel::Error, token.location, "rule already defined");
            errorHandler(ErrorLevel::Info, retval->location, "previous rule definition");
            retval->expression = nullptr;
        }
        next();
        if(token.type == Token::Type::Colon)
        {
            next();
            if(token.type != Token::Type::Identifier)
            {
                errorHandler(ErrorLevel::FatalError, token.location, "missing type name");
            }
            else
            {
                retval->type = getType();
                next();
            }
        }
        if(token.type != Token::Type::Equal)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing =");
            return nullptr;
        }
        next();
        retval->expression = parseExpression<true>();
        if(token.type != Token::Type::Semicolon)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing ;");
            return nullptr;
        }
        next();
        if(retval->type == nullptr)
        {
            if(auto characterClass = dynamic_cast<ast::CharacterClass *>(retval->expression))
            {
                if(characterClass->variableName.empty())
                {
                    retval->type = charType;
                }
            }
        }
        if(retval->type == nullptr)
        {
            retval->type = voidType;
        }
        return retval;
    }
    void parseType()
    {
        assert(token.type == Token::Type::TypedefKeyword);
        next();
        std::string code;
        if(token.type == Token::Type::ColonColon)
        {
            code = "::";
            next();
        }
        if(token.type != Token::Type::Identifier)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing identifier");
            return;
        }
        code += token.value;
        next();
        while(token.type == Token::Type::ColonColon)
        {
            code += "::";
            next();
            if(token.type != Token::Type::Identifier)
            {
                errorHandler(ErrorLevel::FatalError, token.location, "missing identifier");
                return;
            }
            code += token.value;
            next();
        }
        if(token.type != Token::Type::Identifier)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing type name");
            return;
        }
        makeType(std::move(code));
        next();
        if(token.type != Token::Type::Semicolon)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing ;");
            return;
        }
        next();
    }
    ast::TopLevelCodeSnippet *parseTopLevelCodeSnippet()
    {
        assert(token.type == Token::Type::CodeKeyword);
        next();
        ast::TopLevelCodeSnippet::Kind kind = ast::TopLevelCodeSnippet::Kind::Header;
        if(token.type != Token::Type::Identifier)
        {
            errorHandler(ErrorLevel::Error, token.location, "missing code kind");
        }
        else
        {
            if(token.value == "license")
            {
                kind = ast::TopLevelCodeSnippet::Kind::License;
            }
            else if(token.value == "header")
            {
                kind = ast::TopLevelCodeSnippet::Kind::Header;
            }
            else if(token.value == "source")
            {
                kind = ast::TopLevelCodeSnippet::Kind::Source;
            }
            else
            {
                errorHandler(ErrorLevel::Error,
                             token.location,
                             "invalid code kind: expected license, header, or source");
            }
            next();
        }
        if(token.type != Token::Type::CodeSnippet)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing code snippet");
            return nullptr;
        }
        if(!token.substitutions.empty())
        {
            errorHandler(ErrorLevel::Error,
                         token.location,
                         "code substitutions not allowed in top-level code");
        }
        auto retval = arena.make<ast::TopLevelCodeSnippet>(token.location, kind, token.value);
        next();
        return retval;
    }
    ast::Grammar *parseGrammar()
    {
        auto grammarLocation = token.location;
        std::vector<ast::Nonterminal *> nonterminals;
        std::vector<ast::TopLevelCodeSnippet *> topLevelCodeSnippets;
        while(token.type != Token::Type::EndOfFile)
        {
            if(token.type == Token::Type::TypedefKeyword)
            {
                parseType();
            }
            else if(token.type == Token::Type::CodeKeyword)
            {
                topLevelCodeSnippets.push_back(parseTopLevelCodeSnippet());
            }
            else
            {
                nonterminals.push_back(parseRule());
            }
        }
        for(const auto &nameAndNonterminal : nonterminalTable)
        {
            if(!std::get<1>(nameAndNonterminal)->expression)
            {
                errorHandler(ErrorLevel::Error,
                             std::get<1>(nameAndNonterminal)->location,
                             "rule not defined");
            }
        }
        for(auto nonterminal : nonterminals)
        {
            if(nonterminal->settings.caching)
            {
                nonterminal->settings.caching = nonterminal->expression->defaultNeedsCaching();
            }
        }
        for(auto nonterminalReference : nonterminalReferences)
        {
            if(!nonterminalReference->variableName.empty()
               && nonterminalReference->value->type->isVoid)
            {
                errorHandler(ErrorLevel::Error,
                             nonterminalReference->location,
                             "can't create a void variable");
            }
        }
        for(auto nonterminal : nonterminals)
        {
            nonterminal->settings.canAcceptEmptyString = true;
            nonterminal->settings.hasLeftRecursion = true;
        }
        for(bool done = false; !done;)
        {
            done = true;
            for(auto nonterminal : nonterminals)
            {
                if(nonterminal->settings.canAcceptEmptyString)
                {
                    nonterminal->settings.canAcceptEmptyString =
                        nonterminal->expression->canAcceptEmptyString();
                    if(!nonterminal->settings.canAcceptEmptyString)
                        done = false;
                }
            }
        }
        for(bool done = false; !done;)
        {
            done = true;
            for(auto nonterminal : nonterminals)
            {
                if(nonterminal->settings.hasLeftRecursion)
                {
                    nonterminal->settings.hasLeftRecursion =
                        nonterminal->expression->hasLeftRecursion();
                    if(!nonterminal->settings.hasLeftRecursion)
                        done = false;
                }
            }
        }
        for(auto nonterminal : nonterminals)
        {
            if(nonterminal->settings.hasLeftRecursion)
            {
                errorHandler(ErrorLevel::Error, nonterminal->location, "left-recursive rule");
            }
        }
        return arena.make<ast::Grammar>(
            std::move(grammarLocation), std::move(topLevelCodeSnippets), std::move(nonterminals));
    }
};

ast::Grammar *parseGrammar(Arena &arena, ErrorHandler &errorHandler, const Source *source)
{
    assert(source);
    return Parser(arena, errorHandler, source).parseGrammar();
}
