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

#include "raii_counter.hpp"

RAIICounter::RAIICounter() :
    _count(0)
{
}

RAIICounter::scope_counted::scope_counted(RAIICounter& counter) :
    _counter(counter)
{
    ++(_counter._count);
}

RAIICounter::scope_counted::scope_counted(const scope_counted& org) :
    _counter(org._counter)
{
    ++(_counter._count);
}

RAIICounter::scope_counted::~scope_counted()
{
    --(_counter._count);
}
