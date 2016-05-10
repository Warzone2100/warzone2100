#ifndef TEMPLATE_H
#define TEMPLATE_H

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
bool researchedTemplate(DROID_TEMPLATE *psCurr, int player, bool allowRedundant = false, bool verbose = false);

void listTemplates();

#endif // TEMPLATE_H
