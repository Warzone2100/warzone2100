/*
 * Function.h
 *
 * Definitions for the Structure Functions.
 *
 */
#ifndef _function_h
#define _function_h

#include "objectdef.h"


//holder for all functions
extern FUNCTION		**asFunctions;
extern UDWORD		numFunctions;

//lists the current Upgrade level that can be applied to a structure through research
//extern FUNCTION_UPGRADE		*apProductionUpgrades[MAX_PLAYERS];
//extern UDWORD		numProductionUpgrades;
//extern FUNCTION_UPGRADE		*apResearchUpgrades[MAX_PLAYERS];
//extern UDWORD		numResearchUpgrades;
//extern FUNCTION_UPGRADE		*apArmourUpgrades[MAX_PLAYERS];
//extern UDWORD		numArmourUpgrades;
//extern FUNCTION_UPGRADE		*apBodyUpgrades[MAX_PLAYERS];
//extern UDWORD		numBodyUpgrades;
//extern FUNCTION_UPGRADE		*apRepairUpgrades[MAX_PLAYERS];
//extern UDWORD		numRepairUpgrades;
//extern FUNCTION_UPGRADE		*apResistanceUpgrades[MAX_PLAYERS];
//extern UDWORD		numResistanceUpgrades;
//extern FUNCTION_UPGRADE		*apWeaponUpgrades[MAX_PLAYERS];
//extern UDWORD		numWeaponUpgrades;

extern BOOL loadFunctionStats(char *pFunctionData, UDWORD bufferSize);

//load the specific stats for each function
extern BOOL loadRepairDroidFunction(char *pData);
extern BOOL loadPowerGenFunction(char *pData);
extern BOOL loadResourceFunction(char *pData);
extern BOOL loadProduction(char *pData);
extern BOOL loadProductionUpgradeFunction(char *pData);
extern BOOL loadResearchFunction(char *pData);
extern BOOL loadResearchUpgradeFunction(char *pData);
extern BOOL loadHQFunction(char *pData);
extern BOOL loadWeaponUpgradeFunction(char *pData);
extern BOOL loadWallFunction(char *pData);
extern BOOL loadStructureUpgradeFunction(char *pData);
extern BOOL loadWallDefenceUpgradeFunction(char *pData);
extern BOOL loadRepairUpgradeFunction(char *pData);
extern BOOL loadPowerUpgradeFunction(char *pData);
extern BOOL loadDroidRepairUpgradeFunction(char *pData);
extern BOOL loadDroidECMUpgradeFunction(char *pData);
extern BOOL loadDroidBodyUpgradeFunction(char *pData);
extern BOOL loadDroidSensorUpgradeFunction(char *pData);
extern BOOL loadDroidConstUpgradeFunction(char *pData);
extern BOOL loadReArmFunction(char *pData);
extern BOOL loadReArmUpgradeFunction(char *pData);

//extern BOOL loadFunction(char *pData, UDWORD functionType);
//extern BOOL loadDefensiveStructFunction(char *pData);
//extern BOOL loadArmourUpgradeFunction(char *pData);
//extern BOOL loadPowerRegFunction(char *pData);
//extern BOOL loadPowerRelayFunction(char *pData);
//extern BOOL loadRadarMapFunction(char *pData);
//extern BOOL loadRepairUpgradeFunction(char *pData);
//extern BOOL loadResistanceUpgradeFunction(char *pData);
//extern BOOL loadBodyUpgradeFunction(char *pData);

extern void productionUpgrade(FUNCTION *pFunction, UBYTE player);
extern void researchUpgrade(FUNCTION *pFunction, UBYTE player);
extern void powerUpgrade(FUNCTION *pFunction, UBYTE player);
extern void reArmUpgrade(FUNCTION *pFunction, UBYTE player);
extern void repairFacUpgrade(FUNCTION *pFunction, UBYTE player);
extern void weaponUpgrade(FUNCTION *pFunction, UBYTE player);
extern void structureUpgrade(FUNCTION *pFunction, UBYTE player);
extern void wallDefenceUpgrade(FUNCTION *pFunction, UBYTE player);
extern void structureBodyUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
extern void structureArmourUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
extern void structureResistanceUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
extern void structureProductionUpgrade(STRUCTURE *psBuilding);
extern void structureResearchUpgrade(STRUCTURE *psBuilding);
extern void structurePowerUpgrade(STRUCTURE *psBuilding);
extern void structureRepairUpgrade(STRUCTURE *psBuilding);
extern void structureSensorUpgrade(STRUCTURE *psBuilding);
extern void structureReArmUpgrade(STRUCTURE *psBuilding);
extern void structureECMUpgrade(STRUCTURE *psBuilding);
extern void sensorUpgrade(FUNCTION *pFunction, UBYTE player);
extern void repairUpgrade(FUNCTION *pFunction, UBYTE player);
extern void ecmUpgrade(FUNCTION *pFunction, UBYTE player);
extern void constructorUpgrade(FUNCTION *pFunction, UBYTE player);
extern void bodyUpgrade(FUNCTION *pFunction, UBYTE player);
extern void droidSensorUpgrade(DROID *psDroid);
extern void droidECMUpgrade(DROID *psDroid);
extern void droidBodyUpgrade(FUNCTION *pFunction, DROID *psDroid);
extern void upgradeTransporterDroids(DROID *psTransporter, 
                              void(*pUpgradeFunction)(DROID *psDroid));

//extern void armourUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
//extern void repairUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
//extern void bodyUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);
//extern void resistanceUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding);

extern BOOL FunctionShutDown();

#endif //_function_h

