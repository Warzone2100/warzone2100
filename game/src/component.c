/*	
	Component.c
	Draws component objects - oh yes indeed.
	Alex M, Pumpkin Studios, EIDOS Interactive, 1997. 
	
*/

#include "Frame.h"
#include "Base.h"
#include "Droid.h"
#include "action.h"
#include "order.h"
#include "Component.h"
#include "ivisdef.h" //ivis matrix code
#include "geo.h" //ivis matrix code
#include "piedef.h" //ivis matrix code
#include "pieState.h" //ivis render code
#include "Lighting.h"


#define TESTLOWPOLY

#define GAME2PSX(x) (((x)*4096)/360)

#ifdef WIN32
#define GetRadius(x) ((x)->sradius)
#else
#define GetRadius(x) ((x)->radius)
#endif

#include "Stats.h"
#ifdef WIN32
#include "PieMode.h"
#endif
#include "Objects.h"
#include "Display.h"
#include "Geometry.h"
#include "Display3d.h"
#include "Map.h"
#include "GTime.h"
#include "IntDisplay.h"
#include "MiscImd.h"
#include "Effects.h"
#include "E3Demo.h"
#include "Transporter.h"
#include "Projectile.h"
#ifdef PSX
#include "drawimd_psx.h"
#include "primatives.h"
#include <inline_c.h>
#include <gtemac.h>

extern void psxPushMatrix(void);
extern void psxPopMatrix(void);
extern void psxSaveMatrix(void);
extern void psxRestoreMatrix(void);
extern void psxiV_MatrixBegin(void);
extern void psxiV_ITRANSLATE(SDWORD x, SDWORD y, SDWORD z);
extern void psxiV_MatrixRotateZ(SWORD IVISangle );
extern void psxiV_MatrixRotateX(SWORD angle );
extern void psxiV_MatrixRotateY(SWORD angle );
extern void psxiV_RotateZMatrix(SWORD angle );
extern void psxiV_RotateXMatrix(SWORD angle );
extern void psxiV_RotateYMatrix(SWORD angle );
extern void psxiV_TRANSLATE(SDWORD x, SDWORD y, SDWORD z);

UWORD SelectedThisFrame;

#endif

void unsetMatrix(void);
void setMatrix(iVector *Position,iVector *Rotation,iVector *CameraPos,BOOL RotXYZ);

#define MAX_GROUP_SIZE	10

#define NUM_GIBLETS	5
#define BLIP_ANIM_DURATION			200

#define	DEFAULT_COMPONENT_TRANSLUCENCY	128

#define	DROID_EMP_SPREAD	(20 - rand()%40)

#ifdef PSX
#define NO_VTOL_TURRETS 	// don't draw turret if it's not a VTOL because they look crap.

static PIE PieParams;
static SDWORD ShadowSizeX=0;
static SDWORD ShadowSizeZ=0;
static BOOL bDrawShadow=TRUE;
static BOOL bInButton;
static BOOL ComponentTransparent;
static UWORD ComponentTransMode;
#endif

UDWORD	droidScale = 100;
void displayComponentTemplate(DROID_TEMPLATE *psTemplate);
//void displayComponentObject(BASE_OBJECT *psObj);
#ifdef WIN32
static void displayCompObj(BASE_OBJECT *psObj,iVector *mountRotation, BOOL bButton);
#else
// Returns false if nothing rendered.
static BOOL displayCompObj(BASE_OBJECT *psObj,iVector *mountRotation, BOOL bButton);
#endif
static iIMDShape *getLeftPropulsionIMD(DROID *psDroid);
static iIMDShape *getRightPropulsionIMD(DROID *psDroid);

static UDWORD getStructureHeight(STRUCTURE *psStructure);

BOOL		leftFirst;

SDWORD		droidLightLevel = 224;
UDWORD		lightInterval=15;
UDWORD		lightLastChanged;
SDWORD		lightSpeed=2;
extern UDWORD selectedPlayer;

UBYTE		PlayerColour[MAX_PLAYERS];// = {0,1,2,3,4,5,6,7}

// Colour Lookups
// use col = MAX_PLAYERS for anycolour (see multiint.c)
BOOL setPlayerColour(UDWORD player, UDWORD col)
{
	if(player >MAX_PLAYERS || col >MAX_PLAYERS)
	{
		DBERROR(("setplayercolour: wrong values"));
		return FALSE;
	}
	PlayerColour[(UBYTE)player] = (UBYTE)col;
	return TRUE;
}

UBYTE getPlayerColour(UDWORD pl)
{
	return PlayerColour[pl];
}

void initPlayerColours(void)
{
	UBYTE i;
	for(i=0;i<MAX_PLAYERS;i++)
	{
		PlayerColour[i] = i;
	}
}


void updateLightLevels(void)
{
	if(gameTime>(lightLastChanged+lightInterval))
	{
	   	droidLightLevel+=lightSpeed;
		lightLastChanged = gameTime;
	 	if(droidLightLevel>255 OR droidLightLevel<128)
		{
			if(lightSpeed>0)
			{
					lightSpeed = -lightSpeed;
			}
			else
			{
					lightSpeed = -lightSpeed;
			}
		}
	}
}


#ifdef WIN32
void setMatrix(iVector *Position,iVector *Rotation,iVector *CameraPos,BOOL RotXYZ)
{
	

	iVector BSPCameraPos;
	OBJPOS Camera = {0,0,0};

	Camera.pitch=-45;
	Camera.yaw=0;

//	Rotation->y=0;

	GetRealCameraPos(&Camera,Position->z,&BSPCameraPos);
//	SetBSPCameraPos(BSPCameraPos.x,BSPCameraPos.y,BSPCameraPos.z);	
	// Fixes BSP drawing in buttons. eg Player HQ.
	SetBSPCameraPos(BSPCameraPos.x,500,BSPCameraPos.z);	
	SetBSPObjectPos(0,0,0);			// For imd button the bsp is sourced at 0,0,0


	SetBSPObjectRot(DEG(-Rotation->y),0);								// Droid rotation





   	pie_MatBegin();

	pie_TRANSLATE(Position->x,Position->y,Position->z);

	if(RotXYZ) {
		pie_MatRotX(DEG(Rotation->x));
		pie_MatRotY(DEG(Rotation->y));
		pie_MatRotZ(DEG(Rotation->z));
	} else {
		pie_MatRotY(DEG(Rotation->y));
		pie_MatRotX(DEG(Rotation->x));
		pie_MatRotZ(DEG(Rotation->z));
	}
}


void unsetMatrix(void)
{
	pie_MatEnd();
}
#else

static UWORD ButtonOTIndex = ORDERING_BUTTONRENDERING;


void setComponentButtonOTIndex(UWORD OTIndex)
{
	ButtonOTIndex = OTIndex;
}


void setMatrix(iVector *Position,iVector *Rotation,iVector *CameraPos,BOOL RotXYZ)
{
	iVector BSPCameraPos;
	OBJPOS Camera = {0,0,0};

	Camera.pitch=-45;
	Camera.yaw=0;

//	Rotation->y=0;

	GetRealCameraPos(&Camera,Position->z,&BSPCameraPos);
//	SetBSPCameraPos(BSPCameraPos.x,BSPCameraPos.y,BSPCameraPos.z);	
	// Fixes BSP drawing in buttons. eg Player HQ.
	SetBSPCameraPos(BSPCameraPos.x,500,BSPCameraPos.z);	
	SetBSPObjectPos(0,0,0);			// For imd button the bsp is sourced at 0,0,0


	SetBSPObjectRot(GAME2PSX(-Rotation->y),0);								// Droid rotation

	psxPushMatrix();
	psxiV_MatrixBegin();
	psxiV_MatrixAspectCorrect();

	geomTranslate(Position);
//	psxiV_TRANSLATE(Position->x,Position->y,Position->z);

	if(RotXYZ) {
		geomRotateMatrixYXZDeg(Rotation);
//		psxiV_MatrixRotateY(DEG(Rotation->y));
//		psxiV_MatrixRotateX(DEG(Rotation->x));
//		psxiV_MatrixRotateZ(DEG(Rotation->z));
	} else {
		geomRotateMatrixXYZDeg(Rotation);
//		psxiV_MatrixRotateX(DEG(Rotation->x));
//		psxiV_MatrixRotateY(DEG(Rotation->y));
//		psxiV_MatrixRotateZ(DEG(Rotation->z));
	}

	psxiv_SetIMDZ(ButtonOTIndex);
	psxUseMatrix();
}


void unsetMatrix(void)
{
	psxPopMatrix();
}
#endif



UDWORD getComponentDroidRadius(DROID *psDroid)
{
	UNUSEDPARAMETER(psDroid);

	return 100;
}


UDWORD getComponentDroidTemplateRadius(DROID_TEMPLATE *psDroid)
{
	UNUSEDPARAMETER(psDroid);

	return 100;
}


UDWORD getComponentRadius(BASE_STATS *psComponent)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;

	compID = StatIsComponent(psComponent);
	if (compID > 0)	{
		StatGetComponentIMD(psComponent, compID,&ComponentIMD, &MountIMD);
//		ComponentIMD = StatGetComponentIMD(psComponent, compID);
		if(ComponentIMD)
		{
			return GetRadius(ComponentIMD);
		}
		else
		{
// NULL component,should this be an error?
		}
	}

//	DBERROR(("Not a valid component"));
	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if ( (StatIsComponent(psComponent) != COMP_WEAPON) ||
		  ((WEAPON_STATS *)psComponent)->weaponSubClass != WSC_BOMB )
	{ 
		DBPRINTF(("ComponentPIE == NULL : File : %s Line : %d\n",__FILE__,__LINE__));
	}

#ifdef WIN32
	return COMPONENT_RADIUS;
#else
	return PSX_BUTTON_SCALE;		// ffs 
#endif
}


UDWORD getResearchRadius(BASE_STATS *Stat)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;

	if(ResearchIMD) {
		return GetRadius(ResearchIMD);
	}

//	DBERROR(("Null IMD in getResearchRadius()"));
	DBPRINTF(("ResearchPIE == NULL : File : %s Line : %d\n",__FILE__,__LINE__));

	return 100;
}


//#ifdef WIN32
UDWORD getStructureSize(STRUCTURE *psStructure)
{
	UDWORD size;
	//radius based on base plate size

	size = psStructure->pStructureType->baseWidth;
	if (psStructure->pStructureType->baseBreadth > size)
	{
		size = psStructure->pStructureType->baseBreadth;
	}
	return (size);
}
//#else
UDWORD getStructureRadius(STRUCTURE *psStructure)
{
	iIMDShape *baseImd;
	SDWORD Radius = GetRadius(psStructure->sDisplay.imd);

	baseImd = psStructure->pStructureType->pBaseIMD;
	if(baseImd!=NULL) {
		if(GetRadius(baseImd) > Radius) {
			Radius = GetRadius(baseImd);
		}
	}

	return Radius;
}
//#endif


