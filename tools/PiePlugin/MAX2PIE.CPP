/**********************************************************************
 *<
	FILE: max2pie.cpp

	DESCRIPTION:  .PIE file export module for Pumpkin Studios

	CREATED BY: Annette Bell, based on sample code by Tom Hudson

	HISTORY: created 26 November 1996

	MODIFIED BY: Jeremy Sallis, to include animation transforms

  	HISTORY: modified 9 October 1997

 *>	Copyright (c) 1996, All Rights Reserved.
 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#pragma warning (disable : 4201 4214 4115 4514 4100 4097 4244)
//#include "MAX.h"
//#include "stdmat.h"
//#include "bmmlib.h"
//#include "splshape.h"
//#include "decomp.h"
//#include "trig.h"

#include "Max.h"
#include "mtldef.h"
#include "imtl.h"
#include "stdmat.h"
#include "istdplug.h"
#include "interpik.h"
#include "decomp.h"

#pragma warning (default : 4201 4214 4115 4100 4097 4244)

/***************************************************************************/

#include "max2pie.h"
#include "maxpidef.h"

TCHAR *GetString(int id);
#include "bmtex3.h"

/***************************************************************************/

#define BIT24	BMM_TRUE_32
#define MAXPOLYGONS			1000
#define MAXPOINTS			3000
#define TEX_BITMAP_WIDTH	1024
#define TEX_BITMAP_HEIGHT	1024

//flags for polygons - drawID
#define TEXTURED		0x200
#define TRPARENT		0x800
#define DOUBLE_SIDED	0x2000
#define TEXTURE_ANIMATION	 0x4000	// corresponds to iV_IMD_TEXANIM

/***************************************************************************/

HINSTANCE hInstance;

/***************************************************************************/

TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if (hInstance)
	{
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	}
	return NULL;
}

/***************************************************************************/

static int MessageBox(int s1, int s2, int option = MB_OK) 
{
	TSTR str1(GetString(s1));
	TSTR str2(GetString(s2));
	return MessageBox(GetActiveWindow(), str1, str2, option);
}

/***************************************************************************/

static int Alert(int s1, int s2 = IDS_PIEEXP, int option = MB_OK) 
{
	return MessageBox(s1, s2, option);
}

/***************************************************************************/

//this is turning into plain C code!
static void rotateTexCoord(Point3* texCoord, float angle, float width, float height)
{
	//translate centre of texture to 0,0, rotate coords, then translate back
//	texCoord->x -= (width/2.0);
//	texCoord->y -= (height/2.0);
//	float newX = texCoord->x * cos(angle) - texCoord->y * sin(angle);
//	float newY = texCoord->x * sin(angle) + texCoord->y * cos(angle);
	float newX = ((texCoord->x-width/2) * cos(angle)) - ((texCoord->y-
		height/2) * sin(angle));
	float newY = ((texCoord->x-width/2) * sin(angle)) + ((texCoord->y-
		height/2) * cos(angle));
	texCoord->x = newX + width/2;
	texCoord->y = newY + height/2;
}

/***************************************************************************/

static int PIERoundUp(float Coord, int MaxValue)
{
	Coord += (float)0.5;
	//don't want to allow the coord to be rounded up to a value greater 
	//than the width or height of the texture bitmap
	if ((int)Coord > MaxValue)
	{
		return MaxValue;
	}
	else if (Coord < 0)
	{
#if 0
			Alert(IDS_NEGATIVE_COORD);
#else
			return 0;
#endif
	}
	return ((int)Coord);
	
}

/***************************************************************************/

#define MAX_OBJECTS		200
#define MAX_MATERIALS	50

/***************************************************************************/

typedef struct export_image_tag
{
	TCHAR		*pName;
} EXPORT_IMAGE;

/***************************************************************************/

#define MAX_CONNECTORS 4
#define CONNECTOR_0 "connector"
#define CONNECTOR_1 "vtol"
#define CONNECTOR_2 "left propulsion"
#define CONNECTOR_3 "right propulsion"

enum PIE_EXPORT_TYPE	{ ONE_OBJECT, ANIM_FRAMES, ANIM_TRANS };

/***************************************************************************/

//forward references
class WorkFile;
class PIEObject;

//class definition
class PIEExport : public SceneExport 
{
private:
	int m_ObjectCount;

public:
	//variables
	int m_BitmapCount;

	/* anim private data */
	PIE_EXPORT_TYPE	exType;
	BOOL			bAnim;

	/* connector private data */
	int				NumOfObjects;
	DWORD			connectorCount;
	Point3			connectors[MAX_CONNECTORS];

	//the program should really allocate memory dynamically for these!
	PIEObject* ObjectList[MAX_OBJECTS];
	EXPORT_IMAGE* pBitmapList[MAX_OBJECTS*MAX_MATERIALS];
	//constructor/destructor
	PIEExport();
	~PIEExport();

	//functions
	int	ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2

	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void ShowAbout(HWND hWnd);		// Show DLL's "About..." box
#ifdef MAX25
	int	DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts);	// Export file
#else
	int	DoExport(const TCHAR *filename,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0); // Export	file
#endif
	
	PIEObject& CreatePIEObject();
	PIEObject* GetPIEObject(int i)
		{ return ObjectList[i];}
	
	int GetObjectCount()
		{ return m_ObjectCount;}

	//add the bitmaps used for materials into the one file for exporting
	EXPORT_IMAGE* CheckTextureFile(BitmapInfo& bInfo, Bitmap& BitmapToCopy);
};

/***************************************************************************/

//PIE Material structure to hold attributes that are required to export in PIE format
typedef struct PIE_Material_Tag
{
	int		BitmapWidth;			
	int		BitmapHeight;
	float	StartX;					//where to start in the texture map
	float	StartY;					//derived from the offset value
	float	TextureWidth;
	float	TextureHeight;
	BOOL	UNegative;				//whether the tiling is negative or not
	BOOL	VNegative;
	int		drawID;					//specifies draw charactristics
	float	angleInc;				//number of times texture bitmap has been rotated
	int		texAnimationFrames;
	float	playbackRate;
} PIE_Material;

/***************************************************************************/

#define X				0
#define Y				1
#define Z				2
#define MAT_ID			0

/***************************************************************************/

//class to contain all the details needed for exporting to PIE format
class PIEObject
{
private:
	//variables
	UVVert* pTextureVerts;
	int		NumOfPolygons;
	int		NumOfMaterials;
	int		NumOfPoints;

	/* anim variables */
	TCHAR		szFmtStr[16];

public:
	//variables
	PIE_Material arrMaterial[MAX_MATERIALS];
	int	Polygons[MAXPOLYGONS][5];					//Number of vertices + up to 4 vertices  
	int TexturePolygons[MAXPOLYGONS][5];			//materialID + up to 4 vertices
	Point3 Points[MAXPOINTS];
	Matrix3 WorldMatrix;

	//constructor/destructor
	PIEObject()
	{ NumOfPolygons = NumOfMaterials = NumOfPoints = 0;
	  pTextureVerts = NULL;
	}

	~PIEObject()	{};			//use default

	//functions
	BOOL SetMaterialProperties(INode* pTheNode, PIEExport* pExport);
	void SetPolygonData(Mesh& TheMesh);
	void SetPolygonDataNew(Mesh& TheMesh);
	UVVert* GetTextureVerts()	{return pTextureVerts;}

	BOOL output3VertexPoly(FILE* stream, WorkFile& theFile, const TCHAR* filename,
		int polyNum, int objectNum, int textureHeight);
	BOOL output4VertexPoly(FILE* stream, WorkFile& theFile, const TCHAR* filename,
		int polyNum, int objectNum, int textureHeight);

private:
	void SetNumOfMaterials(int Quantity)
		{NumOfMaterials = Quantity;}
	void SetTextureVerts(UVVert* pMaterialVertices)
		{pTextureVerts = pMaterialVertices;}
	
	//stores the vertex in the list for export if not already present
	int StoreVertex(Point3& Vertex);

	//calculate the poly vertices and texture vertices
	int CalcPolygon(int EdgeNumber, int CurrentPoly, Mesh& CurrentMesh,
							Face& CurrentFace);
	
	//find a polygon in the given mesh, with 2 vertices that match those input
	int FindPolygon(DWORD Vertex0, DWORD Vertex1, Mesh& TheMesh, int CurrentPoly);
	
