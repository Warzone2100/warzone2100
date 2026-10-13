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

/** @file video_codec_mf_win.cpp
 *
 * VP8/VP9 decoding through a Media Foundation Transform.
 */

#include "lib/framework/frame.h"
#include "webm_video_codec.h"
#include "vp9_bitstream.h"

#include <objbase.h>
#include <oleauto.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mftransform.h>
#include <codecapi.h>
#include <icodecapi.h>

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace
{

// the Microsoft VPx decoder (undocumented)
const GUID kCLSID_MSVPxDecoderMFT =
	{ 0xE3AAF548, 0xC9A4, 0x4C6E, { 0x23, 0x4D, 0x5A, 0xDA, 0x37, 0x4B, 0x00, 0x00 } };

// larger than any video texture, and checked before the size reaches the MFT's buffer allocation
constexpr unsigned kMaxDimension = 16384;

// mingw-w64 only declares MF_LOW_LATENCY when WINVER targets Windows 10.
const GUID kMF_LOW_LATENCY =
	{ 0x9C27891A, 0xED7A, 0x40E1, { 0x88, 0xE8, 0xB2, 0x27, 0x27, 0xA0, 0x24, 0xEE } };

template <typename T>
class WZComPtr
{
public:
	WZComPtr() = default;
	WZComPtr(const WZComPtr&) = delete;
	WZComPtr& operator=(const WZComPtr&) = delete;
	WZComPtr(WZComPtr&& other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
	WZComPtr& operator=(WZComPtr&& other) noexcept
	{
		if (this != std::addressof(other)) { reset(); m_ptr = other.m_ptr; other.m_ptr = nullptr; }
		return *this;
	}
	~WZComPtr() { reset(); }

	T* get() const { return m_ptr; }
	T* operator->() const { return m_ptr; }
	explicit operator bool() const { return m_ptr != nullptr; }
	T** operator&() { reset(); return &m_ptr; }
	void attach(T *ptr) { reset(); m_ptr = ptr; }
	void reset() { if (m_ptr) { m_ptr->Release(); m_ptr = nullptr; } }

private:
	T *m_ptr = nullptr;
};

struct MfApi
{
	HRESULT (WINAPI *startup)(ULONG, DWORD) = nullptr;
	HRESULT (WINAPI *shutdown)(void) = nullptr;
	HRESULT (WINAPI *enumEx)(GUID, UINT32, const MFT_REGISTER_TYPE_INFO*, const MFT_REGISTER_TYPE_INFO*, IMFActivate***, UINT32*) = nullptr;
	HRESULT (WINAPI *createMediaType)(IMFMediaType**) = nullptr;
	HRESULT (WINAPI *createSample)(IMFSample**) = nullptr;
	HRESULT (WINAPI *createMemoryBuffer)(DWORD, IMFMediaBuffer**) = nullptr;
	HRESULT (WINAPI *createAlignedMemoryBuffer)(DWORD, DWORD, IMFMediaBuffer**) = nullptr;

	bool valid() const { return startup != nullptr; }
};

const MfApi& mfApi()
{
	static const MfApi api = []() {
		MfApi result;
		HMODULE mod = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!mod)
		{
			debug(LOG_VIDEO, "mfplat.dll is not available on this system");
			return result;
		}
		auto sym = [mod](const char *name) { return reinterpret_cast<void*>(GetProcAddress(mod, name)); };
		void *symbols[] = {
			sym("MFStartup"), sym("MFShutdown"), sym("MFTEnumEx"), sym("MFCreateMediaType"),
			sym("MFCreateSample"), sym("MFCreateMemoryBuffer"), sym("MFCreateAlignedMemoryBuffer"),
		};
		for (void *s : symbols)
		{
			if (!s)
			{
				debug(LOG_VIDEO, "mfplat.dll is missing expected exports");
				return result;
			}
		}
		result.shutdown = reinterpret_cast<decltype(result.shutdown)>(symbols[1]);
		result.enumEx = reinterpret_cast<decltype(result.enumEx)>(symbols[2]);
		result.createMediaType = reinterpret_cast<decltype(result.createMediaType)>(symbols[3]);
		result.createSample = reinterpret_cast<decltype(result.createSample)>(symbols[4]);
		result.createMemoryBuffer = reinterpret_cast<decltype(result.createMemoryBuffer)>(symbols[5]);
		result.createAlignedMemoryBuffer = reinterpret_cast<decltype(result.createAlignedMemoryBuffer)>(symbols[6]);
		result.startup = reinterpret_cast<decltype(result.startup)>(symbols[0]);
		return result;
	}();
	return api;
}

/** An MTA thread that owns every COM and MF object this file creates (SDL puts the main thread in an STA) */
class MfThread
{
public:
	~MfThread() { stop(); }

	bool start()
	{
		if (!mfApi().valid())
		{
			return false;
		}
		std::unique_lock<std::mutex> lock(m_mutex);
		m_thread = std::thread([this]() { threadMain(); });
		m_cvDone.wait(lock, [this]() { return m_initDone; });
		const bool ok = m_initOk;
		lock.unlock();
		if (!ok)
		{
			stop();
		}
		return ok;
	}

	void stop()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_thread.joinable())
			{
				return;
			}
			m_quit = true;
		}
		m_cvWork.notify_one();
		m_thread.join();
	}

	void runVoid(const std::function<void()>& fn)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!m_thread.joinable())
		{
			return;
		}
		m_job = &fn;
		m_jobDone = false;
		m_cvWork.notify_one();
		m_cvDone.wait(lock, [this]() { return m_jobDone; });
	}

	bool run(const std::function<bool()>& fn)
	{
		bool result = false;
		runVoid([&result, &fn]() { result = fn(); });
		return result;
	}

