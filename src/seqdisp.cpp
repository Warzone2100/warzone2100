/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/**
 * @file seqdisp.c
 *
 * Functions for the display of the Escape Sequences (FMV).
 *
 */

#include "lib/framework/frame.h"

#include <string.h>
#include <physfs.h>

#include "lib/framework/file.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/sequence/sequence.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"

#include "seqdisp.h"

#include "warzoneconfig.h"
#include "hci.h"//for font
#include "loop.h"
#include "design.h"
#include "wrappers.h"
#include "init.h" // For fileLoadBuffer
#include "urlrequest.h"
#include "urlhelpers.h"

/***************************************************************************/
/*
 *	local Definitions
 */
/***************************************************************************/
#define MAX_TEXT_OVERLAYS 32
#define MAX_SEQ_LIST	  6
#define SUBTITLE_BOX_MIN 430
#define SUBTITLE_BOX_MAX 480

// NOTE: The original game never had a true fullscreen mode for FMVs on >640x480 screens.
// They would just use double sized videos, and move the text to that area.
// Since we *do* offer fullscreen FMVs, this isn't really needed anymore, depending
// on how we want to handle the text.
static int D_W2 = 0;	// Text width offset
static int D_H2 = 0;	// Text height offset

struct SEQTEXT
{
	char pText[MAX_STR_LENGTH];
	UDWORD x;
	UDWORD y;
	double startTime;
	double endTime;
	bool	bSubtitle;
};

struct SEQLIST
{
	WzString         pSeq;					//name of the sequence to play
	WzString         pAudio;				//name of the wav to play
	bool		bSeqLoop;					//loop this sequence
	int             currentText;			// current number of text messages for this seq
	SEQTEXT		aText[MAX_TEXT_OVERLAYS];	//text data to display for this sequence

	SEQLIST() : bSeqLoop(false), currentText(0)
	{
		memset(aText, 0, sizeof(aText));
	}
	void reset()
	{
		bSeqLoop = false;
		currentText = 0;
		memset(aText, 0, sizeof(aText));
		pSeq.clear();
		pAudio.clear();
	}
};
/***************************************************************************/
/*
 *	local Variables
 */
/***************************************************************************/

static bool bAudioPlaying = false;
static bool bHoldSeqForAudio = false;
static bool bSeqSubtitles = true;
static bool bSeqPlaying = false;
static WzString currVideoName;
static WzString currAudioName;
static std::shared_ptr<VideoProvider> aVideoProvider;
static SEQLIST aSeqList[MAX_SEQ_LIST];
static SDWORD currentSeq = -1;
static SDWORD currentPlaySeq = -1;

// local rendered text cache
static std::vector<WzText> wzCachedSeqText;

/***************************************************************************/
/*
 *	local ProtoTypes
 */
/***************************************************************************/

enum VIDEO_RESOLUTION
{
	VIDEO_PRESELECTED_RESOLUTION,
	VIDEO_USER_CHOSEN_RESOLUTION,
};

static bool seq_StartFullScreenVideo(const WzString& videoName, const WzString& audioName, VIDEO_RESOLUTION resolution);

class OnDemandVideoDownloader
{
public:
	// Allows queueing requests for video data
	// Returns true if the request was queued (either with the current call or previously)
	// Returns false if an error with a prior request for this video data occurred, or the request cannot be satisfied
	bool requestVideoData(const WzString& videoName);

	// Obtains a pointer to the video data (if available), queues a download if not (unless an error has occurred previously trying to obtain it - see getVideoDataError)
	std::shared_ptr<const std::vector<char>> getVideoData(const WzString& videoName);

	uint32_t getVideoDataRequestProgress(const WzString& videoName);

	// Returns whether an error occurred trying to get the video data
	bool getVideoDataError(const WzString& videoName);

	// Clear all cached requests
	void clear();

	// Set base path for requests
	void setBaseURLPath(const WzString& basePath);

