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

extern BOOL loadFunctionStats(SBYTE *pFunctionData, UDWORD bufferSize);

//load the specific stats for each function
extern BOOL loadRepairDroidFunction(SBYTE *pData);//, UDWORD functionType);
extern BOOL loadPowerGenFunction(SBYTE *pData);//, UDWORD functionType);
extern BOOL loadResourceFunction(SBYTE *pData);
extern BOOL loadProduction(SBYTE *pData);//, UDWORD functionType);
extern BOOL loadProductionUpgradeFunction(SBYTE *pData);//, UDWORD functionType);
extern BOOL loadResearchFunction(SBYTE *pData);
extern BOOL loadResearchUpgradeFunction(SBYTE *pData);//, UDWORD functionType);
extern BOOL loadHQFunction(SBYTE *pData);
extern BOOL loadWeaponUpgradeFunction(SBYTE *pData);
extern BOOL loadWallFunction(SBYTE *pData);
extern BOOL loadStructureUpgradeFunction(SBYTE *pData);
extern BOOL loadWallDefenceUpgradeFunction(SBYTE *pData);
extern BOOL loadRepairUpgradeFunction(SBYTE *pData);
extern BOOL loadPowerUpgradeFunction(SBYTE *pData);
extern BOOL loadDroidRepairUpgradeFunction(SBYTE *pData);
extern BOOL loadDroidECMUpgradeFunction(SBYTE *pData);
extern BOOL loadDroidBodyUpgradeFunction(SBYTE *pData);
extern BOOL loadDroidSensorUpgradeFunction(SBYTE *pData);
extern BOOL loadDroidConstUpgradeFunction(SBYTE *pData);
extern BOOL loadReArmFunction(SBYTE *pData);
extern BOOL loadReArmUpgradeFunction(SBYTE *pData);

//extern BOOL loadFunction(SBYTE *pData, UDWORD functionType);
//extern BOOL loadDefensiveStructFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadArmourUpgradeFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadPowerRegFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadPowerRelayFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadRadarMapFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadRepairUpgradeFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadResistanceUpgradeFunction(SBYTE *pData);//, UDWORD functionType);
//extern BOOL loadBodyUpgradeFunction(SBYTE *pData);//, UDWORD functionType);

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

