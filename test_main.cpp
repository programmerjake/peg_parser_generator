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
#include "test.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

using namespace parser;

namespace
{
void test(std::string input)
{
    try
    {
        auto output = Parser(input).parseGoal();
        if(!output.empty())
            std::cout << input << "\n" << output << std::endl;
    }
    catch(Parser::ParseError &e)
    {
    }
}

template <typename RE>
std::string makeInput(RE &re)
{
    const char chars[] = "enw+-*/%012.() ";
    std::size_t inputSize = 20;
    std::string retval;
    retval.reserve(inputSize + 1);
    for(std::size_t i = 0; i < inputSize; i++)
    {
        retval += chars[std::uniform_int_distribution<std::size_t>(
            0, sizeof(chars) / sizeof(chars[0]) - 1 - 1)(re)];
    }
    retval += ';';
    return retval;
}
}

#if 1
int main()
{
    std::mt19937_64 re;
    for(std::size_t i = 0; i < 1000000; i++)
    {
        if(i % 10000 == 0)
            std::cout << "                  " << i << std::endl;
        test(makeInput(re));
    }
    return 0;
}
#else
int main(int argc, char **argv)
{
    std::vector<std::size_t> lineStartingPositions;
    try
    {
        std::string input;
        std::cin.exceptions(std::ios::badbit);
        while(true)
        {
            int ch = std::cin.get();
            if(ch == std::char_traits<char>::eof())
                break;
            if(ch == '\r')
            {
                input += static_cast<char>(ch);
                if(std::cin.peek() == '\n')
                    input += static_cast<char>(std::cin.get());
                lineStartingPositions.push_back(input.size());
            }
            else if(ch == '\n')
            {
                input += static_cast<char>(ch);
                lineStartingPositions.push_back(input.size());
            }
            else
            {
                input += static_cast<char>(ch);
            }
        }
        std::cout << Parser(input).parseGoal() << std::endl;
    }
    catch(Parser::ParseError &e)
    {
        std::size_t line =
            1 + lineStartingPositions.size()
            + (lineStartingPositions.rbegin() - std::lower_bound(lineStartingPositions.rbegin(),
                                                                 lineStartingPositions.rend(),
                                                                 e.location,
                                                                 std::greater<std::size_t>()));
        std::size_t column =
            line <= 1 ? e.location + 1 : e.location - lineStartingPositions[line - 2] + 1;
        std::cerr << "stdin:" << line << ":" << column << ": error: " << e.message << std::endl;
        return 1;
    }
    catch(std::ios::failure &e)
    {
        std::cerr << "io error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
#endif