	//store the polygon vertices and texture vertices
	void StoreQuad(Mesh& TheMesh, DWORD Vertex0, DWORD Vertex1, 
				  DWORD Vertex2, DWORD Vertex3, int firstPoly, 
				  int secondPoly, int position, int join);

	/* anim private funcs */
	TSTR	Format( int value );
	TSTR	Format( float value );
	TSTR	Format( Point3 value );
	TSTR	Format( Color value );
	TSTR	Format( AngAxis value );
	TSTR	Format( Quat value );
	TSTR	Format( ScaleValue value );
	void	CommaScan( TCHAR* buf );

public:
	int GetNumOfPolygons()
		{return NumOfPolygons;}
	int GetNumOfPoints()
		{return NumOfPoints;}
	int GetNumOfMaterials()
		{return NumOfMaterials;}

	/* anim public funcs */
	BOOL	CheckForAnimation( INode* node, Interface *ip,
								BOOL& bPos, BOOL& bRot, BOOL& bScale );
	void	ExportAnimKeys( FILE* stream, const TCHAR *szFileName,
							INode* node, Interface *ip, int indentLevel );

};

/***************************************************************************/

BOOL PIEObject::SetMaterialProperties(INode* pTheNode, PIEExport* pExport)
{	

	Mtl *pMaterial = pTheNode->GetMtl();

	if (pMaterial == NULL)
	{
		SetNumOfMaterials(0);
		arrMaterial[0].drawID = 0;
		return TRUE;
	}
	
	//determine the number of sub-materials - if any
	int SubMats = pMaterial->NumSubMtls();
	
	//must enter loop at least once
	int Index = 0;
	do
	{
		//get the material for the face
		Mtl *pFaceMaterial;
		if (SubMats > 0)
		{
			pFaceMaterial = pMaterial->GetSubMtl(Index);
			SetNumOfMaterials(SubMats);
		}
		else
		{
			pFaceMaterial = pMaterial;
			SetNumOfMaterials(1);
		}

		//SET THE DRAWID
		arrMaterial[Index].drawID = TEXTURED;

		//check material for transparency
		if ((((StdMat*)pFaceMaterial)->GetOpacity(0) != 1.0) ||
			(((StdMat*)pFaceMaterial)->MapEnabled(ID_OP)))
		{
			arrMaterial[Index].drawID += TRPARENT;
		}
		//check material for double-sided
		if (((StdMat*)pFaceMaterial)->GetTwoSided())
		{
			arrMaterial[Index].drawID += DOUBLE_SIDED;
		}

		//check bitmap is present for texture - solid colour NOT possible at present
		if (!((StdMat*)pFaceMaterial)->MapEnabled(ID_DI))
		{
			Alert(IDS_NOBITMAP);
			return FALSE;
		}
	
		BitmapTex *pBitmap = (BitmapTex*)pFaceMaterial->GetActiveTexmap();
		if (pBitmap != NULL)
		{
			//get the bitmap details to store
			BitmapInfo bInfo;
			bInfo.SetName(pBitmap->GetMapName());
			//TheManager is the MAX instance of a Bitmap Manager
			Bitmap* pTextureBitmap = TheManager->Load(&bInfo);
			
			//check the file format is the necessary type ?
			//check the file extension ?

			arrMaterial[Index].BitmapWidth = pTextureBitmap->Width();
			arrMaterial[Index].BitmapHeight = pTextureBitmap->Height();

			//add the texture to the export file if not already used
			EXPORT_IMAGE *pImage = pExport->CheckTextureFile(bInfo, *pTextureBitmap);

			//store the rotation of the bitmap
			StdUVGen* pUVGen = pBitmap->GetUVGen();
			arrMaterial[Index].angleInc = pUVGen->GetAng(0);

			//get the offsets of the bitmap on the face
			float UOffset = pUVGen->GetUOffs(0);
			float VOffset = pUVGen->GetVOffs(0);
			float UTiling = pUVGen->GetUScl(0);
			float VTiling = pUVGen->GetVScl(0);

			float clipU = ((BMTex *)pBitmap)->GetClipU(0);
			float clipV = ((BMTex *)pBitmap)->GetClipV(0);
			float clipW = ((BMTex *)pBitmap)->GetClipW(0);
			float clipH = ((BMTex *)pBitmap)->GetClipH(0);

			int ticksPerFrame = GetTicksPerFrame();

			//get texture aniamtion data
			arrMaterial[Index].playbackRate = ((BMTex *)pBitmap)->GetPlaybackRate();
			arrMaterial[Index].texAnimationFrames = (int(((BMTex *)pBitmap)->GetStartTime()))/ticksPerFrame;
			if (arrMaterial[Index].texAnimationFrames != 0)
			{
				arrMaterial[Index].drawID += TEXTURE_ANIMATION;
			}

			//assume offset and tilling apply within placement window
			float localWidth = arrMaterial[Index].BitmapWidth * clipW;
			float localHeight = arrMaterial[Index].BitmapHeight * clipH;

			float TextureWidth = fabs(UTiling * localWidth);
			float TextureHeight = fabs(VTiling * localHeight);
			//determine start and end coords for the textures
			if (UTiling > 0)
			{
				arrMaterial[Index].StartX = (clipU * arrMaterial[Index].BitmapWidth) + 
											(localWidth - (UTiling * localWidth))/2 - 
											(UOffset * TextureWidth);
				arrMaterial[Index].UNegative = FALSE;
			}
			else
			{
				arrMaterial[Index].StartX = (clipU * arrMaterial[Index].BitmapWidth) + 
											(localWidth + (UTiling * localWidth))/2 + 
											(UOffset * TextureWidth);
				arrMaterial[Index].UNegative = TRUE;
			}
			//StartY is measured from bottom left corner of the bitmap
			if (VTiling > 0)
			{
				arrMaterial[Index].StartY = (clipV * arrMaterial[Index].BitmapHeight) +
											(localHeight - (VTiling * localHeight))/2 -
											(VOffset * TextureHeight);
				arrMaterial[Index].VNegative = FALSE;
			}
			else
			{
				arrMaterial[Index].StartY = (clipV * arrMaterial[Index].BitmapHeight) +
											(localHeight + (VTiling * localHeight))/2 +
											(VOffset * TextureHeight);
				arrMaterial[Index].VNegative = TRUE;
			}
			arrMaterial[Index].TextureWidth = TextureWidth;
			arrMaterial[Index].TextureHeight = TextureHeight;
		}
		//Increment the loop index 
		Index++;
	} while (Index < SubMats);
	return TRUE;
}

void PIEObject::SetPolygonData(Mesh& TheMesh)
{
	int NumOfFaces = TheMesh.getNumFaces();
	if (!NumOfFaces)
	{
		return;
	}

	//create array of BOOLS to flag when polygons have been dealt with
	BOOL*	paPolygons = (BOOL*)malloc (sizeof(BOOL) * NumOfFaces);
	if (!paPolygons)
	{
		//abandon the program
		OutputDebugString("Cannot allocate memory for BOOLS for polygons dealt with");
		assert(0);
	}
	memset(paPolygons, 0, sizeof(BOOL) * NumOfFaces);

	//set the pointer to the texture vertices
	if (TheMesh.numTVerts > 0)
	{
		SetTextureVerts(TheMesh.tVerts);
	}

	Face* pFaceList = TheMesh.faces;
	//for each face in the mesh
	for (int faceNum=0; faceNum < NumOfFaces; faceNum++)
	{
		//if the face is not hidden store the polygon id 
		//and the vertices
		if (!pFaceList->Hidden())
		{
			//if haven't already dealt with this
			if (!(*(paPolygons+faceNum)))
			{
				BOOL PolygonStored = FALSE;
				//for each of the three edges
				for (int point=0; point<3; point++)
				{
					int CombinedPoly = CalcPolygon(point,faceNum,TheMesh,*pFaceList);
					//check that value returned is a valid polygon id and not a different 
					//materialed polygon
					if (CombinedPoly >= 0) 
					{
						//once dealt with - don't need to look at other edges
						*(paPolygons+CombinedPoly) = TRUE;
						PolygonStored = TRUE;
						break;
					}
				}
				if (!PolygonStored)
				{
					//need to store 3 vertex polygon
					//store the number of vertices
					Polygons[NumOfPolygons][0] = 3;
					TVFace* pTextureFaces = NULL;
					if (TheMesh.numTVerts > 0)
					{
						pTextureFaces = TheMesh.tvFace+faceNum;
					}
					for (int point=0; point<3; point++)
					{
						//store the vertex and return the id
						int VertexNumber = StoreVertex(TheMesh.getVert(pFaceList->getVert(point)));
						Polygons[NumOfPolygons][point+1] = VertexNumber;
						//store the texture vertex for the polygon
						if (TheMesh.numTVerts > 0)
						{
							TexturePolygons[NumOfPolygons][point+1] = pTextureFaces->getTVert(point);
						}
					}
				}
				//set the material for the polygon
				TexturePolygons[NumOfPolygons][MAT_ID] = pFaceList->getMatID();
				NumOfPolygons++;			
				*(paPolygons+faceNum) = TRUE;
			}
		}
		pFaceList++;
	}
	delete (paPolygons);
}

