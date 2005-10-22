/**********************************************************************
 *<
	FILE: BMTEX.CPP

	DESCRIPTION: BMTEX 2D Texture map.

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "mtlhdr.h"
#include "mtlres.h"
#include "stdmat.h"
#include <bmmlib.h>
#include "texutil.h"


// ParamBlock entries
#define PB_CLIPU	0
#define PB_CLIPV	1
#define PB_CLIPW	2
#define PB_CLIPH	3
#define PB_JITTER   4
#define NPARAMS 5

// Image filtering types
#define FILTER_PYR     0
#define FILTER_SAT     1
#define FILTER_NADA	   2

// Alpha source types
#define ALPHA_FILE 	0
#define ALPHA_RGB	2
#define ALPHA_NONE	3

// End conditions:
#define END_LOOP     0
#define END_PINGPONG 1
#define END_HOLD     2

#define NCOLS 0

class BMTex;
class BMTexDlg;

static Class_ID bmTexClassID(BMTEX_CLASS_ID,0);
static AColor black(0.0f,0.0f,0.0f,0.0f);

static void LoadFailedMsg(const TCHAR* name) {
	if (name==NULL) return;
	TCHAR msg[128];
	wsprintf(msg,GetString(IDS_DS_FILE_NOT_FOUND), name);
	MessageBox(NULL, msg, GetString(IDS_DS_BITMAP_TEXTURE_ERR), MB_TASKMODAL);
	}

static void InvalidNameMsg(const TCHAR* name) {
	if (name==NULL) return;
	TCHAR msg[128];
	wsprintf(msg,_T("Invalid File Name: %s"), name);
	MessageBox(NULL, msg, GetString(IDS_DS_BITMAP_TEXTURE_ERR), MB_TASKMODAL);
	}



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
		BMSampler() { bm = NULL; }
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
		BMAlphaSampler() { bm = NULL; }
		void Init(BMTex *bmt);
		int PlaceUV(ShadeContext &sc, float &u, float &v, int iu, int iv);
		void  PlaceUVFilter(ShadeContext &sc, float &u, float &v, int iu, int iv);
		AColor Sample(ShadeContext& sc, float u,float v) { return AColor(0,0,0,0);}
		AColor SampleFilter(ShadeContext& sc, float u,float v, float du, float dv) { return AColor(0,0,0,0);}
		float SampleMono(ShadeContext& sc, float u,float v);
		float SampleMonoFilter(ShadeContext& sc, float u,float v, float du, float dv);
	} ;



//------------------------------------------------------------------------
// BMCropper
//------------------------------------------------------------------------
class BMCropper:public CropCallback {
	BMTexDlg *dlg;
	BMTex *tex;
	BOOL mode;
	float u0,v0,w0,h0;
	public:
	float GetInitU() { return u0; }
	float GetInitV() { return v0; }
	float GetInitW() { return w0; }
	float GetInitH() { return h0; }
	BOOL GetInitMode() { return mode; }
	void SetValues(float u, float v, float w, float h, BOOL md);
	void OnClose();
	void Init(BMTexDlg *tx, TimeValue t);
	};

//------------------------------------------------------------------------
// BMTexDlg
//------------------------------------------------------------------------
class BMTexDlg: public ParamDlg , public DADMgr{
	friend class BMCropper;
	public:
		HWND hwmedit;	 // window handle of the materials editor dialog
		IMtlParams *ip;
		BMTex *theBMTex;	 // current BMTex being edited.
		HWND hPanel; // Rollup pane
		HWND hTime; // Time Rollup pane
		TimeValue curTime; 
		ParamDlg *uvGenDlg;
		ParamDlg *texoutDlg;
		ISpinnerControl *startSpin,*rateSpin;
		ISpinnerControl *clipUSpin,*clipVSpin,*clipWSpin,*clipHSpin,*jitterSpin;
		ICustButton *iName;
		BOOL valid;
		BOOL isActive;
		BOOL cropping;
		BMCropper cropper;

		//-----------------------------
		BMTexDlg(HWND hwMtlEdit, IMtlParams *imp, BMTex *m); 
		~BMTexDlg();
		BOOL PanelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
		BOOL TimeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
		void HandleNameButton();
		void LoadDialog(BOOL draw);  // stuff params into dialog
		void ReloadDialog();
		void BMNameChanged();
		void UpdateMtlDisplay() { ip->MtlChanged(); }
		void ActivateDlg(BOOL onOff) {}
		void StuffBMNameField(HWND hwndDlg);
		void EnableAlphaButtons(BOOL isNew=FALSE);
		void EnableViewImage();
		void Invalidate() { valid = FALSE;	InvalidateRect(hPanel,NULL,0); }
		BOOL KeyAtCurTime(int id);
		void ShowCropImage();
		void RemoveCropImage();

		// methods inherited from ParamDlg:
		Class_ID ClassID() {return bmTexClassID;  }
		void SetThing(ReferenceTarget *m);
		ReferenceTarget* GetThing() { return (ReferenceTarget *)theBMTex; }
		void DeleteThis() { delete this;  }	
		void SetTime(TimeValue t);

		// DADMgr methods
		// called on the draggee to see what if anything can be dragged from this x,y
		SClass_ID GetDragType(HWND hwnd, POINT p) { return BITMAPDAD_CLASS_ID; }
		// called on potential dropee to see if can drop type at this x,y
		BOOL OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew) {
			if (hfrom==hto) return FALSE;
			return (type==BITMAPDAD_CLASS_ID)?1:0;
			}
		int SlotOwner() { return OWNER_MTL_TEX;	}
	    ReferenceTarget *GetInstance(HWND hwnd, POINT p, SClass_ID type);
		void Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type);
		BOOL  LetMeHandleLocalDAD() { return 0; } 
	};

class BMTexNotify: public BitmapNotify {
	public:
	BMTex *tex;
	void SetTex(BMTex *tx) { tex  = tx; }
	int Changed(ULONG flags);
	};

class BMTexPostLoad;

//--------------------------------------------------------------
// BMTex: A 2D texture map
//--------------------------------------------------------------

class BMTex: public BitmapTex {
	friend class BMTexPostLoad;
	friend class BMTexDlg;
	friend class BMSampler;
	friend class BMAlphaSampler;
	friend class BMCropper;
	UVGen *uvGen;		   // ref #0
	IParamBlock *pblock;   // ref #1
	TextureOutput *texout; // ref #2
	BitmapInfo bi;
	TSTR bmName; // for loading old files only
	Bitmap *thebm;
	BMTexNotify bmNotify;
	TexHandle *texHandle;
	float pbRate;
	TimeValue startTime;
	Interval ivalid;
	BMSampler mysamp;
	BMAlphaSampler alphasamp;
	BOOL applyCrop;
	BOOL loadingOld;
	BOOL placeImage;
	BOOL randPlace;
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
	BMTexDlg *paramDlg;
	int texTime;
	Interval texValid;
	Interval clipValid;
	float rumax,rumin,rvmax,rvmin;
	public:
		BMTex();
		~BMTex();
		ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
		void Update(TimeValue t, Interval& valid);
		void Reset();
		Interval Validity(TimeValue t) { Interval v; Update(t,v); return ivalid; }
		TSTR GetFullName();

		void SetOutputLevel(TimeValue t, float v) {texout->SetOutputLevel(t,v); }
		void SetFilterType(int ft);
		void SetAlphaSource(int as);
		void SetEndCondition(int endcond) { endCond = endcond; }
		void SetAlphaAsMono(BOOL onoff) { alphaAsMono = onoff; }
		void SetAlphaAsRGB(BOOL onoff) { alphaAsRGB = onoff; }
		void SetPremultAlpha(BOOL onoff) { premultAlpha = onoff; }
		void SetMapName(TCHAR *name) { bi.SetName(name); }
		void SetStartTime(TimeValue t) { startTime = t; }
		void SetPlaybackRate(float r) { pbRate = r; }

		void SetClipU(TimeValue t, float f) { clipu  = f; pblock->SetValue( PB_CLIPU, t, f);		}
		void SetClipV(TimeValue t, float f) { clipv  = f; pblock->SetValue( PB_CLIPV, t, f);		}
		void SetClipW(TimeValue t, float f) { clipw  = f; pblock->SetValue( PB_CLIPW, t, f);		}
		void SetClipH(TimeValue t, float f) { cliph  = f; pblock->SetValue( PB_CLIPH, t, f);		}
		void SetJitter(TimeValue t, float f) { cliph  = f; pblock->SetValue( PB_JITTER, t, f);		}

		int GetFilterType() { return filterType; }
		int GetAlphaSource() { return alphaSource; }
		int GetEndCondition() { return endCond; }
		BOOL GetAlphaAsMono(BOOL onoff) { return alphaAsMono; }
		BOOL GetAlphaAsRGB(BOOL onoff) { return alphaAsRGB; }
		BOOL GetPremultAlpha(BOOL onoff) { return premultAlpha; }
		TCHAR *GetMapName() { return (TCHAR *)bi.Name(); }
		TimeValue GetStartTime() { return startTime; }
		float GetPlaybackRate() { return pbRate; }
		StdUVGen* GetUVGen() { return (StdUVGen*)uvGen; }
		TextureOutput* GetTexout() { return texout; }
		Bitmap *GetBitmap(TimeValue t) { LoadBitmap(t); 	return thebm; }
	    float GetClipU(TimeValue t) { return pblock->GetFloat( PB_CLIPU, t); }
	    float GetClipV(TimeValue t) { return pblock->GetFloat( PB_CLIPV, t); }
	    float GetClipW(TimeValue t) { return pblock->GetFloat( PB_CLIPW, t); }
	    float GetClipH(TimeValue t) { return pblock->GetFloat( PB_CLIPH, t); }
	    float GetJitter(TimeValue t) { return pblock->GetFloat( PB_JITTER, t); }
		void StuffCropValues(); // stuff new values into the cropping VFB
		
		void UpdtSampler() {
			mysamp.Init(this);
			alphasamp.Init(this);
			}

		void NotifyChanged();
		void FreeBitmap();
 		BMMRES LoadBitmap(TimeValue t);
		int CalcFrame(TimeValue t);
		void ScaleBitmapBumpAmt(float f);
		void ReloadBitmap();

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
		UVGen *GetTheUVGen() { return uvGen; }
					
		int RenderBegin(TimeValue t, ULONG flags) {	inRender = TRUE;return 1; 	}
		int RenderEnd(TimeValue t) { 	inRender = FALSE;	return 1; 	}

		int LoadMapFiles(TimeValue t) {	LoadBitmap(t);	return 1;	}
		void RenderBitmap(TimeValue t, Bitmap *bm, float scale3D, BOOL filter);

		Class_ID ClassID() {	return bmTexClassID; }
		SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
		void GetClassName(TSTR& s) { s= GetString(IDS_DS_BITMAP); }  
		void DeleteThis() { delete this; }	

		// Requirements
		ULONG LocalRequirements(int subMtlNum) {
			return uvGen->Requirements(subMtlNum); 
			}

		int NumSubs() { return 3; }  
		Animatable* SubAnim(int i);
		TSTR SubAnimName(int i);
		int SubNumToRefNum(int subNum) { return subNum; }
		void InitSlotType(int sType) { if (uvGen) uvGen->InitSlotType(sType); }

		// From ref
		int NumRefs() { return 3; }
		RefTargetHandle GetReference(int i);
		void SetReference(int i, RefTargetHandle rtarg);
		int RemapRefOnLoad(int iref); 

		RefTargetHandle Clone(RemapDir &remap = NoRemap());
		RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message );

		// From Animatable
		void EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags) {
			bi.EnumAuxFiles(nameEnum,flags);
			}
		int SetProperty(ULONG id, void *data);
		void FreeAllBitmaps() { 
			FreeBitmap(); 
			}

		// IO
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);

	};