	bool hasBaseURLPath() const { return baseURLPath.has_value(); }

private:
	class RequestDetails
	{
	public:
		~RequestDetails()
		{
			if (status == RequestStatus::Pending && requestHandle)
			{
				urlRequestSetCancelFlag(requestHandle);
			}
		}
	public:
		enum class RequestStatus
		{
			Pending,
			Success,
			Failure
		};
		RequestStatus status = RequestStatus::Pending;
		std::shared_ptr<std::vector<char>> responseData;
		uint32_t progressPercentage = 0;
	private:
		AsyncURLRequestHandle requestHandle;
		friend bool OnDemandVideoDownloader::requestVideoData(const WzString& videoName);
	};
	std::unordered_map<WzString, std::shared_ptr<RequestDetails>> priorRequests;
	optional<WzString> baseURLPath;
};

static WzString urlEncodeVideoPathComponents(const WzString& partialPath)
{
	// Divide up partialPath into path components (by /)
	auto pathComponents = partialPath.split("/");
	// Then urlEncode each path component, and recombine
	WzString result;
	for (size_t i = 0; i < pathComponents.size(); ++i)
	{
		if (i > 0)
		{
			result.append("/");
		}
		result.append(WzString::fromUtf8(urlEncode(pathComponents[i].toUtf8().c_str())));
	}
	return result;
}

bool OnDemandVideoDownloader::requestVideoData(const WzString& videoName)
{
	auto it = priorRequests.find(videoName);
	if (it != priorRequests.end())
	{
		// already requested
		return it->second->status != RequestDetails::RequestStatus::Failure;
	}

	// need to request

	if (!baseURLPath.has_value())
	{
		return false;
	}

	auto requestDetails = std::make_shared<RequestDetails>();
	priorRequests[videoName] = requestDetails;

	URLDataRequest urlRequest;
	urlRequest.url = baseURLPath.value().toUtf8() + urlEncodeVideoPathComponents(videoName).toUtf8();
	urlRequest.onSuccess = [requestDetails](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) {
		std::string urlCopy = url;
		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode != 200)
		{
			wzAsyncExecOnMainThread([httpStatusCode]{
				debug(LOG_WARNING, "Query for video returned HTTP status code: %ld", httpStatusCode);
			});
		}

		if (!data || data->memory == nullptr || data->size == 0)
		{
			// Invalid data response
			wzAsyncExecOnMainThread([requestDetails, urlCopy]{
				debug(LOG_INFO, "Failed to load video: %s (no data)", urlCopy.c_str());
				requestDetails->status = RequestDetails::RequestStatus::Failure;
			});
			return;
		}

		auto memoryBuffer = std::make_shared<std::vector<char>>((char *)data->memory, ((char*)data->memory) + data->size);
		wzAsyncExecOnMainThread([requestDetails, memoryBuffer, urlCopy]{
			debug(LOG_INFO, "Loaded video: %s", urlCopy.c_str());
			requestDetails->status = RequestDetails::RequestStatus::Success;
			requestDetails->responseData = memoryBuffer;
		});
	};
	urlRequest.onFailure = [requestDetails](const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails) {
		std::string urlCopy = url;
		wzAsyncExecOnMainThread([requestDetails, urlCopy]{
			debug(LOG_INFO, "Failed to load video: %s", urlCopy.c_str());
			requestDetails->status = RequestDetails::RequestStatus::Failure;
		});
	};
	urlRequest.progressCallback = [requestDetails](const std::string& url, int64_t dltotal, int64_t dlnow) {
		std::string urlCopy = url;
		float progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
		wzAsyncExecOnMainThread([requestDetails, urlCopy, progress]{
			requestDetails->progressPercentage = std::min<uint32_t>(static_cast<uint32_t>(progress * 100.f), 100);
		});
	};
	urlRequest.maxDownloadSizeLimit = 200 * 1024 * 1024; // response should never be > 200 MB
	auto requestHandle = urlRequestData(urlRequest);
	if (!requestHandle)
	{
		debug(LOG_ERROR, "Failed to request data: %s", urlRequest.url.c_str());
		requestDetails->status = RequestDetails::RequestStatus::Failure;
		return false;
	}

	requestDetails->requestHandle = requestHandle;

	return true;
}

