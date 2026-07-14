// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

/** @file sequence.cpp
 *
 * The FMV playback engine: drives a WZVideoDecoder (container/codec specific,
 * see video_decoder.h), schedules decoded frames against the playback clock,
 * converts YUV to RGBA (with optional scanline emulation), and streams decoded
 * PCM to OpenAL.
 *
 * Clock design: when audio is playing, playback time is derived from the
 * number of samples OpenAL has actually consumed (audio-master); video and
 * subtitles follow it, so main-loop stalls (e.g. modal window resizes)
 * pause the whole presentation instead of desynchronizing it. Without audio, a
 * monotonic clock with stall clamping is used.
 */

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "sequence.h"
#include "video_decoder.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/sound/audio.h"
#include "lib/sound/openal_error.h"
#include "lib/sound/mixer.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include "lib/ivis_opengl/pieclip.h"

#include <AL/al.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <vector>

// Screen dimensions
#define NUM_VERTICES 4
static std::unique_ptr<GFX> videoGfx = nullptr;
static gfx_api::gfxFloat vertices[NUM_VERTICES][2];
static gfx_api::gfxFloat Scrnvidpos[3];

static iV_Image VideoFrameBitmap;			// RGBA frame buffer
static SCANLINE_MODE scanMode = SCANLINES_OFF;
static SCANLINE_MODE use_scanlines = SCANLINES_OFF;
static bool scanlinesDisabled = false;

const size_t texture_width = 1024;
const size_t texture_height = 1024;

/** Streams decoded PCM to a dedicated OpenAL source.
 *
 * Keeps a rotation of NUM_BUFFERS small buffers (~125ms each) queued, and acts
 * as the playback master clock: playbackTime() reports how much audio the
 * listener has actually heard, derived from consumed samples. On main-loop
 * stalls the source underruns and the clock stops advancing; update() restarts
 * playback and the presentation resumes in sync.
 */
class VideoAudioSink
{
	static const size_t NUM_BUFFERS = 8;
	static const size_t BUFFERS_PER_SECOND = 8;
public:
	static std::unique_ptr<VideoAudioSink> create(unsigned channels, unsigned sampleRate)
	{
		auto sink = std::unique_ptr<VideoAudioSink>(new VideoAudioSink(channels, sampleRate));

		alGetError();
		alGenSources(1, &sink->m_source);
		if (sound_GetError() != AL_NO_ERROR)
		{
			debug(LOG_WARNING, "Failed to allocate an OpenAL source for FMV audio; playing without sound");
			return nullptr;
		}
		alGenBuffers(static_cast<ALsizei>(sink->m_buffers.size()), sink->m_buffers.data());
		if (sound_GetError() != AL_NO_ERROR)
		{
			alDeleteSources(1, &sink->m_source);
			sink->m_source = 0;
			debug(LOG_WARNING, "Failed to allocate OpenAL buffers for FMV audio; playing without sound");
			return nullptr;
		}
		sink->m_freeBuffers.assign(sink->m_buffers.begin(), sink->m_buffers.end());

		// set the volume of the FMV based on the user's preferences
		alSourcef(sink->m_source, AL_GAIN, sound_GetUIVolume());
		sound_GetError();

		return sink;
	}

