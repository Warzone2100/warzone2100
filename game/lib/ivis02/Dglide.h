// New glide headers

#define FX_GLIDE_NO_FUNC_PROTO
#include "glide.h"	

/*
** rendering functions
*/
typedef void (FX_CALL *grDrawLine_fpt)( const GrVertex *v1, const GrVertex *v2 );

typedef void (FX_CALL *grDrawPlanarPolygon_fpt)( int nverts, const int ilist[], const GrVertex vlist[] );

typedef void (FX_CALL *grDrawPlanarPolygonVertexList_fpt)( int nverts, const GrVertex vlist[] );

typedef void (FX_CALL *grDrawPoint_fpt)( const GrVertex *pt );

typedef void (FX_CALL *grDrawPolygon_fpt)( int nverts, const int ilist[], const GrVertex vlist[] );

typedef void (FX_CALL *grDrawPolygonVertexList_fpt)( int nverts, const GrVertex vlist[] );

typedef void (FX_CALL *grDrawTriangle_fpt)( const GrVertex *a, const GrVertex *b, const GrVertex *c );

/*
** buffer management
*/
typedef void (FX_CALL *grBufferClear_fpt)( GrColor_t color, GrAlpha_t alpha, FxU16 depth );

typedef int (FX_CALL *grBufferNumPending_fpt)( void );

typedef void (FX_CALL *grBufferSwap_fpt)( int swap_interval );

typedef void (FX_CALL *grRenderBuffer_fpt)( GrBuffer_t buffer );

/*
** error management
*/
// Special function - unchanged from glide.h
typedef void (*GrErrorCallbackFnc_t)( const char *string, FxBool fatal );



typedef void (FX_CALL *grErrorSetCallback_fpt)( GrErrorCallbackFnc_t fnc );

/*
** SST routines
*/
typedef void (FX_CALL *grSstIdle_fpt)(void);

typedef FxU32 (FX_CALL *grSstVideoLine_fpt)( void );

typedef FxBool (FX_CALL *grSstVRetraceOn_fpt)( void );

typedef FxBool (FX_CALL *grSstIsBusy_fpt)( void );

typedef FxBool (FX_CALL *grSstWinOpen_fpt)(
          FxU32                hWnd,
          GrScreenResolution_t screen_resolution,
          GrScreenRefresh_t    refresh_rate,
          GrColorFormat_t      color_format,
          GrOriginLocation_t   origin_location,
          int                  nColBuffers,
          int                  nAuxBuffers);

typedef void (FX_CALL *grSstWinClose_fpt)( void );

typedef FxBool (FX_CALL *grSstControl_fpt)( FxU32 code );

typedef FxBool (FX_CALL *grSstQueryHardware_fpt)( GrHwConfiguration *hwconfig );

typedef FxBool (FX_CALL *grSstQueryBoards_fpt)( GrHwConfiguration *hwconfig );

typedef void (FX_CALL *grSstOrigin_fpt)(GrOriginLocation_t  origin);

typedef void (FX_CALL *grSstSelect_fpt)( int which_sst );

typedef FxU32 (FX_CALL *grSstScreenHeight_fpt)( void );

typedef FxU32 (FX_CALL *grSstScreenWidth_fpt)( void );

typedef FxU32 (FX_CALL *grSstStatus_fpt)( void );

/*
**  Drawing Statistics
*/
typedef void (FX_CALL *grSstPerfStats_fpt)(GrSstPerfStats_t *pStats);

typedef void (FX_CALL *grSstResetPerfStats_fpt)(void);

typedef void (FX_CALL *grResetTriStats_fpt)();

typedef void (FX_CALL *grTriStats_fpt)(FxU32 *trisProcessed, FxU32 *trisDrawn);

/*
** Glide configuration and special effect maintenance functions
*/
typedef void (FX_CALL *grAlphaBlendFunction_fpt)(
                     GrAlphaBlendFnc_t rgb_sf,   GrAlphaBlendFnc_t rgb_df,
                     GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df
                     );

typedef void (FX_CALL *grAlphaCombine_fpt)(
               GrCombineFunction_t function, GrCombineFactor_t factor,
               GrCombineLocal_t local, GrCombineOther_t other,
               FxBool invert
               );

