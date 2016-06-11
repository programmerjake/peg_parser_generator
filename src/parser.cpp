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
#include "arena.h"
#include "source.h"
#include <cassert>
#include <cctype>
#include <unordered_map>

struct Parser final
{
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
        };
        Location location;
        Type type;
        std::string value;
        Token() : location(), type(Type::EndOfFile), value()
        {
        }
        Token(Location location, Type type, std::string value)
            : location(std::move(location)), type(type), value(std::move(value))
        {
        }
    };
    struct Tokenizer final
    {
        static constexpr int eof = -1;
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
                    value += static_cast<char>(get());
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
    std::unordered_map<std::string, ast::Nonterminal *> symbolTable;
    Tokenizer tokenizer;
    Token token;
    Arena &arena;
    ErrorHandler &errorHandler;
    Parser(Arena &arena, ErrorHandler &errorHandler, const Source *source)
        : tokenizer(source),
          token(tokenizer.parseToken(errorHandler)),
          arena(arena),
          errorHandler(errorHandler)
    {
    }
    ast::Nonterminal *getSymbol()
    {
        assert(token.type == Token::Type::Identifier);
        ast::Nonterminal *&retval = symbolTable[token.value];
        if(!retval)
            retval = arena.make<ast::Nonterminal>(
                token.location, token.value, nullptr, ast::Nonterminal::Settings());
        return retval;
    }
    void next()
    {
        token = tokenizer.parseToken(errorHandler);
    }
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
            auto retval = parseExpression();
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
            auto nonterminal = getSymbol();
            auto retval = arena.make<ast::NonterminalExpression>(token.location, nonterminal);
            next();
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
            for(unsigned char ch : token.value)
            {
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
        case Token::Type::Amp:
        {
            auto ampLocation = token.location;
            next();
            auto expression = parsePrimaryExpression();
            return arena.make<ast::FollowedByPredicate>(ampLocation, expression);
        }
        case Token::Type::EMark:
        {
            auto emarkLocation = token.location;
            next();
            auto expression = parsePrimaryExpression();
            return arena.make<ast::NotFollowedByPredicate>(emarkLocation, expression);
        }
        default:
            errorHandler(ErrorLevel::FatalError, token.location, "missing expression");
            return nullptr;
        }
    }
    ast::Expression *parseRepeatOptionalExpression()
    {
        ast::Expression *retval = parsePrimaryExpression();
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
    ast::Expression *parseSequenceExpression()
    {
        ast::Expression *retval = parseRepeatOptionalExpression();
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
                break;
            }
            if(done)
                break;
            auto sequenceLocation = token.location;
            ast::Expression *rhs = parseRepeatOptionalExpression();
            retval = arena.make<ast::Sequence>(std::move(sequenceLocation), retval, rhs);
        }
        return retval;
    }
    ast::Expression *parseExpression()
    {
        ast::Expression *retval = parseSequenceExpression();
        while(token.type == Token::Type::FSlash)
        {
            auto slashLocation = token.location;
            next();
            ast::Expression *rhs = parseSequenceExpression();
            retval = arena.make<ast::OrderedChoice>(std::move(slashLocation), retval, rhs);
        }
        return retval;
    }
    ast::Nonterminal *parseRule()
    {
        if(token.type != Token::Type::Identifier)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing rule name");
            return nullptr;
        }
        auto retval = getSymbol();
        if(retval->expression)
        {
            errorHandler(ErrorLevel::Error, token.location, "rule already defined");
            errorHandler(ErrorLevel::Info, retval->location, "previous rule definition");
            retval->expression = nullptr;
        }
        next();
        if(token.type != Token::Type::Equal)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing =");
            return nullptr;
        }
        next();
        retval->expression = parseExpression();
        if(token.type != Token::Type::Semicolon)
        {
            errorHandler(ErrorLevel::FatalError, token.location, "missing ;");
            return nullptr;
        }
        next();
        return retval;
    }
    ast::Grammar *parseGrammar()
    {
        auto grammarLocation = token.location;
        std::vector<ast::Nonterminal *> nonterminals;
        while(token.type != Token::Type::EndOfFile)
        {
            nonterminals.push_back(parseRule());
        }
        for(const auto &symbolTableEntry : symbolTable)
        {
            if(!std::get<1>(symbolTableEntry)->expression)
            {
                errorHandler(
                    ErrorLevel::Error, std::get<1>(symbolTableEntry)->location, "rule not defined");
            }
        }
        return arena.make<ast::Grammar>(std::move(grammarLocation), std::move(nonterminals));
    }
};

ast::Grammar *parseGrammar(Arena &arena, ErrorHandler &errorHandler, const Source *source)
{
    assert(source);
    return Parser(arena, errorHandler, source).parseGrammar();
}