private:
	void threadMain()
	{
		const HRESULT hrCo = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool coOk = SUCCEEDED(hrCo);
		const bool mfOk = coOk && SUCCEEDED(mfApi().startup(MF_VERSION, MFSTARTUP_NOSOCKET));
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_initOk = mfOk;
			m_initDone = true;
		}
		m_cvDone.notify_all();

		if (mfOk)
		{
			loop();
			mfApi().shutdown();
		}
		if (coOk)
		{
			::CoUninitialize();
		}
	}

	void loop()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (;;)
		{
			m_cvWork.wait(lock, [this]() { return m_quit || m_job != nullptr; });
			if (!m_job)
			{
				return;
			}
			const std::function<void()> *job = m_job;
			lock.unlock();
			(*job)();
			lock.lock();
			m_job = nullptr;
			m_jobDone = true;
			m_cvDone.notify_all();
		}
	}

	std::thread m_thread;
	std::mutex m_mutex;
	std::condition_variable m_cvWork;
	std::condition_variable m_cvDone;
	const std::function<void()> *m_job = nullptr;
	bool m_jobDone = false;
	bool m_initDone = false;
	bool m_initOk = false;
	bool m_quit = false;
};

class MfVideoCodec final : public WZWebmVideoCodec
{
public:
	static std::unique_ptr<MfVideoCodec> create(const char *codecId, unsigned width, unsigned height, WzString *detail);

	~MfVideoCodec() override
	{
		m_thread.runVoid([this]() { closeOnThread(); });
		m_thread.stop();
	}

	const char* backendName() const override { return "mediafoundation"; }
	bool providesFramePts() const override { return true; }
	bool failed() const override { return m_failed; }

	bool decode(const uint8_t *data, size_t len, double pts) override
	{
		return m_thread.run([this, data, len, pts]() { return decodeOnThread(data, len, pts); });
	}

	bool getFrame(WZVideoFrameYUV& out) override
	{
		return m_thread.run([this, &out]() { return getFrameOnThread(out); });
	}

	void drain() override
	{
		m_thread.runVoid([this]() {
			m_drainRequested = true;
			issueDrainIfReady();
		});
	}

	void flush() override
	{
		m_thread.runVoid([this]() {
			releaseLockedOutput();
			m_pendingInput.clear();
			if (!m_transform || m_failed)
			{
				return;
			}
			m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
			m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
			m_drainRequested = false;
			m_draining = false;
			m_discontinuity = true;
		});
	}

