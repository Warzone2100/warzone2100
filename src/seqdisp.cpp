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
#include "3rdparty/physfs.hpp"

#include <vector>
#include <regex> // For splitting subtitles
#include <memory> // For shared_ptr

#define SUBTITLE_BOX_MIN 430
#define SUBTITLE_BOX_MAX 480

// NOTE: The original game never had a true fullscreen mode for FMVs on >640x480 screens.
// They would just use double sized videos, and move the text to that area.
// Since we *do* offer fullscreen FMVs, this isn't really needed anymore, depending
// on how we want to handle the text.
static int D_W2 = 0;	// Text width offset
static int D_H2 = 0;	// Text height offset

/**
 * @brief Represents a line of text that should be displayed over a sequence.
 */
class Line
{
public:
	std::string text;
	int x;
	int y;

	/** @brief Start time in seconds.*/
	double startTime;
	double endTime;
	
	/**
	 * @brief Is the line a subtitle?
	 * 
	 * Will affect how the text is printed.
	 */
	bool isSubtitles;
};

class Sequence
{
public:
	WzString pSeq; //name of the sequence to play
	WzString pAudio; //name of the wav to play

	bool loop; //loop this sequence

	/** The lines to be displayed over the sequence.*/
	std::vector<Line> lines;

	Sequence() : loop(false)
	{}

	Sequence(WzString seq, WzString audio, bool l) : pSeq(seq), pAudio(audio), loop(l)
	{}
};

static bool bAudioPlaying = false;
static bool bHoldSeqForAudio = false;
static bool bSeqSubtitles = true;
static bool bSeqPlaying = false;
static WzString aVideoName;

static std::vector<Sequence> seqList;

static SDWORD currentPlaySeq = -1;

// local rendered text cache
static std::vector<WzText> wzCachedSeqText;

enum VIDEO_RESOLUTION
{
	VIDEO_PRESELECTED_RESOLUTION,
	VIDEO_USER_CHOSEN_RESOLUTION,
};

static bool seq_StartFullScreenVideo(const WzString& videoName, const WzString& audioName, VIDEO_RESOLUTION resolution); // why here?

/**
 * Splits a string into parts using one or more delimiters.
 * 
 * @param input The string to be splitted.
 * @param delimiters A list of delimiters. There should be no space between each one and the next.
 *                   For example, ", |" represents three delimiters, namely ',', '|', and the empty space.
 * 
 * @return A vector of the parts.
 */
std::vector<std::string> split(const std::string& input, const std::string& delimiters)
{
    // [....] Matches one of the characters in the brackets
    // for instance, Ca[tr] will match Cat or Car.
    std::regex expr{ "[" + delimiters + "]" };

    std::sregex_token_iterator first{ input.begin(), input.end(), expr, -1 }, last;
    
	return std::vector<std::string>{ first, last };
}

/**
 * Parse srt time into seconds.
 * 
 * The \p time should be in the format "hh:mm:ss,iii" where iii are the milliseconds.
 * 
 * @param time A string that represents the time.
 * 
 * @return The time in seconds.
 */
double srtTimeToSeconds(std::string time)
{
	int hours;
    int minutes;
    int seconds;
    int milliseconds;

    std::vector<std::string> parts = split(time, ":,");
    
	hours = std::stoi(parts[0]);
    minutes = std::stoi(parts[1]);
    seconds = std::stoi(parts[2]);
    milliseconds = std::stoi(parts[3]);

    return hours * 3600 + minutes * 60 + seconds + (milliseconds / 1000.0);
}

/**
 * Reads characters from an input stream and places them into a string.
 * 
 * This function is cross-platform in the sense that it can handle both Windows
 * and linux line endings, no matter what platform you are reading the line from.
 * For example, if you are on linux, and tried to read a file that was
 * written on windows, std::getline() will return a string that includes \r in its
 * tail, instead of skipping it. You can use this function to avoid that issue.
 * 
 * @param input The stream to get data from.
 * @param str The string to put data into.
 * 
 * @return \p input
 */