//#ifdef WIN32
UDWORD getStructureStatSize(STRUCTURE_STATS *Stats)
{
	UDWORD size;
	//radius based on base plate size

	size = Stats->baseWidth;
	if (Stats->baseBreadth > size)
	{
		size = Stats->baseBreadth;
	}
	return (size);
}
//#else
UDWORD getStructureStatRadius(STRUCTURE_STATS *Stats,UDWORD Player)
{
	iIMDShape *baseImd;
	SDWORD Radius = GetRadius(Stats->pIMD);

	UNUSEDPARAMETER(Player);

	baseImd = Stats->pBaseIMD;
	if(baseImd!=NULL) {
		if(GetRadius(baseImd) > Radius) {
			Radius = GetRadius(baseImd);
		}
	}

	return Radius;
}
//#endif

UDWORD getStructureHeight(STRUCTURE *psStructure)
{
    return (getStructureStatHeight(psStructure->pStructureType));
}

UDWORD getStructureStatHeight(STRUCTURE_STATS *psStat)
{
    if (psStat->pIMD)
    {
#ifdef WIN32
        return (psStat->pIMD->ymax - psStat->pIMD->ymin);
#else
        return (psStat->pIMD->ymax);
#endif
    }
    return 0;
}


void displayIMDButton(iIMDShape *IMDShape,
					     iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
	iVector TmpCamPos = {0,0,0};

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);
#ifdef PSX
	
	psxUseMatrix();



#endif

#ifdef WIN32
	pie_SetFogStatus(FALSE);
	pie_Draw3DShape(IMDShape, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0); // ajl changed 0 to selectedPlayer
#else
	if(IMDShape) {
		DrawPie(IMDShape,NULL);
	}
#endif
	unsetMatrix();
}




void displayStructureButton(STRUCTURE *psStructure,
						     iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *baseImd,*strImd,*mountImd,*weaponImd;
	UDWORD			nWeaponStat;
	iVector TmpCamPos = {0,0,0};

    /*HACK HACK HACK! 
    if its a 'tall thin (ie tower)' structure with something on the top - offset the 
    position to show the object on top*/
    if (psStructure->pStructureType->pIMD->nconnectors AND scale == SMALL_STRUCT_SCALE AND 
        getStructureHeight(psStructure) > TOWER_HEIGHT)
    {
#ifdef WIN32
        Position->y -= 20;
#else
        Position->y += 20;	// Not sure why but need to negate it on PSX.
#endif
    }

#ifdef WIN32
	SetBSPObjectPos(0,0,0);
#endif
	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);
#ifdef PSX
	psxUseMatrix();
#endif


	/* Draw the building's base first */
	baseImd = psStructure->pStructureType->pBaseIMD;
#ifdef WIN32
	if(baseImd!=NULL) {
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);		// ajl changed 0 to selectedPlayer
	}
	pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);	// ajl changed 0 to selectedPlayer
	//and draw the turret
	if(psStructure->sDisplay.imd->nconnectors)
	{
		weaponImd = NULL;//weapon is gun ecm or sensor
		mountImd = NULL;
		strImd = psStructure->sDisplay.imd;
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		//if (psStructure->numWeaps > 0)
		if (psStructure->asWeaps[0].nStat > 0)
		{
			nWeaponStat = psStructure->asWeaps[0].nStat;
			weaponImd =  asWeaponStats[nWeaponStat].pIMD;
			mountImd =  asWeaponStats[nWeaponStat].pMountGraphic;
		}

		if (weaponImd == NULL)
		{
			//check for ECM
			if (psStructure->pStructureType->pECM != NULL)
			{
				weaponImd =  psStructure->pStructureType->pECM->pIMD;
				mountImd =  psStructure->pStructureType->pECM->pMountGraphic;
			}
		}

		if (weaponImd == NULL)
		{
			//check for sensor
			if (psStructure->pStructureType->pSensor != NULL)
			{
				weaponImd =  psStructure->pStructureType->pSensor->pIMD;
				mountImd  =  psStructure->pStructureType->pSensor->pMountGraphic;
			}
		}

		//draw Weapon/ECM/Sensor for structure
		if(weaponImd != NULL)
		{
			iV_MatrixBegin();
			iV_TRANSLATE(strImd->connectors->x,strImd->connectors->z,strImd->connectors->y);
			pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation)));
			if (mountImd != NULL)
			{
				pie_Draw3DShape(mountImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);
				if(mountImd->nconnectors)
				{
					iV_TRANSLATE(mountImd->connectors->x,mountImd->connectors->z,mountImd->connectors->y);
				}
			}
			iV_MatrixRotateX(DEG(psStructure->turretPitch));
			pie_Draw3DShape(weaponImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);
			//we have a droid weapon so do we draw a muzzle flash
			iV_MatrixEnd();
		}
	} 
#else	// PSX version.
	renderStructureConnectors(psStructure,NULL,TRUE);
	if(psStructure->sDisplay.imd) {
		DrawPie(psStructure->sDisplay.imd,NULL);
	}
	if(baseImd!=NULL) {
		DrawPie(baseImd,NULL);
	}
#endif
	unsetMatrix();
}

void displayStructureStatButton(STRUCTURE_STATS *Stats,UDWORD Player,
						     iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
#ifdef WIN32
	iIMDShape		*baseImd,*strImd,*mountImd,*weaponImd;
	iVector TmpCamPos = {0,0,0};
	//UDWORD			nWeaponStat;

	UNUSEDPARAMETER(Player);

    /*HACK HACK HACK! 
    if its a 'tall thin (ie tower)' structure stat with something on the top - offset the 
    position to show the object on top*/
    if (Stats->pIMD->nconnectors AND scale == SMALL_STRUCT_SCALE AND 
        getStructureStatHeight(Stats) > TOWER_HEIGHT)
    {
        Position->y -= 20;
    }

    setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);

	/* Draw the building's base first */
	baseImd = Stats->pBaseIMD;

	if(baseImd!=NULL) {
		pie_Draw3DShape(baseImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);// ajl changed 0 to selectedPlayer
	}
	pie_Draw3DShape(Stats->pIMD, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);// ajl changed 0 to selectedPlayer

	//and draw the turret
	if(Stats->pIMD->nconnectors)
	{
		weaponImd = NULL;//weapon is gun ecm or sensor
		mountImd = NULL;
		strImd = Stats->pIMD;
		//get an imd to draw on the connector priority is weapon, ECM, sensor
		//check for weapon
		//if (Stats->numWeaps > 0)
        //can only have the one
        if (Stats->psWeapStat != NULL)
		{
			/*nWeaponStat = Stats->defaultWeap;
			weaponImd =  Stats->asWeapList[nWeaponStat]->pIMD;
			mountImd =  Stats->asWeapList[nWeaponStat]->pMountGraphic;*/
            weaponImd = Stats->psWeapStat->pIMD;
            mountImd = Stats->psWeapStat->pMountGraphic;
		}

		if (weaponImd == NULL)
		{
			//check for ECM
			if (Stats->pECM != NULL)
			{
				weaponImd =  Stats->pECM->pIMD;
				mountImd =  Stats->pECM->pMountGraphic;
			}
		}

		if (weaponImd == NULL)
		{
			//check for sensor
			if (Stats->pSensor != NULL)
			{
				weaponImd =  Stats->pSensor->pIMD;
				mountImd  =  Stats->pSensor->pMountGraphic;
			}
		}

		//draw Weapon/ECM/Sensor for structure
		if(weaponImd != NULL)
		{
			iV_MatrixBegin();
			iV_TRANSLATE(strImd->connectors->x,strImd->connectors->z,strImd->connectors->y);
			pie_MatRotY(DEG(0));
			if (mountImd != NULL)
			{
				pie_Draw3DShape(mountImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);
				if(mountImd->nconnectors)
				{
					iV_TRANSLATE(mountImd->connectors->x,mountImd->connectors->z,mountImd->connectors->y);
				}
			}
			iV_MatrixRotateX(DEG(0));
			pie_Draw3DShape(weaponImd, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);
			//we have a droid weapon so do we draw a muzzle flash
			iV_MatrixEnd();
		}
	} 

	unsetMatrix();
#else
 	iIMDShape		*baseImd;
	iVector TmpCamPos = {0,0,0};

	UNUSEDPARAMETER(Player);

    /*HACK HACK HACK! 
    if its a 'tall thin (ie tower)' structure stat with something on the top - offset the 
    position to show the object on top*/
    if (Stats->pIMD->nconnectors AND scale == SMALL_STRUCT_SCALE AND 
        getStructureStatHeight(Stats) > TOWER_HEIGHT)
    {
        Position->y += 20;
    }

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);

	psxUseMatrix();


	/* Draw the building's base first */
	baseImd = Stats->pBaseIMD;

	PieParams.Flags = 0;
	if(ComponentTransparent) {
		PieParams.Flags |= PIE_TRANSPARENT;
		PieParams.TransMode = ComponentTransMode;
	}

	if(Stats->pIMD) {
		renderStructureStatConnectors(Stats);
		DrawPie(Stats->pIMD,&PieParams);
	}
	if(baseImd!=NULL) {
		DrawPie(baseImd,&PieParams);
	}

	unsetMatrix();
#endif
}


    
// Render a component given a BASE_STATS structure.
//
void displayComponentButton(BASE_STATS *Stat, iVector *Rotation,iVector *Position,
                            BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *ComponentIMD = NULL;
	iIMDShape *MountIMD = NULL;
	SDWORD compID;
	iVector TmpCamPos = {0,0,0};

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);
#ifdef PSX
	psxUseMatrix();
#endif

	compID = StatIsComponent(Stat);
	if (compID > 0)	{
		StatGetComponentIMD(Stat, compID,&ComponentIMD, &MountIMD);
//		ComponentIMD = StatGetComponentIMD(Stat, compID);
	}
	else
	{
		unsetMatrix();
		return;
	}

	/* VTOL bombs are only stats allowed to have NULL ComponentIMD */
	if ( (ComponentIMD == NULL) &&
		 ((StatIsComponent(Stat) != COMP_WEAPON) ||
		  ((WEAPON_STATS *)Stat)->weaponSubClass != WSC_BOMB) )
	{
		DBPRINTF(("ComponentPIE == NULL : File : %s Line : %d\n",__FILE__,__LINE__));
//		DBERROR(("ComponentIMD == NULL"));
	}



#ifdef WIN32
	if(MountIMD)
	{
		pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);// ajl changed 0 to selectedPlayer
	}
	if(ComponentIMD)
	{
		pie_Draw3DShape(ComponentIMD, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);// ajl changed 0 to selectedPlayer
	}
#else
//	if( (Rotation->y % 360 > 270) || (Rotation->y % 360 < 90) ) {
		PieParams.Flags = 0;
		if(ComponentTransparent) {
			PieParams.Flags |= PIE_TRANSPARENT;
			PieParams.TransMode = ComponentTransMode;
		}

		if(ComponentIMD) {
			DrawPie(ComponentIMD,&PieParams);
		}
		if(MountIMD) {
			DrawPie(MountIMD,&PieParams);
		}
//	} else {
//		if(MountIMD) {
//			DrawPie(MountIMD,NULL);
//		}
//		DrawPie(ComponentIMD,NULL);
//	}

#endif

	unsetMatrix();
}


