

/*

  3dfx non-dll test prog

*/
#include <windows.h>
#include "frame.h"

#define dllName "glide2x.dll"

#include "Dglide.h"		// new glide.h - old glide.h MUST be renamed glideold.h!



void NoGlideDLLError(void);

HINSTANCE dllHandle = NULL;

// Pointers generated from dll

ConvertAndDownloadRle_fpt	ConvertAndDownloadRle=(void *)NoGlideDLLError;
grAADrawLine_fpt	grAADrawLine=(void *)NoGlideDLLError;
grAADrawPoint_fpt	grAADrawPoint=(void *)NoGlideDLLError;
grAADrawPolygon_fpt	grAADrawPolygon=(void *)NoGlideDLLError;
grAADrawPolygonVertexList_fpt	grAADrawPolygonVertexList=(void *)NoGlideDLLError;
grAADrawTriangle_fpt	grAADrawTriangle=(void *)NoGlideDLLError;
grAlphaBlendFunction_fpt	grAlphaBlendFunction=(void *)NoGlideDLLError;
grAlphaCombine_fpt	grAlphaCombine=(void *)NoGlideDLLError;
grAlphaControlsITRGBLighting_fpt	grAlphaControlsITRGBLighting=(void *)NoGlideDLLError;
grAlphaTestFunction_fpt	grAlphaTestFunction=(void *)NoGlideDLLError;
grAlphaTestReferenceValue_fpt	grAlphaTestReferenceValue=(void *)NoGlideDLLError;
grBufferClear_fpt	grBufferClear=(void *)NoGlideDLLError;
grBufferNumPending_fpt	grBufferNumPending=(void *)NoGlideDLLError;
grBufferSwap_fpt	grBufferSwap=(void *)NoGlideDLLError;
grCheckForRoom_fpt	grCheckForRoom=(void *)NoGlideDLLError;
grChromakeyMode_fpt	grChromakeyMode=(void *)NoGlideDLLError;
grChromakeyValue_fpt	grChromakeyValue=(void *)NoGlideDLLError;
grClipWindow_fpt	grClipWindow=(void *)NoGlideDLLError;
grColorCombine_fpt	grColorCombine=(void *)NoGlideDLLError;
grColorMask_fpt	grColorMask=(void *)NoGlideDLLError;
grConstantColorValue4_fpt	grConstantColorValue4=(void *)NoGlideDLLError;
grConstantColorValue_fpt	grConstantColorValue=(void *)NoGlideDLLError;
grCullMode_fpt	grCullMode=(void *)NoGlideDLLError;
grDepthBiasLevel_fpt	grDepthBiasLevel=(void *)NoGlideDLLError;
grDepthBufferFunction_fpt	grDepthBufferFunction=(void *)NoGlideDLLError;
grDepthBufferMode_fpt	grDepthBufferMode=(void *)NoGlideDLLError;
grDepthMask_fpt	grDepthMask=(void *)NoGlideDLLError;
grDisableAllEffects_fpt	grDisableAllEffects=(void *)NoGlideDLLError;
grDitherMode_fpt	grDitherMode=(void *)NoGlideDLLError;
grDrawLine_fpt	grDrawLine=(void *)NoGlideDLLError;
grDrawPlanarPolygon_fpt	grDrawPlanarPolygon=(void *)NoGlideDLLError;
grDrawPlanarPolygonVertexList_fpt	grDrawPlanarPolygonVertexList=(void *)NoGlideDLLError;
grDrawPoint_fpt	grDrawPoint=(void *)NoGlideDLLError;
grDrawPolygon_fpt	grDrawPolygon=(void *)NoGlideDLLError;
grDrawPolygonVertexList_fpt	grDrawPolygonVertexList=(void *)NoGlideDLLError;
grDrawTriangle_fpt	grDrawTriangle=(void *)NoGlideDLLError;
grErrorSetCallback_fpt	grErrorSetCallback=(void *)NoGlideDLLError;
grFogColorValue_fpt	grFogColorValue=(void *)NoGlideDLLError;
grFogMode_fpt	grFogMode=(void *)NoGlideDLLError;
grFogTable_fpt	grFogTable=(void *)NoGlideDLLError;
grGammaCorrectionValue_fpt	grGammaCorrectionValue=(void *)NoGlideDLLError;
grGlideGetState_fpt	grGlideGetState=(void *)NoGlideDLLError;
grGlideGetVersion_fpt	grGlideGetVersion=(void *)NoGlideDLLError;
grGlideInit_fpt	grGlideInit=(void *)NoGlideDLLError;
grGlideSetState_fpt	grGlideSetState=(void *)NoGlideDLLError;
grGlideShamelessPlug_fpt	grGlideShamelessPlug=(void *)NoGlideDLLError;
grGlideShutdown_fpt	grGlideShutdown=(void *)NoGlideDLLError;
grHints_fpt	grHints=(void *)NoGlideDLLError;
grLfbConstantAlpha_fpt	grLfbConstantAlpha=(void *)NoGlideDLLError;
grLfbConstantDepth_fpt	grLfbConstantDepth=(void *)NoGlideDLLError;
grLfbLock_fpt	grLfbLock=(void *)NoGlideDLLError;
grLfbReadRegion_fpt	grLfbReadRegion=(void *)NoGlideDLLError;
grLfbUnlock_fpt	grLfbUnlock=(void *)NoGlideDLLError;
grLfbWriteColorFormat_fpt	grLfbWriteColorFormat=(void *)NoGlideDLLError;
grLfbWriteColorSwizzle_fpt	grLfbWriteColorSwizzle=(void *)NoGlideDLLError;
grLfbWriteRegion_fpt	grLfbWriteRegion=(void *)NoGlideDLLError;
grRenderBuffer_fpt	grRenderBuffer=(void *)NoGlideDLLError;
grResetTriStats_fpt	grResetTriStats=(void *)NoGlideDLLError;
grSplash_fpt	grSplash=(void *)NoGlideDLLError;
// grSstConfigPipeline_fpt	grSstConfigPipeline=(void *)NoGlideDLLError;
grSstControl_fpt	grSstControl=(void *)NoGlideDLLError;
grSstIdle_fpt	grSstIdle=(void *)NoGlideDLLError;
grSstIsBusy_fpt	grSstIsBusy=(void *)NoGlideDLLError;
grSstOrigin_fpt	grSstOrigin=(void *)NoGlideDLLError;
grSstPerfStats_fpt	grSstPerfStats=(void *)NoGlideDLLError;
grSstQueryBoards_fpt	grSstQueryBoards=(void *)NoGlideDLLError;
grSstQueryHardware_fpt	grSstQueryHardware=(void *)NoGlideDLLError;
grSstResetPerfStats_fpt	grSstResetPerfStats=(void *)NoGlideDLLError;
grSstScreenHeight_fpt	grSstScreenHeight=(void *)NoGlideDLLError;
grSstScreenWidth_fpt	grSstScreenWidth=(void *)NoGlideDLLError;
grSstSelect_fpt	grSstSelect=(void *)NoGlideDLLError;
grSstStatus_fpt	grSstStatus=(void *)NoGlideDLLError;
grSstVRetraceOn_fpt	grSstVRetraceOn=(void *)NoGlideDLLError;
//grSstVidMode_fpt	grSstVidMode=(void *)NoGlideDLLError;
grSstVideoLine_fpt	grSstVideoLine=(void *)NoGlideDLLError;
grSstWinClose_fpt	grSstWinClose=(void *)NoGlideDLLError;
grSstWinOpen_fpt	grSstWinOpen=(void *)NoGlideDLLError;
grTexCalcMemRequired_fpt	grTexCalcMemRequired=(void *)NoGlideDLLError;
grTexClampMode_fpt	grTexClampMode=(void *)NoGlideDLLError;
grTexCombine_fpt	grTexCombine=(void *)NoGlideDLLError;
grTexCombineFunction_fpt	grTexCombineFunction=(void *)NoGlideDLLError;
grTexDetailControl_fpt	grTexDetailControl=(void *)NoGlideDLLError;
grTexDownloadMipMap_fpt	grTexDownloadMipMap=(void *)NoGlideDLLError;
grTexDownloadMipMapLevel_fpt	grTexDownloadMipMapLevel=(void *)NoGlideDLLError;
grTexDownloadMipMapLevelPartial_fpt	grTexDownloadMipMapLevelPartial=(void *)NoGlideDLLError;
grTexDownloadTable_fpt	grTexDownloadTable=(void *)NoGlideDLLError;
grTexDownloadTablePartial_fpt	grTexDownloadTablePartial=(void *)NoGlideDLLError;
grTexFilterMode_fpt	grTexFilterMode=(void *)NoGlideDLLError;
grTexLodBiasValue_fpt	grTexLodBiasValue=(void *)NoGlideDLLError;
grTexMaxAddress_fpt	grTexMaxAddress=(void *)NoGlideDLLError;
grTexMinAddress_fpt	grTexMinAddress=(void *)NoGlideDLLError;
grTexMipMapMode_fpt	grTexMipMapMode=(void *)NoGlideDLLError;
grTexMultibase_fpt	grTexMultibase=(void *)NoGlideDLLError;
grTexMultibaseAddress_fpt	grTexMultibaseAddress=(void *)NoGlideDLLError;
grTexNCCTable_fpt	grTexNCCTable=(void *)NoGlideDLLError;
grTexSource_fpt	grTexSource=(void *)NoGlideDLLError;
grTexTextureMemRequired_fpt	grTexTextureMemRequired=(void *)NoGlideDLLError;
grTriStats_fpt	grTriStats=(void *)NoGlideDLLError;
//gu3dfGetInfo_fpt	gu3dfGetInfo=(void *)NoGlideDLLError;
//gu3dfLoad_fpt	gu3dfLoad=(void *)NoGlideDLLError;
//guAADrawTriangleWithClip_fpt	guAADrawTriangleWithClip=(void *)NoGlideDLLError;
//guAlphaSource_fpt	guAlphaSource=(void *)NoGlideDLLError;
//guColorCombineFunction_fpt	guColorCombineFunction=(void *)NoGlideDLLError;
//guDrawPolygonVertexListWithClip_fpt	guDrawPolygonVertexListWithClip=(void *)NoGlideDLLError;
//guDrawTriangleWithClip_fpt	guDrawTriangleWithClip=(void *)NoGlideDLLError;
//guEncodeRLE16_fpt	guEncodeRLE16=(void *)NoGlideDLLError;
//guEndianSwapBytes_fpt	guEndianSwapBytes=(void *)NoGlideDLLError;
//guEndianSwapWords_fpt	guEndianSwapWords=(void *)NoGlideDLLError;
//guFogGenerateExp2_fpt	guFogGenerateExp2=(void *)NoGlideDLLError;
//guFogGenerateExp_fpt	guFogGenerateExp=(void *)NoGlideDLLError;
//guFogGenerateLinear_fpt	guFogGenerateLinear=(void *)NoGlideDLLError;
//guFogTableIndexToW_fpt	guFogTableIndexToW=(void *)NoGlideDLLError;
//guMPDrawTriangle_fpt	guMPDrawTriangle=(void *)NoGlideDLLError;
//guMPInit_fpt	guMPInit=(void *)NoGlideDLLError;
//guMPTexCombineFunction_fpt	guMPTexCombineFunction=(void *)NoGlideDLLError;
//guMPTexSource_fpt	guMPTexSource=(void *)NoGlideDLLError;
//guMovieSetName_fpt	guMovieSetName=(void *)NoGlideDLLError;
//guMovieStart_fpt	guMovieStart=(void *)NoGlideDLLError;
//guMovieStop_fpt	guMovieStop=(void *)NoGlideDLLError;
guTexAllocateMemory_fpt	guTexAllocateMemory=(void *)NoGlideDLLError;
guTexChangeAttributes_fpt	guTexChangeAttributes=(void *)NoGlideDLLError;
guTexCombineFunction_fpt	guTexCombineFunction=(void *)NoGlideDLLError;
//guTexCreateColorMipMap_fpt	guTexCreateColorMipMap=(void *)NoGlideDLLError;
guTexDownloadMipMap_fpt	guTexDownloadMipMap=(void *)NoGlideDLLError;
guTexDownloadMipMapLevel_fpt	guTexDownloadMipMapLevel=(void *)NoGlideDLLError;
guTexGetCurrentMipMap_fpt	guTexGetCurrentMipMap=(void *)NoGlideDLLError;
guTexGetMipMapInfo_fpt	guTexGetMipMapInfo=(void *)NoGlideDLLError;
guTexMemQueryAvail_fpt	guTexMemQueryAvail=(void *)NoGlideDLLError;
guTexMemReset_fpt	guTexMemReset=(void *)NoGlideDLLError;
guTexSource_fpt	guTexSource=(void *)NoGlideDLLError;