std::istream& crossPlatformGetLine(std::istream& input, std::string& str)
{
    str.clear();

    std::istream::sentry se(input, true);
    std::streambuf* buf = input.rdbuf();

    while(true)
	{
        int c = buf->sbumpc();

        switch (c)
		{
		case '\r': // Windows
            if(buf->sgetc() == '\n')
                buf->sbumpc();
            return input;
        case '\n': // Linux
            return input;
        case std::streambuf::traits_type::eof():
            if(str.empty())
                input.setstate(std::ios::eofbit);
            return input;
        default:
            str += (char)c;
        }
    }
}

/**
 * Get the name of the subtitles file for a specific sequence.
 * 
 * The function will first try to find a subtitles file that match the current selected
 * language of the game. If it does not find it, it will look for an English one.
 * 
 * @param seqName The name of sequence (with or without an extension).
 * 
 * @return The name of the subtitles file or empty string if not found.
 */
std::string getSubtitleFile(std::string seqName)
{
	// remove extension
	seqName = seqName.substr(0, seqName.find_last_of('.'));

	std::string language = getLanguage();
	std::string translation_file = "sequenceaudio/" + language + "_" + seqName + ".srt";
	std::string en_file = "sequenceaudio/" + seqName + ".srt";

	if (PHYSFS_exists(translation_file.c_str()))
	{
		return translation_file;
	}
	else if (PHYSFS_exists(en_file.c_str()))
	{
		return en_file;
	}
	else
	{
		return "";
	}
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
	switch (war_GetFMVmode())
	{
	case FMV_1X:
		{
			// Native (1x)
			const int x = (screenWidth - 320) / 2;
			const int y = (screenHeight - 240) / 2;
			seq_SetDisplaySize(320, 240, x, y);
			break;
		}
	case FMV_2X:
		{
			// Double (2x)
			const int x = (screenWidth - 640) / 2;
			const int y = (screenHeight - 480) / 2;
			seq_SetDisplaySize(640, 480, x, y);
			break;
		}
	case FMV_FULLSCREEN:
		seq_SetDisplaySize(screenWidth, screenHeight, 0, 0);
		break;

	default:
		ASSERT(!"invalid FMV mode", "Invalid FMV mode: %u", (unsigned int)war_GetFMVmode());
		break;
	}
}

static bool seq_StartFullScreenVideo(const WzString& videoName, const WzString& audioName, VIDEO_RESOLUTION resolution)
{
	WzString aAudioName("sequenceaudio/" + audioName);

	bHoldSeqForAudio = false;
	aVideoName = WzString("sequences/" + videoName);

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

	if (!seq_Play(aVideoName.toUtf8().c_str()))
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
		// NOT controlled by sliders for now?
		static const float maxVolume = 1.f;

		bAudioPlaying = audio_PlayStream(aAudioName.toUtf8().c_str(), maxVolume, nullptr, nullptr) ? true : false;
		ASSERT(bAudioPlaying == true, "unable to initialise sound %s", aAudioName.toUtf8().c_str());
	}

	return true;
}