// Render a research item given a BASE_STATS structure.
//
void displayResearchButton(BASE_STATS *Stat,
						     iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
	iIMDShape *ResearchIMD = ((RESEARCH *)Stat)->pIMD;
	iIMDShape *MountIMD = ((RESEARCH *)Stat)->pIMD2;
	iVector TmpCamPos = {0,0,0};


	if(ResearchIMD)
	{
		setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
		scaleMatrix(scale);
#ifdef PSX
		psxUseMatrix();

		PieParams.Flags = 0;
		if(ComponentTransparent) {
			PieParams.Flags |= PIE_TRANSPARENT;
			PieParams.TransMode = ComponentTransMode;
		}
#endif

		if(MountIMD) {
#ifdef WIN32
			pie_Draw3DShape(MountIMD, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);	// ajl, added colourthing using selectedPlayer
#else
			DrawPie(MountIMD,&PieParams);
#endif
		}
#ifdef WIN32
		pie_Draw3DShape(ResearchIMD, 0, getPlayerColour(selectedPlayer), pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);		//ajl, added colourthing using selectedPlayer
#else
		DrawPie(ResearchIMD,&PieParams);
#endif

		unsetMatrix();
	}

//	DBERROR(("Null IMD in displayResearchButton()"));
}



// Render a composite droid given a DROID_TEMPLATE structure.
//
void displayComponentButtonTemplate(DROID_TEMPLATE *psTemplate,
									iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
	static DROID Droid;	// Made static to reduce stack usage.
	SDWORD difference;
	iVector mountRotation = {0,0,0};
 	iVector TmpCamPos = {0,0,0};

	/* init to NULL */
	memset( &Droid, 0, sizeof(DROID) );

	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);
#ifdef PSX
	psxUseMatrix();
#endif

// Decide how to sort it.

	difference = Rotation->y%360;

	if((difference>0 AND difference <180) OR difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

	droidSetBits(psTemplate,&Droid);
	Droid.player = (UBYTE)selectedPlayer;

	Droid.x=Droid.y=Droid.z=0;	// important for psx stuff ...

	//draw multi component object as a button object
	displayCompObj((BASE_OBJECT*)&Droid,&mountRotation,TRUE);

	unsetMatrix();
}


// Render a composite droid given a DROID structure.
//
void displayComponentButtonObject(DROID *psDroid,
								  iVector *Rotation,iVector *Position,BOOL RotXYZ, SDWORD scale)
{
	SDWORD		difference;
	iVector		mountRotation = {0,0,0};
 	iVector		TmpCamPos = {0,0,0};
	
	setMatrix(Position,Rotation,&TmpCamPos,RotXYZ);
	scaleMatrix(scale);
#ifdef PSX
	psxUseMatrix();
#endif

// Decide how to sort it.
	difference = Rotation->y%360;

	if((difference>0 AND difference <180) OR difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

// And render the composite object.
	//draw multi component object as a button object
	displayCompObj((BASE_OBJECT*)psDroid,&mountRotation,TRUE);

	unsetMatrix();
}



#ifdef WIN32	// Start of PC version.

/* Assumes matrix context is already set */
void displayComponentObject(BASE_OBJECT *psObj)
{
DROID		*psDroid;
//iIMDShape	*psShape;
iVector		position,rotation,mountRotation;	//,null;
//iPoint		screenCoords;
//SDWORD		dummyZ;
int32		xShift,zShift;
UDWORD		worldAngle;
SDWORD		difference;
SDWORD		frame;
PROPULSION_STATS	*psPropStats;
UDWORD	tileX,tileY;
MAPTILE	*psTile;

	psDroid = (DROID *)psObj;
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;


	worldAngle = (UDWORD) ((UDWORD)player.r.y/DEG_1)%360;
	difference = (worldAngle-psObj->direction);

	if((difference>0 AND difference <180) OR difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

	/* Push the matrix */
	pie_MatBegin();

	/* Get internal tile units coordinates */
	xShift = player.p.x & (TILE_UNITS-1);
	zShift = player.p.z & (TILE_UNITS-1);

	/* Mask out to tile_units resolution */
	pie_TRANSLATE(xShift,0,-zShift);

	/* Get the real position */
	position.x = (psDroid->x - player.p.x) - terrainMidX*TILE_UNITS;
	position.z = terrainMidY*TILE_UNITS - (psDroid->y - player.p.z);
	position.y = psDroid->z;

	//{
	//	position.y += bobTransporterHeight();
	//}
	//if(psPropStats->propulsionType == LIFT)
	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		position.y += bobTransporterHeight();
	}

	/* Get all the pitch,roll,yaw info */
	rotation.y = -(SDWORD)psDroid->direction;
	rotation.x = psDroid->pitch;
	rotation.z = psDroid->roll;


	/* Get rotation info for the mounting too (holds the gun */
	mountRotation.y = -(SDWORD)psDroid->turretRotation;
	mountRotation.x = psDroid->turretPitch;
	mountRotation.z = 0;

	/* Translate origin */
	pie_TRANSLATE(position.x,position.y,position.z);

	/* Rotate for droid */
	pie_MatRotY(DEG(rotation.y));
	pie_MatRotX(DEG(rotation.x));
	pie_MatRotZ(DEG(rotation.z));

	if( (gameTime-psDroid->timeLastHit < GAME_TICKS_PER_SEC) AND psDroid->lastHitWeapon == WSC_ELECTRONIC)
	{
		objectShimmy( (BASE_OBJECT*) psDroid );
	}

    if (psDroid->lastHitWeapon == WSC_EMP AND 
        (gameTime - psDroid->timeLastHit < EMP_DISABLE_TIME))
    {
        iVector			position;

        //add an effect on the droid
		position.x = psDroid->x + DROID_EMP_SPREAD;
		position.y = psDroid->z + rand()%8;;
		position.z = psDroid->y + DROID_EMP_SPREAD;
        effectGiveAuxVar(90+rand()%20);
		addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_PLASMA,FALSE,NULL,0);
    }

	if (godMode || (psDroid->visible[selectedPlayer] == UBYTE_MAX) OR demoGetStatus())
	{
		//ingame not button object
		displayCompObj(psObj,&mountRotation,FALSE);
	}
	else
	{
	   
		// make sure it's not over water.
		tileX = psDroid->x/TILE_UNITS;
		tileY = psDroid->y/TILE_UNITS;
		// double check it's on map
		if( (tileX>=0) AND 
			(tileY>=0) AND
			(tileX<mapWidth) AND
			(tileY<mapHeight) )		
		{
			psTile = mapTile(tileX,tileY);
			if(TERRAIN_TYPE(psTile) != TER_WATER)
			{
			   	frame = gameTime/BLIP_ANIM_DURATION + psDroid->id; //visible[selectedPlayer];
			   	pie_Draw3DShape(getImdFromIndex(MI_BLIP), frame, 0, pie_MAX_BRIGHT_LEVEL, 0, pie_ADDITIVE, (psDroid->visible[selectedPlayer]/2));
			// 	pie_Draw3DShape(blipImd, frame, 0, pie_MAX_BRIGHT_LEVEL, 0, pie_TRANSLUCENT, 128);
				/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
			}
		}
	}
	pie_MatEnd();
}