	const WzString& mftName() const { return m_mftName; }

private:
	bool openOnThread(const GUID& inputSubtype, unsigned width, unsigned height);
	bool activateTransform(const GUID& inputSubtype, unsigned width, unsigned height);
	bool configureTransform(const GUID& inputSubtype, unsigned width, unsigned height);
	bool setInputType(const GUID& inputSubtype, unsigned width, unsigned height);
	bool setOutputType();
	bool refreshOutputLayout();
	bool flushPendingInput();
	void issueDrainIfReady();
	bool queueInput(const uint8_t *data, size_t len, double pts);
	bool decodeOnThread(const uint8_t *data, size_t len, double pts);
	bool getFrameOnThread(WZVideoFrameYUV& out);
	void releaseLockedOutput();
	void closeOnThread();

	MfThread m_thread;
	WZComPtr<IMFActivate> m_activate;		// null when the transform came from CoCreateInstance
	WZComPtr<IMFTransform> m_transform;
	WZComPtr<IMFSample> m_outputSample;
	bool m_mftAllocatesSamples = false;

	std::deque<WZComPtr<IMFSample>> m_pendingInput;
	std::vector<WZVideoFrameSpan> m_frameSpans;
	bool m_splitSuperframes = false;
	bool m_discontinuity = false;
	WZVp9ColorConfig m_color;			// from the last key frame

	WZComPtr<IMFSample> m_lockedSample;
	WZComPtr<IMFMediaBuffer> m_lockedBuffer;
	WZComPtr<IMF2DBuffer> m_locked2D;
	BYTE *m_lockedData = nullptr;
	bool m_locked = false;

	WZVideoPlaneFormat m_planeFormat = WZVideoPlaneFormat::NV12;
	unsigned m_requestedWidth = 0;
	unsigned m_requestedHeight = 0;
	unsigned m_displayWidth = 0;
	unsigned m_displayHeight = 0;
	unsigned m_codedWidth = 0;
	unsigned m_codedHeight = 0;
	LONG m_defaultStride = 0;
	bool m_drainRequested = false;
	bool m_draining = false;
	bool m_failed = false;
	bool m_errorLogged = false;
	WzString m_mftName;
};

bool MfVideoCodec::setInputType(const GUID& inputSubtype, unsigned width, unsigned height)
{
	WZComPtr<IMFMediaType> type;
	if (FAILED(mfApi().createMediaType(&type)))
	{
		return false;
	}
	if (FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))
	    || FAILED(type->SetGUID(MF_MT_SUBTYPE, inputSubtype)))
	{
		return false;
	}
	MFSetAttributeSize(type.get(), MF_MT_FRAME_SIZE, width, height);
	MFSetAttributeRatio(type.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	return SUCCEEDED(m_transform->SetInputType(0, type.get(), 0));
}

bool MfVideoCodec::setOutputType()
{
	static const struct { const GUID *subtype; WZVideoPlaneFormat format; } candidates[] = {
		{ &MFVideoFormat_NV12, WZVideoPlaneFormat::NV12 },
		{ &MFVideoFormat_IYUV, WZVideoPlaneFormat::I420 },
		{ &MFVideoFormat_I420, WZVideoPlaneFormat::I420 },
	};

	for (const auto& candidate : candidates)
	{
		for (DWORD i = 0; ; ++i)
		{
			WZComPtr<IMFMediaType> type;
			if (FAILED(m_transform->GetOutputAvailableType(0, i, &type)))
			{
				break;
			}
			GUID subtype = {};
			if (FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) || subtype != *candidate.subtype)
			{
				continue;
			}
			if (FAILED(m_transform->SetOutputType(0, type.get(), 0)))
			{
				continue;
			}
			m_planeFormat = candidate.format;
			return refreshOutputLayout();
		}
	}
	debug(LOG_VIDEO, "Media Foundation decoder offers no supported 8-bit YUV420 output format");
	return false;
}