typedef struct
{
	void *FuncPtr;
	char *DllFuncName;
} FUNC3DFX;


void NoGlideDLLError(void)
{
	ASSERT((FALSE,"Glide function called with missing DLL"));
}


FUNC3DFX Functions3DFX[]=
{
	{	&ConvertAndDownloadRle,			"_ConvertAndDownloadRle@64"	},
	{	&grAADrawLine,					"_grAADrawLine@8"	},
	{	&grAADrawPoint,					"_grAADrawPoint@4"	},
	{	&grAADrawPolygon,				"_grAADrawPolygon@12"	},
	{	&grAADrawPolygonVertexList,	 	"_grAADrawPolygonVertexList@8"	},
	{	&grAADrawTriangle,				"_grAADrawTriangle@24"	},
	{	&grAlphaBlendFunction,			"_grAlphaBlendFunction@16"	},
	{	&grAlphaCombine,					"_grAlphaCombine@20"	},
	{	&grAlphaControlsITRGBLighting,  	"_grAlphaControlsITRGBLighting@4"	},
	{	&grAlphaTestFunction,			"_grAlphaTestFunction@4"	},
	{	&grAlphaTestReferenceValue,	   	"_grAlphaTestReferenceValue@4"	},
	{	&grBufferClear,					"_grBufferClear@12"	},
	{	&grBufferNumPending,				"_grBufferNumPending@0"	},
	{	&grBufferSwap,					"_grBufferSwap@4"	},
	{	&grCheckForRoom,					"_grCheckForRoom@4"	},
	{	&grChromakeyMode,				"_grChromakeyMode@4"	},
	{	&grChromakeyValue,				"_grChromakeyValue@4"	},
	{	&grClipWindow,					"_grClipWindow@16"	},
	{	&grColorCombine,					"_grColorCombine@20"	},
	{	&grColorMask,					"_grColorMask@8"	},
	{	&grConstantColorValue4,			"_grConstantColorValue4@16"	},
	{	&grConstantColorValue,			"_grConstantColorValue@4"	},
	{	&grCullMode,						"_grCullMode@4"	},
	{	&grDepthBiasLevel,				"_grDepthBiasLevel@4"	},
	{	&grDepthBufferFunction,			"_grDepthBufferFunction@4"	},
	{	&grDepthBufferMode,				"_grDepthBufferMode@4"	},
	{	&grDepthMask,					"_grDepthMask@4"	},
	{	&grDisableAllEffects,			"_grDisableAllEffects@0"	},
	{	&grDitherMode,					"_grDitherMode@4"	},
	{	&grDrawLine,						"_grDrawLine@8"	},
	{	&grDrawPlanarPolygon,			"_grDrawPlanarPolygon@12"	},
	{	&grDrawPlanarPolygonVertexList,	"_grDrawPlanarPolygonVertexList@8"	},
	{	&grDrawPoint,					"_grDrawPoint@4"	},
	{	&grDrawPolygon,					"_grDrawPolygon@12"	},
	{	&grDrawPolygonVertexList,  		"_grDrawPolygonVertexList@8"	},
	{	&grDrawTriangle,					"_grDrawTriangle@12"	},
	{	&grErrorSetCallback,				"_grErrorSetCallback@4"	},
	{	&grFogColorValue,				"_grFogColorValue@4"	},
	{	&grFogMode,						"_grFogMode@4"	},
	{	&grFogTable,						"_grFogTable@4"	},
	{	&grGammaCorrectionValue,			"_grGammaCorrectionValue@4"	},
	{	&grGlideGetState,				"_grGlideGetState@4"	},
	{	&grGlideGetVersion,				"_grGlideGetVersion@4"	},
	{	&grGlideInit,					"_grGlideInit@0"	},
	{	&grGlideSetState,				"_grGlideSetState@4"	},
	{	&grGlideShamelessPlug,			"_grGlideShamelessPlug@4"	},
	{	&grGlideShutdown,				"_grGlideShutdown@0"	},
	{	&grHints,						"_grHints@8"	},
	{	&grLfbConstantAlpha,				"_grLfbConstantAlpha@4"	},
	{	&grLfbConstantDepth,				"_grLfbConstantDepth@4"	},
	{	&grLfbLock,						"_grLfbLock@24"	},
	{	&grLfbReadRegion,				"_grLfbReadRegion@28"	},
	{	&grLfbUnlock,					"_grLfbUnlock@8"	},
	{	&grLfbWriteColorFormat,			"_grLfbWriteColorFormat@4"	},
	{	&grLfbWriteColorSwizzle,			"_grLfbWriteColorSwizzle@8"	},
	{	&grLfbWriteRegion,				"_grLfbWriteRegion@32"	},
	{	&grRenderBuffer,					"_grRenderBuffer@4"	},
	{	&grResetTriStats,				"_grResetTriStats@0"	},
	{	&grSplash,						"_grSplash@20"	},
//	{	&grSstConfigPipeline,			"_grSstConfigPipeline@12"	},
	{	&grSstControl,					"_grSstControl@4"	},
	{	&grSstIdle,						"_grSstIdle@0"	},
	{	&grSstIsBusy,					"_grSstIsBusy@0"	},
	{	&grSstOrigin,					"_grSstOrigin@4"	},
	{	&grSstPerfStats,					"_grSstPerfStats@4"	},
	{	&grSstQueryBoards,				"_grSstQueryBoards@4"	},
	{	&grSstQueryHardware,				"_grSstQueryHardware@4"	},
	{	&grSstResetPerfStats,			"_grSstResetPerfStats@0"	},
	{	&grSstScreenHeight,				"_grSstScreenHeight@0"	},
	{	&grSstScreenWidth,				"_grSstScreenWidth@0"	},
	{	&grSstSelect,					"_grSstSelect@4"	},
	{	&grSstStatus,					"_grSstStatus@0"	},
	{	&grSstVRetraceOn,			"_grSstVRetraceOn@0"	},
//	{	&grSstVidMode,			"_grSstVidMode@8"	},
	{	&grSstVideoLine,			"_grSstVideoLine@0"	},
	{	&grSstWinClose,			"_grSstWinClose@0"	},
	{	&grSstWinOpen,			"_grSstWinOpen@28"	},
	{	&grTexCalcMemRequired,			"_grTexCalcMemRequired@16"	},
	{	&grTexClampMode,			"_grTexClampMode@12"	},
	{	&grTexCombine,			"_grTexCombine@28"	},
	{	&grTexCombineFunction,			"_grTexCombineFunction@8"	},
	{	&grTexDetailControl,			"_grTexDetailControl@16"	},
	{	&grTexDownloadMipMap,			"_grTexDownloadMipMap@16"	},
	{	&grTexDownloadMipMapLevel,			"_grTexDownloadMipMapLevel@32"	},
	{	&grTexDownloadMipMapLevelPartial,			"_grTexDownloadMipMapLevelPartial@40"	},
	{	&grTexDownloadTable,			"_grTexDownloadTable@12"	},
	{	&grTexDownloadTablePartial,			"_grTexDownloadTablePartial@20"	},
	{	&grTexFilterMode,			"_grTexFilterMode@12"	},
	{	&grTexLodBiasValue,			"_grTexLodBiasValue@8"	},
	{	&grTexMaxAddress,			"_grTexMaxAddress@4"	},
	{	&grTexMinAddress,			"_grTexMinAddress@4"	},
	{	&grTexMipMapMode,			"_grTexMipMapMode@12"	},
	{	&grTexMultibase,			"_grTexMultibase@8"	},
	{	&grTexMultibaseAddress,			"_grTexMultibaseAddress@20"	},
	{	&grTexNCCTable,			"_grTexNCCTable@8"	},
	{	&grTexSource,			"_grTexSource@16"	},
	{	&grTexTextureMemRequired,			"_grTexTextureMemRequired@8"	},
	{	&grTriStats,			"_grTriStats@8"	},
//	{	&gu3dfGetInfo,			"_gu3dfGetInfo@8"	},
//	{	&gu3dfLoad,			"_gu3dfLoad@8"	},
//	{	&guAADrawTriangleWithClip,			"_guAADrawTriangleWithClip@12"	},
//	{	&guAlphaSource,			"_guAlphaSource@4"	},
//	{	&guColorCombineFunction,			"_guColorCombineFunction@4"	},
//	{	&guDrawPolygonVertexListWithClip,			"_guDrawPolygonVertexListWithClip@8"	},
//	{	&guDrawTriangleWithClip,			"_guDrawTriangleWithClip@12"	},
//	{	&guEncodeRLE16,			"_guEncodeRLE16@16"	},
//	{	&guEndianSwapBytes,			"_guEndianSwapBytes@4"	},
//	{	&guEndianSwapWords,			"_guEndianSwapWords@4"	},
//	{	&guFogGenerateExp2,			"_guFogGenerateExp2@8"	},
//	{	&guFogGenerateExp,			"_guFogGenerateExp@8"	},
//	{	&guFogGenerateLinear,			"_guFogGenerateLinear@12"	},
//	{	&guFogTableIndexToW,			"_guFogTableIndexToW@4"	},
//	{	&guMPDrawTriangle,			"_guMPDrawTriangle@12"	},
//	{	&guMPInit,			"_guMPInit@0"	},
//	{	&guMPTexCombineFunction,			"_guMPTexCombineFunction@4"	},
//	{	&guMPTexSource,			"_guMPTexSource@8"	},
//	{	&guMovieSetName,			"_guMovieSetName@4"	},
//	{	&guMovieStart,			"_guMovieStart@0"	},
//	{	&guMovieStop,			"_guMovieStop@0"	},
	{	&guTexAllocateMemory,			"_guTexAllocateMemory@60"	},
	{	&guTexChangeAttributes,			"_guTexChangeAttributes@48"	},
	{	&guTexCombineFunction,			"_guTexCombineFunction@8"	},
//	{	&guTexCreateColorMipMap,			"_guTexCreateColorMipMap@0"	},
	{	&guTexDownloadMipMap,			"_guTexDownloadMipMap@12"	},
	{	&guTexDownloadMipMapLevel,			"_guTexDownloadMipMapLevel@12"	},
	{	&guTexGetCurrentMipMap,			"_guTexGetCurrentMipMap@4"	},
	{	&guTexGetMipMapInfo,			"_guTexGetMipMapInfo@4"	},
	{	&guTexMemQueryAvail,			"_guTexMemQueryAvail@4"	},
	{	&guTexMemReset,			"_guTexMemReset@0"	},
	{	&guTexSource,			"_guTexSource@4"	},
	{	NULL,NULL }
};