std::shared_ptr<const std::vector<char>> OnDemandVideoDownloader::getVideoData(const WzString& videoName)
{
	auto it = priorRequests.find(videoName);
	if (it != priorRequests.end())
	{
		// already requested
		if (it->second->status == RequestDetails::RequestStatus::Success)
		{
			return it->second->responseData;
		}
		else
		{
			return nullptr;
		}
	}

	requestVideoData(videoName);
	return nullptr;
}

uint32_t OnDemandVideoDownloader::getVideoDataRequestProgress(const WzString& videoName)
{
	auto it = priorRequests.find(videoName);
	if (it != priorRequests.end())
	{
		// already requested
		switch (it->second->status)
		{
			case RequestDetails::RequestStatus::Pending:
				return it->second->progressPercentage;
			case RequestDetails::RequestStatus::Success:
				return 100;
			default:
				return 0;
		}
	}
	return 0;
}

bool OnDemandVideoDownloader::getVideoDataError(const WzString& videoName)
{
	auto it = priorRequests.find(videoName);
	if (it != priorRequests.end())
	{
		return it->second->status == RequestDetails::RequestStatus::Failure;
	}
	return false;
}

void OnDemandVideoDownloader::clear()
{
	priorRequests.clear();
}

void OnDemandVideoDownloader::setBaseURLPath(const WzString& basePath)
{
	baseURLPath = basePath;
}

static OnDemandVideoDownloader onDemandVideoProvider;

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

bool seq_hasVideos()
{
	return PHYSFS_exists("sequences/devastation.ogg") || onDemandVideoProvider.hasBaseURLPath();
}

void seq_setOnDemandVideoURL(const WzString& videoBaseURL)
{
	onDemandVideoProvider.setBaseURLPath(videoBaseURL);
}

/* Renders a video sequence specified by filename to a buffer*/
bool seq_RenderVideoToBuffer(const WzString &sequenceName, int seqCommand)
{
	static enum
	{
		VIDEO_NOT_PLAYING,
		VIDEO_PLAYING,
		VIDEO_FINISHED,
	} videoPlaying = VIDEO_NOT_PLAYING;
	static enum
	{
		VIDEO_LOOP,
		VIDEO_HOLD_LAST_FRAME,
	} frameHold = VIDEO_LOOP;

	if (seqCommand == SEQUENCE_KILL)
	{
		//stop the movie
		seq_Shutdown();
		bSeqPlaying = false;
		frameHold = VIDEO_LOOP;
		videoPlaying = VIDEO_NOT_PLAYING;
		return true;
	}

	if (!bSeqPlaying && frameHold == VIDEO_LOOP)
	{
		//start the ball rolling

		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		/* We do *NOT* want to use the user-choosen resolution when we
		 * are doing intelligence videos.
		 */
		videoPlaying = seq_StartFullScreenVideo(sequenceName, WzString(), VIDEO_PRESELECTED_RESOLUTION) ? VIDEO_PLAYING : VIDEO_FINISHED;
		bSeqPlaying = true;
	}

	if (videoPlaying != VIDEO_FINISHED)
	{
		videoPlaying = seq_Update() ? VIDEO_PLAYING : VIDEO_FINISHED;
	}

	if (videoPlaying == VIDEO_FINISHED)
	{
		seq_Shutdown();
		bSeqPlaying = false;
		frameHold = VIDEO_HOLD_LAST_FRAME;
		videoPlaying = VIDEO_NOT_PLAYING;
		return false;
	}

	return true;
}


static void seq_SetUserResolution()
{
	glm::uvec2 video_size;

	switch (war_GetFMVmode())
	{
	case FMV_1X:
		// Native (1x)
		video_size = {320, 240};
		break;

	case FMV_2X:
		// Double (2x)
		video_size = {640, 480};
		break;

	case FMV_FULLSCREEN:
	{
		const auto scaled_width = std::min(screenWidth / 4.0f, screenHeight / 3.0f) * 4.0f;
		video_size = {static_cast<int>(scaled_width), static_cast<int>(scaled_width * 3 / 4.0f)};
		break;
	}

	default:
		ASSERT(!"invalid FMV mode", "Invalid FMV mode: %u", (unsigned int)war_GetFMVmode());
		return;
	}

	const int x = (screenWidth - video_size.x) / 2;
	const int y = (screenHeight - video_size.y) / 2;
	seq_SetDisplaySize(video_size.x, video_size.y, x, y);
}