bool MfVideoCodec::refreshOutputLayout()
{
	WZComPtr<IMFMediaType> type;
	if (FAILED(m_transform->GetOutputCurrentType(0, &type)))
	{
		return false;
	}

	UINT32 width = 0, height = 0;
	if (FAILED(MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &width, &height)) || width == 0 || height == 0
	    || width > kMaxDimension || height > kMaxDimension)
	{
		return false;
	}
	m_codedWidth = width;
	m_codedHeight = height;

	UINT32 stride = 0;
	m_defaultStride = (SUCCEEDED(type->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride)) && stride > 0)
	                  ? static_cast<LONG>(stride) : static_cast<LONG>(m_codedWidth);

	// a mid-stream resolution change may leave the surface smaller than the container's size
	m_displayWidth = std::min(m_requestedWidth, m_codedWidth);
	m_displayHeight = std::min(m_requestedHeight, m_codedHeight);

	MFT_OUTPUT_STREAM_INFO info;
	memset(&info, 0, sizeof(info));
	if (FAILED(m_transform->GetOutputStreamInfo(0, &info)))
	{
		return false;
	}
	m_mftAllocatesSamples = (info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) != 0;

	m_outputSample.reset();
	if (!m_mftAllocatesSamples)
	{
		WZComPtr<IMFMediaBuffer> buffer;
		if (info.cbSize == 0 || FAILED(mfApi().createAlignedMemoryBuffer(info.cbSize, MF_16_BYTE_ALIGNMENT, &buffer)))
		{
			return false;
		}
		if (FAILED(mfApi().createSample(&m_outputSample)) || FAILED(m_outputSample->AddBuffer(buffer.get())))
		{
			return false;
		}
	}
	return true;
}

bool MfVideoCodec::configureTransform(const GUID& inputSubtype, unsigned width, unsigned height)
{
	if (!setInputType(inputSubtype, width, height))
	{
		return false;
	}
	return setOutputType();
}

bool MfVideoCodec::activateTransform(const GUID& inputSubtype, unsigned width, unsigned height)
{
	MFT_REGISTER_TYPE_INFO inputInfo = { MFMediaType_Video, inputSubtype };
	IMFActivate **activates = nullptr;
	UINT32 count = 0;

	// SYNCMFT selects software transforms (hardware ones are asynchronous and need a D3D device manager)
	const UINT32 flags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER;
	if (SUCCEEDED(mfApi().enumEx(MFT_CATEGORY_VIDEO_DECODER, flags, &inputInfo, nullptr, &activates, &count)))
	{
		for (UINT32 i = 0; i < count; ++i)
		{
			WZComPtr<IMFActivate> activate;
			activate.attach(activates[i]);
			if (m_transform)
			{
				continue;
			}
			WZComPtr<IMFTransform> transform;
			if (FAILED(activate->ActivateObject(IID_PPV_ARGS(&transform))))
			{
				continue;
			}
			m_transform = std::move(transform);
			if (!configureTransform(inputSubtype, width, height))
			{
				m_transform.reset();
				activate->ShutdownObject();
				continue;
			}
			LPWSTR name = nullptr;
			UINT32 nameLen = 0;
			if (SUCCEEDED(activate->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute, &name, &nameLen)))
			{
				char utf8[256] = {0};
				WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8, sizeof(utf8) - 1, nullptr, nullptr);
				m_mftName = utf8;
				CoTaskMemFree(name);
			}
			m_activate = std::move(activate);
		}
		CoTaskMemFree(activates);
	}

	if (!m_transform)
	{
		WZComPtr<IMFTransform> transform;
		if (SUCCEEDED(CoCreateInstance(kCLSID_MSVPxDecoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&transform))))
		{
			m_transform = std::move(transform);
			if (configureTransform(inputSubtype, width, height))
			{
				m_mftName = "Microsoft VPx Decoder MFT";
			}
			else
			{
				m_transform.reset();
			}
		}
	}

	return m_transform.get() != nullptr;
}

bool MfVideoCodec::openOnThread(const GUID& inputSubtype, unsigned width, unsigned height)
{
	m_requestedWidth = width;
	m_requestedHeight = height;
	m_splitSuperframes = (inputSubtype == MFVideoFormat_VP90);
	if (!activateTransform(inputSubtype, width, height))
	{
		return false;
	}
	if (m_displayWidth == 0 || m_displayHeight == 0)
	{
		return false;
	}

	WZComPtr<IMFAttributes> attributes;
	if (SUCCEEDED(m_transform->GetAttributes(&attributes)))
	{
		attributes->SetUINT32(kMF_LOW_LATENCY, TRUE);
	}

	WZComPtr<ICodecAPI> codecApi;
	if (SUCCEEDED(m_transform->QueryInterface(IID_PPV_ARGS(&codecApi))))
	{
		VARIANT value;
		VariantInit(&value);
		V_VT(&value) = VT_UI4;
		V_UI4(&value) = wzWebmVideoCodecDecodeThreads();
		codecApi->SetValue(&CODECAPI_AVDecNumWorkerThreads, &value);
	}

	m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
	m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
	return true;
}