typedef void (FX_CALL *grAlphaControlsITRGBLighting_fpt)( FxBool enable );

typedef void (FX_CALL *grAlphaTestFunction_fpt)( GrCmpFnc_t function );

typedef void (FX_CALL *grAlphaTestReferenceValue_fpt)( GrAlpha_t value );

typedef void (FX_CALL *grChromakeyMode_fpt)( GrChromakeyMode_t mode );

typedef void (FX_CALL *grChromakeyValue_fpt)( GrColor_t value );

typedef void (FX_CALL *grClipWindow_fpt)( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy );

typedef void (FX_CALL *grColorCombine_fpt)(
               GrCombineFunction_t function, GrCombineFactor_t factor,
               GrCombineLocal_t local, GrCombineOther_t other,
               FxBool invert );

typedef void (FX_CALL *grColorMask_fpt)( FxBool rgb, FxBool a );

typedef void (FX_CALL *grCullMode_fpt)( GrCullMode_t mode );

typedef void (FX_CALL *grConstantColorValue_fpt)( GrColor_t value );

typedef void (FX_CALL * grConstantColorValue4_fpt)( float a, float r, float g, float b );

typedef void (FX_CALL * grDepthBiasLevel_fpt)( FxI16 level );

typedef void (FX_CALL * grDepthBufferFunction_fpt)( GrCmpFnc_t function );

typedef void (FX_CALL * grDepthBufferMode_fpt)( GrDepthBufferMode_t mode );

typedef void (FX_CALL * grDepthMask_fpt)( FxBool mask );

typedef void (FX_CALL * grDisableAllEffects_fpt)( void );

typedef void (FX_CALL * grDitherMode_fpt)( GrDitherMode_t mode );

typedef void (FX_CALL * grFogColorValue_fpt)( GrColor_t fogcolor );

typedef void (FX_CALL * grFogMode_fpt)( GrFogMode_t mode );

typedef void (FX_CALL * grFogTable_fpt)( const GrFog_t ft[GR_FOG_TABLE_SIZE] );

typedef void (FX_CALL * grGammaCorrectionValue_fpt)( float value );

typedef void (FX_CALL *grSplash_fpt)(float x, float y, float width, float height, FxU32 frame);

/*
** texture mapping control functions
*/
typedef FxU32 (FX_CALL * grTexCalcMemRequired_fpt)(
                     GrLOD_t lodmin, GrLOD_t lodmax,
                     GrAspectRatio_t aspect, GrTextureFormat_t fmt);

typedef FxU32 (FX_CALL * grTexTextureMemRequired_fpt)( FxU32     evenOdd,
                                 GrTexInfo *info   );

typedef FxU32 (FX_CALL * grTexMinAddress_fpt)( GrChipID_t tmu );


typedef FxU32 (FX_CALL * grTexMaxAddress_fpt)( GrChipID_t tmu );


typedef void (FX_CALL * grTexNCCTable_fpt)( GrChipID_t tmu, GrNCCTable_t table );

typedef void (FX_CALL * grTexSource_fpt)( GrChipID_t tmu,
             FxU32      startAddress,
             FxU32      evenOdd,
             GrTexInfo  *info );

typedef void (FX_CALL * grTexClampMode_fpt)(
               GrChipID_t tmu,
               GrTextureClampMode_t s_clampmode,
               GrTextureClampMode_t t_clampmode
               );

typedef void (FX_CALL * grTexCombine_fpt)(
             GrChipID_t tmu,
             GrCombineFunction_t rgb_function,
             GrCombineFactor_t rgb_factor, 
             GrCombineFunction_t alpha_function,
             GrCombineFactor_t alpha_factor,
             FxBool rgb_invert,
             FxBool alpha_invert
             );

typedef void (FX_CALL * grTexCombineFunction_fpt)(
                     GrChipID_t tmu,
                     GrTextureCombineFnc_t fnc
                     );

typedef void (FX_CALL * grTexDetailControl_fpt)(
                   GrChipID_t tmu,
                   int lod_bias,
                   FxU8 detail_scale,
                   float detail_max
                   );

