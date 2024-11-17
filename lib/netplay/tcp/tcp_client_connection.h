/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "lib/netplay/client_connection.h"

namespace tcp
{

struct Socket;

class TCPClientConnection : public IClientConnection
{
public:

	explicit TCPClientConnection(Socket* rawSocket);
	virtual ~TCPClientConnection() override;

	virtual net::result<ssize_t> readAll(void* buf, size_t size, unsigned timeout) override;
	virtual net::result<ssize_t> readNoInt(void* buf, size_t maxSize, size_t* rawByteCount) override;
	virtual net::result<ssize_t> writeAll(const void* buf, size_t size, size_t* rawByteCount) override;
	virtual bool readReady() const override;
	virtual void flush(size_t* rawByteCount) override;
	virtual void enableCompression() override;
	virtual void useNagleAlgorithm(bool enable) override;
	virtual std::string textAddress() const override;

private:

	Socket* socket_;

	friend class TCPConnectionPollGroup;
};

} // namespace tcp
