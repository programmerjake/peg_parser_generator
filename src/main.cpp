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
#include "arena.h"
#include "parser.h"
#include "source.h"
#include "error.h"
#include "code_generator.h"
#include "ast/grammar.h"
#include "ast/dump_visitor.h"
#include <iostream>
#include <sstream>
#include <fstream>

int main()
{
    Arena arena;
    DefaultErrorHandler errorHandler;
    try
    {
        const Source *source = Source::load(arena, errorHandler, "test.grammar");
        ast::Grammar *grammar = parseGrammar(arena, errorHandler, source);
        if(!errorHandler.anyErrors)
        {
            std::ostringstream headerStream, sourceStream;
            CodeGenerator::makeCPlusPlus11(sourceStream, headerStream, "test.h")->generateCode(grammar);
            std::ofstream os;
            os.open("test.h");
            os << headerStream.str();
            os.close();
            os.open("test.cpp");
            os << sourceStream.str();
            os.close();
            ast::DumpVisitor visitor(std::cout);
            grammar->visit(visitor);
        }
    }
    catch(FatalError &)
    {
    }
    if(errorHandler.anyErrors)
        return 1;
    return 0;
}
