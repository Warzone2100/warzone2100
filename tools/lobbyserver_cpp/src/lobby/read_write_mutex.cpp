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

#include "read_write_mutex.hpp"
#include <boost/thread.hpp>
#include "raii_counter.hpp"
#include <cassert>

class ReadWriteMutex::impl : boost::noncopyable
{
    public:
        impl() :
            _readerCount(0),
            _currentWriter(false)
        {
        }

        inline void acquireReadLock() const
        {
            boost::mutex::scoped_lock lock(_mutex);

            // require a while loop here, since when the writerFinished condition is notified
            // we should not allow readers to lock if there is a writer waiting
            // if there is a writer waiting, we continue waiting

            // If there currently is a writer lock, or writers are pending to lock then wait
            while (_pendingWriters != 0 || _currentWriter)
            {
                // Block here to prevent busy waiting, we'll be awakened as soon as a writer has finished
                _writerFinished.wait(lock);
            }

            ++_readerCount;
        }

        inline void releaseReadLock() const
        {
            boost::mutex::scoped_lock lock(_mutex);
            assert(_readerCount != 0);
            --_readerCount;

            if (_readerCount == 0)
            {
                // notify all pending writers that there currently are no more readers
                _allReadersFinished.notify_all();
            }
        }

        inline void acquireWriteLock()
        {
            boost::mutex::scoped_lock lock(_mutex);

            // ensure subsequent readers block
            RAIICounter::scope_counted pendWriter(_pendingWriters);

            // wait until all reader locks are released
            if (_readerCount != 0)
            {
                // * Release our mutex lock
                // * Block until we're notified there are no more readers holding a lock
                // * Reacquire our mutex lock (or block until we do so)
                _allReadersFinished.wait(lock);
            }

            // only become a writer when there currently is none
            while (_currentWriter)
            {
                // block until the current writer releases its lock
                _writerFinished.wait(lock);
            }

            // Yay! We're now the current writer, make sure others know we are
            _currentWriter = true;
        }

        inline void releaseWriteLock()
        {
            boost::mutex::scoped_lock lock (_mutex);
            _currentWriter = false;
            _writerFinished.notify_all();
        }

    private:
        mutable boost::mutex      _mutex;

        mutable volatile unsigned int _readerCount;
        mutable boost::condition  _allReadersFinished;

        RAIICounter               _pendingWriters;
        volatile bool             _currentWriter;
        mutable boost::condition  _writerFinished;
};

// Mutex's constructor and destructor; handle construction/destruction of our PIMPL (Private IMPLementation)
ReadWriteMutex::ReadWriteMutex() :
        pimpl(new impl)
{
}

ReadWriteMutex::~ReadWriteMutex()
{
    delete pimpl;
}

// Constructor and destructor for readonly lock
ReadWriteMutex::scoped_readonlylock::scoped_readonlylock(const ReadWriteMutex& mutex) : _mutex(mutex)
{
    _mutex.pimpl->acquireReadLock();
}

ReadWriteMutex::scoped_readonlylock::~scoped_readonlylock()
{
    _mutex.pimpl->releaseReadLock();
}

// Constructor and destructor for write lock
ReadWriteMutex::scoped_lock::scoped_lock(ReadWriteMutex& mutex) : _mutex(mutex)
{
    _mutex.pimpl->acquireWriteLock();
}

ReadWriteMutex::scoped_lock::~scoped_lock()
{
    _mutex.pimpl->releaseWriteLock();
}