int PIEObject::CalcPolygon(int EdgeNumber, int CurrentPoly, Mesh& CurrentMesh, 
							Face& CurrentFace)
{
	DWORD Vertex0 = 0;
	DWORD Vertex1 = 1;
	DWORD Vertex2 = 2;
	Face* pAdjacentFace;

	//check visability
	if (CurrentFace.getEdgeVis(EdgeNumber)==0)
	{
		//edge is invisible - find the connecting polygon
		switch (EdgeNumber)
		{
			case 0:
			{
				Vertex0 = CurrentFace.getVert(0);
				Vertex1 = CurrentFace.getVert(1);
				break;
			}
			case 1:
			{
				Vertex0 = CurrentFace.getVert(1);
				Vertex1 = CurrentFace.getVert(2);
				break;
			}
			case 2:
			{
				Vertex0 = CurrentFace.getVert(2);
				Vertex1 = CurrentFace.getVert(0);
			}
		}
		int polyID = FindPolygon(Vertex0,Vertex1,CurrentMesh,CurrentPoly);
		if (polyID < 0)
		{
//			Alert(Couldn't find the polygon - ERROR);
			return -1;
		}

		if (polyID > CurrentPoly)
		{
			//polygon not already been dealt with
			pAdjacentFace = CurrentMesh.faces + polyID;
			
			//check polygon does not have a different material to this one
			if (CurrentFace.getMatID() != pAdjacentFace->getMatID())
			{
				return -1;
			}
			
			Vertex0 = CurrentFace.getVert(0);
			Vertex1 = CurrentFace.getVert(1);
			Vertex2 = CurrentFace.getVert(2);
			
			//determine the fourth vertex
			for (int i=0; i < 3; i++)
			{
				if ((pAdjacentFace->getVert(i) != Vertex0) &&
					(pAdjacentFace->getVert(i) != Vertex1) &&
					(pAdjacentFace->getVert(i) != Vertex2))
				{
					break;
				}
			}

			StoreQuad(CurrentMesh,Vertex0,Vertex1,Vertex2,pAdjacentFace->getVert(i),
				CurrentPoly, polyID, i, EdgeNumber);

			return polyID;
		}
	}
	return -1;
}

#define FoundFirst	0x01
#define FoundSecond 0x02
#define FoundBoth	0x03

int PIEObject::FindPolygon(DWORD Vertex0, DWORD Vertex1, Mesh& TheMesh, int CurrentPoly)
{
	int Found = 0;	
	int NumFaces = TheMesh.getNumFaces();
	Face* pTheFace = TheMesh.faces;

	//look through all the faces
	for (int face=0; face < NumFaces; face++)
	{
		Found = 0;
		//ignore the polygon we are comparing with!
		if (face != CurrentPoly)
		{
			//for each face, look through the three vertices
			for (int j=0; j<3; j++)
			{
				if (pTheFace->getVert(j) == Vertex0)
				{
					Found |= FoundFirst;
				}
				else if (pTheFace->getVert(j) == Vertex1)
				{
					Found |= FoundSecond;
				}
			}
			if (Found == FoundBoth)
			{
				//both vertices have been found, so return the index into the face list
				return face;
			}
		}
		pTheFace++;
	}
	//no match, so return error code
	return -1;
}


void PIEObject::StoreQuad(Mesh& TheMesh, DWORD Vertex0, DWORD Vertex1, 
						  DWORD Vertex2, DWORD Vertex3, int firstPoly, 
						  int secondPoly, int position, int join)
{
	TVFace* pTextureFaces = NULL;
	TVFace* pTextureTwo = NULL;
	
	if (TheMesh.numTVerts > 0)
	{
		pTextureFaces = TheMesh.tvFace + firstPoly;
		pTextureTwo = TheMesh.tvFace + secondPoly;
	}
	
	//store the number of vertices
	Polygons[NumOfPolygons][0] = 4;

	DWORD aVertex[4];
	DWORD aTexVertex[4];

	switch (join)
	{
		case 0:
		{
			aVertex[0] = Vertex0;
			aVertex[1] = Vertex3;
			aVertex[2] = Vertex1;
			aVertex[3] = Vertex2;
			if (TheMesh.numTVerts > 0)
			{
				aTexVertex[0] = pTextureFaces->getTVert(0);
				aTexVertex[2] = pTextureFaces->getTVert(1);
				aTexVertex[3] = pTextureFaces->getTVert(2);
				aTexVertex[1] = pTextureTwo->getTVert(position);
			}
			break;
		}
		case 1:
		{
			aVertex[0] = Vertex0;
			aVertex[1] = Vertex1;
			aVertex[2] = Vertex3;
			aVertex[3] = Vertex2;
			if (TheMesh.numTVerts > 0)
			{
				aTexVertex[0] = pTextureFaces->getTVert(0);
				aTexVertex[1] = pTextureFaces->getTVert(1);
				aTexVertex[3] = pTextureFaces->getTVert(2);
				aTexVertex[2] = pTextureTwo->getTVert(position);
			}
			break;
		}
		case 2:
		{
			aVertex[0] = Vertex0;
			aVertex[1] = Vertex1;
			aVertex[2] = Vertex2;
			aVertex[3] = Vertex3;
			if (TheMesh.numTVerts > 0)
			{
				aTexVertex[0] = pTextureFaces->getTVert(0);
				aTexVertex[1] = pTextureFaces->getTVert(1);
				aTexVertex[2] = pTextureFaces->getTVert(2);
				aTexVertex[3] = pTextureTwo->getTVert(position);
			}
		}
	}

	for (int vert=1; vert < 5; vert++)
	{
		//store the vertex and return the id
		int VertexNumber = StoreVertex(TheMesh.getVert(aVertex[vert-1]));
		Polygons[NumOfPolygons][vert] = VertexNumber;
		//store the texture vertex for the polygon
		if (TheMesh.numTVerts > 0)
		{
			TexturePolygons[NumOfPolygons][vert] = aTexVertex[vert-1];
		}
		else
		{
			TexturePolygons[NumOfPolygons][vert] = 0;
		}

	}
}


// Handy file class

class WorkFile 
{
private:
	FILE *stream;
	
public:
	WorkFile(const TCHAR *filename,const TCHAR *mode) 
		{ stream = NULL; Open(filename, mode); };
	~WorkFile() 
		{ Close(); };
	FILE * Stream() 
		{ return stream; };
	int Close()
		{ int result=0; if(stream) result=fclose(stream); stream = NULL; return result; }
	void Open(const TCHAR *filename,const TCHAR *mode) 
		{ Close(); stream = _tfopen(filename,mode); }
};

int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	//removes warning when compiling
	(void)lpvReserved;

	hInstance = hinstDLL;

	if ( !controlsInit ) 
	{
		controlsInit = TRUE;
		
		// initialize Win95 controls
		InitCommonControls();
	}
	switch(fdwReason) 
	{
		case DLL_PROCESS_ATTACH:
			//MessageBox(NULL,_T("PIEIMP.DLL: DllMain"),_T("PIEIMP"),MB_OK);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(TRUE);
}

//------------------------------------------------------

class PIEClassDesc:public ClassDesc 
{
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new PIEExport; }
	const TCHAR *	ClassName() { return GetString(IDS_PUMPKIN); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	//the class ID is made up by the function in the help files!
	Class_ID		ClassID() { return Class_ID(Class_ID(0x71901f6f, 0x3f4e21a9)); }
	const TCHAR* 	Category() { return GetString(IDS_SCENEEXPORT);  }
};

