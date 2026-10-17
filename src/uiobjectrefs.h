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
/** \file
 *  The UI's references to game objects: everything outside the simulation that keeps a pointer to a droid,
 *  structure or feature from one frame to the next
 *  (the camera's tracking target, the sensor view, the order form, the transporter screen,
 *  the object forms' lists and highlights, ...).
 *
 *  Freeing an object clears the references to it (uiObjectFreed, called by ~BASE_OBJECT,
 *  which every way of freeing an object goes through: the tick after a death,
 *  the bulk frees of a level change or an in-process restore).
 *  The UI code still checks ->died for the tick an object is dead but not yet freed.
 *  It is never left holding a freed one, ex. when several ticks run before a frame is drawn
 *  (catching up, a spectator's or a replay seek's fast-forward).
 *
 *  Each holder's module visits its references (the *VisitUiObjectRefs functions below),
 *  and the visitor says which to clear.
 *  Clearing one may do more than null it (the camera stops tracking, a form is refreshed or closed).
 */

#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

struct BASE_OBJECT;

/// Called with each reference held. Returns true to clear it.
using UiObjectRefVisitor = std::function<bool (const BASE_OBJECT *)>;

/// Visit a pointer reference: if it is set and the visitor says so, null it. Returns whether it was cleared.
template <typename T>
inline bool visitUiObjectRef(T *&ref, const UiObjectRefVisitor &visit)
{
	if (ref != nullptr && visit(ref))
	{
		ref = nullptr;
		return true;
	}
	return false;
}

void visitUiObjectRefs(const UiObjectRefVisitor &visit);

/// An object is being freed: clear every UI reference to it. (~BASE_OBJECT)
void uiObjectFreed(const BASE_OBJECT *psObj);

/// Turned off at the end of the shutdown: objects destroyed after that (static ones, at exit) have no UI left to tell,
/// and the holders themselves may already be destroyed
void setUiObjectRefsEnabled(bool enabled);

/// Test check: every UI reference is to a live object (one in an object list, or dead but not yet freed).
/// Asserts for each one that isn't, and returns how many weren't.
size_t checkUiObjectRefs();

/// What the viewer had selected, kept across an in-process restore (by id: the restore makes every object anew)
struct UiViewerState
{
	std::vector<std::pair<uint32_t, uint32_t>> selectedDroids;      ///< (id, player)
	std::vector<std::pair<uint32_t, uint32_t>> selectedStructures;  ///< (id, player)
};

/// Before an in-process restore replaces every object: note the selection,
/// and close what shows objects of the world being replaced.
/// The camera stays where the viewer left it: it isn't part of a snapshot.
UiViewerState uiBeforeObjectsReplaced();

/// After it: select again whatever of the selection exists in the restored world
/// (a camera that was tracking a selected droid then tracks it again),
/// refresh the interface, and have the terrain rebuilt from the restored map.
void uiAfterObjectsReplaced(const UiViewerState &state);

// The holders, by module (each in its own .cpp)
void hciVisitUiObjectRefs(const UiObjectRefVisitor &visit);         // hci.cpp: build placement, object forms
void intOrderVisitUiObjectRefs(const UiObjectRefVisitor &visit);    // intorder.cpp: the order form
void transporterVisitUiObjectRefs(const UiObjectRefVisitor &visit); // transporter.cpp, mission.cpp: transporter screen and timer
void warCamVisitUiObjectRefs(const UiObjectRefVisitor &visit);      // warcam.cpp: the tracking camera
void display3dVisitUiObjectRefs(const UiObjectRefVisitor &visit);   // display3d.cpp: the sensor view
void displayVisitUiObjectRefs(const UiObjectRefVisitor &visit);     // display.cpp: cursor evaluation
void selectionVisitUiObjectRefs(const UiObjectRefVisitor &visit);   // selection.cpp: "next unit" cycling
