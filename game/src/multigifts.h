/* 
 *multigifts. h
 *
 * multiplayer game, gifts and deathmatch relevant funcs.
 */

extern void requestAlliance		(UBYTE from ,UBYTE to,BOOL prop,BOOL allowAudio);						
extern void breakAlliance		(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio);
extern void formAlliance		(UBYTE p1, UBYTE p2,BOOL prop,BOOL allowAudio);
extern void sendAlliance		(UBYTE from, UBYTE to, UBYTE state,SDWORD value);
extern BOOL recvAlliance		(NETMSG *pMsg,BOOL allowAudio);

extern BOOL sendGift			(UDWORD type,UDWORD to);
extern BOOL recvGift			(NETMSG *pMsg);

extern VOID technologyGiveAway				(STRUCTURE *pS);
extern VOID recvMultiPlayerRandomArtifacts	(NETMSG *pMsg);
extern VOID addMultiPlayerRandomArtifacts	(UDWORD quantity,SDWORD type);
extern VOID processMultiPlayerArtifacts		(VOID);

extern VOID	giftArtifact					(UDWORD owner,UDWORD x,UDWORD y);

// deathmatch stuff
extern BOOL	addOilDrum						(UDWORD count);
extern VOID	giftPower						(UDWORD from,UDWORD to,BOOL send);
//extern BOOL	addDMatchDroid					(UDWORD count);
//extern BOOL	foundDMatchDroid				(UDWORD player,UDWORD x,UDWORD y);
//extern BOOL deathmatchCheck					(VOID);

//extern BOOL	dMatchWinner					(UDWORD winplayer,BOOL bcast);
//extern BOOL	recvdMatchWinner				(NETMSG *pMsg);


#define RADAR_GIFT		1
#define DROID_GIFT		2
#define RESEARCH_GIFT	3
#define POWER_GIFT		4
#define STRUCTURE_GIFT		5