/* Assumes matrix context is already set */
void displayCompObj(BASE_OBJECT *psObj,iVector *mountRotation, BOOL bButton)
{
	DROID				*psDroid;
	iIMDShape			*psShape, *psJet = NULL;
	iVector				null;
	iPoint				screenCoords;
	SDWORD				dummyZ, iConnector;
	PROPULSION_STATS	*psPropStats;
	SDWORD				frame;
	SDWORD				pieFlag, iPieData;
	UDWORD				brightness, specular;
//	SDWORD				centreX,centreZ;
	UDWORD				colour;
	UDWORD				bDarkSide = FALSE;
	
   	/* Cast the droid pointer */
	psDroid = (DROID *)psObj;
	if( (gameTime-psDroid->timeLastHit < GAME_TICKS_PER_SEC/4 ) AND psDroid->lastHitWeapon == WSC_ELECTRONIC AND !gamePaused())
	{
		colour = getPlayerColour(rand()%MAX_PLAYERS);
		bDarkSide = TRUE;
	}
	else
	{
		colour = getPlayerColour(psDroid->player);
	}

	//set pieflag for button object or ingame object
	if ( bButton )
	{
		pieFlag = pie_BUTTON;
	}
	else
	{
		pieFlag = 0;
	}

	if(!pieFlag)
	{
   //		centreX = ( player.p.x + ((visibleXTiles/2)<<TILE_SHIFT) );
   //		centreZ = ( player.p.z + ((visibleYTiles/2)<<TILE_SHIFT) );
		brightness = lightDoFogAndIllumination(psDroid->illumination,getCentreX() - psDroid->x,getCentreZ() - psDroid->y, &specular);
	}
	else
	{
		brightness = pie_MAX_BRIGHT_LEVEL;
		specular = 0;
	}

//	/* No auxilliary rotation */
	null.x = null.y = null.z = 0;

	/* We've got a z value here _and_ screen coords of origin */
	dummyZ = pie_RotProj(&null,&screenCoords);

	/* Draw the propulsion and body imds here */
	/* Establish the propulsion - this is more complex if two parts */
/*
	if(leftFirst)
	{
		psShape = getLeftPropulsionIMD(psDroid);
		if(psShape!=NULL)
		{
			iV_PIEDraw(psShape,psDroid->player);
		}
	}
	else
	{	
		psShape = getRightPropulsionIMD(psDroid);
		if(psShape!=NULL)
		{
			iV_PIEDraw(psShape,psDroid->player);
		}
	}
*/
//	scaleMatrix(droidScale);

	/* set default components transparent */
	if ( psDroid->asBits[COMP_PROPULSION].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		iPieData = 0;
	}

	psShape = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if(psShape!=NULL)
	{
		pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
	}

	/* set default components transparent */
	if ( psDroid->asBits[COMP_BODY].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	/* Get the body graphic now*/
	psShape = BODY_IMD(psDroid,psDroid->player);
	if(psShape!=NULL)
	{
		// FIXME
		if ( psDroid->droidType == DROID_PERSON)
		{
			/* draw body if not animating */
			if ( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == FALSE )
			{
				// FIXME - hideous....!!!!
				scaleMatrix(75);
				if(pie_GetRenderEngine()==ENGINE_GLIDE)
				{
					pie_Draw3DShape(psShape, 0, psDroid->player-6, brightness, specular, pieFlag | pie_NO_BILINEAR, iPieData);
				}
				else
				{
					pie_Draw3DShape(psShape, 0, psDroid->player-6, brightness, specular, pieFlag, iPieData);
				}
			}
		}
		//else if( psDroid->droidType == DROID_CYBORG)
        else if (cyborgDroid(psDroid))
		{
			/* draw body if cyborg not animating */
			if ( psDroid->psCurAnim == NULL || psDroid->psCurAnim->bVisible == FALSE )
			{
				pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
			}
		}
		else
		{
			pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData );
		}
	}

	/* get propulsion stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( (PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer") );

	/* render vtol jet if flying - horrible hack - GJ */
	if (((psPropStats->propulsionType == LIFT) &&
		//(psDroid->droidType != DROID_CYBORG)) && (!bButton))
        (!cyborgDroid(psDroid))) && (!bButton))
	{
		/* show flame if above ground */
		if ( psDroid->sMove.Status != MOVEINACTIVE )
 		{
			/* draw flame if found  */

			/* GJ TODO: add flame-finding code here */
            psJet = asBodyStats[psDroid->asBits[COMP_BODY].nStat].pFlameIMD;
			
			if ( psJet != NULL )
			{
				pie_Draw3DShape( psJet, getStaticTimeValueRange(100,psJet->numFrames), colour /*getPlayerColour(psDroid->player)*/, brightness, specular, pie_ADDITIVE, 200);
			}
		}
	}

	//don't change the screen coords of an object if drawing it in a button
	if (!bButton)
	{
		/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
		calcScreenCoords(psDroid);
	}

	/* set default components transparent */
	if ( psDroid->asWeaps[0].nStat              == 0 &&
		 psDroid->asBits[COMP_SENSOR].nStat     == 0 &&
		 psDroid->asBits[COMP_ECM].nStat        == 0 &&
		 psDroid->asBits[COMP_BRAIN].nStat      == 0 &&
		 psDroid->asBits[COMP_REPAIRUNIT].nStat == 0 &&
		 psDroid->asBits[COMP_CONSTRUCT].nStat  == 0    )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	/* Indenting here is only to show new matrix context */
	{
		psShape = BODY_IMD(psDroid,psDroid->player);
		pie_MatBegin();
		if( psShape->nconnectors )
		{
			/* vtol weapons attach to connector 2 (underneath);
			 * all others to connector 1 */
			if ( (psPropStats->propulsionType == LIFT) &&
				  psDroid->droidType == DROID_WEAPON )
			{
				iConnector = 1;
			}
			else
			{
				iConnector = 0;
			}

			/* Now we need to move for the mount point */
			pie_TRANSLATE( psShape->connectors[iConnector].x,
						   psShape->connectors[iConnector].z,
						   psShape->connectors[iConnector].y  );
	
			/* Rotate the turret */
			pie_MatRotY(DEG(mountRotation->y));
			//dont pitch the turret
//			pie_MatRotZ(DEG(mountRotation->z));
	
			/* vtol weapons inverted */
			if ( iConnector == 1 )
			{
				pie_MatRotZ( DEG_360/2 );//this might affect gun rotation
			}

		//SEPERATE Mount IMDs now...
		/*	Get the mounting graphic - we've already moved to the right position 
			Allegedly - all droids will have a mount graphic so this shouldn't
			fall on it's arse......*/
		//psShape = MOUNT_IMD(psDroid,psDroid->player);
		/* Draw it */
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_TRANSPORTER:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_WEAPON:
			case DROID_COMMAND:		// command droids have a weapon to store all the graphics
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				/* Double check that the weapon droid actually has any */
				//if(psDroid->numWeaps)
                if(psDroid->asWeaps[0].nStat > 0 OR psDroid->droidType == DROID_DEFAULT)
				{
					psShape = WEAPON_MOUNT_IMD(psDroid,psDroid->player);
					/* Draw it */
					//if(psDroid->numWeaps) already done this check above?!
					{
						pie_TRANSLATE(0,0,psDroid->asWeaps[0].recoilValue/3);
					}
					if(psShape)
					{
						pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
					}
			
					//if(psDroid->numWeaps) already done this check above?!
					{
						pie_TRANSLATE(0,0,psDroid->asWeaps[0].recoilValue);
					}

					/* translate for weapon mount point if cyborg */
					//if( psDroid->droidType == DROID_CYBORG &&
                    if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
					{
						pie_TRANSLATE( psShape->connectors[0].x,
									   psShape->connectors[0].z,
									   psShape->connectors[0].y  );
					}

					/* vtol weapons inverted */
					if ( iConnector == 1 )
					{
						//pitch the barrel down
						pie_MatRotX(DEG(-mountRotation->x));
					}
					else
					{
						//pitch the barrel up
						pie_MatRotX(DEG(mountRotation->x));
					}

					/* Get the weapon (gun?) graphic */
					psShape = WEAPON_IMD(psDroid,psDroid->player);
					/* Draw it */
					if(psShape)
					{
						pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
					}
					//we have a droid weapon so do we draw a muzzle flash
					if( psShape && psShape->nconnectors )
					{
						/* Now we need to move to the end fo the barrel */
						pie_TRANSLATE( psShape->connectors[0].x,
									   psShape->connectors[0].z,
									   psShape->connectors[0].y  );
						//and draw the muzzle flash
						//animate for the duration of the flash only
						psShape = MUZZLE_FLASH_PIE(psDroid,psDroid->player);
						if(psShape)
						{
							//assume no clan colours formuzzle effects
							if ((psShape->numFrames == 0) || (psShape->animInterval <= 0))//no anim so display one frame for a fixed time
							{
								if (gameTime < (psDroid->asWeaps->lastFired + BASE_MUZZLE_FLASH_DURATION))
								{
   								pie_Draw3DShape(psShape, 0, 0, brightness, 0, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);//muzzle flash
   //								pie_Draw3DShape(psShape, 0, 0, brightness, 0, pieFlag, 0);//muzzle flash
								}
							}
							else
							{
								frame = (gameTime - psDroid->asWeaps->lastFired)/psShape->animInterval;
								if (frame < psShape->numFrames)
								{
									pie_Draw3DShape(psShape, frame, 0, brightness, 0, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);//muzzle flash
	//								pie_Draw3DShape(psShape, frame, 0, brightness, 0, pieFlag, 0);//muzzle flash
								}
							}
						}
					}
				}
				break;

			case DROID_SENSOR:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = SENSOR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
//					pie_MatRotY(DEG(getStaticTimeValueRange(7920,360)));
//					pie_MatRotY(DEG(psDroid->turretRotation));
					pie_Draw3DShape(psShape, 0,colour/*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
				
				/* Get the sensor graphic, assuming it's there */
				psShape = SENSOR_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
//					pie_MatRotY(DEG(getStaticTimeValueRange(7920,360)));
//					pie_MatRotY(DEG(psDroid->turretRotation));
					pie_Draw3DShape(psShape, 0,colour/*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
				break;

			case DROID_CONSTRUCT:
            case DROID_CYBORG_CONSTRUCT:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = CONSTRUCT_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour/*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}

                /* translate for construct mount point if cyborg */
                if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
				{
					pie_TRANSLATE( psShape->connectors[0].x,
								   psShape->connectors[0].z,
								   psShape->connectors[0].y  );
				}

                /* Get the construct graphic assuming it's there */
				psShape = CONSTRUCT_IMD(psDroid,psDroid->player);

                /* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour/*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
		
				break;
			case DROID_ECM:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = ECM_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
				
				/* Get the ECM graphic assuming it's there.... */
				psShape = ECM_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0, colour/*getPlayerColour(psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
					break;
			case DROID_REPAIR:
            case DROID_CYBORG_REPAIR:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = REPAIR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour /*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
				}
				
                /* translate for construct mount point if cyborg */
                if (cyborgDroid(psDroid) && psShape && psShape->nconnectors)
				{
					pie_TRANSLATE( psShape->connectors[0].x,
								   psShape->connectors[0].z,
								   psShape->connectors[0].y  );
				}
				
                /* Get the Repair graphic assuming it's there.... */
                psShape = REPAIR_IMD(psDroid,psDroid->player);
	
				/* Draw it */
				if(psShape)
				{
					pie_Draw3DShape(psShape, 0,colour /*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
					if(psShape->nconnectors AND psDroid->action == DACTION_DROIDREPAIR)
					{
						pie_TRANSLATE( psShape->connectors[0].x,
									   psShape->connectors[0].z,
									   psShape->connectors[0].y  );
						pie_TRANSLATE(0,-20,0);
						
						psShape = getImdFromIndex(MI_FLAME);

						/* Rotate for droid */
						pie_MatRotY(DEG((SDWORD)psDroid->direction));
						pie_MatRotX(DEG(-psDroid->pitch));
						pie_MatRotZ(DEG(-psDroid->roll));
					   	pie_MatRotY(DEG(-mountRotation->y));

						iV_MatrixRotateY(-player.r.y);         
						iV_MatrixRotateX(-player.r.x);         
							/* Dither on software */
						if(pie_GetRenderEngine() == ENGINE_4101)
						{
							pie_SetDitherStatus(TRUE);
						}

					   	pie_Draw3DShape(psShape, getStaticTimeValueRange(100,psShape->numFrames), 0, brightness, 0, pie_ADDITIVE, 140);
					  		/* Dither off software */
						if(pie_GetRenderEngine() == ENGINE_4101)
						{
							pie_SetDitherStatus(FALSE);
						}

						iV_MatrixRotateX(player.r.x);     
						iV_MatrixRotateY(player.r.y);         
					}
				}
					break;
			case DROID_PERSON:
				// no extra mounts for people
				break;
			default:
				ASSERT((FALSE, "Whoa! Weirdy type of droid found in drawComponentObject!!!"));
				break;
			}
		}
		/*	We've also got a handle on the psShape here for the weapon which has a connector to point to
			muzzle flash attachment points - just grab it from psShape->connectors->[x|y|z] */

		/* Pop Matrix */
		pie_MatEnd();
	} // end of illustrative indentation - see above

	/* set default components transparent */
	if ( psDroid->asBits[COMP_PROPULSION].nStat == 0 )
	{
		pieFlag  |= pie_TRANSLUCENT;
		iPieData  = DEFAULT_COMPONENT_TRANSLUCENCY;
	}
	else
	{
		pieFlag  &= ~pie_TRANSLUCENT;
		iPieData = 0;
	}

	psShape = (leftFirst ? getRightPropulsionIMD(psDroid) : getLeftPropulsionIMD(psDroid));
	if(psShape!=NULL)
	{
		pie_Draw3DShape(psShape, 0,colour /*getPlayerColour( psDroid->player)*/, brightness, specular, pieFlag, iPieData);
	}


/*
	if(leftFirst)
	{
		psShape = getRightPropulsionIMD(psDroid);
		if(psShape!=NULL)
		{
			iV_PIEDraw(psShape,psDroid->player);
		}
	}
	else
	{	
		psShape = getLeftPropulsionIMD(psDroid);
		if(psShape!=NULL)
		{
			iV_PIEDraw(psShape,psDroid->player);
		}
	}
	*/
}

#else	// Start of the PSX version.

static BOOL DoLight = FALSE;
static SDWORD LightDx,LightDz;
static SVECTOR ZeroVector={0,0,0};

UWORD TransporterOTZ = 64;

/* Assumes matrix context is already set */
BOOL displayComponentObject(BASE_OBJECT *psObj)
{
	DROID		*psDroid;
	iVector		position,rotation,mountRotation;
	int32		xShift,zShift;
	UDWORD		worldAngle;
	SDWORD		difference;
	SDWORD		frame;
	SWORD		OTz;
	BOOL		Return = FALSE;

	/* Cast the droid pointer */
	psDroid = (DROID *)psObj;

	worldAngle = (UDWORD) ((UDWORD)player.r.y/DEG_1)%360;
	difference = (worldAngle-psObj->direction);

	if((difference>0 AND difference <180) OR difference<-180)
	{
		leftFirst = FALSE;
	}
	else
	{
		leftFirst = TRUE;
	}

	/* Get the real position */
	position.x = (psDroid->x - player.p.x) - terrainMidX*TILE_UNITS;
	position.z = terrainMidY*TILE_UNITS - (psDroid->y - player.p.z);
	position.y = psDroid->z;

	DoLight = TRUE;
	LightDx = position.x;
	LightDz = position.z;

	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		position.y += bobTransporterHeight();
	}

	/* Get all the pitch,roll,yaw info */
	rotation.y = -(SDWORD)psDroid->direction;
	rotation.x = psDroid->pitch;
	rotation.z = psDroid->roll;

	/* Get rotation info for the mounting too (holds the gun */
	mountRotation.y = -(SDWORD)psDroid->turretRotation;
	mountRotation.x = psDroid->turretPitch;
	mountRotation.z = 0;


// debug code to test turret angles
//	mountRotation.y = (frameGetFrameNumber())%360;		// yaw
//	mountRotation.x = 45;								// pitch
  

	if (godMode || (psDroid->visible[selectedPlayer] == UBYTE_MAX) OR demoGetStatus())
	{
		/* Push the matrix */
		psxPushMatrix();

		/* Get internal tile units coordinates */
		xShift = player.p.x & (TILE_UNITS-1);
		zShift = player.p.z & (TILE_UNITS-1);

		/* Mask out to tile_units resolution */
		psxiV_TRANSLATE(xShift,0,-zShift);
		SetBSPObjectPos(psDroid->x,-psDroid->z,psDroid->y);	// world x,y,z coord of droid - middle value is height above map
//		SetBSPObjectPos(position.x,position.y,position.z);	// world x,y,z coord of droid - middle value is height above map
		SetBSPObjectRot(GAME2PSX(psDroid->direction),0);								// Droid rotation
		/* Translate origin */
		psxiV_TRANSLATE(position.x,-position.y,position.z);

		/* Rotate for droid */
		psxiV_RotateYMatrix(-DEG(rotation.y));
		psxiV_RotateXMatrix(-DEG(rotation.x));
		psxiV_RotateZMatrix(-DEG(rotation.z));

		scaleMatrix(DROID_SCALE);

		psxUseMatrix();

		psxiv_SetZBias(-4);
		gte_ldv0(&ZeroVector);
    	gte_rtps();
		gte_stszotz(&OTz);

		if(psDroid->droidType == DROID_TRANSPORTER) {
//			SetIMDRenderingMode(USE_MAXZ,0);
//			SetIMDRenderingMode(USE_MINZ,0);
			TransporterOTZ = OTz-32;
			SetIMDRenderingMode(USE_FIXEDZ,OTz-32);
			psxiv_SetIMDZ(OTz-32);
		} else {
			OTz -= 8;
			SetIMDRenderingMode(USE_FIXEDZ,OTz);
			psxiv_SetIMDZ(OTz);
		}

		Return = displayCompObj(psObj,&mountRotation, FALSE);

		SetIMDRenderingMode(USE_MAXZ,0);
//		psxiv_SetZBias(-4);
		psxPopMatrix();
	}
	else
	{
//		frame = (gameTime/(3*UBYTE_MAX - psDroid->visible[selectedPlayer]));
		frame = gameTime/BLIP_ANIM_DURATION + psDroid->id; //visible[selectedPlayer];
		rendMultiPIERot(getImdFromIndex(MI_BLIP),frame,&position,&rotation,NULL,100,NULL);

		/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
		calcScreenCoords(psDroid);
	}

	return Return;
}


