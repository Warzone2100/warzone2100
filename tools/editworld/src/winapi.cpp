/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

	$Revision$
	$Id$
	$HeadURL$
*/

#include <windows.h>
#include "winapi.hpp"
#include <boost/scoped_array.hpp>
#include <string>
#include <cassert>

using boost::scoped_array;

namespace Win
{
    Error::Error() :
        _errCode(-1),
        _isErr(false),
        _errMsg(NULL)
    {}

    Error::Error(const int error_code) :
        _errCode(error_code),
        _isErr(true),
        _errMsg(NULL)
    {}

    Error::operator bool() const
    {
        return _isErr;
    }

    Error::~Error() throw()
    {
        if (_errMsg)
        {
            // Free the chunk of memory FormatMessageA gave us
            LocalFree(_errMsg);
        }
    }

    const char* Error::what() const throw()
    {
        if (!_isErr)
            return "";

        if (!_errMsg)
        {
            // Retrieve a string describing the error number (uses LocalAlloc() to allocate memory for _errMsg)
            ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, _errCode, 0, reinterpret_cast<char*>(&_errMsg), 0, NULL);
        }

        return _errMsg;
    }

    Error Error::GetLast()
    {
        const DWORD err = ::GetLastError();

        // A rare situation, but if it happens that GetLastError() returns
        // zero it definitly indicates that the last function was successful.
        if (err == 0)
            return Error();

        return Error(err);
    }

    std::string GetCurrentDirectory()
    {
        // Determine the required buffer size to contain the current directory
        const DWORD curDirSize = GetCurrentDirectoryA(0, NULL);

        // GetCurrentDirectoryA returns zero only on an error, and this case is
        // particularily rare, so throw an exception.
        if (curDirSize == 0)
            throw Error::GetLast();

        // Create a buffer of the required size
        // @note: We're using a scoped_array here, instead of a variably sized
        //        array, because MSVC doesn't support variably sized arrays.
        scoped_array<char> curDir(new char[curDirSize]);

        // Retrieve the current directory
        ::GetCurrentDirectoryA(curDirSize, curDir.get());

        // Return the current directory as a STL string
        return std::string(curDir.get());
    }

    static Error ShellExecute(const HINSTANCE err)
    {
        /* According to MSDN the return value of ShellExecute "is cast as an
         * HINSTANCE for backward compatibility with 16-bit Windows
         * applications. It is not a true HINSTANCE, however. The only thing
         * that can be done with the returned HINSTANCE is to cast it to an int
         * and compare it with the value 32 or one of the error codes below."
         */
        const int err_code = reinterpret_cast<int>(err);

        /* Further MSDN says "Returns a value greater than 32 if successful, or
         * an error value that is less than or equal to 32 otherwise."
         *
         * So return successful when (err_code > 32), determine and the error
         * otherwise.
         */
        if (err_code > 32)
            return Error();

        // Construct an Error instance indicating an error has occurred.
        return Error(err_code);
    }

    static int __ShellExecuteShowCmd(const show_command showCmd)
    {
        switch(showCmd)
        {
            case show_command_hide:
                return SW_HIDE;

            case show_command_maximize:
                return SW_MAXIMIZE;

            case show_command_minimize:
                return SW_MINIMIZE;

            case show_command_restore:
                return SW_RESTORE;

            case show_command_show:
                return SW_SHOW;

            case show_command_default:
                return SW_SHOWDEFAULT;

            case show_command_maximized:
                return SW_SHOWMAXIMIZED;

            case show_command_minnotactive:
                return SW_SHOWMINIMIZED;

            case show_command_na:
                return SW_SHOWNA;

            case show_command_notactive:
                return SW_SHOWNOACTIVATE;

            case show_command_normal:
                return SW_SHOWNORMAL;

            case show_command_last__:
                // Only here to prevent warnings about unused enumeration values
                break;
        }

        // We shouldn't ever get here
        assert(!"__ShellExecute: invalid show_command");
        return 0;
    }

    Error ShellExecute(HWND hwnd,
                       const char* operation,
                       const char* file,
                       const char* parameters,
                       const char* directory,
                       show_command ShowCmd)
    {
        const HINSTANCE ret = ::ShellExecuteA(hwnd, operation, file, parameters, directory, __ShellExecuteShowCmd(ShowCmd));

        // Convert the return value into an Error instance
        return ShellExecute(ret);
    }

    Error ShellExecute(HWND hwnd,
                         const wchar_t* operation,
                         const wchar_t* file,
                         const wchar_t* parameters,
                         const wchar_t* directory,
                         show_command ShowCmd)
    {
        const HINSTANCE ret = ::ShellExecuteW(hwnd, operation, file, parameters, directory, __ShellExecuteShowCmd(ShowCmd));

        // Convert the return value into an Error instance
        return ShellExecute(ret);
    }
}
