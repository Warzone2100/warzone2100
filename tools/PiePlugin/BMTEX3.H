#ifndef _INCLUDED_BMTEX3_
#define _INCLUDED_BMTEX3_

// All this just so we can get the clip window values from the material.....

//#include "mtlhdr.h"
//#include "mtlres.h"

#include "stdmat.h"
#include <bmmlib.h>
#include "texutil.h"
#include "iparamm2.h"
#include "macrorec.h"
#include "notify.h"

// JBW: IDs for ParamBlock2 blocks and parameters
// Parameter and ParamBlock IDs, bmtex_time
enum { bmtex_params,bmtex_time};  // pblock ID

enum 
{ 
	bmtex_clipu,bmtex_clipv,bmtex_clipw,bmtex_cliph,
	bmtex_jitter,bmtex_usejitter,
	bmtex_apply,bmtex_crop_place,
	bmtex_filtering,
	bmtex_monooutput,
	bmtex_rgboutput,
	bmtex_alphasource,
	bmtex_premultalpha,
	bmtex_bitmap,
	bmtex_coords,	 // access for UVW mapping
	bmtex_output,	 //output window
	bmtex_filename   // bitmap filename virtual parameter, JBW 2/23/99
};


enum 
{ 
	bmtex_start,bmtex_playbackrate, bmtex_endcondition
};

static Class_ID bmTexClassID(BMTEX_CLASS_ID,0);
class BMTex;

#define IDS_DS_BITMAP	0
//------------------------------------------------------------------------
// BMSampler
//------------------------------------------------------------------------

class BMSampler: public MapSampler {
	Bitmap *bm;
	BMTex *tex;
	int alphaSource;
	float u0,v0,u1,v1,ufac,vfac,ujit,vjit;
	int bmw,bmh,clipx, clipy, cliph;
	float fclipw,fcliph, fbmh, fbmw;
	public:
		BMSampler() { bm = NULL; ufac = vfac = 0.0f; }
		void Init(BMTex *bmt);
		int PlaceUV(ShadeContext& sc, float &u, float &v, int iu, int iv);
		void PlaceUVFilter(ShadeContext& sc, float &u, float &v, int iu, int iv);
		AColor Sample(ShadeContext& sc, float u,float v);
		AColor SampleFilter(ShadeContext& sc, float u,float v, float du, float dv);
//		float SampleMono(ShadeContext& sc, float u,float v);
//		float SampleMonoFilter(ShadeContext& sc, float u,float v, float du, float dv);
	} ;

//------------------------------------------------------------------------
// BM\AlphaSampler
//------------------------------------------------------------------------
class BMAlphaSampler: public MapSampler {
	Bitmap *bm;
	BMTex *tex;
	float u0,v0,u1,v1,ufac,vfac,ujit,vjit;
	int bmw,bmh,clipx, clipy, cliph;
	float fclipw,fcliph, fbmh, fbmw;
	public:
		BMAlphaSampler() { bm = NULL; ufac = vfac = fbmw = fbmh = 0.0f; }
		void Init(BMTex *bmt);
		int PlaceUV(ShadeContext &sc, float &u, float &v, int iu, int iv);
		void  PlaceUVFilter(ShadeContext &sc, float &u, float &v, int iu, int iv);
		AColor Sample(ShadeContext& sc, float u,float v) { return AColor(0,0,0,0);}
		AColor SampleFilter(ShadeContext& sc, float u,float v, float du, float dv) { return AColor(0,0,0,0);}
		float SampleMono(ShadeContext& sc, float u,float v);
		float SampleMonoFilter(ShadeContext& sc, float u,float v, float du, float dv);
	} ;


class BMTexNotify: public BitmapNotify {
	public:
	BMTex *tex;
	void SetTex(BMTex *tx) { tex  = tx; }
	int Changed(ULONG flags);
	};

class BMTex: public BitmapTex {
	friend class BMTexPostLoad;
	friend class BMTexDlg;
	friend class BMTexDlgProc;
	friend class BMSampler;
	friend class BMAlphaSampler;
	friend class BMCropper;

	static ParamDlg* uvGenDlg;	
	static ParamDlg* texoutDlg;

	UVGen *uvGen;		   // ref #0

	
	TextureOutput *texout; // ref #2

	BMTexNotify bmNotify;
	TexHandle *texHandle;
	Interval ivalid;
	BMSampler mysamp;
	BMAlphaSampler alphasamp;
	public:
		BOOL isParm2;