static bool seqPlayOrQueueFetch(const WzString& videoName, const WzString& audioName)
{
	aVideoProvider.reset();
	currVideoName = videoName;
	currAudioName = audioName;

	// Try to find local sequences
	WzString aVideoName = WzString("sequences/" + videoName);

	PHYSFS_file *fpInfile = PHYSFS_openRead(aVideoName.toUtf8().c_str());
	if (fpInfile == nullptr)
	{
		wz_info("unable to open '%s' for playback", aVideoName.toUtf8().c_str());
		fpInfile = PHYSFS_openRead("novideo.ogg");
	}

	if (fpInfile != nullptr)
	{
		aVideoProvider = makeVideoProvider(fpInfile, videoName);
	}
	else
	{
		// Try to download video from on-demand provider
		auto videoData = onDemandVideoProvider.getVideoData(videoName);
		if (!videoData)
		{
			if (onDemandVideoProvider.getVideoDataError(videoName))
			{
				seq_Shutdown();
				return false;
			}
			// Otherwise, request is queued
			// Return true and handle waiting inside seq_UpdateFullScreenVideo
			// (Each later call to seq_UpdateFullScreenVideo will check if still waiting, and then dispkay the download progress before itself kicking off seq_Play once the buffer is ready)
			return true;
		}

		aVideoProvider = makeVideoProvider(videoData, videoName);
	}

	if (!seq_Play(aVideoProvider))
	{
		seq_Shutdown();
		return false;
	}

	if (audioName.isEmpty())
	{
		bAudioPlaying = false;
	}
	else
	{
		ASSERT(audioName.isEmpty(), "Unexpected audio stream specified: %s (for video: %s)", audioName.toUtf8().c_str(), videoName.toUtf8().c_str());

		// NOT controlled by sliders for now?
		static const float maxVolume = 1.f;

		WzString aAudioName("sequenceaudio/" + audioName);
		bAudioPlaying = audio_PlayStream(aAudioName.toUtf8().c_str(), maxVolume, nullptr, nullptr) ? true : false;
		ASSERT(bAudioPlaying == true, "unable to initialise sound %s", aAudioName.toUtf8().c_str());
	}

	return true;
}

static bool seqPlayOrQueueFetch_Retry()
{
	return seqPlayOrQueueFetch(currVideoName, currAudioName);
}

//full screenvideo functions
// "audioName" doesn't appear to be used? (seems to always be set to empty string?) // past-due (2022)
static bool seq_StartFullScreenVideo(const WzString& videoName, const WzString& audioName, VIDEO_RESOLUTION resolution)
{
	bHoldSeqForAudio = false;
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	/* We do not want to enter loop_SetVideoPlaybackMode() when we are
	 * doing intelligence videos.
	 */
	if (resolution == VIDEO_USER_CHOSEN_RESOLUTION)
	{
		//start video mode
		if (loop_GetVideoMode() == 0)
		{
			// check to see if we need to pause, and set font each time
			cdAudio_Pause();
			loop_SetVideoPlaybackMode();
			iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		}

		// set the dimensions to show full screen or native or ...
		seq_SetUserResolution();
	}

	return seqPlayOrQueueFetch(videoName, audioName);
}

void update_user_resolution()
{
	static glm::uvec2 previousScreenResolution = {0, 0};
	if (previousScreenResolution.x != screenWidth || previousScreenResolution.y != screenHeight)
	{
		seq_SetUserResolution();
		previousScreenResolution = {screenWidth, screenHeight};
	}
}