static PIEClassDesc PIEDesc;


__declspec( dllexport ) const TCHAR * LibDescription() 
{ 
	return GetString(IDS_PIEEXPORTDLL); 
}

__declspec( dllexport ) int LibNumberClasses() 
{ 
	return 1; 
}

__declspec( dllexport ) ClassDesc * LibClassDesc(int i) 
{
	switch(i) 
	{
		case 0: 
			return &PIEDesc; 
			break;
		default: 
			return 0; 
			break;
	}
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion() 
{ 
	return VERSION_3DSMAX; 
}


//
// .PIE export module functions follow:
//

PIEExport::PIEExport() 
{
	m_ObjectCount = 0;
	m_BitmapCount = 0;
}

PIEExport::~PIEExport() 
{
	//delete the array of PIEObjects
	for (int i=0; i < m_ObjectCount; i++)
	{
		delete ObjectList[i];
	}
	//delete the array of EXPORT_IMAGE
	for (int j=0; j < m_BitmapCount; j++)
	{
		delete pBitmapList[j]->pName;
		delete pBitmapList[j];
	}
}

int PIEExport::ExtCount() 
{
	return 1;
}

// Extensions supported for import/export modules
const TCHAR * PIEExport::Ext(int n) 
{		
	switch(n) {
		case 0:
			return _T("PIE");
		}
	return _T("");
	}

// Long ASCII description (i.e. "Targa 2.0 Image File")
const TCHAR * PIEExport::LongDesc() 
{
	return GetString(IDS_PUMPKINPIEFILE);
}
	
// Short ASCII description (i.e. "Targa")
const TCHAR * PIEExport::ShortDesc() 
{
	return GetString(IDS_PUMPKIN);
}

// ASCII Author name
const TCHAR * PIEExport::AuthorName() 
{			
	return GetString(IDS_ANNETTE_BELL);
}

// ASCII Copyright message
const TCHAR * PIEExport::CopyrightMessage() 
{	
	return GetString(IDS_COPYRIGHT_PUMPKIN);
}

// Other message #1
const TCHAR * PIEExport::OtherMessage1() 
{		
	return _T("");
}

// Other message #2
const TCHAR * PIEExport::OtherMessage2() 
{	
	return _T("");
}

// Version number * 100 (i.e. v3.01 = 301)
unsigned int PIEExport::Version() 
{				
	return 100;
}

void PIEExport::ShowAbout(HWND hWnd) 
{			
	//removes warning when compiling
	(void)hWnd;
}

PIEObject& PIEExport::CreatePIEObject()
{
	PIEObject* pObject = new (PIEObject);
	ObjectList[m_ObjectCount++] = pObject;
	return *(pObject);
}

class MySceneEntry 
{
	public:
		INode *node;
		Object *obj;
		MySceneEntry *next;
		MySceneEntry(INode *n, Object *o) { node = n; obj = o; next = NULL; }
};

class MySceneEnumProc : public ITreeEnumProc 
{
	public:
		//variables
		Interface	*i;
		MySceneEntry *head;
		MySceneEntry *tail;
		IScene		*theScene;
		int			count;
		TimeValue	time;

		//constructor/destructor
		MySceneEnumProc(IScene *scene, TimeValue t, Interface *i);
		~MySceneEnumProc();

		//functions
		int	Count() { return count; }
		void Append(INode *node, Object *obj);
		int	callback( INode *node );
		Box3 Bound();
		MySceneEntry *operator[](int index);
};

MySceneEnumProc::MySceneEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	time = t;
	theScene = scene;
	count = 0;
	head = tail = NULL;
	this->i = i;
	theScene->EnumTree(this);
}