	~VideoAudioSink()
	{
		if (m_source != 0)
		{
			alSourceStop(m_source);
			alSourcei(m_source, AL_BUFFER, 0);	// detach all buffers
			alDeleteSources(1, &m_source);
			alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());
			sound_GetError();
		}
	}

	/** Reclaim played buffers, refill from the decoder, and (re)start playback */
	void update(WZVideoDecoder& decoder)
	{
		// reclaim processed buffers, crediting them to the playback clock
		ALint processed = 0;
		alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
		while (processed-- > 0)
		{
			ALuint buf = 0;
			alSourceUnqueueBuffers(m_source, 1, &buf);
			ALint bytes = 0;
			alGetBufferi(buf, AL_SIZE, &bytes);
			m_samplesConsumed += static_cast<uint64_t>(bytes) / (m_channels * sizeof(int16_t));
			m_freeBuffers.push_back(buf);
		}

		// refill & queue
		while (!m_freeBuffers.empty() && !m_inputExhausted)
		{
			while (m_stagingFill < m_samplesPerBuffer && !m_inputExhausted)
			{
				double pts = 0.0;
				size_t samples = decoder.decodeAudio(m_staging.data() + m_stagingFill * m_channels,
				                                     m_samplesPerBuffer - m_stagingFill, pts);
				if (samples == 0)
				{
					m_inputExhausted = true;
					break;
				}
				if (!m_havePts)
				{
					m_firstPts = pts;
					m_havePts = true;
				}
				m_stagingFill += samples;
			}

			if (m_stagingFill == 0)
			{
				break;
			}

			ALuint buf = m_freeBuffers.back();
			m_freeBuffers.pop_back();
			alBufferData(buf, (m_channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
			             m_staging.data(), static_cast<ALsizei>(m_stagingFill * m_channels * sizeof(int16_t)),
			             static_cast<ALsizei>(m_rate));
			alSourceQueueBuffers(m_source, 1, &buf);
			m_stagingFill = 0;
		}

		// start playback / recover from underrun
		// (safe at end-of-stream: fully-played buffers were unqueued above, so queued == 0 there)
		ALint queued = 0;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
		ALint state = AL_STOPPED;
		alGetSourcei(m_source, AL_SOURCE_STATE, &state);
		if (queued > 0 && state != AL_PLAYING)
		{
			debug(LOG_VIDEO, "starting/resuming FMV audio source");
			alSourcePlay(m_source);
			m_started = true;
		}
		sound_GetError();
	}

	bool started() const { return m_started; }

	/** All input decoded, queued and played out? */
	bool finished()
	{
		if (!m_inputExhausted || m_stagingFill > 0)
		{
			return false;
		}
		ALint queued = 0;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
		ALint state = AL_STOPPED;
		alGetSourcei(m_source, AL_SOURCE_STATE, &state);
		return queued == 0 && state != AL_PLAYING;
	}

	/** Presentation time (seconds) of the audio the listener has heard so far */
	double playbackTime()
	{
		if (!m_started)
		{
			return m_firstPts;
		}
		ALint sampleOffset = 0;
		alGetSourcei(m_source, AL_SAMPLE_OFFSET, &sampleOffset);	// position within the currently-queued buffers
		double t = m_firstPts + static_cast<double>(m_samplesConsumed + static_cast<uint64_t>(std::max(sampleOffset, 0))) / m_rate;
		m_lastTime = std::max(m_lastTime, t);	// guard against small backwards steps around buffer requeueing
		return m_lastTime;
	}

private:
	VideoAudioSink(unsigned channels, unsigned sampleRate)
	: m_channels(channels)
	, m_rate(sampleRate)
	, m_samplesPerBuffer(std::max<size_t>(sampleRate / BUFFERS_PER_SECOND, 1))
	{
		m_staging.resize(m_samplesPerBuffer * m_channels);
	}

private:
	ALuint m_source = 0;
	std::array<ALuint, NUM_BUFFERS> m_buffers;
	std::vector<ALuint> m_freeBuffers;
	unsigned m_channels;
	unsigned m_rate;
	size_t m_samplesPerBuffer;		// per channel
	std::vector<int16_t> m_staging;
	size_t m_stagingFill = 0;		// samples (per channel) currently staged
	uint64_t m_samplesConsumed = 0;	// samples (per channel) of fully-played, unqueued buffers
	double m_firstPts = 0.0;
	double m_lastTime = 0.0;
	bool m_havePts = false;
	bool m_started = false;
	bool m_inputExhausted = false;
};

/** The playback clock (seconds since the start of the video).
 *
 * Audio-master while audio is playing; otherwise a monotonic clock that clamps
 * away main-loop stalls (so a blocked event loop pauses rather than skips the
 * presentation), staying continuous with the audio clock when audio ends.
 */
class PlaybackClock
{
	static constexpr double STALL_THRESHOLD = 0.2;	// seconds between ticks considered a stall
public:
	/** Advance the clock; call once per seq_Update() tick before reading now() */
	void tick(VideoAudioSink *audioSink)
	{
		const auto realNow = std::chrono::steady_clock::now();
		if (!m_epochValid)
		{
			m_epoch = realNow;
			m_lastTick = realNow;
			m_epochValid = true;
		}

		const double tickDelta = std::chrono::duration<double>(realNow - m_lastTick).count();
		if (tickDelta > STALL_THRESHOLD)
		{
			// the main loop stalled (window resize, load hitch, ...): pause the clock across it
			m_stallAdjustment += tickDelta - STALL_THRESHOLD;
		}
		m_lastTick = realNow;

		const double wallTime = std::chrono::duration<double>(realNow - m_epoch).count() - m_stallAdjustment;
		if (audioSink && audioSink->started() && !audioSink->finished())
		{
			// audio-master: drive the presentation from what the listener actually hears
			const double audioTime = audioSink->playbackTime();
			m_wallOffset = audioTime - wallTime;	// keep the fallback clock continuous with audio
			m_current = std::max(m_current, audioTime);
		}
		else
		{
			m_current = std::max(m_current, wallTime + m_wallOffset);
		}
	}