BOOL InitGlideDLL(void)
{

	FUNC3DFX *pFunc;


   dllHandle=LoadLibrary(dllName);
   if (dllHandle==NULL)
   {
// Unable to find the glide DLL
		return FALSE;
   }


	pFunc=Functions3DFX;

	while(pFunc->DllFuncName!=NULL)
	{
		void *FuncPtr;
		void **DestFunc;
		
		FuncPtr= GetProcAddress(dllHandle,pFunc->DllFuncName);

		if (FuncPtr==NULL)
		{
			DBPRINTF(("%s not found in DLL\n",pFunc->DllFuncName));
			return FALSE;
		}


		DestFunc=pFunc->FuncPtr;

		*DestFunc=FuncPtr;

		pFunc++;	// Next entry
	}


   return TRUE;
}



BOOL ShutdownGlideDLL(void)
{
 	if (dllHandle!=NULL)
		FreeLibrary(dllHandle);

	dllHandle=NULL;
	return TRUE;
}



/*
void main(void)
{
	char version[80];

   GrHwConfiguration hwConfig;

	InitGlideDLL();	// initialse glide dll


		grGlideGetVersion( version );
		printf("Glide DLL found; version %s\n",version);


	grGlideInit();

//	grGlideInit();
//	grSstQueryBoards(&hwConfig);

//	printf("There are %d cards.\n",hwConfig.num_sst);  

	ShutdownGlideDLL();
}
*/