MySceneEnumProc::~MySceneEnumProc() 
{
	while(head) 
	{
		MySceneEntry *next = head->next;
		delete head;
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

int MySceneEnumProc::callback(INode *node) 
{
	Object *obj = node->EvalWorldState(time).obj;

	char* name = node->GetName();
	switch(obj->SuperClassID())
	{
		case GEOMOBJECT_CLASS_ID: 
			if(obj->CanConvertToType(triObjectClassID)) 
			{
				Append(node, obj);
			}
			break;
		case HELPER_CLASS_ID://connectors 
				Append(node, obj);
			break;
		default:
			break;
	}
	return TREE_CONTINUE;	// Keep on enumeratin'!
}


void MySceneEnumProc::Append(INode *node, Object *obj) 
{
	MySceneEntry *entry = new MySceneEntry(node, obj);
	if(tail)
	{
		tail->next = entry;
	}
	tail = entry;
	if(!head)
	{
		head = entry;
	}
	count++;	
}

Box3 MySceneEnumProc::Bound() 
{
	Box3 bound;
	bound.Init();
	MySceneEntry *e = head;
	ViewExp *vpt = i->GetViewport(NULL);
	while(e) 
	{
		Box3 bb;
		e->obj->GetWorldBoundBox(time, e->node, vpt, bb);
		bound += bb;
		e = e->next;
	}
	return bound;
}

MySceneEntry *MySceneEnumProc::operator[](int index) 
{
	MySceneEntry *e = head;
	while(index) 
	{
		e = e->next;
		index--;
	}
	return e;
}

/***************************************************************************/
// Return an indentation string

void
GetIndent( TSTR &str, int indentLevel )
{
	str = "";
	for ( int i=0; i<indentLevel; i++ )
	{
		str += "\t";
	}
}

/***************************************************************************/
// max uses left hand coord system - sam uses right (or is it
// the other way around?!), so change the sign of z

BOOL
WriteObjVertices( FILE *stream, PIEObject* pObject )
{
	for (int j=0; j < pObject->GetNumOfPoints(); j++)
	{
		//transform the point into world coords
		Point3 WorldPoint = pObject->Points[j] * pObject->WorldMatrix;
		//Rotate 90 degrees about the x axis for Sams engine
		Matrix3 RotationM(TRUE);
		RotationM.RotateX(-90*DEG_TO_RAD);
		Point3 CurrentPoint = WorldPoint * RotationM;
		if (fprintf(stream,"\t%d %d %d\n",(int)CurrentPoint[X],
			(int)CurrentPoint[Y],-(int)CurrentPoint[Z])<0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/***************************************************************************/

BOOL
WriteObjPolys( FILE *stream, PIEObject* pObject, WorkFile & theFile,
				const TCHAR *filename, int ObjectInc )
{
	for ( int j=0; j < pObject->GetNumOfPolygons(); j++ )
	{
		if (pObject->Polygons[j][0] == 3)
		{
			if (!pObject->output3VertexPoly(stream, theFile, filename,
				j, ObjectInc, 0))
			{
				return FALSE;
			}
		}
		else 
		{
			if (!pObject->output4VertexPoly(stream, theFile, filename,
				j, ObjectInc, 0))
			{
				return FALSE;
			}
		}

		if(fprintf(stream,"\n")<0)
		{
			return FALSE;
		}	
	}

	return TRUE;
}

/***************************************************************************/

int pie_object_create( ExpInterface *ei, Interface *gi, PIEExport *exp )
{
	char* name;
	DWORD helperCount = 0;
	Matrix3		tm;
	AffineParts	ap;
	Point3		pos;

	//create the bitmap to hold all the textures used within the scene
	BitmapInfo bInfo;
	bInfo.SetWidth(TEX_BITMAP_WIDTH);
	bInfo.SetHeight(TEX_BITMAP_HEIGHT);
	// Make sure there are nodes we're interested in!
	// Ask the scene to enumerate all its nodes so we can determine if there 
	// are any we can use
	MySceneEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	// Any useful nodes?
	if(!myScene.Count()) 
	{
		Alert(IDS_NODATATOEXPORT);
		return 1;
	}

	/* count valid objects */
	exp->NumOfObjects = 0;
	exp->connectorCount = 0;

	//Get the Data
	for(int ox=0; ox < myScene.Count(); ++ox) 
	{
		//for each node in the scene
		INode *n = myScene[ox]->node;

		/* get mesh */
		Object *obj = myScene[ox]->obj;
		TriObject *tri = (TriObject *) myScene[ox]->obj->ConvertToType(
											gi->GetTime(), triObjectClassID );

		PIEObject& TheObject = exp->CreatePIEObject();

		/* valid object only if more than 2 vertices and 1 polygon */
		if ( obj->SuperClassID() == GEOMOBJECT_CLASS_ID && tri != NULL &&
			 tri->mesh.numVerts > 2 && tri->mesh.numFaces > 0 )
		{
			exp->NumOfObjects++;

			//Get the world transformation coords for each node in the scene
			TheObject.WorldMatrix = n->GetObjectTM(0);

			//set the PIEObject's materials
			if (!TheObject.SetMaterialProperties(n, exp))
			{
				return -1;
			}
			
			//set up the PIEObject's data
			TheObject.SetPolygonData(tri->mesh);

			/* catch material assigned to object with no texture verts */
			if ( TheObject.GetNumOfMaterials() > 0 &&
				 TheObject.GetTextureVerts() == NULL )
			{
				MessageBox( GetActiveWindow(), GetString(IDS_NO_TEXTURE_VERTS),
							n->GetName(), MB_OK );
				return -1;
			}
		}
		else if (obj->SuperClassID() == HELPER_CLASS_ID)
		{
			helperCount ++;
			/* get overall matrix */
			tm = n->GetObjectTM(0);

			/* decompose matrix */
			decomp_affine(tm, &ap);

			/* get translations */
			pos = ap.t;

			name = n->GetName();
			if (stricmp(name,CONNECTOR_0)==0)
			{
				exp->connectors[0] = pos;
				if (exp->connectorCount < 1)
					exp->connectorCount = 1;
			}
			else if (stricmp(name,CONNECTOR_1)==0)
			{
				exp->connectors[1] = pos;
				if (exp->connectorCount < 2)
					exp->connectorCount = 2;
			}
			else if (stricmp(name,CONNECTOR_2)==0)
			{
				exp->connectors[2] = pos;
				if (exp->connectorCount < 3)
					exp->connectorCount = 3;
			}
			else if (stricmp(name,CONNECTOR_3)==0)
			{
				exp->connectors[3] = pos;
				if (exp->connectorCount < 4)
					exp->connectorCount = 4;
			}
			//no error warning for unassociated helper objects
			//as groups then trigger warning
/*			else
			{
				Alert(IDS_HELPERERROR);
			}
*/
		}
	}

	return 1;
}

/***************************************************************************/

int ani_save( const TCHAR *filename, ExpInterface *ei, Interface *gi, 
				PIEExport *exp ) 
{
	MySceneEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	// Any useful nodes?
	if(!myScene.Count()) 
	{
		Alert(IDS_NODATATOEXPORT);
		return 1;
	}

	/* init export type to no anims */
	exp->exType = ONE_OBJECT;

	/* check all nodes for export type tags */
	TSTR	tStrKey = "ANIM_TYPE", tStrUser;
	for ( int iNode=0; iNode<myScene.Count(); iNode++ )
	{
		if ( myScene[iNode]->node->GetUserPropString(
									tStrKey, tStrUser ) == TRUE )
		{
			if ( stricmp( tStrUser, "ANIM_TRANSFORM" ) == 0 )
			{
				exp->exType = ANIM_TRANS;
			}
			else if ( stricmp( tStrUser, "ANIM_FRAMES" ) == 0 )
			{
				exp->exType = ANIM_FRAMES;
			}
		}
	}

	/* check nodes to ascertain whether anim data present */
	BOOL		bPos, bRot, bScale;
	PIEObject	*pTheObject;
	INode		*testNode;
	int			NumOfObjects = 0;

	exp->bAnim = FALSE;

	for ( int t=0; t<myScene.Count(); t++ )
	{
		pTheObject = exp->GetPIEObject(t);
		testNode = myScene[t]->node;

		/* check object for animation */
		if ( pTheObject->CheckForAnimation( testNode, gi, bPos, bRot, bScale) == TRUE &&
			 exp->bAnim == FALSE )
		{
			exp->bAnim = TRUE;
		}

		if ( myScene[t]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID  &&
			 pTheObject->GetNumOfPoints() > 2 &&
			 pTheObject->GetNumOfPolygons() > 0  )
		{
			/* increment number of geometry objects found */
			NumOfObjects++;
		}
	}

	/* alert if no anim data present */
	if ( exp->bAnim == TRUE )
	{
		if ( exp->exType == ONE_OBJECT || exp->exType == ANIM_FRAMES )
		{
			Alert( IDS_ANIMDATAPRESENT );
			return 0;
		}
	}
	else if ( exp->exType == ANIM_TRANS )
	{
		Alert( IDS_NOANIMDATAPRESENT );
		return 0;
	}

	/* dump anim data if required */
	if ( exp->bAnim == TRUE || exp->exType == ANIM_FRAMES )
	{
		/* set anim file name */
		TCHAR	szAnimFile[256], *cPos = strstr( filename, ".pie" );
		if ( cPos <= filename )
		{
			cPos = strstr( filename, ".PIE" );
		}
		int		iDotPos = cPos - filename + 1;
		strncpy( szAnimFile, filename, iDotPos );
		szAnimFile[iDotPos] = '\0';
		TCHAR	szPIEFile[256];
		strcpy( szPIEFile, szAnimFile );
		strcat( szAnimFile, "ani" );
		strcat( szPIEFile, "pie" );

		/* open anim file */
		WorkFile animFile( szAnimFile, _T("w") );
		FILE *stream = animFile.Stream();
		if ( stream == NULL )
		{
			Alert(IDS_WRITEERROR);
			animFile.Close();
			return 0;
		}

		/* get pie filename from full path */
		TCHAR		*szName = strrchr( (TCHAR *) szPIEFile, '\\' ) + 1;

		/* dump header */
		fprintf( stream, "/* header format: anim3d [filename] [frames]" );
		fprintf( stream, "[framerate] [num objects]            */\n" );
		fprintf( stream, "/* anim line: [frame] [x] [y] [z] [xrot] [yrot] [zrot]" );
		fprintf( stream, "[xscale] [yscale] [zscale] */\n" );

		if ( exp->exType == ANIM_TRANS )
		{
			TimeValue	start = gi->GetAnimRange().Start();
			TimeValue	end   = gi->GetAnimRange().End();
			int			delta = GetTicksPerFrame();
			int			iSamples = ((end - start) / delta) + 1;
	
			fprintf( stream,"%s \"%s\" %i %i %i\n{",
					GetString(IDS_ANIM_TRANS), szName,
					iSamples, GetFrameRate(), NumOfObjects );
		}
		else
		{
			fprintf( stream,"%s \"%s\" %i %i\n{\n",
					GetString(IDS_ANIM_FRAMES), szName,
					NumOfObjects, GetFrameRate() );
		}

		int iObj = 0;

		/* dump anim data */
		for (int iN=0; iN < myScene.Count(); iN++)
		{
			TSTR		indent;
			GetIndent( indent, 1 );

			INode		*node = myScene[iN]->node;
			PIEObject	*pTheObject = exp->GetPIEObject(iN);

			if ( myScene[iN]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
				 pTheObject->GetNumOfPoints() > 2 &&
				 pTheObject->GetNumOfPolygons() > 0  )
			{
				if ( exp->exType == ANIM_TRANS )
				{
					fprintf( stream,"\n%s%s %i \"%s\"\n%s{\n", indent.data(),
							 GetString(IDS_ANIM_OBJECT), iObj,
							 node->GetName(), indent.data() );

					pTheObject->ExportAnimKeys( stream, szPIEFile, node, gi, 1 );

					fprintf(stream,"%s}\n", indent.data());
				}
				else
				{
					fprintf(stream, "\t%3d\t0 0 0 0 0 0 %4i %4i %4i\n", iObj,
							INT_SCALE, INT_SCALE, INT_SCALE );
				}

				/* update geom object count */
				iObj++;
			}
		}

		fprintf(stream,"}\n");

		if(animFile.Close())
		{
			Alert(IDS_WRITEERROR);
			return 0;
		}
	}

	return 1;	
}

/***************************************************************************/

int pie_save( const TCHAR *filename, ExpInterface *ei, Interface *gi, 
				PIEExport *exp ) 
{
	MySceneEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	// Output the file as text
	WorkFile theFile(filename,_T("w"));
	FILE *stream = theFile.Stream();
	if(!stream) 
	{
		Alert(IDS_CANTCREATE);
		return -1;						// Didn't open!
	}

	/* Write header stuff */
	if(fprintf(stream,"PIE 2\n")<0)
	{
		wrterr:
		Alert(IDS_WRITEERROR);
		theFile.Close();
		remove(filename);
		return 0;
	}
	
	if(fprintf(stream,"TYPE 200\n")<0)
	{
		goto wrterr;
	}
	//code the texture file to be the same name as the PIE file
	TCHAR	*pTextureFile = NULL;
	TCHAR	szTexName[256];
	if (exp->m_BitmapCount == 0)
	{
		//no textures
		if(fprintf(stream,"NOTEXTURE 0 NOTEXTURE 0 0\n")<0)
		{
			goto wrterr;
		}
	}
	else
	{
		//check for valid type 
		strcpy( szTexName, exp->pBitmapList[0]->pName );
		pTextureFile = strstr(szTexName,".bmp");
		if (pTextureFile == NULL)
		{
			pTextureFile = strstr(szTexName,".BMP");
			if (pTextureFile <= szTexName)
			{
				Alert(IDS_INCORRECT_BIT);
				theFile.Close();
				remove(szTexName);
				return 0;
			}
		}
		*pTextureFile = '\0';

		//first remove the path 
		while((*pTextureFile != '\\') && (pTextureFile >= szTexName))
		{
			pTextureFile--;
		}

		pTextureFile++;

		strcat(pTextureFile,".pcx");
		if(fprintf(stream,"TEXTURE 0 %s 256 256\n",pTextureFile)<0)
		{
			goto wrterr;
		}
	}

	if ( exp->exType == ONE_OBJECT )
	{
		if(fprintf(stream,"LEVELS 1\n")<0)
		{
			goto wrterr;
		}
		if(fprintf(stream,"LEVEL 1\n")<0)
		{
			goto wrterr;
		}
	}

	int TotalPoints = 0;
	int TotalPolygons = 0;
	for (int i=0; i<myScene.Count(); i++)
	{
		PIEObject* pTheObject = exp->GetPIEObject(i);

		if ( myScene[i]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
			 pTheObject->GetNumOfPoints() > 2 &&
			 pTheObject->GetNumOfPolygons() > 0  )
		{
			TotalPoints += pTheObject->GetNumOfPoints();
			TotalPolygons += pTheObject->GetNumOfPolygons();
		}
	}
			
	int ObjectInc = 0;
	int	k;

	/* loop over objects in scene */
	if ( exp->exType == ONE_OBJECT )
	{
		if(fprintf(stream,"POINTS %d\n",TotalPoints)<0)
		{
			goto wrterr;
		}

		for (k=0; k<myScene.Count(); k++)
		{
			PIEObject* pTheObject = exp->GetPIEObject(k);

			if ( myScene[k]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
				 pTheObject->GetNumOfPoints() > 2 &&
				 pTheObject->GetNumOfPolygons() > 0  )
			{
				if ( WriteObjVertices( stream, pTheObject ) == FALSE )
				{
					goto wrterr;
				}
			}
		}

		if(fprintf(stream,"POLYGONS %d\n",TotalPolygons)<0)
		{
			goto wrterr;
		}

		for (k=0; k < myScene.Count(); k++)
		{
			PIEObject* pTheObject = exp->GetPIEObject(k);

			if ( myScene[k]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
				 pTheObject->GetNumOfPoints() > 2 &&
				 pTheObject->GetNumOfPolygons() > 0 )
			{
				if ( WriteObjPolys( stream, pTheObject, theFile,
									filename, ObjectInc ) == FALSE )
				{
					goto wrterr;
				}

				ObjectInc += pTheObject->GetNumOfPoints();
			}
		}
	}
	else
	{
		if(fprintf(stream,"LEVELS %i\n", exp->NumOfObjects)<0)
		{
			goto wrterr;
		}

		int	iLevel = 0;

		for (int k=0; k < myScene.Count(); k++)
		{
			PIEObject* pTheObject = exp->GetPIEObject(k);

			if ( myScene[k]->obj->SuperClassID() == GEOMOBJECT_CLASS_ID &&
				 pTheObject->GetNumOfPoints() > 2 &&
				 pTheObject->GetNumOfPolygons() > 0  )
			{
				iLevel++;

				if(fprintf(stream,"LEVEL %i\n", iLevel)<0)
				{
					goto wrterr;
				}

				if(fprintf(stream,"POINTS %d\n",pTheObject->GetNumOfPoints())<0)
				{
					goto wrterr;
				}

				if ( WriteObjVertices( stream, pTheObject ) == FALSE )
				{
					goto wrterr;
				}

				if(fprintf(stream,"POLYGONS %d\n",pTheObject->GetNumOfPolygons())<0)
				{
					goto wrterr;
				}

				if ( WriteObjPolys( stream, pTheObject, theFile,
									filename, ObjectInc ) == FALSE )
				{
					goto wrterr;
				}
			}
		}
	}

	if ( exp->exType == ONE_OBJECT )
	{
		if ((exp->connectorCount > 0) && (exp->connectorCount <= MAX_CONNECTORS))
		{
			if(fprintf(stream,"CONNECTORS %d\n",exp->connectorCount)<0)
			{
				goto wrterr;
			}
			for (i = 0; i < (int)exp->connectorCount; i++)
			{
				if(fprintf(stream,"\t%d %d %d\n",(int)exp->connectors[i].x,
							(int)exp->connectors[i].y,(int)exp->connectors[i].z)<0)
				{
					goto wrterr;
				}
			}
		}
	}

	if(theFile.Close())
	{
		goto wrterr;
	}

	// no bmp output for PIE files

	return 1;	
}

/***************************************************************************/

#ifdef MAX25
int PIEExport::DoExport(const TCHAR *filename,ExpInterface *ei,
						Interface *gi, BOOL suppressPrompts) 
#else
int	PIEExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts, DWORD options)
#endif
{
	int				status;

	status = pie_object_create( ei, gi, this );
	if( status > 0 )
	{
		status = ani_save( filename, ei, gi, this );
		if( status > 0 )
		{
			status = pie_save( filename, ei, gi, this );
		}
	}

	if(status == 0)
		return 1;		// Dialog cancelled
	if(status > 0)
	{
		#ifdef DBGPIE
			DebugPrint("PIE status OK!\n");
		#endif
	}

	#ifdef DBGPIE
	else
	if(status < 0)
	{
		DebugPrint("Error somewhere in PIE!\n");
	}
	#endif
	
	return(status);
}

/***************************************************************************/

int PIEObject::StoreVertex(Point3& Vertex)
{
	//for each point currently in the list
	for (int i=0; i < NumOfPoints; i++)
	{
		//if the point is already in the list, don't store it
		if (Points[i] == Vertex)
		{
			return i;
		}
	}
	Points[NumOfPoints++] = Vertex;
	return (NumOfPoints-1);
}

/***************************************************************************/

//add the bitmaps used for materials into the one file for exporting
EXPORT_IMAGE* PIEExport::CheckTextureFile(BitmapInfo& bInfo, Bitmap& BitmapToCopy)
{
	//look through the list of bitmaps that have already been used to ensure
	//that the same one is not saved twice
	for (int i=0; i < m_BitmapCount; i++)
	{
		if (!stricmp(bInfo.Name(),pBitmapList[i]->pName))
		{
			return pBitmapList[i];
		}
	}
	//bitmap not already in list, so add it
	pBitmapList[i] = new(EXPORT_IMAGE);
	//create a copy of the bitmap name
	pBitmapList[i]->pName = (TCHAR*) malloc(strlen(bInfo.Name())+1);
	if (!pBitmapList[i]->pName)
	{
		//abandon the program
		OutputDebugString("Cannot allocate memory for copying texture image");
		assert(0);
	}

	strcpy(pBitmapList[i]->pName, bInfo.Name());
	
	//increment the count of bitmaps used for the textures
	m_BitmapCount++;

	return pBitmapList[i];	
}



BOOL PIEObject::output3VertexPoly(FILE* stream, WorkFile& theFile, const TCHAR* filename,
								  int polyNum, int objectNum , int textureHeight)
{
	int MaterialID;

	if (GetNumOfMaterials() <= 1)
	{
		MaterialID = 0;
	}
	else
	{
		MaterialID = TexturePolygons[polyNum][MAT_ID];
	}

	//Polygons must be defined in a clockwise direction
	int vertex1 = 3, vertex2 = 2, vertex3 = 1;

	if (fprintf(stream,"\t%x %d %d %d %d",
		arrMaterial[MaterialID].drawID,
		Polygons[polyNum][0],		//number of polygons
		Polygons[polyNum][vertex1]+objectNum,
		Polygons[polyNum][vertex2]+objectNum,
		Polygons[polyNum][vertex3]+objectNum) < 0)
	{
wrterr:
		Alert(IDS_WRITEERROR);
		theFile.Close();
		remove(filename);
		return 0;
	}

	//if animated textures output animation data
	if (arrMaterial[MaterialID].drawID & TEXTURE_ANIMATION)
	{
		if (fprintf(stream," %d %d %d %d",
			(int)arrMaterial[MaterialID].texAnimationFrames,
			(int)arrMaterial[MaterialID].playbackRate,
			(int)arrMaterial[MaterialID].TextureWidth,
			(int)arrMaterial[MaterialID].TextureHeight) < 0)
		{
				goto wrterr;
		}
	}
		
	if (arrMaterial[MaterialID].drawID && TEXTURED)
	{
		float StartX = arrMaterial[MaterialID].StartX;
		float StartY = arrMaterial[MaterialID].StartY;
		if (arrMaterial[MaterialID].UNegative)
		{
			StartX += (arrMaterial[MaterialID].TextureWidth);
		}
		if (arrMaterial[MaterialID].VNegative)
		{
			StartY += (arrMaterial[MaterialID].TextureHeight);
		}
		
		float TextureX, TextureY;
		Point3 texturePoint;

		//for three vertices
		for(int v = 0; v < 3; v++)
		{
			//get the texture coords
			if (v == 0)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex1]);
			}
			else if (v == 1)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex2]);
			}
			else if (v == 2)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex3]);
			}

			//calculate the coords for this bmp
			if (arrMaterial[MaterialID].UNegative)
			{
				TextureX = StartX - texturePoint.x * 
					(arrMaterial[MaterialID].TextureWidth);
			}
			else
			{
				TextureX = StartX + texturePoint.x * 
					(arrMaterial[MaterialID].TextureWidth);
			}
			if (arrMaterial[MaterialID].VNegative)
			{
				TextureY = StartY - ((float)1.0 - texturePoint.y) * 
					(arrMaterial[MaterialID].TextureHeight);
			}
			else
			{
				TextureY = StartY + ((float)1.0 - texturePoint.y) * 
					(arrMaterial[MaterialID].TextureHeight);
			}
			//rotate the texture coords if necessary
			if (arrMaterial[MaterialID].angleInc)
			{
				Point3 textureBox;
				textureBox.x = TextureX;
				textureBox.y = TextureY;

#if 0
				Alert(IDS_ROTATION_ERROR);

				if ((arrMaterial[MaterialID].UNegative && arrMaterial[MaterialID].VNegative) ||
					(!arrMaterial[MaterialID].UNegative && !arrMaterial[MaterialID].VNegative))
				{
					rotateTexCoord(&textureBox,
						arrMaterial[MaterialID].angleInc,
						(float)arrMaterial[MaterialID].BitmapWidth,
						(float)arrMaterial[MaterialID].BitmapHeight);
				}
				else
				{
					rotateTexCoord(&textureBox,
						-arrMaterial[MaterialID].angleInc,
						(float)arrMaterial[MaterialID].BitmapWidth,
						(float)arrMaterial[MaterialID].BitmapHeight);
				}

				TextureX = textureBox.x;
				TextureY = textureBox.y;
#else			
				TextureX = 0.0f;
				TextureY = 0.0f;
#endif
			}

			//convert Texture coords to ints, rounding up if necessary
			if (fprintf(stream," %d %d", PIERoundUp(TextureX, arrMaterial[MaterialID].BitmapWidth), 
				PIERoundUp(TextureY, arrMaterial[MaterialID].BitmapHeight)) < 0 )
			{
				goto wrterr;
			}
		}
	}
	return TRUE;
}