typedef void (FX_CALL * grTexFilterMode_fpt)(
                GrChipID_t tmu,
                GrTextureFilterMode_t minfilter_mode,
                GrTextureFilterMode_t magfilter_mode
                );


typedef void (FX_CALL * grTexLodBiasValue_fpt)(GrChipID_t tmu, float bias );

typedef void (FX_CALL * grTexDownloadMipMap_fpt)( GrChipID_t tmu,
                     FxU32      startAddress,
                     FxU32      evenOdd,
                     GrTexInfo  *info );

typedef void (FX_CALL * grTexDownloadMipMapLevel_fpt)( GrChipID_t        tmu,
                          FxU32             startAddress,
                          GrLOD_t           thisLod,
                          GrLOD_t           largeLod,
                          GrAspectRatio_t   aspectRatio,
                          GrTextureFormat_t format,
                          FxU32             evenOdd,
                          void              *data );

typedef void (FX_CALL * grTexDownloadMipMapLevelPartial_fpt)( GrChipID_t        tmu,
                                 FxU32             startAddress,
                                 GrLOD_t           thisLod,
                                 GrLOD_t           largeLod,
                                 GrAspectRatio_t   aspectRatio,
                                 GrTextureFormat_t format,
                                 FxU32             evenOdd,
                                 void              *data,
                                 int               start,
                                 int               end );


typedef void (FX_CALL * ConvertAndDownloadRle_fpt)( GrChipID_t        tmu,
                        FxU32             startAddress,
                        GrLOD_t           thisLod,
                        GrLOD_t           largeLod,
                        GrAspectRatio_t   aspectRatio,
                        GrTextureFormat_t format,
                        FxU32             evenOdd,
                        FxU8              *bm_data,
                        long              bm_h,
                        FxU32             u0,
                        FxU32             v0,
                        FxU32             width,
                        FxU32             height,
                        FxU32             dest_width,
                        FxU32             dest_height,
                        FxU16             *tlut);

typedef void (FX_CALL * grCheckForRoom_fpt)(FxI32 n);

typedef void (FX_CALL *grTexDownloadTable_fpt)( GrChipID_t   tmu,
                    GrTexTable_t type, 
                    void         *data );

typedef void (FX_CALL *grTexDownloadTablePartial_fpt)( GrChipID_t   tmu,
                           GrTexTable_t type, 
                           void         *data,
                           int          start,
                           int          end );

typedef void (FX_CALL * grTexMipMapMode_fpt)( GrChipID_t     tmu, 
                 GrMipMapMode_t mode,
                 FxBool         lodBlend );

typedef void (FX_CALL * grTexMultibase_fpt)( GrChipID_t tmu,
                FxBool     enable );

typedef void (FX_CALL *grTexMultibaseAddress_fpt)( GrChipID_t       tmu,
                       GrTexBaseRange_t range,
                       FxU32            startAddress,
                       FxU32            evenOdd,
                       GrTexInfo        *info );

/*
** utility texture functions
*/
typedef GrMipMapId_t (FX_CALL * guTexAllocateMemory_fpt)(
                    GrChipID_t tmu,
                    FxU8 odd_even_mask,
                    int width, int height,
                    GrTextureFormat_t fmt,
                    GrMipMapMode_t mm_mode,
                    GrLOD_t smallest_lod, GrLOD_t largest_lod,
                    GrAspectRatio_t aspect,
                    GrTextureClampMode_t s_clamp_mode,
                    GrTextureClampMode_t t_clamp_mode,
                    GrTextureFilterMode_t minfilter_mode,
                    GrTextureFilterMode_t magfilter_mode,
                    float lod_bias,
                    FxBool trilinear
                    );

typedef FxBool (FX_CALL * guTexChangeAttributes_fpt)(
                      GrMipMapId_t mmid,
                      int width, int height,
                      GrTextureFormat_t fmt,
                      GrMipMapMode_t mm_mode,
                      GrLOD_t smallest_lod, GrLOD_t largest_lod,
                      GrAspectRatio_t aspect,
                      GrTextureClampMode_t s_clamp_mode,
                      GrTextureClampMode_t t_clamp_mode,
                      GrTextureFilterMode_t minFilterMode,
                      GrTextureFilterMode_t magFilterMode
                      );