bool seq_UpdateFullScreenVideo()
{
	update_user_resolution();

	if (!aVideoProvider)
	{
		// request has been queued
		// try again and see if it's available
		if (!seqPlayOrQueueFetch_Retry())
		{
			return false;
		}

		if (!aVideoProvider)
		{
			// still don't have the video data yet, but it's being fetched
			// display loading message
			if (wzCachedSeqText.size() < 2)
			{
				wzCachedSeqText.resize(2);
			}
			wzCachedSeqText[0].setText(WzString::fromUtf8(astringf("%s (%" PRIu32 "%%)...", _("Loading video"), onDemandVideoProvider.getVideoDataRequestProgress(currVideoName))), font_scaled);
			wzCachedSeqText[0].render((pie_GetVideoBufferWidth() - wzCachedSeqText[0].width()) / 2, (pie_GetVideoBufferHeight() - wzCachedSeqText[0].height()) / 2, WZCOL_WHITE);
			return true;
		}

		// otherwise, we have the data - continue
	}

	int i;
	bool bMoreThanOneSequenceLine = false;
	bool stillPlaying;

	unsigned int subMin = SUBTITLE_BOX_MAX + D_H2;
	unsigned int subMax = SUBTITLE_BOX_MIN + D_H2;

	//get any text lines over bottom of the video
	double frameTime = seq_GetFrameTime();
	for (i = 0; i < MAX_TEXT_OVERLAYS; i++)
	{
		SEQTEXT seqtext = aSeqList[currentPlaySeq].aText[i];
		if (seqtext.pText[0] != '\0')
		{
			if (seqtext.bSubtitle)
			{
				if (((frameTime >= seqtext.startTime) && (frameTime <= seqtext.endTime)) ||
				    aSeqList[currentPlaySeq].bSeqLoop) //if its a looped video always draw the text
				{
					if (subMin > seqtext.y && seqtext.y > SUBTITLE_BOX_MIN)
					{
						subMin = seqtext.y;
					}
					if (subMax < seqtext.y)
					{
						subMax = seqtext.y;
					}
				}
			}

			if (frameTime >= seqtext.endTime && frameTime < seqtext.endTime)
			{
//				if (pbClear != nullptr)
//				{
//					*pbClear = CLEAR_BLACK;
//				}
			}
		}
	}

	subMin -= D_H2;//adjust video window here because text is already ofset for big screens
	subMax -= D_H2;

	if (subMin < SUBTITLE_BOX_MIN)
	{
		subMin = SUBTITLE_BOX_MIN;
	}
	if (subMax > SUBTITLE_BOX_MAX)
	{
		subMax = SUBTITLE_BOX_MAX;
	}

	if (subMax > subMin)
	{
		bMoreThanOneSequenceLine = true;
	}

	//call sequence player to download last frame
	stillPlaying = seq_Update();
	//print any text over the video
	frameTime = seq_GetFrameTime();

	for (i = 0; i < MAX_TEXT_OVERLAYS; i++)
	{
		SEQTEXT currentText = aSeqList[currentPlaySeq].aText[i];
		if (currentText.pText[0] != '\0')
		{
			if (((frameTime >= currentText.startTime) && (frameTime <= currentText.endTime)) ||
			    (aSeqList[currentPlaySeq].bSeqLoop)) //if its a looped video always draw the text
			{
				if (i >= wzCachedSeqText.size())
				{
					wzCachedSeqText.resize(i + 1);
				}
				if (bMoreThanOneSequenceLine)
				{
					currentText.x = 20 + D_W2;
				}
				wzCachedSeqText[i].setText(&(currentText.pText[0]), font_scaled);
				wzCachedSeqText[i].render(currentText.x - 1, currentText.y - 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x - 1, currentText.y + 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x + 1, currentText.y - 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x + 1, currentText.y + 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x, currentText.y, WZCOL_WHITE);
			}
		}
	}
	if (!stillPlaying || bHoldSeqForAudio)
	{
		if (bAudioPlaying)
		{
			if (aSeqList[currentPlaySeq].bSeqLoop)
			{
				seq_Shutdown();

				if (!seq_Play(aVideoProvider))
				{
					bHoldSeqForAudio = true;
				}
			}
			else
			{
				bHoldSeqForAudio = true;
			}
			return true;//should hold the video
		}
		else
		{
			return false;//should terminate the video
		}
	}

	return true;
}

void seqReleaseAll()
{
	seq_Shutdown();
	wzCachedSeqText.clear();
}

bool seq_StopFullScreenVideo()
{
	if (!seq_AnySeqLeft())
	{
		loop_ClearVideoPlaybackMode();
	}

	seq_Shutdown();

	wzCachedSeqText.clear();

	return true;
}