extern UBYTE IMDdebug;

#define NUM_LIGHTJETS	3
#define NUM_MEDIUMJETS	3
#define NUM_HEAVYJETS	5

iVector vtolLightJetOffsets[NUM_LIGHTJETS] = {
	{-25,0,12},
	{25,0,12},
	{0,-14,36},
};

iVector vtolMediumJetOffsets[NUM_MEDIUMJETS] = {
	{-39,0,7},
	{39,0,7},
	{0,-18,67},
};

iVector vtolHeavyJetOffsets[NUM_HEAVYJETS] = {
	{-45,0,24},
	{45,0,24},
	{-37,0,-23},
	{37,0,-23},
	{0,-31,67},
};

extern void rend2DEffect(iIMDShape *Shape,UWORD frameNumber,iVector *Offset,UDWORD Scale,PIE *Params);
extern void GetMeshRGB(UDWORD xPos,UDWORD zPos,UBYTE *DstRGB);


void compSetTransMode(BOOL Transparent,UWORD TransMode)
{
	ComponentTransparent = Transparent;
	ComponentTransMode = TransMode;
}


#if(1)

extern QUAD dragQuad;


// Render a shadow for a ground unit.
//
void displayShadow(void)
{
	// Draw a shadow for the droid based on the size of the body
	if (ShadowSizeZ>10) {
	 	DrawShadow(ShadowSizeX,ShadowSizeZ);
	}
}


// Render a shadow for a vtol.
//
void displayVTolShadow(void)
{
}


// Render a shadow for a transporter.
//
void displayTransporterShadow(void)
{
}


// Render Vtol jet exhaust effects.
//
void displayVtolJets(DROID *psDroid)
{
	iIMDShape	*psJet;
	iVector 	*JetOffsets;
	UWORD 		NumJets,i;

	/* show flame if above ground */
	if(!bInButton) {
		if(psDroid->sMove.Status != MOVEINACTIVE) {
			psJet = NULL;
			JetOffsets = NULL;
/*
			switch ( (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->size )
			{
				case SIZE_LIGHT:
					psJet = getImdFromIndex(MI_VTOL_LIGHT);
					JetOffsets = vtolLightJetOffsets;
					NumJets = NUM_LIGHTJETS;
					break;
				case SIZE_MEDIUM:
					psJet = getImdFromIndex(MI_VTOL_MEDIUM);
					JetOffsets = vtolMediumJetOffsets;
					NumJets = NUM_MEDIUMJETS;
					break;
				case SIZE_HEAVY:
					psJet = getImdFromIndex(MI_VTOL_HEAVY);
					JetOffsets = vtolHeavyJetOffsets;
					NumJets = NUM_HEAVYJETS;
					break;
			}
*/
			/* draw flame if found  */
			if ( psJet != NULL )
			{
				static PIE PieParams;

				PieParams.Flags = PIE_COLOURED | PIE_TRANSPARENT | PIE_OTZ;
				PieParams.ColourRGB[0] = 
				PieParams.ColourRGB[1] = 
				PieParams.ColourRGB[2] = 64;
				PieParams.TransMode = TRANSMODE_ADDITIVE;
				PieParams.OtZ = psxiv_GetIMDZ()+2;
				for(i=0; i<NumJets; i++) {
					rend2DEffect(psJet,rand()%IMDGetNumFrames(psJet),&JetOffsets[i],50,&PieParams);
				}
			}
		}
	}
}


// Render units propulsion.
//
void displayPropulsionFirst(DROID *psDroid)
{
	iIMDShape	*psShape;

	psShape = (leftFirst ? getRightPropulsionIMD(psDroid) : getLeftPropulsionIMD(psDroid));
	if(psShape != NULL) {
		if(psShape->xmax>ShadowSizeX) ShadowSizeX = psShape->xmax;
		if(psShape->zmax>ShadowSizeZ) ShadowSizeZ = psShape->zmax;
		DrawPie(psShape,&PieParams);
	}
}


// Render units propulsion.
//
void displayPropulsionLast(DROID *psDroid)
{
	iIMDShape	*psShape;

	psShape = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if(psShape != NULL) {
		if(psShape->xmax>ShadowSizeX) ShadowSizeX = psShape->xmax;
		if(psShape->zmax>ShadowSizeZ) ShadowSizeZ = psShape->zmax;
		DrawPie(psShape,&PieParams);
	}
}


// Render units body.
//
void displayBody(DROID *psDroid)
{
	iIMDShape	*psShape;

	// Draw the body.
	psShape = BODY_IMD(psDroid,psDroid->player);
	if(psShape != NULL) {
		if(psShape->xmax>ShadowSizeX) ShadowSizeX = psShape->xmax;
		if(psShape->zmax>ShadowSizeZ) ShadowSizeZ = psShape->zmax;
		DrawPie(psShape,&PieParams);
	}
}


// Render units turret.
//
void displayTurret(DROID *psDroid,iVector *mountRotation,iIMDShape *psMount,iIMDShape *psTool)
{
	iIMDShape	*psShape;

	psxPushMatrix();

	psShape = BODY_IMD(psDroid,psDroid->player);

	// If no connectors on the body then just return.
	if(psShape->nconnectors == 0) {
		return;
	}

	if(psShape->nconnectors) {
		psxiV_TRANSLATE(psShape->connectors->x,-psShape->connectors->z,psShape->connectors->y);
		psxiV_RotateYMatrix(-DEG(mountRotation->y));
		psxiV_RotateXMatrix(-DEG(mountRotation->x));
	}

	if(!bInButton) {
		// If it's a sensor then rotate the turret.
		if( (!gameTimeIsStopped()) && (psDroid->droidType == DROID_SENSOR) ) {
			psxiV_RotateYMatrix(DEG(getStaticTimeValueRange(7920,360)));
		}
	}

	psxUseMatrix();

	if(psTool) {
		DrawPie(psTool,&PieParams);
	}

	if(psMount)	{
		DrawPie(psMount,&PieParams);
	}

	psxPopMatrix();
	psxUseMatrix();
}


// Render a units turret with a weapon and any relevant recoil and muzzle flash.
//
void displayWeaponTurret(DROID *psDroid,iVector *mountRotation,
						iIMDShape *psMount,iIMDShape *psWeapon,UWORD Connector)
{
	iIMDShape	*psShape;

	psShape = BODY_IMD(psDroid,psDroid->player);

	// If no connectors on the body then just return.
	if(psShape->nconnectors == 0) {
		return;
	}

	psxPushMatrix();

	if(Connector > psShape->nconnectors-1) {
		DBPRINTF(("displayWeaponTurret : Bad connector %d %d\n",Connector,psShape->nconnectors));
		Connector = 0;
	}

	if(psShape->nconnectors) {
		psxiV_TRANSLATE(psShape->connectors[Connector].x,
						-psShape->connectors[Connector].z,
						psShape->connectors[Connector].y);

		psxiV_RotateYMatrix(-DEG(mountRotation->y));		// Turret direction.
//		psxiV_RotateXMatrix(-DEG(mountRotation->x));		// Turret pitch.

		// If using second connector then assume it's a VTOL and invert the mount
		if(Connector > 0) {
			psxiV_RotateZMatrix( DEG_360/2 );
		}
	}

	// Push the mount's matrix, now gets drawn at the end which is better for sorting.
	psxPushMatrix();

	// Do recoil.
	if(!bInButton) {
		if( (psDroid->droidType == DROID_WEAPON) && psDroid->asWeaps[0].nStat > 0 ) {
			psxiV_TRANSLATE(0,0,(psDroid->asWeaps[0].recoilValue*DROID_SCALE)/100);
		}
	}

	// Barrel pitch.
	psxiV_RotateXMatrix(-DEG(mountRotation->x));

	psxUseMatrix();

	// Draw the weapon.
	if(psWeapon) {
		// Translate for weapon mount point if cyborg.
		if( (psDroid->droidType == DROID_CYBORG ||
			 psDroid->droidType == DROID_CYBORG_SUPER) &&
			 psMount->nconnectors )
		{
				psxiV_TRANSLATE(psMount->connectors[0].x,
								-psMount->connectors[0].z,
								psMount->connectors[0].y);
				psxUseMatrix();
		}

		DrawPie(psWeapon,&PieParams);

		// Draw muzzle flash if needed.
		if( (!bInButton) && psWeapon->nconnectors) {
			iIMDShape *psFlashShape=NULL;
/*
			psFlashShape = getImdFromIndex(MI_VTOL_LIGHT);
*/
			if(psFlashShape)
			{
				UDWORD AnimInt;
				SDWORD frame;

				AnimInt= IMDGetAnimInterval(psFlashShape);
				if (AnimInt==0) AnimInt=1;	// Dummy Value !!!!

				frame = (gameTime - psDroid->asWeaps->lastFired)/AnimInt;
				if (frame < IMDGetNumFrames(psFlashShape))
				{
					PIE PieParams;

					/* Now we need to move to the end of the barrel */
					psxiV_TRANSLATE(psWeapon->connectors[0].x,
									-psWeapon->connectors[0].z,
									psWeapon->connectors[0].y);
					psxUseMatrix();

					PieParams.Flags = PIE_COLOURED | PIE_TRANSPARENT | PIE_OTZ;
					PieParams.ColourRGB[0] = 128;
					PieParams.ColourRGB[1] = 128;
					PieParams.ColourRGB[2] = 0;
					PieParams.TransMode = TRANSMODE_ADDITIVE;
					PieParams.OtZ = psxiv_GetIMDZ();
					rend2DEffect(psFlashShape,frame%AnimInt,NULL,50,&PieParams);
				}
			}
		}
	}

	psxPopMatrix();
	psxUseMatrix();

	// Draw the mount.
	if(psMount)	{
		DrawPie(psMount,&PieParams);
	}

	psxPopMatrix();
	psxUseMatrix();
}


