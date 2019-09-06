
////////////////////////////////////////////////////////////////////////////////
// Video playback related logic
////////////////////////////////////////////////////////////////////////////////

// Be aware that eventVideoDone will be triggered for each video
// and will be received by the mission script's eventVideoDone, should it exist.

//;; ## camPlayVideos(videos)
//;;
//;; If videos is an array, queue up all of them for immediate playing. This
//;; function will play one video sequence should one be provided. Also,
//;; should a sound file be in a string (pcvX.ogg)  __camEnqueueVideos() will recognize it
//;; as a sound to play before a video. Of which is only supported when passed as
//;; an array.
//;;
function camPlayVideos(videos)
{
	if (videos instanceof Array)
	{
		__camVideoSequences = videos;
		__camEnqueueVideos();
	}
	else
	{
		if (!camIsString(videos))
		{
			camDebug("Non string passed in to play a video. Skipping.");
			return;
		}
		hackAddMessage(videos, MISS_MSG, CAM_HUMAN_PLAYER, true);
	}
}

//////////// privates

// Play all stored videos and sounds one after the other.
// cam_eventVideoDone will call this function repeatedly.
function __camEnqueueVideos()
{
	if (__camVideoSequences.length === 0)
	{
		return; //Nothing to play
	}

	const SOUND_IDENTIFER = ".ogg";
	var what = __camVideoSequences[0];

	if (!camIsString(what))
	{
		camDebug("Non string found in __camVideoSequences. Stopping playback.");
		return;
	}

	// Check if this is a sound to play before some sequence.
	if (what.indexOf(SOUND_IDENTIFER) !== -1)
	{
		playSound(what);
		queue("__camEnqueueVideos", camSecondsToMilliseconds(3.2)); //more than enough for most sounds.
	}
	else
	{
		hackAddMessage(what, MISS_MSG, CAM_HUMAN_PLAYER, true);
	}

	__camVideoSequences.shift();
}