typedef void (FX_CALL * guTexCombineFunction_fpt)(
                     GrChipID_t tmu,
                     GrTextureCombineFnc_t fnc
                     );

typedef GrMipMapId_t (FX_CALL * guTexGetCurrentMipMap_fpt)( GrChipID_t tmu );

typedef GrMipMapInfo * (FX_CALL * guTexGetMipMapInfo_fpt)( GrMipMapId_t mmid );

typedef FxU32 (FX_CALL * guTexMemQueryAvail_fpt)( GrChipID_t tmu );

typedef void (FX_CALL * guTexMemReset_fpt)( void );

typedef void (FX_CALL * guTexDownloadMipMap_fpt)(
                    GrMipMapId_t mmid,
                    const void *src,
                    const GuNccTable *table
                    );

typedef void (FX_CALL * guTexDownloadMipMapLevel_fpt)(
                         GrMipMapId_t mmid,
                         GrLOD_t lod,
                         const void **src
                         );
typedef void (FX_CALL * guTexSource_fpt)( GrMipMapId_t id );

/*
** linear frame buffer functions
*/

typedef FxBool (FX_CALL *grLfbLock_fpt)( GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode,
           GrOriginLocation_t origin, FxBool pixelPipeline, 
           GrLfbInfo_t *info );

typedef FxBool (FX_CALL *grLfbUnlock_fpt)( GrLock_t type, GrBuffer_t buffer );

typedef void (FX_CALL * grLfbConstantAlpha_fpt)( GrAlpha_t alpha );

typedef void (FX_CALL * grLfbConstantDepth_fpt)( FxU16 depth );

typedef void (FX_CALL * grLfbWriteColorSwizzle_fpt)(FxBool swizzleBytes, FxBool swapWords);

typedef void (FX_CALL *grLfbWriteColorFormat_fpt)(GrColorFormat_t colorFormat);


typedef FxBool (FX_CALL *grLfbWriteRegion_fpt)( GrBuffer_t dst_buffer, 
                  FxU32 dst_x, FxU32 dst_y, 
                  GrLfbSrcFmt_t src_format, 
                  FxU32 src_width, FxU32 src_height, 
                  FxI32 src_stride, void *src_data );

typedef FxBool (FX_CALL *grLfbReadRegion_fpt)( GrBuffer_t src_buffer,
                 FxU32 src_x, FxU32 src_y,
                 FxU32 src_width, FxU32 src_height,
                 FxU32 dst_stride, void *dst_data );


/*
**  Antialiasing Functions
*/
typedef void (FX_CALL *grAADrawLine_fpt)(const GrVertex *v1, const GrVertex *v2);

typedef void (FX_CALL *grAADrawPoint_fpt)(const GrVertex *pt );

typedef void (FX_CALL *grAADrawPolygon_fpt)(const int nverts, const int ilist[], const GrVertex vlist[]);

typedef void (FX_CALL *grAADrawPolygonVertexList_fpt)(const int nverts, const GrVertex vlist[]);

typedef void (FX_CALL *grAADrawTriangle_fpt)(
                 const GrVertex *a, const GrVertex *b, const GrVertex *c,
                 FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias
                 );

/*
** glide management functions
*/
typedef void (FX_CALL *grGlideInit_fpt)( void );

typedef void (FX_CALL *grGlideShutdown_fpt)( void );

typedef void (FX_CALL *grGlideGetVersion_fpt)( char version[80] );

typedef void (FX_CALL *grGlideGetState_fpt)( GrState *state );

typedef void (FX_CALL *grGlideSetState_fpt)( const GrState *state );

typedef void (FX_CALL *grGlideShamelessPlug_fpt)(const FxBool on);

typedef void (FX_CALL *grHints_fpt)(GrHint_t hintType, FxU32 hintMask);







