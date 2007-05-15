/*
    Warzone 2100 Lobbyserver, serves as a meeting place to set up games
    Copyright (C) 2007  Giel van Schijndel

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    $Revision$
    $Id$
    $HeadURL$
*/

#ifndef _INCLUDE_READ_WRITE_MUTEX_HPP_
#define _INCLUDE_READ_WRITE_MUTEX_HPP_

#include <boost/utility.hpp>

// Special mutex class which allows multiple readers at the same time
// When a writer requests access, no simultaneous readers or writers are allowed
// This class favours writers over readers (i.e. as long as there are writers
// in the queue they will be allowed before any readers are)
class ReadWriteMutex : boost::noncopyable
{
    public:
        ReadWriteMutex();
        ~ReadWriteMutex();

        // Mutex lock classes

        // Read lock, can always be acquired as long as no writers are pending or locked the mutex
        // This lock additionally doesn't require a non-const mutex to be constructed from
        class scoped_readonlylock : boost::noncopyable
        {
            public:
                scoped_readonlylock(const ReadWriteMutex& mutex);
                ~scoped_readonlylock();

            private:
                const ReadWriteMutex& _mutex;
        };

        // Write lock, name and behaviour is similar to boost's mutex::scoped_lock
        // (which is the reason for the similar name)
        // This lock requires a non-const mutex to be constructed from
        class scoped_lock : boost::noncopyable
        {
            public:
                scoped_lock(ReadWriteMutex& mutex);
                ~scoped_lock();

            private:
                ReadWriteMutex& _mutex;
        };


    private:
        class impl;
        impl* pimpl;
};

#endif // _INCLUDE_READ_WRITE_MUTEX_HPP_