	double now() const { return m_current; }

private:
	std::chrono::steady_clock::time_point m_epoch;
	std::chrono::steady_clock::time_point m_lastTick;
	bool m_epochValid = false;
	double m_stallAdjustment = 0.0;
	double m_wallOffset = 0.0;
	double m_current = 0.0;
};

/** All state for one video playback session */
struct VideoPlayback
{
	std::shared_ptr<VideoProvider> provider;
	std::unique_ptr<WZVideoDecoder> decoder;
	std::unique_ptr<VideoAudioSink> audioSink;
	PlaybackClock clock;

	// the next decoded-but-not-yet-displayed frame (planes borrowed from the decoder)
	WZVideoFrameYUV pendingFrame;
	bool havePendingFrame = false;
	bool videoExhausted = false;

	double frameDuration = 1.0 / 25.0;
	double displayedFrameTime = 0.0;	// pts of the frame currently on screen
	double lastDisplayClockTime = 0.0;	// clock time when we last put a frame on screen

	// frame & dropped frame counters
	int frames = 0;
	int dropped = 0;
};

static std::unique_ptr<VideoPlayback> playback;

/** Allocates memory to hold the decoded video frame */
static void Allocate_videoFrame(unsigned videoWidth, unsigned videoHeight)
{
	scanMode = seq_getScanlinesDisabled() ? SCANLINES_OFF : seq_getScanlineMode();
	const unsigned height_factor = (scanMode ? 2 : 1);
	size_t expected_size = static_cast<size_t>(videoWidth) * videoHeight * 4 * height_factor;

	VideoFrameBitmap.allocate(videoWidth, videoHeight * height_factor, 4, true);
	ASSERT(VideoFrameBitmap.data_size() == expected_size, "Allocated size does not match expected size!");
}

#ifndef __BIG_ENDIAN__
const int Rshift = 0;
const int Gshift = 8;
const int Bshift = 16;
const int Ashift = 24;
// RGBmask is used only after right-shifting, so ignore the leftmost bit of each byte
const int RGBmask = 0x007f7f7f;
const int Amask = 0xff000000;
#else
const int Rshift = 24;
const int Gshift = 16;
const int Bshift = 8;
const int Ashift = 0;
const int RGBmask = 0x7f7f7f00;
const int Amask = 0x000000ff;
#endif
#define Vclip( x )	( (x > 0) ? ((x < 255) ? x : 255) : 0 )

/** Convert a decoded YUV420 frame to RGBA (with optional scanline emulation) and upload it */
static void video_upload_frame(const WZVideoFrameYUV& frame)
{
	const int video_width = static_cast<int>(frame.width);
	const int video_height = static_cast<int>(frame.height);
	const int half_width = video_width / 2;
	unsigned char *pRGBABitmapData = VideoFrameBitmap.bmp_w();

	auto setRGBAFramePixel = [pRGBABitmapData](int pixelOffset, uint32_t rgbaValue) {
		memcpy(&pRGBABitmapData[(pixelOffset * 4)], &rgbaValue, sizeof(uint32_t));
	};

	int rgb_offset = 0;
	for (int y = 0; y < video_height; y++)
	{
		int y_offset = y * frame.yStride;
		int uv_offset = (y >> 1) * frame.uvStride;

		for (int x = 0; x < half_width; x++)
		{
			int Y = frame.y[y_offset++] - 16;
			const int U = frame.u[uv_offset] - 128;
			const int V = frame.v[uv_offset++] - 128;

			int A = 298 * Y;
			const int C = 409 * V;

			int R = Vclip((A + C + 128) >> 8);
			int G = Vclip((A - 100 * U - (C >> 1) + 128) >> 8);
			int B = Vclip((A + 516 * U + 128) >> 8);

			uint32_t rgba = (R << Rshift) | (G << Gshift) | (B << Bshift) | (0xFF << Ashift);

			setRGBAFramePixel(rgb_offset, rgba);
			if (scanMode == SCANLINES_50)
			{
				// halve the rgb values for a dimmed scanline
				setRGBAFramePixel(rgb_offset + video_width, (rgba >> 1 & RGBmask) | Amask);
			}
			else if (scanMode == SCANLINES_BLACK)
			{
				setRGBAFramePixel(rgb_offset + video_width, Amask);
			}
			rgb_offset++;

			// second pixel, U and V (and thus C) are the same as before.
			Y = frame.y[y_offset++] - 16;
			A = 298 * Y;

			R = Vclip((A + C + 128) >> 8);
			G = Vclip((A - 100 * U - (C >> 1) + 128) >> 8);
			B = Vclip((A + 516 * U + 128) >> 8);

			rgba = (R << Rshift) | (G << Gshift) | (B << Bshift) | (0xFF << Ashift);
			setRGBAFramePixel(rgb_offset, rgba);
			if (scanMode == SCANLINES_50)
			{
				// halve the rgb values for a dimmed scanline
				setRGBAFramePixel(rgb_offset + video_width, (rgba >> 1 & RGBmask) | Amask);
			}
			else if (scanMode == SCANLINES_BLACK)
			{
				setRGBAFramePixel(rgb_offset + video_width, Amask);
			}
			rgb_offset++;
		}
		if (scanMode)
		{
			rgb_offset += video_width;
		}
	}

	videoGfx->updateTexture(VideoFrameBitmap);
}

