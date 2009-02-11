/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2009  Warzone Resurrection Project

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

/** @file
 *  Typesafe C++ interface for some Windows API functions
 */

#ifndef __INCLUDED_WINAPI_HPP__
#define __INCLUDED_WINAPI_HPP__

#include <windows.h>
#include <string>
#include <stdexcept>
#include <boost/strong_typedef.hpp>

#ifdef ShellExecute
# undef ShellExecute
#endif

#ifdef FormatMessage
# undef FormatMessage
#endif

#ifdef GetCurrentDirectory
# undef GetCurrentDirectory
#endif

namespace Win
{
    /** A wrapping class for Windows API errors that can be thrown.
     */
    class Error : public std::exception
    {
        public:
            /** Default constructor, creates a no-error instance.
             */
            Error();

            /** Constructor creating an error instance.
             *  \param error_code the Windows API's error code.
             */
            Error(int error_code);

            virtual ~Error() throw();

            /** You can use this to check wether an error has actually
             *  occurred.
             *  \return true if an error has occurred, false otherwise
             */
            operator bool() const;

            /** Formats the error message as a string, usable for output to a
             *  user.
             *  \return the formatted error string.
             */
            virtual const char* what() const throw();

            /** \return an instance of the last error that occurred
             */
            static Error GetLast();

        private:
            const int     _errCode;
            const bool    _isErr;

            // We will have "logical" constness for this variable
            mutable char* _errMsg;
    };

    /** \return the current working directory
     *  \throw Error when failing to retrieve the current working directory.
     *               Something will be badly wrong when this happens.
     */
    std::string GetCurrentDirectory();

    /** @note Don't depend on this typedef remaining ::HWND for ever, it might
     *        be wrapped in some class later on.
     */
    BOOST_STRONG_TYPEDEF(::HWND, HWND);

    enum show_command
    {
        show_command_hide,
        show_command_maximize,
        show_command_minimize,
        show_command_restore,
        show_command_show,
        show_command_default,
        show_command_maximized,
        show_command_minnotactive,
        show_command_na,
        show_command_notactive,
        show_command_normal,

        // MUST be the last one in the list!
        show_command_last__,
    };

    /** An overloaded interface to ShellExecuteA
     */
    Error ShellExecute(HWND hwnd,
                       const char* operation,
                       const char* file,
                       const char* parameters,
                       const char* directory,
                       show_command ShowCmd);

    /** An overloaded interface to ShellExecuteW
     */
    Error ShellExecute(HWND hwnd,
                       const wchar_t* operation,
                       const wchar_t* file,
                       const wchar_t* parameters,
                       const wchar_t* directory,
                       show_command ShowCmd);

    /** An easier interface to the ShellExecute functions.
     */
    template <typename charT>
    Error ShellExecute(const charT* fileName,
                       const charT* operation = NULL,
                       const charT* parameters = NULL,
                       const charT* directory = NULL,
                       HWND hwnd = HWND(NULL),
                       show_command showCmd = show_command_normal)
    {
        return ShellExecute(hwnd, operation, fileName, parameters, directory, showCmd);
    }

    // Apparently MSVC6 has trouble with the template instantation required for this function
#if 0
    /** An easier interface to the ShellExecute functions allowing to use std::string as parameters.
     */
    template<typename charT, typename traits, typename Allocator>
    Error ShellExecute(const std::basic_string<charT, traits, Allocator>& fileName   = std::basic_string<charT, traits, Allocator>(),
                       const std::basic_string<charT, traits, Allocator>& operation  = std::basic_string<charT, traits, Allocator>(),
                       const std::basic_string<charT, traits, Allocator>& parameters = std::basic_string<charT, traits, Allocator>(),
                       const std::basic_string<charT, traits, Allocator>& directory  = std::basic_string<charT, traits, Allocator>(),
                       HWND hwnd = HWND(NULL),
                       show_command showCmd = show_command_normal)
    {
        const charT * const file = fileName.c_str();
        const charT * const op     = operation.empty()  ? NULL : operation.c_str();
        const charT * const params = parameters.empty() ? NULL : parameters.c_str();
        const charT * const dir    = directory.empty()  ? NULL : directory.c_str();

        return ShellExecute(hwnd, op, file, params, dir, showCmd);
    }
#endif
}

#endif // __INCLUDED_WINAPI_HPP__