UDWORD Reject0;


void ClearSelectedThisFrame(void)
{
	SelectedThisFrame = 0;
}

// Update a droids screen info and check for drag box selection.
// Returns TRUE if on screen.
//
BOOL updateDroidScreenInfo(DROID *psDroid)
{
	SDWORD dummyZ;
	UWORD ScrCoord[2];
	POINT pt;

	// If not being rendered in a button then update it's screen coords and
	// check for drag box selection.
	if(!bInButton) {
		// Get the objects screen x,y and z.
		gte_ldv0(&ZeroVector);
	   	gte_rtps();
		gte_stsxy((long *)&ScrCoord[0]);
		gte_stszotz(&dummyZ);

		psDroid->sDisplay.screenX = PSXToX(ScrCoord[0]);
		psDroid->sDisplay.screenY = PSXToY(ScrCoord[1]);
		psDroid->sDisplay.screenR = 22;

		// Is it on screen?
		if( (ScrCoord[0] > ((UWORD)GetDisplayWidth())) ||
			(ScrCoord[1] > ((UWORD)GetDisplayHeight())) ) {
			Reject0++;
			return FALSE;
		}

		// Don't do selection check if it's a transporter
		if(psDroid->droidType == DROID_TRANSPORTER) {
			return TRUE;
		}

		if(psDroid->player == selectedPlayer) {
			if(SelectedThisFrame < MAX_GROUP_SIZE) {
				if(dragBox3D.status == DRAG_RELEASED) {
					pt.x = psDroid->sDisplay.screenX;
					pt.y = psDroid->sDisplay.screenY;
					if(inQuad(&pt,&dragQuad) AND psDroid->player == selectedPlayer) {
	//					psDroid->selected = TRUE;
						SelectDroid(psDroid);
						SelectedThisFrame++;
					}
				}
			}
		}
	}

	return TRUE;
}


// Render a normal unit, ie weapon droid, construction droid etc.
//
BOOL displayDefaultUnit(DROID *psDroid,iVector *mountRotation,UBYTE PropType)
{
	iIMDShape *psShape;
	iIMDShape *psMount;
	iIMDShape *psTool;

//	if(!updateDroidScreenInfo(psDroid)) {
//		return FALSE;
//	}

	// Render mounts,weapons etc.
	switch(psDroid->droidType) {
		case DROID_CONSTRUCT:	// Constructor droid
			// Draw turret and tipper thing..
			displayTurret(psDroid,mountRotation,
							CONSTRUCT_MOUNT_IMD(psDroid,psDroid->player),
							CONSTRUCT_IMD(psDroid,psDroid->player));
			break;

		case DROID_DEFAULT:		// Default droid
		case DROID_COMMAND:		// Command droid
		case DROID_WEAPON:		// Weapon droid
			// Draw turret and weapon.
			displayWeaponTurret(psDroid,mountRotation,
							WEAPON_MOUNT_IMD(psDroid,psDroid->player),
							WEAPON_IMD(psDroid,psDroid->player),0);
			break;

		case DROID_SENSOR:		// Sensor droid
			// Draw turret and sensor.
			displayTurret(psDroid,mountRotation,
							SENSOR_MOUNT_IMD(psDroid,psDroid->player),
							SENSOR_IMD(psDroid,psDroid->player));
			break;

		case DROID_ECM:			// ECM droid
			// Draw turret and ecm.
			displayTurret(psDroid,mountRotation,
							ECM_MOUNT_IMD(psDroid,psDroid->player),
							ECM_IMD(psDroid,psDroid->player));
			break;

		case DROID_REPAIR:		// Repair droid
			// Draw turret and repairer.
			displayTurret(psDroid,mountRotation,
							REPAIR_MOUNT_IMD(psDroid,psDroid->player),
							REPAIR_IMD(psDroid,psDroid->player));
			break;

		default:
			DBPRINTF(("displayDefaultUnit : Bad droid type %d\n",psDroid->droidType));
	}

	// Render body and propulsion. Shuffle the order of body and propulsion
	// if it's a hover droid so it still sorts ok.
	if(PropType == HOVER) {
		// Draw the body.
		displayBody(psDroid);

		// Draw first propulsion.
		displayPropulsionFirst(psDroid);

		// Draw second propulsion.
		displayPropulsionLast(psDroid);
	} else {
		// Draw first propulsion.
		displayPropulsionFirst(psDroid);

		// Draw the body.
		displayBody(psDroid);

		// Draw second propulsion.
		displayPropulsionLast(psDroid);

		displayShadow();
	}

	return TRUE;
}


// Render a VTOL unit
//
BOOL displayVTolUnit(DROID *psDroid,iVector *mountRotation)
{
	iIMDShape *psShape;

//	if(!updateDroidScreenInfo(psDroid)) {
//		return FALSE;
//	}

	// Draw first propulsion.
	displayPropulsionFirst(psDroid);

	// Draw the body.
	displayBody(psDroid);

	// Draw second propulsion.
	displayPropulsionLast(psDroid);

	// Draw the jet exhaust effects.
	displayVtolJets(psDroid);

	// Render mounts,weapons etc.
	switch(psDroid->droidType) {
		case DROID_WEAPON:		// Weapon droid
			// Draw turret and weapon.
			displayWeaponTurret(psDroid,mountRotation,
							WEAPON_MOUNT_IMD(psDroid,psDroid->player),
							WEAPON_IMD(psDroid,psDroid->player),1);
			// Draw the vtol's shadow.
			displayVTolShadow();
			break;

		case DROID_TRANSPORTER:	// Transporter
			// Draw turret and weapon.
			displayWeaponTurret(psDroid,mountRotation,
							WEAPON_MOUNT_IMD(psDroid,psDroid->player),
							WEAPON_IMD(psDroid,psDroid->player),1);
			// Draw the transporter's shadow.
			displayTransporterShadow();
			break;

		default:
			DBPRINTF(("displayVTolUnit : Bad droid type %d\n",psDroid->droidType));
	}

	return TRUE;
}


// Render a persion unit
//
BOOL displayPersonUnit(DROID *psDroid,iVector *mountRotation)
{
	iIMDShape	*psShape;

//	if(!updateDroidScreenInfo(psDroid)) {
//		return FALSE;
//	}

	psShape = BODY_IMD(psDroid,psDroid->player);

	if(psShape != NULL) {
		// Draw body if not animating.
		if( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == FALSE )
		{
			DrawPie(psShape,&PieParams);
		}
	}
	
	return TRUE;
}


// Render a cyborgn unit
//
BOOL displayCyborgUnit(DROID *psDroid,iVector *mountRotation,UBYTE PropType)
{
	iIMDShape	*psShape;

//	if(!updateDroidScreenInfo(psDroid)) {
//		return FALSE;
//	}

	psShape = BODY_IMD(psDroid,psDroid->player);

	if(psShape != NULL) {
		// Problem with jumping cyborgs, could just render weapon without the mount, but really
		// needs to be fixed in stats\pies, why don't we just scrap them since there useless anyway.
		if(PropType == JUMP) {
		} else {
			// Draw turret and weapon.
			displayWeaponTurret(psDroid,mountRotation,
							WEAPON_MOUNT_IMD(psDroid,psDroid->player),
							WEAPON_IMD(psDroid,psDroid->player),0);
		}

		// Draw body if not animating.
		if( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == FALSE ) {
			DrawPie(psShape,&PieParams);
		}
	}

	return TRUE;
}


BOOL displayCompObj(BASE_OBJECT *psObj,iVector *mountRotation, BOOL bButton)
{
	DROID *psDroid = (DROID*)psObj;
	PROPULSION_STATS *psPropStats;
	UDWORD Brightness;

	bInButton = bButton;

	if(!updateDroidScreenInfo(psDroid)) {
		return FALSE;
	}

	// Setup unit lighting.
	if(DoLight && !bInButton) {
		PieParams.Flags = PIE_PALETTEID | PIE_COLOURED | PIE_CULLBLACK;
		Brightness = GetObjectMapBrightness(psObj);
		PieParams.ColourRGB[0] = 
		PieParams.ColourRGB[1] = 
		PieParams.ColourRGB[2] = DoIllumination(Brightness,LightDx,LightDz);
		DoLight = FALSE;
	} else {
		PieParams.Flags = PIE_PALETTEID;
	}

	// Was transparency set.
	if(ComponentTransparent) {
		PieParams.Flags |= PIE_TRANSPARENT;
		PieParams.TransMode = ComponentTransMode;
	}

	// Set the palette ID from the player ID.
	PieParams.PaletteID = getPlayerColour(psDroid->player);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

	// Reset shadow size.
	ShadowSizeX = 0;
	ShadowSizeZ = 0;
	// And default to shadow enabled.
	bDrawShadow = TRUE;

	switch(psDroid->droidType) {
		case DROID_WEAPON:		// Weapon droid
		case DROID_SENSOR:		// Sensor droid
		case DROID_ECM:			// ECM droid
		case DROID_CONSTRUCT:	// Constructor droid
		case DROID_COMMAND:		// Command droid
		case DROID_REPAIR:		// Repair droid
		case DROID_DEFAULT:		// Default droid
			if(psPropStats->propulsionType == LIFT) {
				return displayVTolUnit(psDroid,mountRotation);
			} else {
				return displayDefaultUnit(psDroid,mountRotation,psPropStats->propulsionType);
			}
			break;

		case DROID_TRANSPORTER:		// Transporter
			return displayVTolUnit(psDroid,mountRotation);
			break;

		case DROID_PERSON:		// Person
			return displayPersonUnit(psDroid,mountRotation);
			break;

		case DROID_CYBORG:		// Cyborg
        case DROID_CYBORG_CONSTRUCT:
        case DROID_CYBORG_SUPER:
        case DROID_CYBORG_REPAIR:
			return displayCyborgUnit(psDroid,mountRotation,psPropStats->propulsionType);
			break;

		default:
			DBPRINTF(("displayCompObj : Unknown Droid Type\n"));
	}
}