extern ConvertAndDownloadRle_fpt	ConvertAndDownloadRle;
extern grAADrawLine_fpt	grAADrawLine;
extern grAADrawPoint_fpt	grAADrawPoint;
extern grAADrawPolygon_fpt	grAADrawPolygon;
extern grAADrawPolygonVertexList_fpt	grAADrawPolygonVertexList;
extern grAADrawTriangle_fpt	grAADrawTriangle;
extern grAlphaBlendFunction_fpt	grAlphaBlendFunction;
extern grAlphaCombine_fpt	grAlphaCombine;
extern grAlphaControlsITRGBLighting_fpt	grAlphaControlsITRGBLighting;
extern grAlphaTestFunction_fpt	grAlphaTestFunction;
extern grAlphaTestReferenceValue_fpt	grAlphaTestReferenceValue;
extern grBufferClear_fpt	grBufferClear;
extern grBufferNumPending_fpt	grBufferNumPending;
extern grBufferSwap_fpt	grBufferSwap;
extern grCheckForRoom_fpt	grCheckForRoom;
extern grChromakeyMode_fpt	grChromakeyMode;
extern grChromakeyValue_fpt	grChromakeyValue;
extern grClipWindow_fpt	grClipWindow;
extern grColorCombine_fpt	grColorCombine;
extern grColorMask_fpt	grColorMask;
extern grConstantColorValue4_fpt	grConstantColorValue4;
extern grConstantColorValue_fpt	grConstantColorValue;
extern grCullMode_fpt	grCullMode;
extern grDepthBiasLevel_fpt	grDepthBiasLevel;
extern grDepthBufferFunction_fpt	grDepthBufferFunction;
extern grDepthBufferMode_fpt	grDepthBufferMode;
extern grDepthMask_fpt	grDepthMask;
extern grDisableAllEffects_fpt	grDisableAllEffects;
extern grDitherMode_fpt	grDitherMode;
extern grDrawLine_fpt	grDrawLine;
extern grDrawPlanarPolygon_fpt	grDrawPlanarPolygon;
extern grDrawPlanarPolygonVertexList_fpt	grDrawPlanarPolygonVertexList;
extern grDrawPoint_fpt	grDrawPoint;
extern grDrawPolygon_fpt	grDrawPolygon;
extern grDrawPolygonVertexList_fpt	grDrawPolygonVertexList;
extern grDrawTriangle_fpt	grDrawTriangle;
extern grErrorSetCallback_fpt	grErrorSetCallback;
extern grFogColorValue_fpt	grFogColorValue;
extern grFogMode_fpt	grFogMode;
extern grFogTable_fpt	grFogTable;
extern grGammaCorrectionValue_fpt	grGammaCorrectionValue;
extern grGlideGetState_fpt	grGlideGetState;
extern grGlideGetVersion_fpt	grGlideGetVersion;
extern grGlideInit_fpt	grGlideInit;
extern grGlideSetState_fpt	grGlideSetState;
extern grGlideShamelessPlug_fpt	grGlideShamelessPlug;
extern grGlideShutdown_fpt	grGlideShutdown;
extern grHints_fpt	grHints;
extern grLfbConstantAlpha_fpt	grLfbConstantAlpha;
extern grLfbConstantDepth_fpt	grLfbConstantDepth;
extern grLfbLock_fpt	grLfbLock;
extern grLfbReadRegion_fpt	grLfbReadRegion;
extern grLfbUnlock_fpt	grLfbUnlock;
extern grLfbWriteColorFormat_fpt	grLfbWriteColorFormat;
extern grLfbWriteColorSwizzle_fpt	grLfbWriteColorSwizzle;
extern grLfbWriteRegion_fpt	grLfbWriteRegion;
extern grRenderBuffer_fpt	grRenderBuffer;
extern grResetTriStats_fpt	grResetTriStats;
extern grSplash_fpt	grSplash;
//extern grSstConfigPipeline_fpt	grSstConfigPipeline;
extern grSstControl_fpt	grSstControl;
extern grSstIdle_fpt	grSstIdle;
extern grSstIsBusy_fpt	grSstIsBusy;
extern grSstOrigin_fpt	grSstOrigin;
extern grSstPerfStats_fpt	grSstPerfStats;
extern grSstQueryBoards_fpt	grSstQueryBoards;
extern grSstQueryHardware_fpt	grSstQueryHardware;
extern grSstResetPerfStats_fpt	grSstResetPerfStats;
extern grSstScreenHeight_fpt	grSstScreenHeight;
extern grSstScreenWidth_fpt	grSstScreenWidth;
extern grSstSelect_fpt	grSstSelect;
extern grSstStatus_fpt	grSstStatus;
extern grSstVRetraceOn_fpt	grSstVRetraceOn;
//extern grSstVidMode_fpt	grSstVidMode;
extern grSstVideoLine_fpt	grSstVideoLine;
extern grSstWinClose_fpt	grSstWinClose;
extern grSstWinOpen_fpt	grSstWinOpen;
extern grTexCalcMemRequired_fpt	grTexCalcMemRequired;
extern grTexClampMode_fpt	grTexClampMode;
extern grTexCombine_fpt	grTexCombine;
extern grTexCombineFunction_fpt	grTexCombineFunction;
extern grTexDetailControl_fpt	grTexDetailControl;
extern grTexDownloadMipMap_fpt	grTexDownloadMipMap;
extern grTexDownloadMipMapLevel_fpt	grTexDownloadMipMapLevel;
extern grTexDownloadMipMapLevelPartial_fpt	grTexDownloadMipMapLevelPartial;
extern grTexDownloadTable_fpt	grTexDownloadTable;
extern grTexDownloadTablePartial_fpt	grTexDownloadTablePartial;
extern grTexFilterMode_fpt	grTexFilterMode;
extern grTexLodBiasValue_fpt	grTexLodBiasValue;
extern grTexMaxAddress_fpt	grTexMaxAddress;
extern grTexMinAddress_fpt	grTexMinAddress;
extern grTexMipMapMode_fpt	grTexMipMapMode;
extern grTexMultibase_fpt	grTexMultibase;
extern grTexMultibaseAddress_fpt	grTexMultibaseAddress;
extern grTexNCCTable_fpt	grTexNCCTable;
extern grTexSource_fpt	grTexSource;
extern grTexTextureMemRequired_fpt	grTexTextureMemRequired;
extern grTriStats_fpt	grTriStats;
//extern gu3dfGetInfo_fpt	gu3dfGetInfo;
//extern gu3dfLoad_fpt	gu3dfLoad;
//extern guAADrawTriangleWithClip_fpt	guAADrawTriangleWithClip;
//extern guAlphaSource_fpt	guAlphaSource;
//extern guColorCombineFunction_fpt	guColorCombineFunction;
//extern guDrawPolygonVertexListWithClip_fpt	guDrawPolygonVertexListWithClip;
//extern guDrawTriangleWithClip_fpt	guDrawTriangleWithClip;
//extern guEncodeRLE16_fpt	guEncodeRLE16;
//extern guEndianSwapBytes_fpt	guEndianSwapBytes;
//extern guEndianSwapWords_fpt	guEndianSwapWords;
//extern guFogGenerateExp2_fpt	guFogGenerateExp2;
//extern guFogGenerateExp_fpt	guFogGenerateExp;
//extern guFogGenerateLinear_fpt	guFogGenerateLinear;
//extern guFogTableIndexToW_fpt	guFogTableIndexToW;
//extern guMPDrawTriangle_fpt	guMPDrawTriangle;
//extern guMPInit_fpt	guMPInit;
//extern guMPTexCombineFunction_fpt	guMPTexCombineFunction;
//extern guMPTexSource_fpt	guMPTexSource;
//extern guMovieSetName_fpt	guMovieSetName;
//extern guMovieStart_fpt	guMovieStart;
//extern guMovieStop_fpt	guMovieStop;
extern guTexAllocateMemory_fpt	guTexAllocateMemory;
extern guTexChangeAttributes_fpt	guTexChangeAttributes;
extern guTexCombineFunction_fpt	guTexCombineFunction;
//extern guTexCreateColorMipMap_fpt	guTexCreateColorMipMap;
extern guTexDownloadMipMap_fpt	guTexDownloadMipMap;
extern guTexDownloadMipMapLevel_fpt	guTexDownloadMipMapLevel;
extern guTexGetCurrentMipMap_fpt	guTexGetCurrentMipMap;
extern guTexGetMipMapInfo_fpt	guTexGetMipMapInfo;
extern guTexMemQueryAvail_fpt	guTexMemQueryAvail;
extern guTexMemReset_fpt	guTexMemReset;
extern guTexSource_fpt	guTexSource;