		float pbRate;
		TimeValue startTime;
		BOOL applyCrop;
		BOOL loading;
		BOOL loadingOld;
		BOOL placeImage;
		BOOL randPlace;
		BOOL fileNotFound;
		int filterType;
		int alphaSource;
		int rollScroll;
		int endCond;
		int alphaAsMono;
		int alphaAsRGB;
		float clipu, clipv, clipw, cliph, jitter;
		BOOL premultAlpha;
		BOOL isNew;
		BOOL loadFailed; 
		BOOL inRender;
		int texTime;
		Interval texValid;
		Interval clipValid;
		float rumax,rumin,rvmax,rvmin;


		IParamBlock2 *pblock;   // ref #1
		IParamBlock2 *pblockTime;   // ref #3
		static BMTexDlg *paramDlg;
		PBBitmap *bitmapPB;
		BitmapInfo bi;
		TSTR bmName; // for loading old files only
		Bitmap *thebm;

		BMTex();
		~BMTex();
		IMtlParams *ip;
		ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
		void Update(TimeValue t, Interval& valid);
		void Init();
		void Reset();
		Interval Validity(TimeValue t) { Interval v; Update(t,v); return ivalid; }
		TSTR GetFullName();

		void SetOutputLevel(TimeValue t, float v) {texout->SetOutputLevel(t,v); }
		void SetFilterType(int ft);
		void SetAlphaSource(int as);
		void SetEndCondition(int endcond) { 
            #ifndef DESIGN_VER
			pblock->SetValue( bmtex_endcondition, 0, endcond);		
			endCond = endcond; 
            #endif // DESIGN_VER
			}
		void SetAlphaAsMono(BOOL onoff) { 
			pblock->SetValue( bmtex_monooutput, 0, onoff);		
			alphaAsMono = onoff; 
			}
		void SetAlphaAsRGB(BOOL onoff) { 
			pblock->SetValue( bmtex_rgboutput, 0, onoff);		
			alphaAsRGB = onoff; 
			}
		void SetPremultAlpha(BOOL onoff) { 
			pblock->SetValue( bmtex_premultalpha, 0, onoff);		
			premultAlpha = onoff; 
			}
		void SetMapName(TCHAR *name) { 
			if (name) {
				bi.SetName(name); 
				PBBitmap bt(bi);
				pblock->SetValue(bmtex_bitmap, 0, &bt);
				FreeBitmap();
				fileNotFound = FALSE;
				if (paramDlg)
					ReloadBitmap();
				if (TestMtlFlag(MTL_TEX_DISPLAY_ENABLED)) {
					if (ip)
						ip->MtlChanged();
					}
				}
			}
		void SetStartTime(TimeValue t) {
            #ifndef DESIGN_VER
			pblock->SetValue( bmtex_start, 0, t);		
			startTime = t; 
            #endif // DESIGN_VER
			}
		void SetPlaybackRate(float r) { 
            #ifndef DESIGN_VER
			pblock->SetValue( bmtex_playbackrate, 0, r);		
			pbRate = r; 
            #endif // DESIGN_VER
			}

		void SetClipU(TimeValue t, float f);
		void SetClipV(TimeValue t, float f) ;
		void SetClipW(TimeValue t, float f) ;
		void SetClipH(TimeValue t, float f) ;
		void SetJitter(TimeValue t, float f);

		int GetFilterType() { return filterType; }
		int GetAlphaSource() { return alphaSource; }
		int GetEndCondition() { return endCond; }
		BOOL GetAlphaAsMono(BOOL onoff) { return alphaAsMono; }
		BOOL GetAlphaAsRGB(BOOL onoff) { return alphaAsRGB; }
		BOOL GetPremultAlpha(BOOL onoff) { return premultAlpha; }
		TCHAR *GetMapName() {
			UpdateBIName(); // CCJ 4/27/99
			return (TCHAR *)bi.Name();
			}
		TimeValue GetStartTime() { return startTime; }
		float GetPlaybackRate() { return pbRate; }
		StdUVGen* GetUVGen() { return (StdUVGen*)uvGen; }
		TextureOutput* GetTexout() { return texout; }
		Bitmap *GetBitmap(TimeValue t) { LoadBitmap(t); 	return thebm; }
	    float GetClipU(TimeValue t) { 	
	    	return pblock->GetFloat( bmtex_clipu, t); 	
	    	}
	    float GetClipV(TimeValue t) { 							
	    	return pblock->GetFloat( bmtex_clipv, t); 
			}
	    float GetClipW(TimeValue t) { 
			return pblock->GetFloat( bmtex_clipw, t); 
			}
	    float GetClipH(TimeValue t) { 
			return pblock->GetFloat( bmtex_cliph, t); 
			}
	    float GetJitter(TimeValue t) { 
			return pblock->GetFloat( bmtex_jitter, t); 
			}
		void StuffCropValues(); // stuff new values into the cropping VFB
		void EnableStuff();
		void UpdateBIName() {
			pblock->GetValue( bmtex_bitmap, 0,bitmapPB, clipValid );
			if (bitmapPB != NULL)   
				bi = bitmapPB->bi;	  //DS 2/24/99
			else bi.SetName(_T(""));   // DS 3/23/99
			}

