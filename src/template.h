#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "droiddef.h"

//storage
extern DROID_TEMPLATE			*apsDroidTemplates[MAX_PLAYERS];

extern bool allowDesign;

bool initTemplates();
bool shutdownTemplates();
bool storeTemplates();

/** Initialise the template build and power points */
void initTemplatePoints(void);

bool loadDroidTemplates(const char *pDroidData, UDWORD bufferSize);

/// return whether a template is for an IDF droid
bool templateIsIDF(DROID_TEMPLATE *psTemplate);

/// Fills the list with Templates that can be manufactured in the Factory - based on size
void fillTemplateList(std::vector<DROID_TEMPLATE *> &pList, STRUCTURE *psFactory);

/* gets a template from its name - relies on the name being unique */
DROID_TEMPLATE *getTemplateFromUniqueName(const char *pName, unsigned int player);
/* gets a template from its name - relies on the name being unique */
DROID_TEMPLATE *getTemplateFromTranslatedNameNoPlayer(char const *pName);
/*getTemplateFromMultiPlayerID gets template for unique ID  searching all lists */
DROID_TEMPLATE *getTemplateFromMultiPlayerID(UDWORD multiPlayerID);

/*return the name to display for the interface - we don't know if this is
a string ID or something the user types in*/
const char* getTemplateName(const DROID_TEMPLATE *psTemplate);

/// Have we researched the components of this template?
bool researchedTemplate(DROID_TEMPLATE *psCurr, int player, bool allowRedundant = false);

#endif // TEMPLATE_H
