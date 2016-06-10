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

#ifndef ERROR_H_
#define ERROR_H_

#include <exception>
#include <sstream>

enum class ErrorLevel
{
    Info,
    Warning,
    Error,
    FatalError,
};

struct FatalError final : public std::exception
{
    virtual const char *what() const noexcept
    {
        return "FatalError";
    }
};

struct Location;

struct ErrorHandler
{
    virtual ~ErrorHandler() = default;

protected:
    virtual void handleMessage(ErrorLevel errorLevel,
                               const Location &location,
                               const std::string &message) = 0;

private:
    static void writeToStreamHelper(std::ostream &os)
    {
    }
    template <typename T, typename... Args>
    static void writeToStreamHelper(std::ostream &os, T &&arg, Args &&... args)
    {
        writeToStreamHelper(os << std::forward<T>(arg), std::forward<Args>(args)...);
    }

public:
    template <typename... Args>
    void operator()(ErrorLevel errorLevel, const Location &location, Args &&... args)
    {
        std::ostringstream os;
        writeToStreamHelper(os, std::forward<Args>(args)...);
        handleMessage(errorLevel, location, os.str());
        if(errorLevel == ErrorLevel::FatalError)
            throw FatalError();
    }
};

struct DefaultErrorHandler final : public ErrorHandler
{
protected:
    virtual void handleMessage(ErrorLevel errorLevel,
                               const Location &location,
                               const std::string &message) override;
};

#endif /* ERROR_H_ */