void MfVideoCodec::releaseLockedOutput()
{
	if (m_locked)
	{
		if (m_locked2D)
		{
			m_locked2D->Unlock2D();
		}
		else if (m_lockedBuffer)
		{
			m_lockedBuffer->Unlock();
		}
		m_locked = false;
	}
	m_lockedData = nullptr;
	m_locked2D.reset();
	m_lockedBuffer.reset();
	m_lockedSample.reset();
}

void MfVideoCodec::closeOnThread()
{
	releaseLockedOutput();
	m_pendingInput.clear();
	if (m_transform)
	{
		m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
	}
	m_outputSample.reset();
	m_transform.reset();
	if (m_activate)
	{
		m_activate->ShutdownObject();
		m_activate.reset();
	}
}

bool MfVideoCodec::flushPendingInput()
{
	while (!m_pendingInput.empty())
	{
		const HRESULT hr = m_transform->ProcessInput(0, m_pendingInput.front().get(), 0);
		if (hr == MF_E_NOTACCEPTING)
		{
			return false;
		}
		m_pendingInput.pop_front();
		if (FAILED(hr))
		{
			if (!m_errorLogged)
			{
				debug(LOG_ERROR, "IMFTransform::ProcessInput() failed: 0x%08lx", static_cast<unsigned long>(hr));
				m_errorLogged = true;
			}
			return false;
		}
	}
	return true;
}

// the drain goes in behind all queued input, or the tail of the stream is lost
void MfVideoCodec::issueDrainIfReady()
{
	if (!m_transform || m_failed || !m_drainRequested || m_draining || !flushPendingInput())
	{
		return;
	}
	m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
	m_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
	m_draining = true;
}

bool MfVideoCodec::queueInput(const uint8_t *data, size_t len, double pts)
{
	WZComPtr<IMFMediaBuffer> buffer;
	if (FAILED(mfApi().createMemoryBuffer(static_cast<DWORD>(len), &buffer)))
	{
		return false;
	}
	BYTE *dest = nullptr;
	DWORD maxLength = 0;
	if (FAILED(buffer->Lock(&dest, &maxLength, nullptr)))
	{
		return false;
	}
	memcpy(dest, data, len);
	buffer->Unlock();
	buffer->SetCurrentLength(static_cast<DWORD>(len));

	WZComPtr<IMFSample> sample;
	if (FAILED(mfApi().createSample(&sample)) || FAILED(sample->AddBuffer(buffer.get())))
	{
		return false;
	}
	sample->SetSampleTime(static_cast<LONGLONG>(pts * 10000000.0));
	if (m_splitSuperframes && wzVp9FrameIsKeyframe(data, len))
	{
		sample->SetUINT32(MFSampleExtension_CleanPoint, TRUE);
	}
	if (m_discontinuity)
	{
		sample->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
		m_discontinuity = false;
	}
	if (m_splitSuperframes)
	{
		wzVp9ReadColorConfig(data, len, m_color);
	}

	m_pendingInput.push_back(std::move(sample));
	return true;
}

bool MfVideoCodec::decodeOnThread(const uint8_t *data, size_t len, double pts)
{
	releaseLockedOutput();
	if (!m_transform || m_failed || len == 0)
	{
		return false;
	}

	// the transform takes one frame per sample
	if (m_splitSuperframes)
	{
		wzVp9SplitSuperframe(data, len, m_frameSpans);
	}
	else
	{
		m_frameSpans.assign(1, WZVideoFrameSpan{ data, len });
	}

	// only the visible frame of a superframe produces output, so they can share the block's timestamp
	for (const WZVideoFrameSpan& span : m_frameSpans)
	{
		if (!queueInput(span.data, span.len, pts))
		{
			return false;
		}
	}

	if (m_pendingInput.size() > 64)
	{
		debug(LOG_ERROR, "Media Foundation decoder is not consuming input");
		m_failed = true;
		return false;
	}
	flushPendingInput();
	return !m_failed;
}