#else

/* Assumes matrix context is already set - PSX Version */
void displayCompObj(BASE_OBJECT *psObj,iVector *mountRotation, BOOL bButton)
{
	DROID		*psDroid;
	iIMDShape	*psShape;
//	iIMDShape	*psShape1;
//	iVector		position,rotation;	//,null;	//,mountRotation,null;
//	iPoint		screenCoords;
	SDWORD		dummyZ, iConnector;
//	int32		xShift,zShift;
//	UDWORD		worldAngle;
//	SDWORD		difference;
	PROPULSION_STATS	*psPropStats;
	iIMDShape	*psJet;
	iVector 	*JetOffsets;
	UWORD 		NumJets,i;
	SDWORD		frame;
	UDWORD		Brightness;

	static PIE PieParams;
	SDWORD ShadowSizeX=0;
	SDWORD ShadowSizeZ=0;
	BOOL bDrawShadow=TRUE;

	if (bButton==TRUE) bDrawShadow=FALSE;

	/* Cast the droid pointer */
	psDroid = (DROID *)psObj;

	if(DoLight) {
#ifdef USEMESHRGB
		UBYTE MeshRGB[3];

		PieParams.Flags = PIE_PALETTEID | PIE_COLOURED | PIE_CULLBLACK;
		GetMeshRGB(psDroid->x,psDroid->y,MeshRGB);
		DoIlluminationRGB(MeshRGB,PieParams.ColourRGB,LightDx,LightDz);
#else
		PieParams.Flags = PIE_PALETTEID | PIE_COLOURED | PIE_CULLBLACK;
		Brightness = GetObjectMapBrightness(psObj);
		PieParams.ColourRGB[0] = 
		PieParams.ColourRGB[1] = 
		PieParams.ColourRGB[2] = DoIllumination(Brightness,LightDx,LightDz);
#endif
	} else {
		PieParams.Flags = PIE_PALETTEID;
	}
	if(ComponentTransparent) {
		PieParams.Flags |= PIE_TRANSPARENT;
		PieParams.TransMode = ComponentTransMode;
	}
	PieParams.PaletteID = psDroid->player;
	DoLight = FALSE;

//	/* No auxilliary rotation */
//	null.x = null.y = null.z = 0;

	// Check to see if droid data is set up ... if all zero then it's probably from the button rendering stuff 
	// and so alot of the droid data is not set up ... we need to skip the screen coord stuff at least!
	
	//don't change the screen coords of an object if drawing it in a button
//	please check this - AB 27/05/98
	if (!bButton)
	{
		if ((psDroid->x | psDroid->y | psDroid->z)!= 0)
		{
			/* set up all the screen coords stuff - need to REMOVE FROM THIS LOOP */
			calcScreenCoords(psDroid);
		}
	}


	/* We've got a z value here _and_ screen coords of origin */
//	dummyZ = psxiV_RotateProject(&null,&screenCoords);
	gte_ldv0(&ZeroVector);
   	gte_rtps();
	gte_stszotz(&dummyZ);

	/* Draw the propulsion and body imds here */
	/* Establish the propulsion - this is more complex if two parts */

	/* get propulsion stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( (PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer") );

	/* render vtol jet if flying - horrible hack - GJ */
	if (((psPropStats->propulsionType == LIFT) &&
		//(psDroid->droidType != DROID_CYBORG)) && (!bButton))
        (!cyborgDroid(psDroid)) && (!bButton))
	{

		bDrawShadow=FALSE;

		/* show flame if above ground */
		if ( psDroid->sMove.Status != MOVEINACTIVE )
 		{
			psJet = NULL;
			JetOffsets = NULL;
/*
			switch ( (asBodyStats + psDroid->asBits[COMP_BODY].nStat)->size )
			{
				case SIZE_LIGHT:
					psJet = getImdFromIndex(MI_VTOL_LIGHT);
					JetOffsets = vtolLightJetOffsets;
					NumJets = NUM_LIGHTJETS;
					break;
				case SIZE_MEDIUM:
					psJet = getImdFromIndex(MI_VTOL_MEDIUM);
					JetOffsets = vtolMediumJetOffsets;
					NumJets = NUM_MEDIUMJETS;
					break;
				case SIZE_HEAVY:
					psJet = getImdFromIndex(MI_VTOL_HEAVY);
					JetOffsets = vtolHeavyJetOffsets;
					NumJets = NUM_HEAVYJETS;
					break;
			}
*/
			/* draw flame if found  */
			if ( psJet != NULL )
			{
				static PIE PieParams;

				PieParams.Flags = PIE_COLOURED | PIE_TRANSPARENT | PIE_OTZ;
				PieParams.ColourRGB[0] = 
				PieParams.ColourRGB[1] = 
				PieParams.ColourRGB[2] = 64;
				PieParams.TransMode = TRANSMODE_ADDITIVE;
				PieParams.OtZ = psxiv_GetIMDZ()+2;
				for(i=0; i<NumJets; i++) {
					rend2DEffect(psJet,rand()%IMDGetNumFrames(psJet),&JetOffsets[i],50,&PieParams);
				}
			}
		}
	}

	// For the constructor droid we always draw the constructiopn spade first (and hence on top of all other graphics)
	//
	// What used to happed was the propulsions were being drawn on top of the side of the spade. This will fix it
	if (psDroid->droidType==DROID_CONSTRUCT)
	{
		psxPushMatrix();

// Get into the context of the body connectors - horrible, this all need rethinking
		psShape = BODY_IMD(psDroid,psDroid->player);
		if(psShape->nconnectors)
		{
			psxiV_TRANSLATE(psShape->connectors->x,-psShape->connectors->z,psShape->connectors->y);

			psxiV_RotateYMatrix(-DEG(mountRotation->y));
			psxiV_RotateXMatrix(-DEG(mountRotation->x));
		}

		psxUseMatrix();

		/*	Get the mounting graphic - we've already moved to the right position 
		Allegedly - all droids will have a mount graphic so this shouldn't
		fall on it's arse......*/
		psShape = CONSTRUCT_MOUNT_IMD(psDroid,psDroid->player);
		/* Draw it */
		if(psShape)	DrawPie(psShape,&PieParams);
		
		/* Get the construct graphic assuming it's there */
		psShape = CONSTRUCT_IMD(psDroid,psDroid->player);
		/* Draw it */
		if(psShape)	DrawPie(psShape,&PieParams);

		psxPopMatrix();
		psxUseMatrix();

	}


// moved to the end for the mo...
	psShape = (leftFirst ? getRightPropulsionIMD(psDroid) : getLeftPropulsionIMD(psDroid));
	if(psShape!=NULL)
	{
		if (psShape->xmax>ShadowSizeX) ShadowSizeX=psShape->xmax;
		if (psShape->zmax>ShadowSizeZ) ShadowSizeZ=psShape->zmax;
		DrawPie(psShape,&PieParams);
	}

	/* Indenting here is only to show new matrix context */
	{
		psShape = BODY_IMD(psDroid,psDroid->player);

		psxPushMatrix();

 
		/* vtol weapons attach to connector 2 (underneath);
		 * all others to connector 1 */
		if ( (psPropStats->propulsionType == LIFT) &&
			  psDroid->droidType == DROID_WEAPON )
		{
			iConnector = 1;
		}
		else
		{
			iConnector = 0;
		}

//		iV_MatrixBegin();
#ifdef NO_VTOL_TURRETS
		if((psShape->nconnectors) && (iConnector == 0))
#else
		if(psShape->nconnectors)
#endif
		{

			psxiV_TRANSLATE(psShape->connectors[iConnector].x,
							-psShape->connectors[iConnector].z,
							psShape->connectors[iConnector].y);

			psxiV_RotateYMatrix(-DEG(mountRotation->y));
//			psxiV_RotateXMatrix(-DEG(mountRotation->x));
//			psxiV_RotateZMatrix(-iV_DEG(mountRotation->z));

//			/* vtol weapons inverted */
//			if ( iConnector == 1 )
//			{
//				//pitch the barrel down
//				psxiV_RotateYMatrix(-DEG(-mountRotation->x));
//			}
//			else
//			{
//				//pitch the barrel up
//				psxiV_RotateYMatrix(-DEG(mountRotation->x));
//			}
//
//			/* vtol weapons inverted */
//			if ( iConnector == 1 )
//			{
//				psxiV_RotateZMatrix( DEG_360/2 );//this might affect gun rotation
//			}

			//if( (psDroid->droidType == DROID_WEAPON) && psDroid->numWeaps ) {
            if( (psDroid->droidType == DROID_WEAPON) && psDroid->asWeaps[0].nStat > 0 ) {
				psxiV_TRANSLATE(0,0,(psDroid->asWeaps[0].recoilValue*DROID_SCALE)/100);
			}

			psxUseMatrix();


		//SEPERATE Mount IMDs now...
		/*	Get the mounting graphic - we've already moved to the right position 
			Allegedly - all droids will have a mount graphic so this shouldn't
			fall on it's arse......*/
		//psShape = MOUNT_IMD(psDroid,psDroid->player);
		/* Draw it */
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_TRANSPORTER:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_WEAPON:
			case DROID_COMMAND:		// command droids have a weapon to store all the graphics
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
                if(psDroid->asWeaps[0].nStat > 0)
				{
					psShape = WEAPON_MOUNT_IMD(psDroid,psDroid->player);
					/* Draw it */
					if(psShape)
					{
//						if( psDroid->droidType != DROID_CYBORG) {
							DrawPie(psShape,&PieParams);
//						}
					}
								
					/* vtol weapons inverted */
					if ( iConnector == 1 )
					{
						//pitch the barrel down
						psxiV_RotateYMatrix(-DEG(-mountRotation->x));
						psxiV_RotateZMatrix( DEG_360/2 );//this might affect gun rotation
					}
					else
					{
						//pitch the barrel up
						psxiV_RotateYMatrix(-DEG(mountRotation->x));
					}

					/* translate for weapon mount point if cyborg */
					//if( psDroid->droidType == DROID_CYBORG &&
                    if (cyborgDroid(psDroid) &&
						psShape && psShape->nconnectors )
					{
						psxiV_TRANSLATE(psShape->connectors[0].x,
										-psShape->connectors[0].z,
										psShape->connectors[0].y);
					}

					psxUseMatrix();

					/* Get the weapon (gun?) graphic */
					psShape = WEAPON_IMD(psDroid,psDroid->player);
					/* Draw it */
					if(psShape)	{
						DrawPie(psShape,&PieParams);
					}			

					if( psShape && psShape->nconnectors )
					{
						iIMDShape *psFlashShape;

//						psxPushMatrix();
//
//						/* Now we need to move to the end fo the barrel */
//						psxiV_TRANSLATE(psShape->connectors[0].x,
//										-psShape->connectors[0].z,
//										psShape->connectors[0].y);
//						psxUseMatrix();
						//and draw the muzzle flash
						//animate for the duration of the flash only
						psFlashShape = getImdFromIndex(MI_VTOL_LIGHT);	//MUZZLE_FLASH_PIE(psDroid,psDroid->player);
						if(psFlashShape)
						{

							UDWORD AnimInt;

							AnimInt= IMDGetAnimInterval(psFlashShape);

							if (AnimInt==0) AnimInt=1;	// Dummy Value !!!!

							frame = (gameTime - psDroid->asWeaps->lastFired)/AnimInt;
							if (frame < IMDGetNumFrames(psFlashShape))
							{
								PIE PieParams;

								psxPushMatrix();

								/* Now we need to move to the end of the barrel */
								psxiV_TRANSLATE(psShape->connectors[0].x,
												-psShape->connectors[0].z,
												psShape->connectors[0].y);
								psxUseMatrix();

								PieParams.Flags = PIE_COLOURED | PIE_TRANSPARENT | PIE_OTZ;
								PieParams.ColourRGB[0] = 128;
								PieParams.ColourRGB[1] = 128;
								PieParams.ColourRGB[2] = 0;
								PieParams.TransMode = TRANSMODE_ADDITIVE;
								PieParams.OtZ = psxiv_GetIMDZ();
								rend2DEffect(psFlashShape,frame%AnimInt,NULL,50,&PieParams);

								psxPopMatrix();
								psxUseMatrix();
							}
						}

//						psxPopMatrix();
//						psxUseMatrix();
					}
				}
				break;

			case DROID_SENSOR:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = SENSOR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	{
					DrawPie(psShape,&PieParams);
				}			

				
				/* Get the sensor graphic, assuming it's there */
				psShape = SENSOR_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape) 				{
					DrawPie(psShape,&PieParams);
				}			

				break;

			case DROID_CONSTRUCT:
            case DROID_CYBORG_CONSTRUCT:
#if(0)	// now moved so that the spade is drawn first !
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = CONSTRUCT_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	
				{
					DrawPie(psShape,&PieParams);
				}
				
				/* Get the construct graphic assuming it's there */
				psShape = CONSTRUCT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)
				{
					DrawPie(psShape,&PieParams);
				}			
#endif		
				break;
			case DROID_ECM:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = ECM_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	
				{
					DrawPie(psShape,&PieParams);
				}			

				
				/* Get the ECM graphic assuming it's there.... */
				psShape = ECM_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	{
					DrawPie(psShape,&PieParams);
				}			

  				break;
			case DROID_REPAIR:
            case DROID_CYBORG_REPAIR:
				/*	Get the mounting graphic - we've already moved to the right position 
				Allegedly - all droids will have a mount graphic so this shouldn't
				fall on it's arse......*/
				psShape = REPAIR_MOUNT_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	
				{
					DrawPie(psShape,&PieParams);
				}			

				
				/* Get the Repair graphic assuming it's there.... */
				psShape = REPAIR_IMD(psDroid,psDroid->player);
				/* Draw it */
				if(psShape)	{
					DrawPie(psShape,&PieParams);
				}			

  				break;
			default:
				DBERROR(("Whoa! Weirdy type of droid found in drawComponentObject!!!"));
				break;
			}
		}
		/*	We've also got a handle on the psShape here for the weapon which has a connector to point to
			muzzle flash attachment points - just grab it from psShape->connectors->[x|y|z] */
		/* Pop Matrix */
		psxPopMatrix();
	} // end of illustrative indentation - see above

	psxUseMatrix();





	/* Get the body graphic now*/
	psShape = BODY_IMD(psDroid,psDroid->player);

	if(psShape!=NULL)
	{
		if (psShape->xmax>ShadowSizeX) ShadowSizeX=psShape->xmax;
		if (psShape->zmax>ShadowSizeZ) ShadowSizeZ=psShape->zmax;

		if ( psDroid->droidType == DROID_PERSON)
		{
			/* draw body if not animating */
			if ( psDroid->psCurAnim == NULL  || psDroid->psCurAnim->bVisible == FALSE )
			{
//				scaleMatrix(75);
				DrawPie(psShape,&PieParams);
//				pie_Draw3DShape(psShape, 0, psDroid->player-6, brightness, specular, pieFlag | pie_NO_BILINEAR, iPieData);
			}
		}
		//else if( psDroid->droidType == DROID_CYBORG)
        else if (cyborgDroid(psDroid))
		{

			if ( psDroid->psCurAnim != NULL && psDroid->psCurAnim->bVisible != FALSE )
//			if ( psDroid->psCurAnim == NULL || psDroid->psCurAnim->bVisible == FALSE )
			{
				DrawPie(psShape,&PieParams);
//				pie_Draw3DShape(psShape, 0, getPlayerColour(psDroid->player), brightness, specular, pieFlag | pie_NO_BILINEAR, iPieData);
			}
		}
		else
		{
			DrawPie(psShape,&PieParams);
		}
//		if (psShape->xmax>ShadowSizeX) ShadowSizeX=psShape->xmax;
//		if (psShape->zmax>ShadowSizeZ) ShadowSizeZ=psShape->zmax;
//		DrawPie(psShape,&PieParams);
	}

	psShape = (leftFirst ? getLeftPropulsionIMD(psDroid) : getRightPropulsionIMD(psDroid));
	if(psShape!=NULL)
	{
		if (psShape->xmax>ShadowSizeX) ShadowSizeX=psShape->xmax;
		if (psShape->zmax>ShadowSizeZ) ShadowSizeZ=psShape->zmax;
		DrawPie(psShape,&PieParams);
	}

	// Now draw a shadow for the droid based on the size of the body
	if (bDrawShadow==TRUE)
	{
		if (ShadowSizeZ>10)
		 	DrawShadow(ShadowSizeX,ShadowSizeZ);
	}

}
#endif

