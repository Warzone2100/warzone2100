// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file read_scope.cpp
 * Implementation of read producer scope and compile-time layout state queries.
 */

#include "read_scope.h"

#include "layout_subresource.h"

#include "gfx_api.h"
#include "pass_resolve.h"

namespace gfx_api
{

ReadProducerScope classifyReadProducerScope(const ReadDesc& read,
	const std::vector<RenderPassDesc>& descs, size_t consumerIndex,
	const LayoutStateMap& layoutStateAtRead)
{
	switch (read.source)
	{
	case ReadSource::ExplicitTexture:
	case ReadSource::PipelineSurface:
		return ReadProducerScope::External;
	case ReadSource::PassOutput:
		if (read.producerPass >= consumerIndex || read.producerPass >= descs.size())
		{
			return ReadProducerScope::External;
		}
		{
			const auto output = getPassOutputAttachment(descs[read.producerPass],
				read.producerRole, read.attachmentIndex);
			if (!output.has_value() || output->texture == nullptr)
			{
				return ReadProducerScope::External;
			}
			if (layoutStateAtRead.find(layoutSubresourceKey(output->texture, output->arrayLayer, output->mipLevel))
				!= layoutStateAtRead.end())
			{
				return ReadProducerScope::InGraph;
			}
			return ReadProducerScope::External;
		}
	}
	return ReadProducerScope::External;
}

} // namespace gfx_api