bool MfVideoCodec::getFrameOnThread(WZVideoFrameYUV& out)
{
	releaseLockedOutput();
	if (!m_transform || m_failed)
	{
		return false;
	}
	issueDrainIfReady();

	MFT_OUTPUT_DATA_BUFFER outputBuffer;
	DWORD status = 0;
	HRESULT hr = S_OK;

	for (int attempt = 0; attempt < 2; ++attempt)
	{
		memset(&outputBuffer, 0, sizeof(outputBuffer));
		outputBuffer.pSample = m_mftAllocatesSamples ? nullptr : m_outputSample.get();

		hr = m_transform->ProcessOutput(0, 1, &outputBuffer, &status);

		if (outputBuffer.pEvents)
		{
			outputBuffer.pEvents->Release();
			outputBuffer.pEvents = nullptr;
		}
		if (hr != MF_E_TRANSFORM_STREAM_CHANGE)
		{
			break;
		}
		if (outputBuffer.pSample && m_mftAllocatesSamples)
		{
			outputBuffer.pSample->Release();
		}
		outputBuffer.pSample = nullptr;
		if (!setOutputType())
		{
			m_failed = true;
			return false;
		}
	}

	if (FAILED(hr))
	{
		if (outputBuffer.pSample && m_mftAllocatesSamples)
		{
			outputBuffer.pSample->Release();
		}
		// a repeated type change was renegotiated above
		if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && hr != MF_E_TRANSFORM_STREAM_CHANGE && !m_errorLogged)
		{
			debug(LOG_ERROR, "IMFTransform::ProcessOutput() failed: 0x%08lx", static_cast<unsigned long>(hr));
			m_errorLogged = true;
		}
		return false;
	}
	if (!outputBuffer.pSample || (outputBuffer.dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE))
	{
		// NO_SAMPLE leaves a caller-provided sample holding the previous frame
		if (outputBuffer.pSample && m_mftAllocatesSamples)
		{
			outputBuffer.pSample->Release();
		}
		return false;
	}

	if (m_mftAllocatesSamples)
	{
		m_lockedSample.attach(outputBuffer.pSample);
	}
	flushPendingInput();
	issueDrainIfReady();

	IMFSample *sample = outputBuffer.pSample;
	if (FAILED(sample->ConvertToContiguousBuffer(&m_lockedBuffer)))
	{
		releaseLockedOutput();
		return false;
	}

	// Lock2DSize also reports the buffer's extent, for the bounds check below
	LONG pitch = 0;
	BYTE *bufferStart = nullptr;
	DWORD bufferLength = 0;
	bool haveExtent = false;
	if (SUCCEEDED(m_lockedBuffer->QueryInterface(IID_PPV_ARGS(&m_locked2D))))
	{
		WZComPtr<IMF2DBuffer2> buffer2;
		if (SUCCEEDED(m_locked2D->QueryInterface(IID_PPV_ARGS(&buffer2)))
		    && SUCCEEDED(buffer2->Lock2DSize(MF2DBuffer_LockFlags_Read, &m_lockedData, &pitch, &bufferStart, &bufferLength)))
		{
			m_locked = true;
			haveExtent = true;
		}
		else if (SUCCEEDED(m_locked2D->Lock2D(&m_lockedData, &pitch)))
		{
			m_locked = true;
		}
		else
		{
			m_locked2D.reset();
		}
	}
	if (!m_locked)
	{
		DWORD maxLength = 0;
		if (FAILED(m_lockedBuffer->Lock(&m_lockedData, &maxLength, &bufferLength)))
		{
			m_lockedData = nullptr;
			releaseLockedOutput();
			return false;
		}
		m_locked = true;
		bufferStart = m_lockedData;
		haveExtent = true;
		pitch = m_defaultStride;
	}

	// NV12 and I420 alike: a pitch x height luma plane, then half that again of chroma
	const uint64_t planesSize = static_cast<uint64_t>(pitch > 0 ? pitch : 0) * m_codedHeight * 3 / 2;
	const bool outsideBuffer = haveExtent
		&& (m_lockedData < bufferStart || static_cast<uint64_t>(m_lockedData - bufferStart) + planesSize > bufferLength);
	if (!m_lockedData || pitch <= 0 || static_cast<unsigned>(pitch) < m_displayWidth || outsideBuffer)
	{
		if (!m_errorLogged)
		{
			debug(LOG_ERROR, "Media Foundation decoder returned an unusable buffer (pitch %ld, %lu bytes)",
			      static_cast<long>(pitch), static_cast<unsigned long>(bufferLength));
			m_errorLogged = true;
		}
		releaseLockedOutput();
		return false;
	}

	const uint8_t *base = m_lockedData;
	const uint8_t *chroma = base + static_cast<size_t>(pitch) * m_codedHeight;

	out.format = m_planeFormat;
	out.y = base;
	out.yStride = pitch;
	if (m_planeFormat == WZVideoPlaneFormat::NV12)
	{
		out.u = chroma;
		out.v = nullptr;
		out.uvStride = pitch;
	}
	else
	{
		out.u = chroma;
		out.v = chroma + static_cast<size_t>(pitch / 2) * (m_codedHeight / 2);
		out.uvStride = pitch / 2;
	}
	out.width = m_displayWidth;
	out.height = m_displayHeight;
	switch (m_color.colorSpace)
	{
	case 2:	// BT.709
		out.colorMatrix = WZVideoColorMatrix::BT709;
		break;
	case 1:	// BT.601
	case 3:	// SMPTE 170
		out.colorMatrix = WZVideoColorMatrix::BT601;
		break;
	default:	// unknown (VP8 has no color space field) or unsupported
		out.colorMatrix = containerColorMatrix;
		break;
	}
	out.fullRange = m_color.fullRange || (m_color.colorSpace == 0 && containerFullRange);

	LONGLONG sampleTime = 0;
	if (FAILED(sample->GetSampleTime(&sampleTime)))
	{
		releaseLockedOutput();
		return false;
	}
	out.pts = static_cast<double>(sampleTime) / 10000000.0;
	return true;
}

