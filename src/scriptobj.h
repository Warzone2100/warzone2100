/*
 * ScriptObj.h
 *
 * Object access functions for the script library
 *
 */
#ifndef _scriptobj_h
#define _scriptobh_h


// id's for object variables
enum _objids
{
	OBJID_POSX,			// Position of a base object
	OBJID_POSY,
	OBJID_POSZ,
	OBJID_ID,			// id of a base object
	OBJID_PLAYER,		// player of a base object
	OBJID_TYPE,			// type of a base object
	OBJID_ORDER,		// current droid order
	OBJID_DROIDTYPE,	// what type of droid
	OBJID_CLUSTERID,	// which cluster the object is a member of
	OBJID_HEALTH,		// %age damage level
	OBJID_BODY,			// the body component
	OBJID_PROPULSION,	// the propulsion component
	OBJID_WEAPON,		// the weapon component
	OBJID_STRUCTSTAT,	// the stat of a structure
	OBJID_ORDERX,		// order coords.106
	OBJID_ORDERY,
};

// id's for group variables
enum _groupids
{
	GROUPID_POSX,		// average x of a group
	GROUPID_POSY,		// average y of a group
	GROUPID_MEMBERS,	// number of units in a group
	GROUPID_HEALTH,		// average health of a group
};

// Get values from a base object
extern BOOL scrBaseObjGet(UDWORD index);

// Set values from a base object
extern BOOL scrBaseObjSet(UDWORD index);

// convert a base object to a droid if it is the right type
extern BOOL scrObjToDroid(void);

// convert a base object to a structure if it is the right type
extern BOOL scrObjToStructure(void);

// convert a base object to a feature if it is the right type
extern BOOL scrObjToFeature(void);

// Get values from a group
extern BOOL scrGroupObjGet(UDWORD index);

// default value save routine
extern BOOL scrValDefSave(INTERP_TYPE type, UDWORD data, char *pBuffer, UDWORD *pSize);

// default value load routine
extern BOOL scrValDefLoad(SDWORD version, INTERP_TYPE type, char *pBuffer, UDWORD size, UDWORD *pData);


#endif