/** Draw the current video texture to the screen */
static void video_draw()
{
	if (!videoGfx)
	{
		return;
	}

	const auto& modelViewProjectionMatrix = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f) *
	glm::translate(glm::vec3(Scrnvidpos[0], Scrnvidpos[1], Scrnvidpos[2]));

	gfx_api::VideoPSO::get().bind();
	gfx_api::VideoPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec2(0), glm::vec2(0), glm::vec4(1), 0 });
	videoGfx->draw<gfx_api::VideoPSO>(modelViewProjectionMatrix);
}

void update_buffers()
{
	if (videoGfx == nullptr || !playback)
	{
		return;
	}

	const auto& vmeta = playback->decoder->videoMetadata();

	// when using scanlines we need to double the height
	const uint32_t height_factor = ((!seq_getScanlinesDisabled() && seq_getScanlineMode()) ? 2 : 1);
	const gfx_api::gfxFloat vtwidth = (float)vmeta.width / (float)texture_width;
	const gfx_api::gfxFloat vtheight = (float)vmeta.height * height_factor / (float)texture_height;
	gfx_api::gfxFloat texcoords[NUM_VERTICES * 2] = {0.0f, 0.0f, vtwidth, 0.0f, 0.0f, vtheight, vtwidth, vtheight};
	videoGfx->buffers(NUM_VERTICES, vertices, texcoords);
}

bool seq_Play(std::shared_ptr<VideoProvider> video)
{
	debug(LOG_VIDEO, "starting playback of: %s", (video) ? video->filename().toUtf8().c_str() : "");

	if (playback)
	{
		debug(LOG_VIDEO, "previous movie is not yet finished");
		seq_Shutdown();
	}

	seq_setScanlinesDisabled(false);

	if (!video)
	{
		return false;
	}

	auto session = std::make_unique<VideoPlayback>();
	session->provider = video;
	session->decoder = videoDecoderOpen(video);
	if (!session->decoder)
	{
		return false;
	}
	session->videoExhausted = !session->decoder->hasVideo();	// audio-only: end when audio ends

	/* open audio (the sink is also the playback master clock) */
	if (session->decoder->hasAudio() && !audio_Disabled())
	{
		// play the container's default audio track
		const WZAudioTrackMetadata& track = session->decoder->audioTracks().front();
		session->audioSink = VideoAudioSink::create(track.channels, track.sampleRate);
		// a null sink (OpenAL sources exhausted) means we play silently, on the fallback clock
	}

	/* open video */
	videoGfx = std::make_unique<GFX>(GFX_TEXTURE, 2);
	if (session->decoder->hasVideo())
	{
		const WZVideoTrackMetadata& vmeta = session->decoder->videoMetadata();
		if (vmeta.width > texture_width || vmeta.height > texture_height)
		{
			debug(LOG_ERROR, "Video size too large, must be below %zu x %zu!",
			      texture_width, texture_height);
			videoGfx = nullptr;
			return false;
		}

		iV_Image blackFrame;
		blackFrame.allocate(texture_width, texture_height, 4, true);

		// disable scanlines temporarily if the video is too large for the texture or shown too small
		if (vmeta.height * 2 > texture_height || vertices[3][1] < vmeta.height * 2)
		{
			seq_setScanlinesDisabled(true);
		}

		Allocate_videoFrame(vmeta.width, vmeta.height);
		videoGfx->makeCompatibleTexture(&blackFrame, "mem::blackframe");
		videoGfx->updateTexture(blackFrame);
		blackFrame.clear();

		if (vmeta.fps > 0.0)
		{
			session->frameDuration = 1.0 / vmeta.fps;
		}
	}

	playback = std::move(session);
	update_buffers();
	return true;
}

