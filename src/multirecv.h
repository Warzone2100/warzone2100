/*
 * 	MultiRecv.h
 *
 * header for multiplay files that recv netmsgs.
 * this is a little lower level than multiplay.h
 * so we don't include it in other warzone stuff
 * to avoid a load of warnings
 */

extern BOOL recvDroid				(NETMSG *pMsg);
extern BOOL recvDroidInfo			(NETMSG *pMsg);
extern BOOL recvDestroyDroid		(NETMSG *pMsg);
extern BOOL recvGroupOrder			(NETMSG *pMsg);
extern BOOL recvDroidMove			(NETMSG *pMsg);
extern BOOL receiveWholeDroid		(NETMSG *pMsg);
extern BOOL recvDestroyStructure	(NETMSG *pMsg);
//extern BOOL RecvBuild				(NETMSG *pMsg);
extern BOOL recvBuildStarted		(NETMSG *pMsg);
extern BOOL recvBuildFinished		(NETMSG	*pMsg);
//extern BOOL receiveWholeStructure	(NETMSG *pMsg);
extern BOOL recvTemplate			(NETMSG *pMsg);
extern BOOL recvDestroyFeature		(NETMSG *pMsg);
extern BOOL recvDemolishFinished	(NETMSG *pMsg);
extern BOOL recvPing				(NETMSG *pMsg);
extern BOOL	recvRequestDroid		(NETMSG *pMsg);
extern BOOL recvTextMessage			(NETMSG *pMsg);
//extern BOOL recvDroidWaypoint		(NETMSG *pMsg);
extern BOOL recvDroidSecondary		(NETMSG *pMsg);
extern BOOL recvDroidSecondaryAll	(NETMSG *pMsg);
extern BOOL recvDroidEmbark         (NETMSG *pMsg);
extern BOOL recvDroidDisEmbark      (NETMSG *pMsg);
//extern BOOL recvCommandDroid		(NETMSG *pMsg);
extern BOOL recvDroidCheck			(NETMSG *pMsg);
extern BOOL recvStructureCheck		(NETMSG *pMsg);
extern BOOL recvPowerCheck			(NETMSG *pMsg);
extern BOOL recvAlliance			(NETMSG *pMsg,BOOL allowAudio);
//extern BOOL multiPlayerRequest		(NETMSG *pMsg);
extern BOOL recvColourRequest		(NETMSG *pMsg);
extern BOOL sendWholeDroid			(DROID	*pD, DPID dest);
//extern BOOL sendWholeStructure		(STRUCTURE *pS, DPID dest);
extern void recvOptions				(NETMSG *pMsg);
extern void sendOptions				(DPID dest,UDWORD play);
extern BOOL recvScoreSubmission		(NETMSG *pMsg);
extern BOOL recvDestroyExtra		(NETMSG *pMsg);
extern BOOL	recvAudioMsg			(NETMSG *pMsg);
extern BOOL recvHappyVtol			(NETMSG *pMsg);
extern BOOL recvVtolRearm			(NETMSG *pMsg);
extern BOOL recvResearchStatus		(NETMSG *pMsg);
extern BOOL recvLasSat				(NETMSG *pMsg);
extern BOOL	recvMapFileData				(NETMSG *pMsg);
extern BOOL	recvMapFileRequested		(NETMSG *pMsg);


extern BOOL recvTextMessageAI		(NETMSG *pMsg);		//AI multiplayer message
extern BOOL	recvTeamRequest			(NETMSG *pMsg);
