
////////////////////////////////////////////////////////////////////////////////
// Artifacts management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSetArtifacts(artifacts)
//;;
//;; Tell `libcampaign.js` to manage a certain set of artifacts.
//;; The argument is a JavaScript map from object labels to artifact description.
//;; If the label points to a game object, artifact will be placed when this object
//;; is destroyed; if the label is a position, the artifact will be placed instantly.
//;; The label can point to a pre-existing feature artifact on the map too.
//;; Artifact description is a JavaScript object with the following fields:
//;; * `tech` The technology to grant when the artifact is recovered.
//;;   Note that this can be made into an array to make artifacts give out
//;;   more than one technology, if desired.
//;;   On `let me win` cheat, all technologies stored in the artifacts
//;;   managed by this function are automatically granted.
//;;   Additionally, this function would call special event callbacks if they are defined
//;;   in your script, which should be named as follows, where LABEL is the artifact label:
//;; * `camArtifactPickup_LABEL` Called when the player picks up the artifact.
//;;
//;; @param {Object} artifacts
//;; @returns {void}
//;;
function camSetArtifacts(artifacts)
{
	__camArtifacts = artifacts;
	for (const alabel in __camArtifacts)
	{
		__camSetupArtifactData(alabel);
	}
}

//;; ## camAddArtifact(artiLabel, artiTech)
//;;
//;; Adds another artifact to be managed. Will override existing ones if the names match.
//;;
//;; @param {String} artiLabel
//;; @param {String|Array} artiTech
//;; @returns {void}
//;;
function camAddArtifact(artiLabel, artiTech)
{
	if (!camDef(artiLabel) || !camDef(artiTech))
	{
		camDebug("Attempt to add new artifact failed due to undefined name or tech parameter");
		return;
	}
	__camArtifacts[artiLabel] = { tech: artiTech };
	__camSetupArtifactData(artiLabel);
}

//;; ## camDeleteArtifact(artiLabel)
//;;
//;; Deletes the artifact from the list of managed artifacts.
//;;
//;; @param {String} artiLabel
//;; @returns {void}
//;;
function camDeleteArtifact(artiLabel, warnIfNotFound)
{
	if (!camDef(warnIfNotFound))
	{
		warnIfNotFound = true;
	}
	if (!camDef(artiLabel))
	{
		camDebug("Tried to delete undefined artifact label");
		return;
	}
	if (!(artiLabel in __camArtifacts))
	{
		if (warnIfNotFound)
		{
			camDebug("Artifact label doesn't exist in list of artifacts");
		}
		return;
	}
	if (__camArtifacts[artiLabel].placed)
	{
		const obj = getObject(__camGetArtifactLabel(artiLabel));
		if (camDef(obj) && obj !== null)
		{
			camSafeRemoveObject(obj, false);
		}
	}
	delete __camArtifacts[artiLabel];
}

//;; ## camAllArtifactsPickedUp()
//;;
//;; Returns `true` if all artifacts managed by `libcampaign.js` were picked up.
//;;
//;; @returns {boolean}
//;;
function camAllArtifactsPickedUp()
{
	// FIXME: O(n) lookup here
	return __camNumArtifacts === Object.keys(__camArtifacts).length;
}

//;; ## camGetArtifacts()
//;;
//;; Returns the labels of all existing artifacts.
//;;
//;; @returns {Object[]}
//;;
function camGetArtifacts()
{
	const camArti = [];
	for (const alabel in __camArtifacts)
	{
		const artifact = __camArtifacts[alabel];
		const __LIB_LABEL = __camGetArtifactLabel(alabel);
		const obj = getObject(alabel);
		//libcampaign managed artifact that was placed on the map (or map placed artifact).
		if (artifact.placed && getObject(__LIB_LABEL) !== null)
		{
			camArti.push(__LIB_LABEL);
		}
		//Label for artifacts that will drop after an object gets destroyed. Or is manually managed.
		//NOTE: Must check for ID since "alabel" could be a AREA/POSITION label.
		else if (obj !== null && camDef(obj.id))
		{
			camArti.push(alabel);
		}
	}
	return camArti;
}

//////////// privates