bool seq_UpdateFullScreenVideo(int *pbClear) // this function is called in the main loop, but with nullptr argument only
{
	int i;
	bool bMoreThanOneSequenceLine = false;
	bool stillPlaying;

	unsigned int subMin = SUBTITLE_BOX_MAX + D_H2;
	unsigned int subMax = SUBTITLE_BOX_MIN + D_H2;

	//get any text lines over bottom of the video
	double frameTime = seq_GetFrameTime();
	for (i = 0; i < seqList[currentPlaySeq].lines.size(); i++)
	{
		Line seqtext = seqList[currentPlaySeq].lines[i];

		if (seqtext.text[0] != '\0')
		{
			if (seqtext.isSubtitles)
			{
				if (((frameTime >= seqtext.startTime) && (frameTime <= seqtext.endTime)) ||
				    seqList[currentPlaySeq].loop) //if its a looped video always draw the text
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
				if (pbClear != nullptr) // what is the purpose of this?
				{
					*pbClear = CLEAR_BLACK;
				}
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

	for (i = 0; i < seqList[currentPlaySeq].lines.size(); i++)
	{
		Line currentText = seqList[currentPlaySeq].lines[i];

		if (currentText.text[0] != '\0')
		{
			if (((frameTime >= currentText.startTime) && (frameTime <= currentText.endTime)) ||
			    (seqList[currentPlaySeq].loop)) //if its a looped video always draw the text
			{
				if (i >= wzCachedSeqText.size())
				{
					wzCachedSeqText.resize(i + 1);
				}
				if (bMoreThanOneSequenceLine)
				{
					currentText.x = 20 + D_W2;
				}
				wzCachedSeqText[i].setText(currentText.text, font_scaled);
				// draw a grey outline
				wzCachedSeqText[i].render(currentText.x - 1, currentText.y - 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x - 1, currentText.y + 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x + 1, currentText.y - 1, WZCOL_GREY);
				wzCachedSeqText[i].render(currentText.x + 1, currentText.y + 1, WZCOL_GREY);
				// render the actual subtitle in white
				wzCachedSeqText[i].render(currentText.x, currentText.y, WZCOL_WHITE);
			}
		}
	}
	if (!stillPlaying || bHoldSeqForAudio)
	{
		if (bAudioPlaying)
		{
			if (seqList[currentPlaySeq].loop)
			{
				seq_Shutdown();

				if (!seq_Play(aVideoName.toUtf8().c_str()))
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

/**
 * @brief Add a line of text to specific seuquence.
 * 
 * The goal of this function is to make sure that the line is not larger than the buffer,
 * and if this is the case, then to break it into shorter lines.
 * 
 * If both \p xOffset and \p yOffset are zero, then the line will be added below the last line.
 */
void seq_AddLineForVideo(Sequence& sequence, const std::string& text, int xOffset, int yOffset, double startTime, double endTime, SEQ_TEXT_POSITIONING textJustification)
{
	int currentLength; // rename to current_pos or just pos
	std::string currentText = text;

	const unsigned int buffer_width = pie_GetVideoBufferWidth() - xOffset;
	currentLength = text.length();

	// Check the string is short enough to print.
	// If not, keep removing words until it fits.
	while (iV_GetTextWidth(currentText.c_str(), font_scaled) > buffer_width)
	{
		currentLength--;

		while ((text[currentLength] != ' ') && (currentLength > 0))
		{
			currentLength--;
		}
		currentText = text.substr(0, currentLength);
	}

	Line line;
	static int lastX, lastY;

	if (xOffset == 0 && yOffset == 0)
	{
		line.x = lastX;
		line.y = lastY + iV_GetTextLineSize(font_scaled);
	}
	else
	{
		line.x = xOffset + D_W2;
		line.y = yOffset + D_H2;
	}
	lastX = line.x;
	lastY = line.y;

	const int MIN_JUSTIFICATION = 40;
	const int justification = buffer_width - iV_GetTextWidth(currentText.c_str(), font_scaled);

	if (textJustification == SEQ_TEXT_CENTER && currentLength == text.length() && justification > MIN_JUSTIFICATION)
	{
		line.x += (justification / 2);
	}

	line.startTime = startTime;
	line.endTime = endTime;
	line.isSubtitles = textJustification;
	line.text = currentText;

	sequence.lines.push_back(line);

	// if currentLength is less then sourceLength, then there remains text to be displayed.
	if (currentLength < text.length())
	{
		if (textJustification == SEQ_TEXT_CENTER)
		{
			textJustification = SEQ_TEXT_POSITION;
		}

		seq_AddLineForVideo(sequence, text.substr(currentLength + 1), 0, 0, startTime, endTime, textJustification);
	}
}

/**
 * @brief An overload of seq_AddLineForVideo, that adds a line to the last sequence.
 */
void seq_AddLineForVideo(const std::string& text, int xOffset, int yOffset, double startTime, double endTime, SEQ_TEXT_POSITIONING textJustification)
{
	seq_AddLineForVideo(seqList.back(), text, xOffset, yOffset, startTime, endTime, textJustification);
}

/**
 * @brief Add subtitles or text to a sequence from an .srt file.
 * 
 * @param sequence The name of the sequence.
 * @param textFileName The name of the .srt file.
 * @param textJustification The justrification of the text
 * @return true If the text was added successfully to the sequence.
 * @return false If the text could not be added because \p textFileName is invalid or the file does not exist.
 */
static bool seq_AddTextFromFile(Sequence& sequence, const std::string& textFileName, SEQ_TEXT_POSITIONING textJustification)
{
	// NOTE: The original game never had a fullscreen mode for FMVs on >640x480 screens.
	// They would just use double sized videos, and move the text to that area.
	// We just use the full screen for text right now, instead of using offsets.
	// However, depending on reaction, we may use the old style again.
	D_H2 = 0;
	D_W2 = 0;

	std::shared_ptr<PhysFS::ifstream> infile = PhysFS::ifstream::make(textFileName);
	if(!infile)
	{
		return false;
	}

	int x, y;
	double startTime = 0.0, endTime = 0.0;
	std::vector<std::string> lines;
	std::string line;
    int turn = 0;

    while (crossPlatformGetLine(*infile, line))
	{
        if (!line.empty())
		{
            if (turn == 0)
			{
                if (line.find_first_not_of("0123456789") != std::string::npos)
				{
					debug(LOG_ERROR, "%s is not a valid .srt file", textFileName.c_str()); 
                    return false;
                }
                turn++;
                continue;
            }
            if (line.find("-->") != std::string::npos)
			{
                // Split at the " ". If the file is a valid .srt, then the result should be
				// three strings, namely the start time, "-->", and the end time.
                std::vector<std::string> srtTime = split(line, " ");

                startTime = srtTimeToSeconds(srtTime[0]);
                endTime = srtTimeToSeconds(srtTime[2]);
            }
            else
			{
				lines.push_back(line);
            }
            turn++;

        }
        else
		{
            turn = 0;

			int original_y;

			// First, determine where the coordinates of the line.
			// These values are kind of arbitrary. They just look right.
			// Second, add it to the sequence using AddLineForVideo(),
			// to make sure that it gets breaked correctly if it is too long.
			
			if(textJustification == SEQ_TEXT_POSITION)
			{
				original_y = 20;
			}
			else
			{
				original_y = 430;
			}

			for(int i = 0; i < lines.size(); ++i)
			{
				if (i == 0)
				{
					x = (double)pie_GetVideoBufferWidth() / 640. * (double)20;
					y = (double)pie_GetVideoBufferHeight() / 480. * (double)original_y;
				}
				else
				{				
					x = y = 0;
				}
            
				seq_AddLineForVideo(sequence, lines[i], x, y, startTime, endTime, textJustification);
			}

			lines.clear();
        }
    }

	return true;
}

void seq_ClearSeqList()
{
	currentPlaySeq = -1;
	seqList.clear();
}

/**
 * @brief Add a new sequence to the list of sequences to be played.
 * 
 * Additionally, the function will look for the subtitles of the sequence and add them to it.
 *  
 * @param pSeqName The name of the sequence to be played.
 * @param audioName The name of the audio file for the sequence.
 * @param pTextName The name of the .srt file that contains additional text to be displayed
 *                  besides to the subtitles. Could be empty.
 * @param bLoop Determine whether the sequence should be looped or not.
 */
void seq_AddSeqToList(const WzString &seqName, const WzString &audioName, const std::string& textFileName, bool loop)
{
	Sequence seq = Sequence(seqName, audioName, loop);

	if (!textFileName.empty())
	{
		// Ordinary text shouldn't be justified
		std::string textFile = "sequenceaudio/" + textFileName;
		seq_AddTextFromFile(seq, textFile, SEQ_TEXT_POSITION);
	}

	if (bSeqSubtitles)
	{
		std::string subFile = getSubtitleFile(seqName.toUtf8());
		if(!subFile.empty())
		{
			// Subtitles should be center justified
			seq_AddTextFromFile(seq, subFile, SEQ_TEXT_CENTER);
		}
	}

	seqList.push_back(seq);
}

bool seq_AnySeqLeft()
{
	int nextSeq = currentPlaySeq + 1;
	return (nextSeq != seqList.size());
}

void seq_StartNextFullScreenVideo()
{
	bool bPlayedOK;

	currentPlaySeq++;
	bPlayedOK = seq_StartFullScreenVideo(seqList[currentPlaySeq].pSeq, seqList[currentPlaySeq].pAudio, VIDEO_USER_CHOSEN_RESOLUTION);

	if (bPlayedOK == false)
	{
		// don't do the callback if we're playing the win/lose video
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
