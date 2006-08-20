/*
 * ScriptCB.h
 *
 * functions to deal with parameterised script callback triggers.
 *
 */
#ifndef _scriptcb_h
#define _scriptcb_h

//console callback stuff
//---------------------------
#define MAXSTRLEN 255
extern SDWORD ConsolePlayer;
extern SDWORD MultiMsgPlayerTo;
extern SDWORD MultiMsgPlayerFrom;
extern SDWORD beaconX;
extern SDWORD beaconY;
extern char ConsoleMsg[MAXSTRLEN];	//Last console message
extern char MultiplayMsg[MAXSTRLEN];	//Last multiplayer message
extern STRUCTURE	*psScrCBNewStruct;		//for scrCBStructBuilt callback
extern DROID		*psScrCBNewStructTruck;	//for scrCBStructBuilt callback


// The pointer to the droid that was just built for a CALL_NEWDROID
extern DROID		*psScrCBDroidTaken;
extern DROID		*psScrCBNewDroid;
extern STRUCTURE	*psScrCBNewDroidFact;

// deal with unit takover(2)
extern BOOL scrCBDroidTaken(void);

// Deal with a CALL_NEWDROID
extern BOOL scrCBNewDroid(void);

// the attacker and target for a CALL_ATTACKED
extern BASE_OBJECT	*psScrCBAttacker, *psScrCBTarget;

// Deal with a CALL_STRUCT_ATTACKED
extern BOOL scrCBStructAttacked(void);

// Deal with a CALL_DROID_ATTACKED
extern BOOL scrCBDroidAttacked(void);

// Deal with a CALL_ATTACKED
extern BOOL scrCBAttacked(void);

// deal with CALL_BUTTON_PRESSED
extern BOOL scrCBButtonPressed(void);

// the Droid that was selected for a CALL_DROID_SELECTED
extern DROID	*psCBSelectedDroid;

// deal with CALL_DROID_SELECTED
extern BOOL scrCBDroidSelected(void);

// the object that was last killed for a CALL_OBJ_DESTROYED
extern BASE_OBJECT *psCBObjDestroyed;

// deal with a CALL_OBJ_DESTROYED
extern BOOL scrCBObjDestroyed(void);

// deal with a CALL_STRUCT_DESTROYED
extern BOOL scrCBStructDestroyed(void);

// deal with a CALL_DROID_DESTROYED
extern BOOL scrCBDroidDestroyed(void);

// deal with a CALL_FEATURE_DESTROYED
extern BOOL scrCBFeatureDestroyed(void);

// the last object to be seen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjSeen;

// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjViewer;

// deal with a CALL_OBJ_SEEN
extern BOOL scrCBObjSeen(void);

// deal with a CALL_DROID_SEEN
extern BOOL scrCBDroidSeen(void);

// deal with a CALL_STRUCT_SEEN
extern BOOL scrCBStructSeen(void);

// deal with a CALL_FEATURE_SEEN
extern BOOL scrCBFeatureSeen(void);

// deal with a CALL_TRANSPORTER_OFFMAP
extern BOOL scrCBTransporterOffMap(void);

// deal with a CALL_TRANSPORTER_LANDED
extern BOOL scrCBTransporterLanded(void);

// tell the scripts when a cluster is no longer valid
extern SDWORD	scrCBEmptyClusterID;
extern BOOL scrCBClusterEmpty( void );

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
extern DROID *psScrCBVtolOffMap;
extern BOOL scrCBVtolOffMap(void);

/*called when selectedPlayer completes some research*/
extern BOOL scrCBResCompleted(void);

/* when a player leaves the game*/
extern BOOL scrCBPlayerLeft(void);

// alliance offered.
extern BOOL scrCBAllianceOffer(void);
extern UDWORD	CBallFrom,CBallTo;

//Console callback
extern BOOL scrCallConsole(void);
extern BOOL scrCBStructBuilt(void);
extern BOOL scrCallMultiMsg(void);
//extern BOOL scrCallBeacon(void);
extern BOOL scrCBTransporterLandedB(void);

#endif


