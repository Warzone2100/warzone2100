/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2019  Warzone 2100 Project

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

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "lib/framework/wzconfig.h"
#include "droiddef.h"

//storage
extern std::map<int, DROID_TEMPLATE *> droidTemplates[MAX_PLAYERS];

extern bool allowDesign;
extern bool includeRedundantDesigns;


bool initTemplates();

/// Take ownership of template given by pointer.
void addTemplate(int player, DROID_TEMPLATE *psTemplate);

/// Make a duplicate of template given by pointer and store it. Then return pointer to copy.
DROID_TEMPLATE *copyTemplate(int player, DROID_TEMPLATE *psTemplate);

void clearTemplates(int player);
bool shutdownTemplates();
bool storeTemplates();

bool loadDroidTemplates(const char *filename);

/// return whether a template is for an IDF droid
bool templateIsIDF(DROID_TEMPLATE *psTemplate);

/// Fills the list with Templates that can be manufactured in the Factory - based on size
void fillTemplateList(std::vector<DROID_TEMPLATE *> &pList, STRUCTURE *psFactory);

/* gets a template from its name - relies on the name being unique */
DROID_TEMPLATE *getTemplateFromTranslatedNameNoPlayer(char const *pName);

/*getTemplateFromMultiPlayerID gets template for unique ID  searching all lists */
DROID_TEMPLATE *getTemplateFromMultiPlayerID(UDWORD multiPlayerID);

/// Have we researched the components of this template?
bool researchedTemplate(const DROID_TEMPLATE *psCurr, int player, bool allowRedundant = false, bool verbose = false);

void listTemplates();

void saveTemplateCommon(WzConfig &ini, DROID_TEMPLATE *psCurr);
bool loadTemplateCommon(WzConfig &ini, DROID_TEMPLATE &outputTemplate);

#endif // TEMPLATE_H
