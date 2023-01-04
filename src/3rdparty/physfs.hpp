// A heavily modified / reduced version of physfs-hpp
// Original: https://github.com/Ybalrid/physfs-hpp/blob/master/include/physfs.hpp
//
// WZ Modifications:
//	- Reduced down to just the filestream classes, restructured, uses WZ PHYSFS wrappers
//  - Workaround to fix -Wswitch warning
//
// Copyright (c) 2021 Warzone 2100 Project
// Licensed under the same terms as the original license (below).
//
// Original License:
//
//   Copyright (c) 2013 Kevin Howell and others.
//   Copyright (c) 2018 Arthur Brainville (Ybalrid)
//
//   This software is provided 'as-is', without any express or implied warranty.
//   In no event will the authors be held liable for any damages arising from
//   the use of this software.
//
//   Permission is granted to anyone to use this software for any purpose,
//   including commercial applications, and to alter it and redistribute it
//   freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//   claim that you wrote the original software. If you use this software in a
//   product, an acknowledgment in the product documentation would be
//   appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//   3. This notice may not be removed or altered from any source distribution.
//
//       Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//

#ifndef _INCLUDE_PHYSFS_HPP_
#define _INCLUDE_PHYSFS_HPP_

#include <physfs.h>
#include <string>
#include <iostream>

namespace PhysFS {

	typedef enum {
		READ,
		WRITE,
		APPEND
	} mode;

	class base_fstream {
	protected:
		PHYSFS_File * const file;
	public:
		base_fstream(PHYSFS_File * file);
		virtual ~base_fstream();
		size_t length();
	};

	class ifstream : public base_fstream, public std::istream {
	protected:
		ifstream(PHYSFS_File * file);
	public:
		static std::shared_ptr<ifstream> make(std::string const & filename);
		virtual ~ifstream();
	};

	class ofstream : public base_fstream, public std::ostream {
	protected:
		ofstream(PHYSFS_File * file);
	public:
		static std::shared_ptr<ofstream> make(std::string const & filename, mode writeMode = WRITE);
		virtual ~ofstream();
	};

	class fstream : public base_fstream, public std::iostream {
	protected:
		fstream(PHYSFS_File * file);
	public:
		static std::shared_ptr<fstream> make(std::string const & filename, mode openMode = READ);
		virtual ~fstream();
	};
}

#ifdef PHYFSPP_IMPL

#include <streambuf>
#include <string>
#include <string.h>
#include <stdexcept>
#include "lib/framework/physfs_ext.h"

using std::streambuf;
using std::ios_base;

namespace PhysFS {

class fbuf : public streambuf {
private:
	fbuf(const fbuf & other);
	fbuf& operator=(const fbuf& other);

	int_type underflow() {
		if (PHYSFS_eof(file)) {
			return traits_type::eof();
		}
		size_t bytesRead = WZ_PHYSFS_readBytes(file, buffer, bufferSize);
		if (bytesRead < 1) {
			return traits_type::eof();
		}
		setg(buffer, buffer, buffer + bytesRead);
		return (unsigned char) *gptr();
	}

	pos_type seekoff(off_type pos, ios_base::seekdir dir, ios_base::openmode mode) {
		if (dir == std::ios_base::beg)
		{
			PHYSFS_seek(file, pos);
		}
		else if (dir == std::ios_base::cur)
		{
			// subtract characters currently in buffer from seek position
			PHYSFS_seek(file, (PHYSFS_tell(file) + pos) - (egptr() - gptr()));
		}
		else if (dir == std::ios_base::end)
		{
			PHYSFS_seek(file, PHYSFS_fileLength(file) + pos);
		}
		else
		{
			throw std::invalid_argument("invalid seekdir value");
		}

		if (mode & std::ios_base::in) {
			setg(egptr(), egptr(), egptr());
		}
		if (mode & std::ios_base::out) {
			setp(buffer, buffer);
		}
		return PHYSFS_tell(file);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode mode) {
		PHYSFS_seek(file, pos);
		if (mode & std::ios_base::in) {
			setg(egptr(), egptr(), egptr());
		}
		if (mode & std::ios_base::out) {
			setp(buffer, buffer);
		}
		return PHYSFS_tell(file);
	}

	int_type overflow( int_type c = traits_type::eof() ) {
		if (pptr() == pbase() && c == traits_type::eof()) {
			return 0; // no-op
		}
		if (WZ_PHYSFS_writeBytes(file, pbase(), pptr() - pbase()) < 1) {
			return traits_type::eof();
		}
		if (c != traits_type::eof()) {
			if (WZ_PHYSFS_writeBytes(file, &c, 1) < 1) {
				return traits_type::eof();
			}
		}

		return 0;
	}

	int sync() {
		return overflow();
	}

	char * buffer;
	size_t const bufferSize;
protected:
	PHYSFS_File * const file;
public:
	fbuf(PHYSFS_File * file, std::size_t bufferSize = 2048) : bufferSize(bufferSize), file(file) {
		buffer = new char[bufferSize];
		char * end = buffer + bufferSize;
		setg(end, end, end);
		setp(buffer, end);
	}

	~fbuf() {
		sync();
		delete [] buffer;
	}
};

base_fstream::base_fstream(PHYSFS_File* file) : file(file) {
    if (file == nullptr) {
        throw std::invalid_argument("attempted to construct fstream with NULL ptr");
    }
}

base_fstream::~base_fstream() {
	PHYSFS_close(file);
}

size_t base_fstream::length() {
	return PHYSFS_fileLength(file);
}

PHYSFS_File* openWithMode(char const * filename, mode openMode) {
    PHYSFS_File* file = nullptr;
	switch (openMode) {
	case WRITE:
		file = PHYSFS_openWrite(filename);
        break;
	case APPEND:
		file = PHYSFS_openAppend(filename);
        break;
	case READ:
		file = PHYSFS_openRead(filename);
	}
    return file;
}

ifstream::ifstream(PHYSFS_File * file)
	: base_fstream(file), std::istream(new fbuf(file)) {}

std::shared_ptr<ifstream> ifstream::make(std::string const & filename)
{
	PHYSFS_File* file = openWithMode(filename.c_str(), READ);
	if (!file) { return nullptr; }
	return std::shared_ptr<ifstream>(new ifstream(file));
}

ifstream::~ifstream() {
	delete rdbuf();
}

ofstream::ofstream(PHYSFS_File * file)
	: base_fstream(file), std::ostream(new fbuf(file)) {}

std::shared_ptr<ofstream> ofstream::make(std::string const & filename, mode writeMode)
{
	PHYSFS_File* file = openWithMode(filename.c_str(), writeMode);
	if (!file) { return nullptr; }
	return std::shared_ptr<ofstream>(new ofstream(file));
}

ofstream::~ofstream() {
	delete rdbuf();
}

fstream::fstream(PHYSFS_File * file)
	: base_fstream(file), std::iostream(new fbuf(file)) {}

std::shared_ptr<fstream> fstream::make(std::string const & filename, mode openMode)
{
	PHYSFS_File* file = openWithMode(filename.c_str(), openMode);
	if (!file) { return nullptr; }
	return std::shared_ptr<fstream>(new fstream(file));
}

fstream::~fstream() {
	delete rdbuf();
}

} // namespace PhysFS

#endif

#endif /* _INCLUDE_PHYSFS_HPP_ */