BOOL PIEObject::output4VertexPoly(FILE* stream, WorkFile& theFile, const TCHAR* filename,
								  int polyNum, int objectNum, int textureHeight)
{
	int MaterialID;

	if (GetNumOfMaterials() <= 1)
	{
		MaterialID = 0;
	}
	else
	{
		MaterialID = TexturePolygons[polyNum][MAT_ID];
	}

	//Polygons must be defined in a clockwise direction
	int vertex1 = 4, vertex2 = 3, vertex3 = 2, vertex4 = 1;

	if (fprintf(stream,"\t%x %d %d %d %d %d",
		arrMaterial[MaterialID].drawID,
		Polygons[polyNum][0],		//number of polygons
		Polygons[polyNum][vertex1]+objectNum,
		Polygons[polyNum][vertex2]+objectNum,
		Polygons[polyNum][vertex3]+objectNum,
		Polygons[polyNum][vertex4]+objectNum) < 0)
	{
		wrterr:
		Alert(IDS_WRITEERROR);
		theFile.Close();
		remove(filename);
		return 0;
	}

	//if animated textures output animation data
	if (arrMaterial[MaterialID].drawID & TEXTURE_ANIMATION)
	{
		if (fprintf(stream," %d %d %d %d",
			(int)arrMaterial[MaterialID].texAnimationFrames,
			(int)arrMaterial[MaterialID].playbackRate,
			(int)arrMaterial[MaterialID].TextureWidth,
			(int)arrMaterial[MaterialID].TextureHeight) < 0)
		{
				goto wrterr;
		}
	}

	if (arrMaterial[MaterialID].drawID & TEXTURED)
	{
		float StartX = arrMaterial[MaterialID].StartX;
		float StartY = arrMaterial[MaterialID].StartY;
		if (arrMaterial[MaterialID].UNegative)
		{
			StartX += (arrMaterial[MaterialID].TextureWidth);
		}
		if (arrMaterial[MaterialID].VNegative)
		{
			StartY += (arrMaterial[MaterialID].TextureHeight);
		}
		
		float TextureX, TextureY;
		Point3 texturePoint;

		//for four vertices
		for(int v = 0; v < 4; v++)
		{
			//get the texture coords
			if (v == 0)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex1]);
			}
			else if (v == 1)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex2]);
			}
			else if (v == 2)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex3]);
			}
			else if (v == 3)
			{
				texturePoint = *(GetTextureVerts() + TexturePolygons[polyNum][vertex4]);
			}

			//calculate the coords for this bmp
			if (arrMaterial[MaterialID].UNegative)
			{
				TextureX = StartX - texturePoint.x * 
					(arrMaterial[MaterialID].TextureWidth);
			}
			else
			{
				TextureX = StartX + texturePoint.x * 
					(arrMaterial[MaterialID].TextureWidth);
			}
			if (arrMaterial[MaterialID].VNegative)
			{
				TextureY = StartY - ((float)1.0 - texturePoint.y) * 
					(arrMaterial[MaterialID].TextureHeight);
			}
			else
			{
				TextureY = StartY + ((float)1.0 - texturePoint.y) * 
					(arrMaterial[MaterialID].TextureHeight);
			}
			//rotate the texture coords if necessary
			if (arrMaterial[MaterialID].angleInc)
			{
				Point3 textureBox;
				textureBox.x = TextureX;
				textureBox.y = TextureY;

#if 0
				Alert(IDS_ROTATION_ERROR);

				if ((arrMaterial[MaterialID].UNegative && arrMaterial[MaterialID].VNegative) ||
					(!arrMaterial[MaterialID].UNegative && !arrMaterial[MaterialID].VNegative))
				{
					rotateTexCoord(&textureBox,
						arrMaterial[MaterialID].angleInc,
						(float)arrMaterial[MaterialID].BitmapWidth,
						(float)arrMaterial[MaterialID].BitmapHeight);
				}
				else
				{
					rotateTexCoord(&textureBox,
						-arrMaterial[MaterialID].angleInc,
						(float)arrMaterial[MaterialID].BitmapWidth,
						(float)arrMaterial[MaterialID].BitmapHeight);
				}
				
				TextureX = textureBox.x;
				TextureY = textureBox.y;
#else
				TextureX = 0.0f;
				TextureY = 0.0f;
#endif
			}

			//convert Texture coords to ints, rounding up if necessary
			if (fprintf(stream," %d %d", PIERoundUp(TextureX, arrMaterial[MaterialID].BitmapWidth), 
				PIERoundUp(TextureY, arrMaterial[MaterialID].BitmapHeight)) < 0 )
			{
				goto wrterr;
			}
		}
	}
	return TRUE;
}