function __camSetupArtifactData(alabel)
{
	const ai = __camArtifacts[alabel];
	const pos = camMakePos(alabel);
	if (camDef(pos.id))
	{
		const obj = getObject(alabel);
		if (obj && obj.type === FEATURE && obj.stattype === ARTIFACT)
		{
			addLabel(obj, __camGetArtifactLabel(alabel));
			ai.placed = true; // this is an artifact feature on the map itself.
		}
		else
		{
			// will place when object with this id is destroyed
			ai.id = "" + pos.id;
			ai.placed = false;
		}
	}
	else
	{
		// received position or area, place immediately
		const acrate = addFeature(CAM_ARTIFACT_STAT, pos.x, pos.y);
		addLabel(acrate, __camGetArtifactLabel(alabel));
		ai.placed = true;
	}
}

function __camGetArtifactLabel(alabel)
{
	return alabel + "_artifact_label";
}

function __camGetArtifactKey(objlabel)
{
	return objlabel.replace("_artifact_label", "");
}

// called from eventDestroyed
function __camCheckPlaceArtifact(obj)
{
	// FIXME: O(n) lookup here
	const __ALABEL = getLabel(obj);
	if (!camDef(__ALABEL) || !__ALABEL)
	{
		return;
	}
	const ai = __camArtifacts[__ALABEL];
	if (!camDef(ai))
	{
		return;
	}
	if (ai.placed)
	{
		camDebug("Object to which artifact", __ALABEL, "is bound, has died twice");
		return;
	}
	if (ai.tech instanceof Array)
	{
		camTrace("Placing multi-tech granting artifact");
		for (let i = 0; i < ai.tech.length; ++i)
		{
			const __TECH_STRING = ai.tech[i];
			camTrace(i, ":", __TECH_STRING);
		}
	}
	else
	{
		camTrace("Placing", ai.tech);
	}
	const acrate = addFeature(CAM_ARTIFACT_STAT, obj.x, obj.y);
	addLabel(acrate, __camGetArtifactLabel(__ALABEL));
	ai.placed = true;
}

function __camPickupArtifact(artifact)
{
	if (artifact.stattype !== ARTIFACT)
	{
		camDebug("Not an artifact");
		return;
	}
	// FIXME: O(n) lookup here
	const __ALABEL = __camGetArtifactKey(getLabel(artifact));
	const ai = __camArtifacts[__ALABEL];
	if (!camDef(__ALABEL) || !__ALABEL || !camDef(ai))
	{
		camTrace("Artifact", artifact.id, "is not managed");
		return;
	}
	if (Object.hasOwn(ai, "pickedUp") && ai.pickedUp === true)
	{
		camTrace("Already picked up the artifact", __ALABEL);
		return;
	}
	ai.pickedUp = true;

	camTrace("Picked up", ai.tech);
	playSound(cam_sounds.artifactRecovered, artifact.x, artifact.y, artifact.z);
	// artifacts are not self-removing...
	camSafeRemoveObject(artifact);
	if (ai.tech instanceof Array)
	{
		for (let i = 0; i < ai.tech.length; ++i)
		{
			const __TECH_STRING = ai.tech[i];
			enableResearch(__TECH_STRING);
		}
	}
	else
	{
		enableResearch(ai.tech);
	}
	// bump counter before the callback, so that it was
	// actual during the callback
	++__camNumArtifacts;
	const callback = __camGlobalContext()["camArtifactPickup_" + __ALABEL];
	if (camDef(callback))
	{
		callback();
	}

	__camSetupConsoleForVictoryConditions();
}

function __camLetMeWinArtifacts()
{
	for (const alabel in __camArtifacts)
	{
		const ai = __camArtifacts[alabel];
		if (ai.placed)
		{
			const __LABEL = __camGetArtifactLabel(alabel);
			const artifact = getObject(__LABEL);
			if (!camDef(artifact) || !artifact)
			{
				continue;
			}
			__camPickupArtifact(artifact);
		}
		else
		{
			if (ai.tech instanceof Array)
			{
				for (let i = 0; i < ai.tech.length; ++i)
				{
					const __TECH_STRING = ai.tech[i];
					enableResearch(__TECH_STRING);
				}
			}
			else
			{
				enableResearch(ai.tech);
			}
		}
	}
}
