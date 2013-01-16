/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#include "hBox.h"

static hBoxVtbl vtbl;

const classInfo hBoxClassInfo =
{
	&tableClassInfo,
	"hBox"
};

hBox *hBoxCreate(const char *id)
{
	hBox *instance = malloc(sizeof(hBox));

	if (instance == NULL)
	{
		return NULL;
	}

	// Call the constructor
	hBoxInit(instance, id);

	// Return the new object
	return instance;
}

static void hBoxInitVtbl(hBox *self)
{
	static bool initialised = false;

	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.tableVtbl = *(TABLE(self)->vtbl);

		// Overload widget's destroy and addChild methods
		vtbl.tableVtbl.widgetVtbl.destroy     = hBoxDestroyImpl;
		vtbl.tableVtbl.widgetVtbl.addChild    = hBoxAddChildImpl;

		initialised = true;
	}

	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.tableVtbl.widgetVtbl;

	// Set our vtable
	self->vtbl = &vtbl;
}

void hBoxInit(hBox *self, const char *id)
{
	// Init our parent
	tableInit((table *) self, id);

	// Prepare our vtable
	hBoxInitVtbl(self);

	// Set our type
	WIDGET(self)->classInfo = &hBoxClassInfo;
}

void hBoxDestroyImpl(widget *self)
{
	// Call our parents destructor
	tableDestroyImpl(self);
}

void hBoxSetVAlign(hBox *self, vAlign v)
{
	// Set the new alignment scheme
	tableSetDefaultAlign(TABLE(self), LEFT, v);
}

bool hBoxSetPadding(hBox *self, int padding)
{
	return tableSetPadding(TABLE(self), padding, 0);
}

bool hBoxAddChildImpl(widget *self, widget *child)
{
	// We want to add the child to the next free column
	const int column = tableGetColumnCount(TABLE(self)) + 1;

	// Delegate to tableAddChild with row = 1
	return tableAddChild(TABLE(self), child, 1, column);
}