		void UpdtSampler() {
			mysamp.Init(this);
			alphasamp.Init(this);
			}

		void NotifyChanged();
		void FreeBitmap();
 		BMMRES LoadBitmap(TimeValue t, BOOL quiet=TRUE, BOOL reload=FALSE);
		int CalcFrame(TimeValue t);
		void ScaleBitmapBumpAmt(float f);
		void ReloadBitmap();
/*
		void UpdateTexture()
			{
			ReloadBitmap();
			NotifyChanged();
			if (ip != NULL)	ip->MtlChanged();
//			UpdateMtlDisplay(); // redraw viewports
//			return 1;
			}

*/
		// Evaluate the color of map for the context.
		RGBA EvalColor(ShadeContext& sc);
		float EvalMono(ShadeContext& sc);
		Point3 EvalNormalPerturb(ShadeContext& sc);

		void DiscardTexHandle();

		BOOL SupportTexDisplay() { return TRUE; }
		void ActivateTexDisplay(BOOL onoff);
		DWORD GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker);
		void GetUVTransform(Matrix3 &uvtrans) { uvGen->GetUVTransform(uvtrans); }
		int GetTextureTiling() { return  uvGen->GetTextureTiling(); }
		int GetUVWSource() { return uvGen->GetUVWSource(); }
		int GetMapChannel () { return uvGen->GetMapChannel(); }
		UVGen *GetTheUVGen() { return uvGen; }
					
		int RenderBegin(TimeValue t, ULONG flags) {	
			inRender = TRUE;
			return 1; 	
			}
		int RenderEnd(TimeValue t) { 	
			inRender = FALSE;	
			return 1; 	
			}
		int LoadMapFiles(TimeValue t) {	LoadBitmap(t,1,1);	return 1;	}
		void RenderBitmap(TimeValue t, Bitmap *bm, float scale3D, BOOL filter);

		Class_ID ClassID() {	return bmTexClassID; }
		SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
		void GetClassName(TSTR& s) { s= GetString(IDS_DS_BITMAP); }  
		void DeleteThis() { delete this; }	

		// Requirements
		ULONG LocalRequirements(int subMtlNum) {
			return uvGen->Requirements(subMtlNum); 
			}

		void LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq) {  
			uvGen->MappingsRequired(subMtlNum,mapreq,bumpreq); 
			}

		int NumSubs() { return 4; }  
		Animatable* SubAnim(int i);
		TSTR SubAnimName(int i);
		int SubNumToRefNum(int subNum) { return subNum; }
		void InitSlotType(int sType) { if (uvGen) uvGen->InitSlotType(sType); }

		// From ref
		int NumRefs() { return 4; }
		RefTargetHandle GetReference(int i);
		void SetReference(int i, RefTargetHandle rtarg);
		int RemapRefOnLoad(int iref); 

		RefTargetHandle Clone(RemapDir &remap = NoRemap());
		RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message );

		// From Animatable
		void EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags) {
			UpdateBIName(); // DS 2/24/99
			bi.EnumAuxFiles(nameEnum,flags);
			}
		int SetProperty(ULONG id, void *data);
		void FreeAllBitmaps() { 
			FreeBitmap(); 
			}

		// IO
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);

// JBW: direct ParamBlock access is added
		int	NumParamBlocks() { return 2; }					// return number of ParamBlocks in this instance

		IParamBlock2* GetParamBlock(int i) { 
										if (i == 0) return pblock; 
											else if (i == 1) return pblockTime; 
											else return NULL;			 } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) 
													{if (pblock->ID() == id) return pblock ;
													 else if (pblockTime->ID() == id) return pblockTime ;
													else return  NULL; } // return id'd ParamBlock

//		{ return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock
		BOOL SetDlgThing(ParamDlg* dlg);

//watje pops up a bitmap loader dlg
		BitmapLoadDlg();
//watje forces the bitmap to reload and view to be redrawn
		ReloadBitmapAndUpdate();



	};

#endif