/***************************************************************************/
/*
 * Animation stuff stolen from impexp\asciiexp sample directory in
 * maxsdk samples
 */
/***************************************************************************/

#define ALMOST_ZERO 1.0e-3f

/***************************************************************************/

BOOL
EqualPoint3( Point3 p1, Point3 p2 )
{
	if (fabs(p1.x - p2.x) > ALMOST_ZERO)
		return FALSE;
	if (fabs(p1.y - p2.y) > ALMOST_ZERO)
		return FALSE;
	if (fabs(p1.z - p2.z) > ALMOST_ZERO)
		return FALSE;

	return TRUE;
}

/***************************************************************************/
// International settings in Windows could cause a number to be written
// with a "," instead of a ".".
// To compensate for this we need to convert all , to . in order to make the
// format consistent.
/***************************************************************************/

void PIEObject::CommaScan(TCHAR* buf)
{
    for(; *buf; buf++) if (*buf == ',') *buf = '.';
}

/***************************************************************************/

const	int	FLOAT_PRECISION		=	4;
const	int	INT_FIELD_WIDTH		=	7;

/***************************************************************************/

TSTR PIEObject::Format(int value)
{
	TCHAR buf[50], szStr[10];
	
	sprintf( szStr, "%%%ii", INT_FIELD_WIDTH );
	sprintf(buf, szStr, value);
	return buf;
}

