//aiexperience.h
#define MAX_OIL_ENTRIES 600				//(Max number of derricks or oil resources) / 2

#define	MAX_OIL_DEFEND_LOCATIONS  100		//max number of attack locations to store
#define	MAX_OIL_LOCATIONS  300			//max number of oil locations to store
#define	MAX_BASE_DEFEND_LOCATIONS  30	//max number of base locations to store
#define SAME_LOC_RANGE 8				//if within this range, consider it the same loc

extern SDWORD baseLocation[MAX_PLAYERS][MAX_PLAYERS][2];
extern SDWORD oilLocation[MAX_PLAYERS][MAX_OIL_LOCATIONS][2];
extern SDWORD baseDefendLocation[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS][2];
extern SDWORD oilDefendLocation[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS][2];

extern SDWORD baseDefendLocPrior[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS];
extern SDWORD oilDefendLocPrior[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS];

extern	BOOL SavePlayerAIExperience(SDWORD nPlayer, BOOL bNotify);
extern	BOOL LoadPlayerAIExperience(SDWORD nPlayer, BOOL bNotify);

extern	BOOL LoadAIExperience(BOOL bNotify);
extern	BOOL SaveAIExperience(BOOL bNotify);


extern	BOOL ExperienceRecallOil(SDWORD nPlayer);
extern	void InitializeAIExperience(void);
extern	BOOL OilResourceAt(UDWORD OilX,UDWORD OilY, SDWORD VisibleToPlayer);

extern	BOOL ReadAISaveData(SDWORD nPlayer);
extern	BOOL WriteAISaveData(SDWORD nPlayer);

extern	BOOL SetUpInputFile(SDWORD nPlayer);
extern	BOOL SetUpOutputFile(STRING * pMapName,SDWORD nPlayer);

extern	BOOL StoreBaseDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);
extern	BOOL StoreOilDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);

extern	BOOL SortBaseDefendLoc(SDWORD nPlayer);
extern	BOOL SortOilDefendLoc(SDWORD nPlayer);

extern	SDWORD GetOilDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);
extern	SDWORD GetBaseDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);

extern BOOL CanRememberPlayerBaseLoc(SDWORD lookingPlayer, SDWORD enemyPlayer);
extern BOOL CanRememberPlayerBaseDefenseLoc(SDWORD player, SDWORD index);
extern BOOL CanRememberPlayerOilDefenseLoc(SDWORD player, SDWORD index);

extern	void BaseExperienceDebug(SDWORD nPlayer);
extern	void OilExperienceDebug(SDWORD nPlayer);

extern BOOL canRecallOilAt(SDWORD nPlayer, SDWORD x, SDWORD y);
