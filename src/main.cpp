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
#include <string>

std::string removeExtension(std::string fileName)
{
    std::size_t lastSlashPosition = fileName.find_last_of("/\\");
    std::size_t lastDotPosition = fileName.find_last_of('.');
    if(lastDotPosition == std::string::npos)
        return fileName;
    if(lastSlashPosition != std::string::npos && lastSlashPosition + 1 >= lastDotPosition)
    {
        return fileName;
    }
    fileName.resize(lastDotPosition);
    return fileName;
}

std::string removePath(std::string fileName)
{
    std::size_t lastSlashPosition = fileName.find_last_of("/\\");
    if(lastSlashPosition == std::string::npos)
        return fileName;
    fileName.erase(0, lastSlashPosition + 1);
    return fileName;
}

int main(int argc, char **argv)
{
    std::string inputFile = "";
    std::string outputSourceFile = "";
    std::string outputHeaderFile = "";
    bool canParseOptions = true;
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if(canParseOptions)
        {
            if(arg == "--")
            {
                canParseOptions = false;
                continue;
            }
            if(arg == "-h" || arg == "--help")
            {
                std::cout << R"(usage: peg_parser_generator [<options>] <input-file>
Options:
-h
--help             Show this help.
-o<output>         Set the output file name.
)";
                return 0;
            }
            if(arg.compare(0, 2, "-o") == 0)
            {
                if(arg.size() > 2)
                    arg.erase(0, 2);
                else
                {
                    i++;
                    if(i >= argc)
                    {
                        std::cerr << "-o option is missing argument" << std::endl;
                        return 1;
                    }
                    arg = argv[i];
                }
                if(arg.empty())
                {
                    std::cerr << "-o option has empty argument" << std::endl;
                    return 1;
                }
                if(!outputSourceFile.empty())
                {
                    std::cerr << "-o option specified multiple times" << std::endl;
                    return 1;
                }
                if(arg == "-")
                {
                    std::cerr << "invalid output file name" << std::endl;
                    return 1;
                }
                outputSourceFile = std::move(arg);
                continue;
            }
            if(arg.size() > 1 && arg.compare(0, 1, "-") == 0)
            {
                std::cerr << "invalid option" << std::endl;
                return 1;
            }
        }
        if(!inputFile.empty())
        {
            std::cerr << "too many input files" << std::endl;
            return 1;
        }
        if(arg.empty())
        {
            std::cerr << "empty input file name" << std::endl;
            return 1;
        }
        inputFile = std::move(arg);
    }
    Arena arena;
    DefaultErrorHandler errorHandler;
    try
    {
        if(inputFile.empty())
        {
            errorHandler(ErrorLevel::FatalError, Location(), "no input files");
            return 1;
        }
        if(outputSourceFile.empty())
        {
            if(inputFile == "-")
            {
                errorHandler(ErrorLevel::FatalError,
                             Location(),
                             "missing output file name when input file is stdin");
                return 1;
            }
            outputSourceFile = removeExtension(inputFile) + ".cpp";
        }
        else if(outputSourceFile == removeExtension(outputSourceFile))
        {
            outputSourceFile += ".cpp";
        }
        if(outputHeaderFile.empty())
        {
            outputHeaderFile = removeExtension(outputSourceFile) + ".h";
        }
        const Source *source = Source::load(arena, errorHandler, inputFile);
        ast::Grammar *grammar = parseGrammar(arena, errorHandler, source);
        if(!errorHandler.anyErrors)
        {
            std::ostringstream headerStream, sourceStream;
            CodeGenerator::makeCPlusPlus11(sourceStream,
                                           headerStream,
                                           outputHeaderFile,
                                           removePath(outputHeaderFile),
                                           outputSourceFile)->generateCode(grammar);
            std::ofstream os;
            os.open(outputHeaderFile);
            if(!os)
            {
                errorHandler(ErrorLevel::FatalError,
                             Location(),
                             "can't open output file: '",
                             outputHeaderFile,
                             "'");
                return 1;
            }
            os << headerStream.str();
            if(!os)
            {
                errorHandler(ErrorLevel::FatalError,
                             Location(),
                             "io error");
                return 1;
            }
            os.close();
            os.open(outputSourceFile);
            if(!os)
            {
                errorHandler(ErrorLevel::FatalError,
                             Location(),
                             "can't open output file: '",
                             outputSourceFile,
                             "'");
                return 1;
            }
            os << sourceStream.str();
            if(!os)
            {
                errorHandler(ErrorLevel::FatalError,
                             Location(),
                             "io error");
                return 1;
            }
            os.close();
        }
    }
    catch(FatalError &)
    {
    }
    if(errorHandler.anyErrors)
        return 1;
    return 0;
}