std::unique_ptr<MfVideoCodec> MfVideoCodec::create(const char *codecId, unsigned width, unsigned height, WzString *detail)
{
	GUID inputSubtype = {};
	if (strcmp(codecId, "V_VP9") == 0)
	{
		inputSubtype = MFVideoFormat_VP90;
	}
	else if (strcmp(codecId, "V_VP8") == 0)
	{
		inputSubtype = MFVideoFormat_VP80;
	}
	else
	{
		return nullptr;
	}

	if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension)
	{
		if (detail)
		{
			*detail = "unsupported video size";
		}
		return nullptr;
	}

	auto codec = std::unique_ptr<MfVideoCodec>(new MfVideoCodec());
	if (!codec->m_thread.start())
	{
		if (detail)
		{
			*detail = "Media Foundation is not available";
		}
		return nullptr;
	}
	if (!codec->m_thread.run([&codec, &inputSubtype, width, height]() { return codec->openOnThread(inputSubtype, width, height); }))
	{
		codec->m_thread.runVoid([&codec]() { codec->closeOnThread(); });
		if (detail)
		{
			*detail = "no usable Media Foundation decoder for this codec";
		}
		return nullptr;
	}
	if (detail)
	{
		*detail = codec->mftName();
	}
	return codec;
}

} // anonymous namespace

std::unique_ptr<WZWebmVideoCodec> wzCreateMediaFoundationVideoCodec(const char *codecId, unsigned width, unsigned height)
{
	return MfVideoCodec::create(codecId, width, height, nullptr);
}

bool wzMediaFoundationVideoCodecAvailable(WzString& detail)
{
	static WzString probeDetail;
	static const bool available = []() {
		WzString result;
		// the class can stay registered after its Store package is removed
		auto codec = MfVideoCodec::create("V_VP9", 640, 480, &result);
		probeDetail = result;
		return codec != nullptr;
	}();
	detail = probeDetail;
	return available;
}