bool seq_Playing()
{
	return playback != nullptr;
}

/**
 * Display the next frame and play the sound.
 * \return false if the end of the video is reached.
 */
bool seq_Update()
{
	if (!playback)
	{
		debug(LOG_VIDEO, "no movie playing");
		return false;
	}
	VideoPlayback& pb = *playback;

	/* keep the audio sink topped up (this also reclaims played buffers,
	   which is what advances the audio-master clock) */
	if (pb.audioSink)
	{
		pb.audioSink->update(*pb.decoder);
	}

	pb.clock.tick(pb.audioSink.get());
	const double now = pb.clock.now();

	/* make sure a decoded frame is pending */
	if (!pb.havePendingFrame && !pb.videoExhausted && pb.decoder->hasVideo())
	{
		pb.havePendingFrame = pb.decoder->nextVideoFrame(pb.pendingFrame);
		pb.videoExhausted = !pb.havePendingFrame;
	}

	/* running slow? skip frames that are late by more than a frame period,
	   but always show at least one frame per second */
	while (pb.havePendingFrame
	       && (now - pb.pendingFrame.pts) > pb.frameDuration
	       && (now - pb.lastDisplayClockTime) < 1.0)
	{
		pb.dropped++;
		pb.havePendingFrame = pb.decoder->nextVideoFrame(pb.pendingFrame);
		pb.videoExhausted = !pb.havePendingFrame;
	}

	/* are we done? */
	const bool audioDone = !pb.audioSink || pb.audioSink->finished();
	if (!pb.havePendingFrame && pb.videoExhausted && audioDone)
	{
		video_draw();
		seq_Shutdown();
		debug(LOG_VIDEO, "video finished");
		return false;
	}

	/* at or past time for the pending video frame? */
	if (pb.havePendingFrame && pb.pendingFrame.pts <= now)
	{
		video_upload_frame(pb.pendingFrame);
		pb.frames++;
		pb.displayedFrameTime = pb.pendingFrame.pts;
		pb.lastDisplayClockTime = now;
		pb.havePendingFrame = false;	// consumed; the next tick pulls the next frame
	}
	video_draw();

	return true;
}

void seq_Shutdown()
{
	debug(LOG_VIDEO, "seq_Shutdown");

	if (!playback)
	{
		debug(LOG_VIDEO, "movie is not playing");
		return;
	}

	const int frames = playback->frames;
	const int dropped = playback->dropped;

	videoGfx = nullptr;
	VideoFrameBitmap.clear();
	playback.reset();	// tears down the audio sink (OpenAL source/buffers), decoder, and provider

	seq_setScanlinesDisabled(false);

	debug(LOG_VIDEO, " **** frames = %d dropped = %d ****", frames, dropped);
}

int seq_GetFrameNumber()
{
	return playback ? playback->frames : 0;
}

double seq_GetFrameTime()
{
	// presentation time of the frame currently on screen (drives subtitle timing)
	return playback ? playback->displayedFrameTime : 0.0;
}

// this controls the size of the video to display on screen
void seq_SetDisplaySize(int sizeX, int sizeY, int posX, int posY)
{
	vertices[0][0] = 0.0f;
	vertices[0][1] = 0.0f;
	vertices[1][0] = sizeX;
	vertices[1][1] = 0.0f;
	vertices[2][0] = 0.0f;
	vertices[2][1] = sizeY;
	vertices[3][0] = sizeX;
	vertices[3][1] = sizeY;

	Scrnvidpos[0] = posX;
	Scrnvidpos[1] = posY;
	Scrnvidpos[2] = 0.0f;

	update_buffers();
}

void seq_setScanlinesDisabled(bool flag)
{
	scanlinesDisabled = flag;
}

bool seq_getScanlinesDisabled()
{
	return scanlinesDisabled;
}

void seq_setScanlineMode(SCANLINE_MODE mode)
{
	use_scanlines = mode;
}

SCANLINE_MODE seq_getScanlineMode(void)
{
	return use_scanlines;
}