// add a string at x,y or add string below last line if x and y are 0
bool seq_AddTextForVideo(const char *pText, SDWORD xOffset, SDWORD yOffset, double startTime, double endTime, SEQ_TEXT_POSITIONING textJustification)
{
	SDWORD sourceLength, currentLength;
	char *currentText;
	static SDWORD lastX;
	// make sure we take xOffset into account, we don't always start at 0
	const unsigned int buffer_width = pie_GetVideoBufferWidth() - xOffset;

	ASSERT_OR_RETURN(false, aSeqList[currentSeq].currentText < MAX_TEXT_OVERLAYS, "too many text lines");

	sourceLength = strlen(pText);
	currentLength = sourceLength;
	currentText = &(aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].pText[0]);

	//if the string is bigger than the buffer get the last end of the last fullword in the buffer
	if (currentLength >= MAX_STR_LENGTH)
	{
		currentLength = MAX_STR_LENGTH - 1;
		//get end of the last word
		while ((pText[currentLength] != ' ') && (currentLength > 0))
		{
			currentLength--;
		}
	}

	memcpy(currentText, pText, currentLength);
	currentText[currentLength] = 0;//terminate the string what ever

	//check the string is shortenough to print
	//if not take a word of the end and try again
	while ((currentLength > 0) && iV_GetTextWidth(currentText, font_scaled) > buffer_width)
	{
		currentLength--;
		while ((pText[currentLength] != ' ') && (currentLength > 0))
		{
			currentLength--;
		}
		currentText[currentLength] = 0;//terminate the string what ever
	}
	currentText[currentLength] = 0;//terminate the string what ever

	//check if x and y are 0 and put text on next line
	if (((xOffset == 0) && (yOffset == 0)) && (currentLength > 0))
	{
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x = lastX;
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].y =
		    aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText - 1].y + iV_GetTextLineSize(font_scaled);
	}
	else
	{
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x = xOffset + D_W2;
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].y = yOffset + D_H2;
	}
	lastX = aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x;

	const int MIN_JUSTIFICATION = 40;
	const int justification = buffer_width - iV_GetTextWidth(currentText, font_scaled);
	if (textJustification == SEQ_TEXT_JUSTIFY && currentLength == sourceLength && justification > MIN_JUSTIFICATION)
	{
		aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].x += (justification / 2);
	}

	//set start and finish times for the objects
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].startTime = startTime;
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].endTime = endTime;
	aSeqList[currentSeq].aText[aSeqList[currentSeq].currentText].bSubtitle = textJustification;

	aSeqList[currentSeq].currentText++;
	if (aSeqList[currentSeq].currentText >= MAX_TEXT_OVERLAYS)
	{
		aSeqList[currentSeq].currentText = 0;
	}

	//check text is okay on the screen
	if (currentLength < sourceLength)
	{
		//RECURSE x= 0 y = 0 for nextLine
		if (textJustification == SEQ_TEXT_JUSTIFY)
		{
			textJustification = SEQ_TEXT_POSITION;
		}
		seq_AddTextForVideo(&pText[currentLength + 1], 0, 0, startTime, endTime, textJustification);
	}
	return true;
}