#endif	// End of PSX version.


void	destroyFXDroid(DROID	*psDroid)
{
UDWORD	i;
iIMDShape	*psImd;
SDWORD	widthScatter,breadthScatter,heightScatter;
iVector	pos;

 	widthScatter = TILE_UNITS/4;
	breadthScatter = TILE_UNITS/4;
	heightScatter = TILE_UNITS/5;
	for(i=0; i<5; i++)
	{
		pos.x = psDroid->x + widthScatter - rand()%(2*widthScatter);
		pos.z = psDroid->y + breadthScatter - rand()%(2*breadthScatter);
		pos.y = psDroid->z + 16 +heightScatter;
		switch(i)
		{
		case 0:
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
            case DROID_CYBORG_CONSTRUCT:
            case DROID_CYBORG_REPAIR:
			case DROID_WEAPON:
			case DROID_COMMAND:	   
				//if(psDroid->numWeaps)
				if(psDroid->asWeaps[0].nStat > 0)
                {
					psImd = WEAPON_MOUNT_IMD(psDroid,psDroid->player);
				}
				else
				{
//					psImd = debrisImds[rand()%MAX_DEBRIS];
					psImd = getRandomDebrisImd();
				}
				break;
			default:
//				psImd = debrisImds[rand()%MAX_DEBRIS];
				psImd = getRandomDebrisImd();
				break;
			}
			break;
		case 1:
			switch(psDroid->droidType)
			{
			case DROID_DEFAULT:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
            case DROID_CYBORG_CONSTRUCT:
            case DROID_CYBORG_REPAIR:
			case DROID_WEAPON:
			case DROID_COMMAND:	   
				//if(psDroid->numWeaps)
                if(psDroid->asWeaps[0].nStat > 0)
				{
					psImd = WEAPON_IMD(psDroid,psDroid->player);
				}
				else
				{
//					psImd = debrisImds[rand()%MAX_DEBRIS];
					psImd = getRandomDebrisImd();
				}
				break;
			default:
//				psImd = debrisImds[rand()%MAX_DEBRIS];
				psImd = getRandomDebrisImd();
				break;
			}
			break;
		case 2:
		case 3:
		case 4:
//			psImd = debrisImds[rand()%MAX_DEBRIS];
			psImd = getRandomDebrisImd();
			break;
		}
		if(psImd)
		{
			addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,psImd,getPlayerColour(psDroid->player));
		}
		else
		{
//			addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,debrisImds[rand()%MAX_DEBRIS],0);
			addEffect(&pos,EFFECT_GRAVITON,GRAVITON_TYPE_EMITTING_DR,TRUE,getRandomDebrisImd(),0);
		}
	}
}


//void addBodyPartEffect(iVector *position,iIMDShape *psShape)
//{
//	velocity.x = 1-rand()%3;
//	velocity.z = 1-rand()%3;
//	velocity.y = 4+rand()%7; 
//	addEffect(position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,psShape,0);
//}


void	compPersonToBits(DROID *psDroid)
{
	iVector		position;	//,rotation,velocity;
	iIMDShape	*headImd, *legsImd, *armImd, *bodyImd;
	UDWORD		groundHeight;
	UDWORD		col;

	if(!psDroid->visible[selectedPlayer])
	{
		/* We can't see the person or cyborg - so get out */
		return;
	}
	/* get bits pointers according to whether baba or cyborg*/
	//if ( psDroid->droidType == DROID_CYBORG )
    if (cyborgDroid(psDroid))
	{
		headImd = getImdFromIndex(MI_CYBORG_HEAD);
		legsImd = getImdFromIndex(MI_CYBORG_LEGS);
		armImd  = getImdFromIndex(MI_CYBORG_ARM);
		bodyImd = getImdFromIndex(MI_CYBORG_BODY);
	}
	else
	{
		headImd = getImdFromIndex(MI_BABA_HEAD);
		legsImd = getImdFromIndex(MI_BABA_LEGS);
		armImd  = getImdFromIndex(MI_BABA_ARM);
		bodyImd = getImdFromIndex(MI_BABA_BODY);
	}

	/* Get where he's at */
	position.x = psDroid->x;
	position.y = psDroid->z+1;
	groundHeight = psDroid->z;
	position.z = psDroid->y;


	/* Tell about player colour */
	col = getPlayerColour(psDroid->player);

	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,headImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,legsImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,armImd,col);
	addEffect(&position,EFFECT_GRAVITON,GRAVITON_TYPE_GIBLET,TRUE,bodyImd,col);
}


iIMDShape *getLeftPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;
	
	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;
	
	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + LEFT_PROP);

	return *imd;
}

iIMDShape *getRightPropulsionIMD(DROID *psDroid)
{
	UDWORD			bodyStat, propStat;
	iIMDShape		**imd;
	
	bodyStat = psDroid->asBits[COMP_BODY].nStat;
	propStat = psDroid->asBits[COMP_PROPULSION].nStat;

	imd = asBodyStats[bodyStat].ppIMDList;
	imd += (propStat * NUM_PROP_SIDES + RIGHT_PROP);

	return *imd;
}

SDWORD	rescaleButtonObject(SDWORD radius, SDWORD baseScale,SDWORD baseRadius)
{
	SDWORD newScale;
	newScale = 100 * baseRadius;
	newScale /= radius;
	if(baseScale > 0)
	{
		newScale += baseScale;
		newScale /= 2;
	}
	return newScale;
}

