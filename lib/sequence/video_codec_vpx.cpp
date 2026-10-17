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

#include "lib/framework/frame.h"
#include "webm_video_codec.h"

#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#include <cstring>

namespace
{

class VpxVideoCodec final : public WZWebmVideoCodec
{
public:
	static std::unique_ptr<VpxVideoCodec> create(const char *codecId)
	{
		vpx_codec_iface_t *iface = nullptr;
		if (strcmp(codecId, "V_VP8") == 0)
		{
			iface = vpx_codec_vp8_dx();
		}
		else if (strcmp(codecId, "V_VP9") == 0)
		{
			iface = vpx_codec_vp9_dx();
		}
		if (!iface)
		{
			return nullptr;
		}

		auto codec = std::unique_ptr<VpxVideoCodec>(new VpxVideoCodec());
		vpx_codec_dec_cfg_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.threads = wzWebmVideoCodecDecodeThreads();
		if (vpx_codec_dec_init(&codec->m_ctx, iface, &cfg, 0) != VPX_CODEC_OK)
		{
			debug(LOG_ERROR, "vpx_codec_dec_init() failed: %s", vpx_codec_error(&codec->m_ctx));
			return nullptr;
		}
		codec->m_initialized = true;
		return codec;
	}

	~VpxVideoCodec() override
	{
		if (m_initialized)
		{
			vpx_codec_destroy(&m_ctx);
		}
	}

	const char* backendName() const override
	{
		return "libvpx";
	}

	bool decode(const uint8_t *data, size_t len, double pts) override
	{
		(void)pts;
		if (vpx_codec_decode(&m_ctx, data, static_cast<unsigned>(len), nullptr, 0) != VPX_CODEC_OK)
		{
			debug(LOG_WARNING, "vpx_codec_decode() failed: %s", vpx_codec_error(&m_ctx));
			return false;
		}
		return true;
	}

	bool getFrame(WZVideoFrameYUV& out) override
	{
		vpx_codec_iter_t iter = nullptr;
		vpx_image_t *img = vpx_codec_get_frame(&m_ctx, &iter);
		if (!img)
		{
			return false;
		}
		if (img->fmt != VPX_IMG_FMT_I420)
		{
			if (!m_formatErrorLogged)
			{
				debug(LOG_ERROR, "Video is not 8-bit YUV420 (vpx format %d); only VP8/VP9 profile 0 is supported", (int)img->fmt);
				m_formatErrorLogged = true;
			}
			return false;
		}
		out.format = WZVideoPlaneFormat::I420;
		out.y = img->planes[VPX_PLANE_Y];
		out.u = img->planes[VPX_PLANE_U];
		out.v = img->planes[VPX_PLANE_V];
		out.yStride = img->stride[VPX_PLANE_Y];
		out.uvStride = img->stride[VPX_PLANE_U];
		out.width = img->d_w;
		out.height = img->d_h;
		switch (img->cs)
		{
		case VPX_CS_BT_709:
			out.colorMatrix = WZVideoColorMatrix::BT709;
			break;
		case VPX_CS_BT_601:
		case VPX_CS_SMPTE_170:
			out.colorMatrix = WZVideoColorMatrix::BT601;
			break;
		default:	// unknown (VP8 has no color space field) or unsupported
			out.colorMatrix = containerColorMatrix;
			break;
		}
		// VP8 streams always report studio range
		out.fullRange = (img->range == VPX_CR_FULL_RANGE) || (img->cs == VPX_CS_UNKNOWN && containerFullRange);
		return true;
	}

private:
	VpxVideoCodec() { }

	vpx_codec_ctx_t m_ctx;
	bool m_initialized = false;
	bool m_formatErrorLogged = false;
};

} // anonymous namespace

std::unique_ptr<WZWebmVideoCodec> wzCreateVpxVideoCodec(const char *codecId, unsigned width, unsigned height)
{
	(void)width;
	(void)height;
	return VpxVideoCodec::create(codecId);
}

bool wzVpxVideoCodecAvailable(WzString& detail)
{
	detail = vpx_codec_version_str();
	return true;
}