static bool seq_AddTextFromFile(const char *pTextName, SEQ_TEXT_POSITIONING textJustification)
{
	char aTextName[MAX_STR_LENGTH];
	char *pTextBuffer, *pCurrentLine, *pText;
	UDWORD fileSize;
	SDWORD xOffset, yOffset;
	double startTime, endTime;
	const char *seps = "\n";

	// NOTE: The original game never had a fullscreen mode for FMVs on >640x480 screens.
	// They would just use double sized videos, and move the text to that area.
	// We just use the full screen for text right now, instead of using offsets.
	// However, depending on reaction, we may use the old style again.
	D_H2 = 0;				//( pie_GetVideoBufferHeight()- 480)/2;
	D_W2 = 0;				//( pie_GetVideoBufferWidth() - 640)/2;
	ssprintf(aTextName, "sequenceaudio/%s", pTextName);

	if (loadFileToBufferNoError(aTextName, fileLoadBuffer, FILE_LOAD_BUFFER_SIZE, &fileSize) == false)  //Did I mention this is lame? -Q
	{
		return false;
	}

	pTextBuffer = fileLoadBuffer;
	pCurrentLine = strtok(pTextBuffer, seps);
	while (pCurrentLine != nullptr)
	{
		if (*pCurrentLine != '/')
		{
			if (sscanf(pCurrentLine, "%d %d %lf %lf", &xOffset, &yOffset, &startTime, &endTime) == 4)
			{
				// Since all the positioning was hardcoded to specific values, we now calculate the
				// ratio of our screen, compared to what the game expects and multiply that to x, y.
				// This makes the text always take up the full screen, instead of original style.
				xOffset = static_cast<SDWORD>((double)pie_GetVideoBufferWidth() / 640. * (double)xOffset);
				yOffset = static_cast<SDWORD>((double)pie_GetVideoBufferHeight() / 480. * (double)yOffset);
				//get the text
				pText = strrchr(pCurrentLine, '"');
				ASSERT(pText != nullptr, "error parsing text file");
				if (pText != nullptr)
				{
					*pText = (UBYTE)0;
				}
				pText = strchr(pCurrentLine, '"');
				ASSERT(pText != nullptr, "error parsing text file");
				if (pText != nullptr)
				{
					seq_AddTextForVideo(_(&pText[1]), xOffset, yOffset, startTime, endTime, textJustification);
				}
			}
		}
		//get next line
		pCurrentLine = strtok(nullptr, seps);
	}
	return true;
}

//clear the sequence list
void seq_ClearSeqList()
{
	currentSeq = -1;
	currentPlaySeq = -1;
	for (int i = 0; i < MAX_SEQ_LIST; ++i)
	{
		aSeqList[i].reset();
	}
	onDemandVideoProvider.clear();
}

//add a sequence to the list to be played
void seq_AddSeqToList(const WzString &pSeqName, const WzString &audioName, const char *pTextName, bool bLoop)
{
	currentSeq++;

	ASSERT_OR_RETURN(, currentSeq < MAX_SEQ_LIST, "too many sequences");

	onDemandVideoProvider.requestVideoData(pSeqName);

	//OK so add it to the list
	aSeqList[currentSeq].pSeq = pSeqName;
	aSeqList[currentSeq].pAudio = audioName;
	aSeqList[currentSeq].bSeqLoop = bLoop;
	if (pTextName != nullptr)
	{
		// Ordinary text shouldn't be justified
		seq_AddTextFromFile(pTextName, SEQ_TEXT_POSITION);
	}

	if (bSeqSubtitles)
	{
		char aSubtitleName[MAX_STR_LENGTH];
		sstrcpy(aSubtitleName, pSeqName.toUtf8().c_str());

		// check for a subtitle file
		char *extension = strrchr(aSubtitleName, '.');
		if (extension)
		{
			*extension = '\0';
		}
		sstrcat(aSubtitleName, ".txt");

		// Subtitles should be center justified
		seq_AddTextFromFile(aSubtitleName, SEQ_TEXT_JUSTIFY);
	}
}

/*checks to see if there are any sequences left in the list to play*/
bool seq_AnySeqLeft()
{
	int nextSeq = currentPlaySeq + 1;

	//check haven't reached end
	if (nextSeq >= MAX_SEQ_LIST)
	{
		return false;
	}
	return !aSeqList[nextSeq].pSeq.isEmpty();
}

void seq_StartNextFullScreenVideo()
{
	bool	bPlayedOK;

	currentPlaySeq++;
	if (currentPlaySeq >= MAX_SEQ_LIST)
	{
		bPlayedOK = false;
	}
	else
	{
		bPlayedOK = seq_StartFullScreenVideo(aSeqList[currentPlaySeq].pSeq, aSeqList[currentPlaySeq].pAudio, VIDEO_USER_CHOSEN_RESOLUTION);
	}

	if (bPlayedOK == false)
	{
		//don't do the callback if we're playing the win/lose video
		if (getScriptWinLoseVideo())
		{
			displayGameOver(getScriptWinLoseVideo() == PLAY_WIN, false);
		}
	}
}


void seq_SetSubtitles(bool bNewState)
{
	bSeqSubtitles = bNewState;
}

bool seq_GetSubtitles()
{
	return bSeqSubtitles;
}