/***************************************************************************/

TSTR PIEObject::Format(float value)
{
	TCHAR buf[40];
	
	sprintf(buf, szFmtStr, value);
	CommaScan(buf);
	return TSTR(buf);
}

/***************************************************************************/

TSTR PIEObject::Format(Point3 value)
{
	TCHAR buf[120];
	TCHAR fmt[120];
	
	sprintf(fmt, "%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.x, value.y, value.z);

	CommaScan(buf);
	return buf;
}

/***************************************************************************/

TSTR PIEObject::Format(Color value)
{
	TCHAR buf[120];
	TCHAR fmt[120];
	
	sprintf(fmt, "%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.r, value.g, value.b);

	CommaScan(buf);
	return buf;
}

/***************************************************************************/

TSTR PIEObject::Format(AngAxis value)
{
	TCHAR buf[160];
	TCHAR fmt[160];
	
	sprintf(fmt, "%s\t%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.axis.x, value.axis.y, value.axis.z, value.angle);

	CommaScan(buf);
	return buf;
}

/***************************************************************************/

TSTR PIEObject::Format(Quat value)
{
	// A Quat is converted to an AngAxis before output.
	
	Point3 axis;
	float angle;
	AngAxisFromQ(value, &angle, axis);
	
	return Format(AngAxis(axis, angle));
}

/***************************************************************************/

TSTR PIEObject::Format(ScaleValue value)
{
	TCHAR buf[280];
	
	sprintf(buf, "%s %s", Format(value.s), Format(value.q));
	CommaScan(buf);
	return buf;
}

/***************************************************************************/
/*
 * Get hold of the transform controllers for the node
 *
/***************************************************************************/

void PIEObject::ExportAnimKeys( FILE* stream, const TCHAR *szFileName,
								INode* node, Interface *ip, int indentLevel) 
{
	TimeValue	start = ip->GetAnimRange().Start();
	TimeValue	end = ip->GetAnimRange().End();
	TimeValue	t;
	int			delta = GetTicksPerFrame();
	Matrix3		tm;
	AffineParts	ap;
	Quat		prevQ;
	int			iSamples = ((end - start) / delta) + 1;
	Point3		pos, ang, scale;
	TSTR		indent;

	/* set floating point output format */
	sprintf( szFmtStr, "%%4.%df", FLOAT_PRECISION );

	GetIndent( indent, indentLevel );

	prevQ.Identity();

	/* get first (reference) matrix */
	Matrix3		mRef = Inverse(node->GetObjectTM(0));

	for (t=start; t<=end; t+=delta)
	{
		/* get transformation matrix referenced from 1st position
		 * by multiplying by inverse of 1st position matrix
		 */
		tm = mRef * node->GetObjectTM(t);

		/* decompose matrix */
		decomp_affine(tm, &ap);

		/* get translations */
		pos = ap.t * float(INT_SCALE);

		/* get rotations */
		QuatToEuler( ap.q, ang );

		/* convert to degrees */
		ang.x = RadToDeg( ang.x );
		ang.y = RadToDeg( ang.y );
		ang.z = RadToDeg( ang.z );
		ang *= float(INT_SCALE);

		/* get scales */
        ScaleValue	sv(ap.k, ap.u);
        scale = sv.s;
        if (ap.f < 0.0f)
		{
			scale *= -float(INT_SCALE);
		}
		else
		{
			scale *= float(INT_SCALE);
		}

		/* output the sample */
		fprintf(stream, "%s\t%3d\t%s %s %s %s %s %s %s %s %s\n",
				indent.data(),
				(t-start)/delta,
				Format(int(pos.x)),
				Format(int(pos.y)),
				Format(int(pos.z)),
				Format(int(ang.x)),
				Format(int(ang.y)),
				Format(int(ang.z)),
				Format(int(scale.x)),
				Format(int(scale.y)),
				Format(int(scale.z)) );
	}
}

/***************************************************************************/
/*
 * To really see if a node is animated we can step through the animation range
 * and decompose the TM matrix for every frame and examine the components.
 * This way we can identify position, rotation and scale animation separately.
 * 
 * Some controllers makes it problematic to examine the TMContollers instead of
 * the actual TMMatrix. For example, a path controller is a position controller, 
 * but if you turn on follow and banking, it will also affect the rotation component.
 * If we want to, we can examine the position, rotation and scale controllers and
 * if they all are Linear, Hybrid (bezier) or TCB, then we could export the actual keys.
 * This is not at all difficult, but the importer has to know the exact interpolation
 * algorithm in order to use it. The source code to the interpolation routines are available
 * to ADN members.
 * 
 * For an example of how to export actual keys, look at DumpPoint3Keys() below.
 * This method will check the actual controller to determine if the controller is known.
 * If we know how to work this controller, its actual keys will be exported,
 * otherwise the controller will be sampled using the user specified sampling frequency.
 */
/***************************************************************************/

BOOL PIEObject::CheckForAnimation( INode* node, Interface *ip,
									BOOL& bPos, BOOL& bRot, BOOL& bScale )
{
	TimeValue start = ip->GetAnimRange().Start();
	TimeValue end = ip->GetAnimRange().End();
	TimeValue t;
	int delta = GetTicksPerFrame();
	Matrix3 tm;
	AffineParts ap;
	Point3 firstPos;
	float rotAngle, firstRotAngle;
	Point3 rotAxis;
	Point3 firstScaleFactor;

	bPos = bRot = bScale = FALSE;

	for (t=start; t<=end; t+=delta) {
		tm = node->GetNodeTM(t) * Inverse(node->GetParentTM(t));

		decomp_affine(tm, &ap);

		AngAxisFromQ(ap.q, &rotAngle, rotAxis);

		if (t != start) {
			if (!EqualPoint3(ap.t, firstPos)) {
				bPos = TRUE;
			}
			// We examine the rotation angle to see if the rotation component
			// has changed.
			// Although not entirely true, it should work.
			// It is rare that the rotation axis is animated without
			// the rotation angle being somewhat affected.
			if (fabs(rotAngle - firstRotAngle) > ALMOST_ZERO) {
				bRot = TRUE;
			}
			if (!EqualPoint3(ap.k, firstScaleFactor)) {
				bScale = TRUE;
			}			
		}
		else {
			firstPos = ap.t;
			firstRotAngle = rotAngle;
			firstScaleFactor = ap.k;
		}

		// No need to continue looping if all components are animated
		if (bPos && bRot && bScale)
			break;
	}

	return bPos || bRot || bScale;
}

/***************************************************************************/
