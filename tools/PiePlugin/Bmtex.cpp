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


extern HINSTANCE hInstance;

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
		void SetMapName(TCHAR *name) { 
			bi.SetName(name); 
			FreeBitmap();
			if (paramDlg)
				ReloadBitmap();
			}
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
					
		int RenderBegin(TimeValue t, ULONG flags) {	
			inRender = TRUE;
			return 1; 	
			}
		int RenderEnd(TimeValue t) { 	
			inRender = FALSE;	
			return 1; 	
			}
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


//------------------------------------------------------------------------
// BMSampler -- Methods
//------------------------------------------------------------------------
void BMSampler::Init(BMTex *bmt) {
	 bm = bmt->thebm; 
	 tex = bmt; 
	 alphaSource = bmt->alphaSource; 
	 if (bm) {
		u0 = tex->clipu;
		v0 = tex->clipv;
		u1 = tex->clipu + ufac;
		v1 = tex->clipv + vfac;
		ufac= tex->clipw;
		vfac = tex->cliph;
		bmw = bm->Width();
		bmh = bm->Height();
		fbmw = float(bmw-1);
		fbmh = float(bmh-1);
		clipx = int(tex->clipu*fbmw);
		clipy = int(tex->clipv*fbmh);
		fclipw = tex->clipw*fbmw;
		fcliph = tex->cliph*fbmh;
		cliph = int(fcliph);
		ujit = tex->jitter*(1.0f-ufac);
		vjit = tex->jitter*(1.0f-vfac);
		}
	 }

static inline void Jitter(ShadeContext &sc, float ujit, float vjit, float &ru, float &rv, int iu, int iv) {
//	srand(((sc.uTile+593)*14693+(sc.vTile+4991)*2517)&0x7fff);
	srand(((iu+593)*14693+(iv+967)*29517)&0x7fff);
	int urnd = rand()&0x7fff;
	rand();	 // these extra calls seem to make the pattern more random
	rand();	 // these extra calls seem to make the pattern more random
	int vrnd = rand()&0x7fff;
	ru = ujit*float(urnd)/float(32767.0f);
	rv = vjit*float(vrnd)/float(32767.0f);
	}

int BMSampler::PlaceUV(ShadeContext &sc, float &u, float &v, int iu, int iv) {
	if (tex->randPlace) {
		float ru,rv;
		Jitter(sc,ujit,vjit,ru,rv,iu,iv);
		if (u<ru||v<rv||u>ru+ufac||v>rv+vfac) return 0;
		u = (u-ru)/ufac;
		v = (v-rv)/vfac;
		}
	else {
		if (u<u0||v<v0||u>u1||v>v1) return 0;
		u = (u-u0)/ufac;
		v = (v-v0)/vfac;
		}
	return 1;
	}

void BMSampler::PlaceUVFilter(ShadeContext &sc, float &u, float &v, int iu, int iv) {
	if (tex->randPlace) {
		float ru,rv;
		Jitter(sc,ujit,vjit,ru,rv,iu,iv);
		u = (u-ru)/ufac;
		v = (v-rv)/vfac;
		}
	else {
		u = (u-u0)/ufac;
		v = (v-v0)/vfac;
		}
	}

AColor BMSampler::Sample(ShadeContext& sc, float u,float v) {
	BMM_Color_64 c;
	int x,y;
	float fu,fv;
	fu = frac(u);
	fv = 1.0f-frac(v);
	if (tex->applyCrop) {
		if (tex->placeImage) {
			if (!PlaceUV(sc,fu, fv, int(u), int(v))) return black;
			x = (int)(fu*fbmw+0.5f);
			y = (int)(fv*fbmh+0.5f);
			}
		else {
			x = mod(clipx + (int)(fu*fclipw+0.5f),bmw);
			y = mod(clipy + (int)(fv*fcliph+0.5f),bmh);
			}
		}
	else {
		x = (int)(fu*fbmw+0.5f);
		y = (int)(fv*fbmh+0.5f);
		}
	bm->GetLinearPixels(x,y,1,&c);
	switch(alphaSource) {
		case ALPHA_NONE:  c.a = 0xffff; break;
		case ALPHA_RGB:   c.a = (c.r+c.g+c.b)/3; break;
		//  TBD
		// XPCOL needs to be handled in bitmap for filtering. 
		// Need to open a bitmap with this property.
		//	case ALPHA_XPCOL:  break; 
		}
	return c;
	}


AColor BMSampler::SampleFilter(ShadeContext& sc, float u,float v, float du, float dv) {
	BMM_Color_64 c;
	float fu,fv;
	fu = frac(u);
	fv = 1.0f-frac(v);
	if (tex->applyCrop) {
		if (tex->placeImage) {
			PlaceUVFilter(sc,fu, fv, int(u), int(v));
			du /= ufac;
			dv /= vfac;
			float du2 = 0.5f*du;
			float ua = fu-du2;
			float ub = fu+du2;
			if (ub<=0.0f||ua>=1.0f) return black;
			float dv2 = 0.5f*dv;
			float va = fv-dv2;
			float vb = fv+du2;
			if (vb<=0.0f||va>=1.0f) return black;
			BOOL clip = 0;
			if (ua<0.0f) { ua=0.0f; clip = 1; }
			if (ub>1.0f) { ub=1.0f;	clip = 1; }
			if (va<0.0f) { va=0.0f;	clip = 1; }
			if (vb>1.0f) { vb=1.0f;	clip = 1; }
			bm->GetFiltered(fu,fv,du,dv,&c);
			switch(alphaSource) {
				case ALPHA_NONE:  c.a = 0xffff; break;
				case ALPHA_RGB:   c.a = (c.r+c.g+c.b)/3; break;
				}
			AColor ac(c);
			if (clip) {
				float f = ((ub-ua)/du) * ((vb-va)/dv);
				ac *= f;
				}
			return ac;
			}
		else {
			fu = (u0 + ufac*fu);
			fv = (v0 + vfac*fv);
			du *= ufac;
			dv *= vfac;
			bm->GetFiltered(fu,fv,du,dv,&c);
			}
		}

	else 
		bm->GetFiltered(fu,fv,du,dv,&c);
	switch(alphaSource) {
		case ALPHA_NONE:  c.a = 0xffff; break;
		case ALPHA_RGB:   c.a = (c.r+c.g+c.b)/3; break;
		}
	return c;
	}


//------------------------------------------------------------------------
// BMAlphaSampler -- Methods
//------------------------------------------------------------------------
void BMAlphaSampler::Init(BMTex *bmt) { 
	bm = bmt->thebm; 
	tex = bmt; 
	if (bm) {
		u0 = tex->clipu;
		v0 = tex->clipv;
		ufac= tex->clipw;
		vfac = tex->cliph;
		bmw = bm->Width();
		bmh = bm->Height();
		fbmw = float(bmw-1);
		fbmh = float(bmh-1);
		clipx = int(tex->clipu*fbmw);
		clipy = int(tex->clipv*fbmh);
		fclipw = tex->clipw*fbmw;
		fcliph = tex->cliph*fbmh;
		cliph = int(fcliph);
		ujit = tex->jitter*(1.0f-ufac);
		vjit = tex->jitter*(1.0f-vfac);
		}
	}

int BMAlphaSampler::PlaceUV(ShadeContext &sc, float &u, float &v, int iu, int iv) {
	if (tex->randPlace) {
		float ru,rv;
		Jitter(sc,ujit,vjit,ru,rv,iu,iv);
		if (u<ru||v<rv||u>ru+ufac||v>rv+vfac) return 0;
		u = (u-ru)/ufac;
		v = (v-rv)/vfac;
		}
	else {
		if (u<u0||v<v0||u>u1||v>v1) return 0;
		u = (u-u0)/ufac;
		v = (v-v0)/vfac;
		}

	return 1;
	}

void BMAlphaSampler::PlaceUVFilter(ShadeContext &sc, float &u, float &v, int iu, int iv) {
	if (tex->randPlace) {
		float ru,rv;
		Jitter(sc,ujit,vjit,ru,rv,iu,iv);
		u = (u-ru)/ufac;
		v = (v-rv)/vfac;
		}
	else {
		u = (u-u0)/ufac;
		v = (v-v0)/vfac;
		}
	}

float BMAlphaSampler::SampleMono(ShadeContext& sc, float u,float v) {
	BMM_Color_64 c;
	int x,y;
	float fu,fv;
	fu = frac(u);
	fv = 1.0f-frac(v);
	if (tex->applyCrop) {
		if (tex->placeImage) {
			if (!PlaceUV(sc,fu,fv,int(u),int(v))) return 0.0f;
			x = (int)(fu*fclipw);
			y = (int)(fv*fcliph);
			}
		else {
			x = mod(clipx + (int)(fu*fclipw),bmw);
			y = mod(clipy + cliph-1-(int)(fv*fcliph),bmh);
			}
		}
	else {
		x = (int)(fu*fbmw);
		y = bmh-1 - (int)((fv*fbmh));
		}
	bm->GetLinearPixels(x,y,1,&c);
	return( (float)c.a/65536.0f);
	}

float BMAlphaSampler::SampleMonoFilter(ShadeContext& sc, float u,float v, float du, float dv) {
	BMM_Color_64 c;
	float fu,fv;
	fu = frac(u);
	fv = 1.0f-frac(v);
	if (tex->applyCrop) {
		if (tex->placeImage) {
			PlaceUVFilter(sc,fu, fv, int(u), int(v));
			du /= ufac;
			dv /= vfac;
			float du2 = 0.5f*du;
			float ua = fu-du2;
			float ub = fu+du2;
			if (ub<=0.0f||ua>=1.0f) return 0.0f;
			float dv2 = 0.5f*dv;
			float va = fv-dv2;
			float vb = fv+du2;
			if (vb<=0.0f||va>=1.0f) return 0.0f;
			BOOL clip = 0;
			if (ua<0.0f) { ua=0.0f; clip = 1; }
			if (ub>1.0f) { ub=1.0f;	clip = 1; }
			if (va<0.0f) { va=0.0f;	clip = 1; }
			if (vb>1.0f) { vb=1.0f;	clip = 1; }
			bm->GetFiltered(fu,fv,du,dv,&c);
			float alph = (float)c.a/65536.0f;
			if (clip) {
				float f = ((ub-ua)/du) * ((vb-va)/dv);
				alph *= f;
				}
			return alph;
			}
		else {
			fu = frac(u0 + ufac*fu);
			fv = frac(v0 + vfac*fv);
			du *= ufac;
			dv *= vfac;
			}
		}
	bm->GetFiltered(fu,fv,du,dv,&c);
	return( (float)c.a/65536.0f);
	}

//------------------------------------------------------------------------
// BMCropper  -- Methods
//------------------------------------------------------------------------

void BMCropper::Init(BMTexDlg *txdlg, TimeValue t) {
	dlg = txdlg;
	tex = txdlg->theBMTex;
	u0 = tex->GetClipU(t);
	v0 = tex->GetClipV(t);
	w0 = tex->GetClipW(t);
	h0 = tex->GetClipH(t);
	mode= tex->placeImage;
	}

void BMCropper::SetValues(float u, float v, float w, float h, BOOL md) {
	BOOL b = FALSE;
	if (u!=tex->clipu) {
		tex->SetClipU(dlg->ip->GetTime(), u);
		b = TRUE;		
		}
	if (v!=tex->clipv) {
		tex->SetClipV(dlg->ip->GetTime(), v);
		b = TRUE;		
		}
	if (w!=tex->clipw) {
		tex->SetClipW(dlg->ip->GetTime(), w);
		b = TRUE;		
		}
	if (h!=tex->cliph) {
		tex->SetClipH(dlg->ip->GetTime(), h);
		b = TRUE;		
		}
	if (md!=tex->placeImage) {
		tex->placeImage= md;
		b = TRUE;
		}
	if(b) {
		tex->DiscardTexHandle();
		tex->NotifyChanged();
		dlg->UpdateMtlDisplay();
		}
	}

void BMCropper::OnClose(){
	dlg->cropping = FALSE;
	}

//------------------------------------------------------------------------
// BMTexNotify -- Methods
//------------------------------------------------------------------------

int BMTexNotify::Changed(ULONG flags) {
	tex->NotifyChanged();
	tex->DiscardTexHandle();
	return 1;
	}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

int numBMTexs = 0;
class BMTexClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading) { 	return new BMTex; }
	const TCHAR *	ClassName() { return GetString(IDS_DS_BITMAP); }
	SClass_ID		SuperClassID() { return TEXMAP_CLASS_ID; }
	Class_ID 		ClassID() { return bmTexClassID; }
	const TCHAR* 	Category() { return TEXMAP_CAT_2D;  }
	};

static BMTexClassDesc bmTexCD;

ClassDesc* GetBMTexDesc() { return &bmTexCD;  }

static BOOL CALLBACK  PanelDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	BMTexDlg *theDlg;
	if (msg==WM_INITDIALOG) {
		theDlg = (BMTexDlg*)lParam;
		theDlg->hPanel = hwndDlg;
		SetWindowLong(hwndDlg, GWL_USERDATA,lParam);
		}
	else {
	    if ( (theDlg = (BMTexDlg *)GetWindowLong(hwndDlg, GWL_USERDATA) ) == NULL )
			return FALSE; 
		}
	theDlg->isActive = TRUE;
	int	res = theDlg->PanelProc(hwndDlg,msg,wParam,lParam);
	theDlg->isActive = FALSE;
	return res;
	}

static BOOL CALLBACK  TimeDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	BMTexDlg *theDlg;
	if (msg==WM_INITDIALOG) {
		theDlg = (BMTexDlg*)lParam;
		theDlg->hTime = hwndDlg;
		SetWindowLong(hwndDlg, GWL_USERDATA,lParam);
		}
	else {
	    if ( (theDlg = (BMTexDlg *)GetWindowLong(hwndDlg, GWL_USERDATA) ) == NULL )
			return FALSE; 
		}
	theDlg->isActive = TRUE;
	int	res = theDlg->TimeProc(hwndDlg,msg,wParam,lParam);
	theDlg->isActive = FALSE;
	return res;
	}

//------------------------------------------------------------------------
// BMTexDlg -- Methods
//------------------------------------------------------------------------
BMTexDlg::BMTexDlg(HWND hwMtlEdit, IMtlParams *imp, BMTex *m) { 
	hwmedit = hwMtlEdit;
	ip = imp;
	hPanel = NULL;
	theBMTex = m; 
	valid  = FALSE;
	isActive  = FALSE;
	cropping = FALSE;
	iName = NULL;
	uvGenDlg = theBMTex->uvGen->CreateParamDlg(hwmedit, imp);
	hPanel = ip->AddRollupPage( 
		hInstance,
		MAKEINTRESOURCE(IDD_BMTEX),
		PanelDlgProc, 
		GetString(IDS_DS_BITMAP_PARAMS), 
		(LPARAM)this );		
	texoutDlg = theBMTex->texout->CreateParamDlg(hwmedit, imp);
	hTime = ip->AddRollupPage( 
		hInstance,
		MAKEINTRESOURCE(IDD_BMTEX_TIME),
		TimeDlgProc, 
		GetString(IDS_DS_TIME_PARAMS), 
		(LPARAM)this,
		APPENDROLL_CLOSED );		
	curTime = imp->GetTime();
	}

void BMTexDlg::ReloadDialog() {
	Interval valid;
	theBMTex->Update(curTime, valid);
	LoadDialog(FALSE);
	}


void BMTexDlg::SetTime(TimeValue t) {
	Interval valid;
	uvGenDlg->SetTime(t);
	texoutDlg->SetTime(t);
	if (t!=curTime) {		
		curTime = t;
		theBMTex->Update(curTime, valid);
		LoadDialog(FALSE);
		InvalidateRect(hPanel,NULL,0);
		InvalidateRect(hTime,NULL,0);
		if (cropping) {
		    theBMTex->StuffCropValues();
			}
		}
	}

BMTexDlg::~BMTexDlg() {
	RemoveCropImage();
	theBMTex->paramDlg = NULL;
	SetWindowLong(hPanel, GWL_USERDATA, NULL);
	SetWindowLong(hTime, GWL_USERDATA, NULL);
	uvGenDlg->DeleteThis();
 	texoutDlg->DeleteThis();
	ReleaseISpinner(startSpin);
	ReleaseISpinner(rateSpin);
	ReleaseISpinner(clipUSpin);
	ReleaseISpinner(clipVSpin);
	ReleaseISpinner(clipWSpin);
	ReleaseISpinner(clipHSpin);
	ReleaseISpinner(jitterSpin);
	ReleaseICustButton(iName);
	iName = NULL;
	hPanel =  NULL;
	hTime = NULL;
	}

void BMTex::FreeBitmap() {
	if (thebm) {
		thebm->DeleteThis();
		thebm = NULL;
		}
	if (texHandle) {
		texHandle->DeleteThis();
		texHandle = NULL;	
		}
	loadFailed = FALSE;
	}

void BMTexDlg::BMNameChanged() {
	theBMTex->FreeBitmap();
	theBMTex->NotifyChanged();
	StuffBMNameField(hPanel);
	theBMTex->loadFailed = FALSE;
	BMMRES res = theBMTex->LoadBitmap(ip->GetTime());
	if (res==BMMRES_NODRIVER) 
	   	InvalidNameMsg(theBMTex->bi.Name());
	EnableAlphaButtons(TRUE);
    UpdateMtlDisplay();
	}

void BMTexDlg::HandleNameButton() {
	BOOL silent = TheManager->SetSilentMode(TRUE);
	BOOL res = TheManager->SelectFileInputEx(&theBMTex->bi, hPanel, GetString(IDS_DS_SELECT_BMFILE));
	TheManager->SetSilentMode(silent);
	if (res) {
		BMNameChanged();
		}
	}

BOOL BMTexDlg::KeyAtCurTime(int id) { return theBMTex->pblock->KeyFrameAtTime(id,curTime); }

void GetBMName(BitmapInfo& bi, TSTR &fname) { 
	TSTR fullName;
	if (bi.Name()[0]==0)
		fullName = bi.Device();
	else 
		fullName =  bi.Name();
	SplitPathFile(fullName,NULL,&fname);
	}

void BMTexDlg::StuffBMNameField(HWND hwndDlg) {
	TSTR fname;
	GetBMName(theBMTex->bi,fname);
	if (iName) {
		iName->SetText(fname.data());
		iName->SetTooltip(TRUE,fname.data());
		}
	}

void BMTexDlg::ShowCropImage() {
	if (!theBMTex) return;
	Bitmap *bm = theBMTex->thebm;
	if (!bm) { 
		BMMRES res = theBMTex->LoadBitmap(ip->GetTime());
		if (!bm) return;
		}
	cropper.Init(this,curTime);
	cropping = TRUE;
    int junk = bm->Display( GetString(IDS_DS_CROP_TITLE),BMM_CN, FALSE, FALSE, &cropper );
	}

void BMTexDlg::RemoveCropImage() {
	cropping = FALSE;
	if (!theBMTex) return;
	Bitmap *bm = theBMTex->thebm;
	if (!bm) return;
 	bm->UnDisplay( );
	}


static int colID[2] = { IDC_CHECK_COL1, IDC_CHECK_COL2 };

void BMTexDlg::EnableAlphaButtons(BOOL isNew) {
	if (hPanel==NULL) return;
	if (theBMTex->bi.Flags()&MAP_HAS_ALPHA) {
		if (isNew) {
 			theBMTex->SetAlphaSource(ALPHA_FILE);
 			//theBMTex->SetAlphaAsMono(TRUE);
			}
		EnableWindow(GetDlgItem(hPanel,IDC_ALPHA_FILE), 1);
		}
	else {
		if (isNew||theBMTex->alphaSource==ALPHA_FILE) 
 			theBMTex->SetAlphaSource(ALPHA_NONE);
		EnableWindow(GetDlgItem(hPanel,IDC_ALPHA_FILE), 0);
		}
	CheckRadioButton( hPanel, IDC_ALPHA_FILE, IDC_ALPHA_NONE, theBMTex->alphaSource+IDC_ALPHA_FILE);
	CheckRadioButton( hPanel, IDC_BM_CROP, IDC_BM_PLACE, theBMTex->placeImage+IDC_BM_CROP);
	SetCheckBox( hPanel, IDC_BM_JITTER, theBMTex->randPlace);
	}


void BMTexDlg::EnableViewImage() {
	EnableWindow(GetDlgItem(hPanel,IDC_BM_CROP_IMAGE),(_tcslen(theBMTex->bi.Name())>0)?1:0); 	
	}
	
BOOL BMTexDlg::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam ) {
	int id = LOWORD(wParam);
	int code = HIWORD(wParam);
    switch (msg)    {
		case WM_INITDIALOG:
			{

			EnableAlphaButtons();
			CheckRadioButton( hwndDlg, IDC_ALPHA_FILE, IDC_ALPHA_NONE, IDC_ALPHA_FILE+theBMTex->alphaSource);
			CheckRadioButton( hwndDlg, IDC_FILTER_PYR, IDC_FILTER_NADA, IDC_FILTER_PYR+theBMTex->filterType);
			CheckRadioButton( hwndDlg, IDC_BMTEX_RGBOUT, IDC_BMTEX_ALPHAOUT, IDC_BMTEX_RGBOUT+theBMTex->alphaAsMono);
			CheckRadioButton( hwndDlg, IDC_BMTEX_RGBOUT2, IDC_BMTEX_ALPHAOUT2, IDC_BMTEX_RGBOUT2+theBMTex->alphaAsRGB);
			clipUSpin = SetupFloatSpinner(hwndDlg, IDC_CLIP_XSPIN, IDC_CLIP_X,0.0f,1.0f,theBMTex->clipu,.001f);
			clipVSpin = SetupFloatSpinner(hwndDlg, IDC_CLIP_YSPIN, IDC_CLIP_Y,0.0f,1.0f,theBMTex->clipv,.001f);
			clipWSpin = SetupFloatSpinner(hwndDlg, IDC_CLIP_WSPIN, IDC_CLIP_W,.0001f,1.0f,theBMTex->clipw,.001f);
			clipHSpin = SetupFloatSpinner(hwndDlg, IDC_CLIP_HSPIN, IDC_CLIP_H,.0001f,1.0f,theBMTex->cliph,.001f);
			jitterSpin = SetupFloatSpinner(hwndDlg, IDC_JITTER_SPIN, IDC_JITTER_EDIT,0.0f,1.0f,theBMTex->jitter,.01f);
			SetCheckBox( hwndDlg, IDC_ALPHA_PREMULT, theBMTex->premultAlpha);

			SetCheckBox( hwndDlg, IDC_BM_CLIP, theBMTex->applyCrop);
			CheckRadioButton( hwndDlg, IDC_BM_CROP, IDC_BM_PLACE, theBMTex->placeImage+IDC_BM_CROP);
			SetCheckBox( hwndDlg, IDC_BM_JITTER, theBMTex->randPlace);
			EnableViewImage();

			iName = GetICustButton(GetDlgItem(hwndDlg,IDC_BMTEX_NAME));
			iName->SetDADMgr(this);

			StuffBMNameField(hwndDlg);

			return TRUE;
			}
			break;
		case WM_COMMAND:  
		    switch (id) {
				case IDC_BMTEX_NAME: 
					HandleNameButton();
					EnableViewImage();
					break;
				case IDC_ALPHA_FILE:
				case IDC_ALPHA_XPCOL:
				case IDC_ALPHA_RGB:
				case IDC_ALPHA_NONE:
					CheckRadioButton( hwndDlg, IDC_ALPHA_FILE, IDC_ALPHA_NONE, id);
					theBMTex->SetAlphaSource(id-IDC_ALPHA_FILE);
					theBMTex->NotifyChanged();
					theBMTex->DiscardTexHandle();
				    UpdateMtlDisplay();
					break;
				case IDC_FILTER_NADA:
				case IDC_FILTER_PYR:
				case IDC_FILTER_SAT:
					CheckRadioButton( hwndDlg, IDC_FILTER_PYR, IDC_FILTER_NADA, id);
					theBMTex->SetFilterType(id-IDC_FILTER_PYR);
					theBMTex->NotifyChanged();
					break;
				case IDC_BMTEX_RELOAD:
					theBMTex->ReloadBitmap();
					theBMTex->NotifyChanged();
				    UpdateMtlDisplay(); // redraw viewports
					break;
				case IDC_BMTEX_RGBOUT:
				case IDC_BMTEX_ALPHAOUT:
					theBMTex->alphaAsMono = (id==IDC_BMTEX_ALPHAOUT)?1:0;
					theBMTex->NotifyChanged();
					break;
				case IDC_BMTEX_RGBOUT2:
				case IDC_BMTEX_ALPHAOUT2:
					theBMTex->alphaAsRGB = (id==IDC_BMTEX_ALPHAOUT2)?1:0;
					theBMTex->NotifyChanged();
					theBMTex->DiscardTexHandle();
				    UpdateMtlDisplay();
					break;
				case IDC_ALPHA_PREMULT:
					theBMTex->premultAlpha =  GetCheckBox(hwndDlg,IDC_ALPHA_PREMULT);
					theBMTex->NotifyChanged();
					theBMTex->DiscardTexHandle();
				    UpdateMtlDisplay();
					break;
				case IDC_BM_CLIP: 
					theBMTex->applyCrop = GetCheckBox(hwndDlg,id);
					theBMTex->NotifyChanged();
					theBMTex->DiscardTexHandle();
				    UpdateMtlDisplay();
					break;
				case IDC_BM_CROP_IMAGE: 
					ShowCropImage();
					break;
				case IDC_BM_CROP:
				case IDC_BM_PLACE:
					CheckRadioButton( hwndDlg, IDC_BM_CROP, IDC_BM_PLACE, id);
					theBMTex->placeImage = id==IDC_BM_PLACE?1:0;
					theBMTex->NotifyChanged();
					theBMTex->DiscardTexHandle();
				    UpdateMtlDisplay();
					if (cropping) theBMTex->StuffCropValues();
					break;
				case IDC_BM_JITTER:
					theBMTex->randPlace =  GetCheckBox(hwndDlg,id);
					theBMTex->NotifyChanged();
				    UpdateMtlDisplay();
					break;
				}
			break;
		case WM_PAINT: 	
			if (!valid) {
				valid = TRUE;
				ReloadDialog();
				}
			break;
		case CC_SPINNER_CHANGE: 
			if (!theHold.Holding()) theHold.Begin();
			switch (id) {
				case IDC_CLIP_XSPIN: {
					float u = clipUSpin->GetFVal();
					theBMTex->SetClipU(curTime,u);
					clipUSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPU));
					float w = theBMTex->GetClipW(curTime);
					if (u+w>1.0f) {
						w = 1.0f-u;
						theBMTex->SetClipW(curTime,w);
						clipWSpin->SetValue(theBMTex->clipw,FALSE);
						clipWSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPW));
						}
					theBMTex->NotifyChanged();
					if (cropping) theBMTex->StuffCropValues();
					}
					break; 	
				case IDC_CLIP_YSPIN: {
					float v = clipVSpin->GetFVal();
					theBMTex->SetClipV(curTime,v);
					clipVSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPV));
					float h = theBMTex->GetClipH(curTime);
					if (v+h>1.0f) {
						h = 1.0f-v;
						theBMTex->SetClipH(curTime,h);
						clipHSpin->SetValue(theBMTex->cliph,FALSE);
						clipHSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPH));
						}
					theBMTex->NotifyChanged();
					if (cropping) theBMTex->StuffCropValues();
					}
					break; 	
				case IDC_CLIP_WSPIN: {
					float w = clipWSpin->GetFVal();
					theBMTex->SetClipW(curTime,w);
					clipWSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPW));
					float u = theBMTex->GetClipU(curTime);
					if (u+w>1.0f) {
						u = 1.0f-w;
						theBMTex->SetClipU(curTime,u);
						clipUSpin->SetValue(theBMTex->clipu,FALSE);
						clipUSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPU));
						}
					theBMTex->NotifyChanged();
					if (cropping) theBMTex->StuffCropValues();
					}
					break; 	
				case IDC_CLIP_HSPIN: {
					float h = clipHSpin->GetFVal();
					theBMTex->SetClipH(curTime,h);
					clipHSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPH));
					float v = theBMTex->GetClipV(curTime);
					if (v+h>1.0f) {
						v = 1.0f-h;
						theBMTex->SetClipV(curTime,v);
						clipVSpin->SetValue(theBMTex->clipv,FALSE);
						clipVSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPV));
						}
					theBMTex->NotifyChanged();
					if (cropping) theBMTex->StuffCropValues();
					}
					break;
				case IDC_JITTER_SPIN:
					theBMTex->SetJitter(curTime,jitterSpin->GetFVal());
					jitterSpin->SetKeyBrackets(KeyAtCurTime(PB_JITTER));
					theBMTex->NotifyChanged();
					break;
				}
			break;
		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			break;		
		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP: 
			if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER) theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else theHold.Cancel();
			theBMTex->DiscardTexHandle();
			theBMTex->NotifyChanged();
		    UpdateMtlDisplay();
			if (cropping) theBMTex->StuffCropValues();
			break;
		case WM_DESTROY:		
			RemoveCropImage();
			break;
    	}
	return FALSE;
	}


BOOL BMTexDlg::TimeProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam ) {
	int id = LOWORD(wParam);
	int code = HIWORD(wParam);
    switch (msg)    {
		case WM_INITDIALOG:
			{
			startSpin = SetupIntSpinner(hwndDlg, IDC_BMTEX_START_SPIN, IDC_BMTEX_START,-100000 ,100000, 
				theBMTex->startTime/GetTicksPerFrame());
			rateSpin = SetupFloatSpinner(hwndDlg, IDC_BMTEX_RATE_SPIN, IDC_BMTEX_RATE,0.0f, 1000.0f, 1.0f,.1f);
			CheckRadioButton( hwndDlg, IDC_BMTEX_LOOP, IDC_BMTEX_HOLD, IDC_BMTEX_LOOP+theBMTex->endCond);
			return TRUE;
			}
			break;
		case WM_COMMAND:  
		    switch (id) {
				case IDC_BMTEX_LOOP:
				case IDC_BMTEX_HOLD:
				case IDC_BMTEX_PINGPONG:
					theBMTex->endCond = id-IDC_BMTEX_LOOP;
					theBMTex->NotifyChanged();
				    UpdateMtlDisplay();
					break;
				}
			break;
		case CC_SPINNER_CHANGE: 
			if (!theHold.Holding()) theHold.Begin();
			switch (id) {
				case IDC_BMTEX_START_SPIN: 
					theBMTex->startTime = startSpin->GetIVal()*GetTicksPerFrame(); 	
					break;
				case IDC_BMTEX_RATE_SPIN: 
					theBMTex->pbRate = rateSpin->GetFVal(); 	
					break;
				}
			break;
		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			break;		
		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP: 
			if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER) 
				theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else 
				theHold.Cancel();
			theBMTex->NotifyChanged();
		    UpdateMtlDisplay();
			break;

    	}
	return FALSE;
	}


void BMTexDlg::LoadDialog(BOOL draw) {
	if (theBMTex) {
		Interval valid;
		theBMTex->Update(curTime,valid);
		StuffBMNameField(hPanel);
		CheckRadioButton( hPanel, IDC_ALPHA_FILE, IDC_ALPHA_NONE, IDC_ALPHA_FILE+theBMTex->alphaSource);
		CheckRadioButton( hPanel, IDC_FILTER_PYR, IDC_FILTER_NADA, IDC_FILTER_PYR+theBMTex->filterType);
		CheckRadioButton( hPanel, IDC_BMTEX_RGBOUT, IDC_BMTEX_ALPHAOUT, IDC_BMTEX_RGBOUT+theBMTex->alphaAsMono);
		CheckRadioButton( hPanel, IDC_BMTEX_RGBOUT2, IDC_BMTEX_ALPHAOUT2, IDC_BMTEX_RGBOUT2+theBMTex->alphaAsRGB);
		SetCheckBox( hPanel, IDC_ALPHA_PREMULT, theBMTex->premultAlpha);
		SetCheckBox( hPanel, IDC_BM_CLIP, theBMTex->applyCrop);

		startSpin->SetValue(theBMTex->startTime/GetTicksPerFrame(),FALSE);
		rateSpin->SetValue(theBMTex->pbRate,FALSE);
		clipUSpin->SetValue(theBMTex->clipu,FALSE);
		clipVSpin->SetValue(theBMTex->clipv,FALSE);
		clipWSpin->SetValue(theBMTex->clipw,FALSE);
		clipHSpin->SetValue(theBMTex->cliph,FALSE);
		jitterSpin->SetValue(theBMTex->jitter,FALSE);

		clipUSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPU));
		clipVSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPV));
		clipWSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPW));
		clipHSpin->SetKeyBrackets(KeyAtCurTime(PB_CLIPH));
		jitterSpin->SetKeyBrackets(KeyAtCurTime(PB_JITTER));

		CheckRadioButton( hTime, IDC_BMTEX_LOOP, IDC_BMTEX_HOLD, IDC_BMTEX_LOOP+theBMTex->endCond);
		EnableAlphaButtons(FALSE);
		}
	}

void BMTexDlg::SetThing(ReferenceTarget *m) {
	assert (m->ClassID()==bmTexClassID);
	assert (m->SuperClassID()==TEXMAP_CLASS_ID);
 	RemoveCropImage();
	if (theBMTex) {
		theBMTex->paramDlg = NULL;
		}
	theBMTex = (BMTex *)m;
	if (theBMTex) theBMTex->paramDlg = this;
	uvGenDlg->SetThing(theBMTex->uvGen);
	texoutDlg->SetThing(theBMTex->texout);
	LoadDialog(TRUE);
	}


ReferenceTarget *BMTexDlg::GetInstance(HWND hwnd, POINT p, SClass_ID type) {
	DADBitmapCarrier *bmc = GetDADBitmapCarrier();
	TSTR nm = theBMTex->bi.Name();
	bmc->SetName(nm);
	return bmc;
	}

void BMTexDlg::Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type) {
	if (dropThis->SuperClassID()!=BITMAPDAD_CLASS_ID) 
		return;
	DADBitmapCarrier *bmc = (DADBitmapCarrier *)dropThis;
	theBMTex->bi.SetName(bmc->GetName().data());
	BMNameChanged();
	}

//------------------------------------------------------------------------
// BMTex -- Methods
//------------------------------------------------------------------------


#define BMTEX_VERSION 3

static ParamBlockDescID pbdesc1[] = {
	{ TYPE_FLOAT, NULL, TRUE,1} 	// blur
	};   

static ParamBlockDescID pbdesc2[] = {
	{ TYPE_FLOAT, NULL, TRUE,1 }, // clipu
	{ TYPE_FLOAT, NULL, TRUE,2 }, // clipv
	{ TYPE_FLOAT, NULL, TRUE,3 }, // clipw
	{ TYPE_FLOAT, NULL, TRUE,4 }, // cliph
	{ TYPE_FLOAT, NULL, TRUE,5 } // jitter
	};

static ParamVersionDesc oldVersions[] = {
	ParamVersionDesc(pbdesc1, 1, 0),
	ParamVersionDesc(pbdesc1, 1, 1),
	ParamVersionDesc(pbdesc2, 4, 2)
	};

static ParamVersionDesc curVersion(pbdesc2,5,BMTEX_VERSION);

static int name_id[NPARAMS] = { IDS_DS_CLIPU, IDS_DS_CLIPV, IDS_DS_CLIPW, IDS_DS_CLIPH, IDS_DS_JITTERAMT };

void BMTex::Reset() {
	if (uvGen) uvGen->Reset();
	else ReplaceReference( 0, GetNewDefaultUVGen());	
	ReplaceReference( 1, CreateParameterBlock( pbdesc2, NPARAMS, BMTEX_VERSION) );	
	if (texout) texout->Reset();
	else ReplaceReference( 2, GetNewDefaultTextureOutput());
	
	placeImage = FALSE;
	randPlace = FALSE;
	filterType = FILTER_PYR;
	alphaSource = ALPHA_FILE;
	alphaAsMono = FALSE;
	alphaAsRGB = FALSE;
	premultAlpha = TRUE;
	SetClipU(0,0.0f);
	SetClipV(0,0.0f);
	SetClipW(0,1.0f);
	SetClipH(0,1.0f);
	SetJitter(0,1.0f);
	ivalid.SetEmpty();
	clipValid.SetInfinite();
	if (!isNew) {
		FreeBitmap();
		NotifyChanged();
	    if (paramDlg) paramDlg->UpdateMtlDisplay();
		}
	isNew = FALSE;
	rumax = rvmax = -100000.0f;
	rumin = rvmin = +100000.0f;
	}

void BMTex::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

BMTex::BMTex() {
	bmNotify.SetTex(this);
	paramDlg = NULL;
	pblock = NULL;
	uvGen = NULL;
	texout = NULL;
	thebm = NULL;
	texHandle = NULL;
	loadFailed = FALSE;
	inRender = FALSE;
	startTime = 0;
	pbRate = 1.0f;
	endCond = END_LOOP;
	isNew = TRUE;
	applyCrop = FALSE;
	loadingOld = FALSE;
	clipValid = FOREVER;
	Reset();
	rollScroll=0;
	}

static int bmFilterType(int filt_type) {
	switch(filt_type) {
		case FILTER_PYR: return BMM_FILTER_PYRAMID;
		case FILTER_SAT: return BMM_FILTER_SUM;
		default: return BMM_FILTER_NONE;
		}
	}

void BMTex::SetFilterType(int ft) {
	filterType = ft;
	if (thebm)	
		thebm->SetFilter(bmFilterType(filterType));
	}



void BMTex::SetAlphaSource(int as) {
	if (as!=alphaSource) {
		alphaSource =as; 
		mysamp.Init(this);
		alphasamp.Init(this);
		}
	}

static TimeValue modt(TimeValue x, TimeValue m) {
	TimeValue n = (int)(x/m);
	x -= n*m;
	return (x<0)? x+m : x ;
	}

void BMTex::StuffCropValues() {
	if (thebm) thebm->SetCroppingValues(clipu,clipv,clipw,cliph, placeImage );
	}

int BMTex::CalcFrame(TimeValue t) {
	TimeValue tm,dur,td;
	int fstart = bi.FirstFrame();
	int fend = bi.LastFrame();
	int tpf = GetTicksPerFrame();
	tm = TimeValue(float(t-startTime)*pbRate);
	dur = (fend-fstart+1)*GetTicksPerFrame();
	switch (endCond) {
		case END_HOLD:
			if (tm<=0) return fstart;
			if (tm>=dur) return fend;
			return tm/tpf;
		case END_PINGPONG:
			//if ((tm/dur)&1) 
			if ( ((tm>=0) && ((tm/dur)&1)) || ( (tm<0) && !(tm/dur)))
				{
				td = modt(tm,dur);
				return fstart + fend-td/tpf;
				}
			// else fall thrue--
		case END_LOOP:
			td = modt(tm,dur);
			return td/tpf;
		}
	return 0;
	}

BMMRES BMTex::LoadBitmap(TimeValue t) {
	BMMRES status = BMMRES_SUCCESS;
	if (bi.Name()[0]==0)
		if (bi.Device()[0]==0)
			return BMMRES_NODRIVER;
	if (thebm==NULL) {
		bi.SetCurrentFrame(CalcFrame(t));
		BOOL silent = TheManager->SetSilentMode(TRUE);
		SetCursor(LoadCursor(NULL,IDC_WAIT));
		thebm = TheManager->Load(&bi,&status);
		SetCursor(LoadCursor(NULL,IDC_ARROW));
		TheManager->SetSilentMode(silent);
		if (thebm==NULL) {
			return status;
			}
		thebm->SetNotify(&bmNotify);
		thebm->SetFilter(bmFilterType(filterType));
		// fixup for imported files
		if (!(bi.Flags()&MAP_HAS_ALPHA)&&alphaSource==ALPHA_FILE) {
			if (paramDlg) 
				paramDlg->EnableAlphaButtons(FALSE);
			else 
	 			SetAlphaSource(ALPHA_NONE);
			}			
		}
//	else {
		bi.SetCurrentFrame(CalcFrame(t));
		BOOL silent = TheManager->SetSilentMode(TRUE);
		status = thebm->GoTo(&bi);
		TheManager->SetSilentMode(silent);
//		}
	mysamp.Init(this);
	alphasamp.Init(this);
	return status;
	}


void BMTex::RenderBitmap(TimeValue t, Bitmap *bm, float scale3D, BOOL filter) {
	LoadBitmap(t);
	if (thebm) 
		bm->CopyImage(thebm, filter?COPY_IMAGE_RESIZE_HI_QUALITY:COPY_IMAGE_RESIZE_LO_QUALITY, 0);
	}

void BMTex::ReloadBitmap() {
	if (thebm) {
		loadFailed = FALSE;
//		FreeBitmap(); 
//		LoadBitmap(paramDlg->ip->GetTime());
		TheManager->LoadInto(&bi,&thebm,TRUE);
		}
	else {
		loadFailed = FALSE;
		TimeValue t;
		t = paramDlg? paramDlg->ip->GetTime(): GetCOREInterface()->GetTime();
		LoadBitmap(t);
		}
	if (paramDlg) 
		paramDlg->EnableAlphaButtons(FALSE);
	}

void BMTex::DiscardTexHandle() {
	if (texHandle) {
		texHandle->DeleteThis();
		texHandle = NULL;
		}
	}

void BMTex::ActivateTexDisplay(BOOL onoff) {
	if (!onoff) 
		DiscardTexHandle();
	}

static BMM_Color_64 black64 = {0,0,0,0};

DWORD BMTex::GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker) {
	if (texHandle&&texTime==CalcFrame(t)&&texValid.InInterval(t)) 
		return texHandle->GetHandle();
	else {
		DiscardTexHandle();  // DS 9/9/96
		LoadBitmap(t);
		if (thebm==NULL) {
			if (!loadFailed) {
				TSTR fname;
				GetBMName(bi,fname);
				LoadFailedMsg(fname.data());
				loadFailed = TRUE;
				}
			return 0;
			}
		texTime = CalcFrame(t);
		texValid = clipValid;
		int xflags = premultAlpha?0:EX_MULT_ALPHA;
		if (applyCrop) {
			int w = thebm->Width();
			int h = thebm->Height();
			Bitmap *newBM;
			BitmapInfo bif;
			bif.SetName(_T("y8798734"));
			bif.SetType(BMM_TRUE_32);
	   		bif.SetFlags(MAP_HAS_ALPHA);
			if (placeImage) {
				int x0,y0,nw,nh;
				int bmw = thmaker.Size();
				int bmh = int(float(bmw)*float(h)/float(w));
				bif.SetWidth(bmw);
				bif.SetHeight(bmh);
				newBM = TheManager->Create(&bif);
			 	newBM->Fill(0,0,0,0);
				nw = int(float(bmw)*clipw);
				nh = int(float(bmh)*cliph);
				x0 = int(float(bmw-1)*clipu);
				y0 = int(float(bmh-1)*clipv);

				if (nw<1) nw = 1;
				if (nh<1) nh = 1;
				PixelBuf row(nw);
				
				Bitmap *tmpBM;
				BitmapInfo bif2;
				bif2.SetName(_T("xxxx67878"));
				bif2.SetType(BMM_TRUE_32);
				bif2.SetFlags(MAP_HAS_ALPHA);
				bif2.SetWidth(nw);				
				bif2.SetHeight(nh);
				tmpBM = TheManager->Create(&bif2);
				tmpBM->CopyImage(thebm, COPY_IMAGE_RESIZE_LO_QUALITY, 0);
				BMM_Color_64*  p1 = row.Ptr();
				for (int y = 0; y<nh; y++) {
					tmpBM->GetLinearPixels(0,y, nw, p1);
					if (alphaAsRGB) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].r = p1[ix].g = p1[ix].b = p1[ix].a;
						}
					if (alphaSource==ALPHA_NONE||premultAlpha) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].a = 0xffff;
						}
					else if (alphaSource==ALPHA_RGB) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].a = (p1[ix].r+p1[ix].g+p1[ix].b)/3;;
						}
					newBM->PutPixels(x0,y+y0, nw, p1);
					}
				tmpBM->DeleteThis();
				texHandle = thmaker.CreateHandle(newBM,uvGen->SymFlags(), xflags);
				newBM->DeleteThis();
				}
			else {
				int x0,y0,nw,nh;
				nw = int(float(w)*clipw);
				nh = int(float(h)*cliph);
				x0 = int(float(w-1)*clipu);
				y0 = int(float(h-1)*clipv);
				if (nw<1) nw = 1;
				if (nh<1) nh = 1;
				bif.SetWidth(nw);
				bif.SetHeight(nh);
				PixelBuf row(nw);
				newBM = TheManager->Create(&bif);
				BMM_Color_64*  p1 = row.Ptr();
				for (int y = 0; y<nh; y++) {
					thebm->GetLinearPixels(x0,y+y0, nw, p1);
					if (alphaAsRGB) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].r = p1[ix].g = p1[ix].b = p1[ix].a;
						}
					if (alphaSource==ALPHA_NONE||premultAlpha) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].a = 0xffff;
						}
					else if (alphaSource==ALPHA_RGB) {
						for (int ix =0; ix<nw; ix++) 
							p1[ix].a = (p1[ix].r+p1[ix].g+p1[ix].b)/3;;
						}
					newBM->PutPixels(0,y, nw, p1);
					}
				texHandle = thmaker.CreateHandle(newBM,uvGen->SymFlags(), xflags);
				newBM->DeleteThis();
				}
			}
		else {
			if (alphaAsRGB) xflags |= EX_RGB_FROM_ALPHA;
			texHandle = thmaker.CreateHandle(thebm,uvGen->SymFlags(), xflags);
			}
		return texHandle->GetHandle();
		}
	}
							 
//----------------------------------------------------------- 

AColor BMTex::EvalColor(ShadeContext& sc) {
	if (!sc.doMaps) return black;
	AColor c;
	if (sc.GetCache(this,c)) 
		return c; 
	IPoint3 sp = sc.ScreenCoord();
	if (gbufID) sc.SetGBufferID(gbufID);
	if (thebm==NULL) 
		return black;
	if (alphaAsRGB)	{
		float a = texout->Filter(uvGen->EvalUVMapMono(sc,&alphasamp,filterType!=FILTER_NADA));
		c = AColor(a,a,a,a);
		}
	else {
		c = texout->Filter(uvGen->EvalUVMap(sc,&mysamp,filterType!=FILTER_NADA));
		if (!premultAlpha) c= AColor(c.r*c.a, c.g*c.a, c.b*c.a, c.a);
		}
	sc.PutCache(this,c); 
	return c;
	}

float BMTex::EvalMono(ShadeContext& sc) {
	if (!sc.doMaps||thebm==NULL) 
		return 0.0f;
	float f;
	if (sc.GetCache(this,f)) 
		return f; 
	if (gbufID) sc.SetGBufferID(gbufID);
	if (alphaAsMono) 
		f = texout->Filter(uvGen->EvalUVMapMono(sc,&alphasamp,filterType!=FILTER_NADA));
	else 
		f = texout->Filter(uvGen->EvalUVMapMono(sc,&mysamp,filterType!=FILTER_NADA));
	sc.PutCache(this,f); 
	return f;
	}

Point3 BMTex::EvalNormalPerturb(ShadeContext& sc) {
	Point3 dPdu, dPdv;
	Point2 dM;
	if (!sc.doMaps) return Point3(0,0,0);
	if (gbufID) sc.SetGBufferID(gbufID);
	if (thebm==NULL) 
		return Point3(0,0,0);
	uvGen->GetBumpDP(sc,dPdu,dPdv);  // get bump basis vectors
	if (alphaAsMono) 
		dM =(.01f)*uvGen->EvalDeriv(sc,&alphasamp,filterType!=FILTER_NADA);
	else 
		dM =(.01f)*uvGen->EvalDeriv(sc,&mysamp,filterType!=FILTER_NADA);

#if 0
	// Blinn's algorithm
	Point3 N = sc.Normal();
	Point3 uVec = CrossProd(N,dPdv);
	Point3 vVec = CrossProd(N,dPdu);
	return texout->Filter(-dM.x*uVec+dM.y*vVec);
#else						 
	return texout->Filter(dM.x*dPdu+dM.y*dPdv);
#endif
	}

RefTargetHandle BMTex::Clone(RemapDir &remap) {
	BMTex *mnew = new BMTex();
	*((MtlBase*)mnew) = *((MtlBase*)this);  // copy superclass stuff
	mnew->ReplaceReference(0,remap.CloneRef(uvGen));
	mnew->ReplaceReference(1,remap.CloneRef(pblock));
	mnew->ReplaceReference(2,remap.CloneRef(texout));
	mnew->filterType  = filterType;
	mnew->alphaSource = alphaSource;
	mnew->alphaAsMono = alphaAsMono;
	mnew->alphaAsRGB  = alphaAsRGB;
	mnew->premultAlpha = premultAlpha;
	mnew->endCond     = endCond;
	mnew->applyCrop   = applyCrop;
	mnew->ivalid.SetEmpty();
	mnew->placeImage  = placeImage;
	mnew->randPlace   = randPlace;
	mnew->applyCrop   = applyCrop;
	mnew->startTime  = startTime;
	mnew->pbRate  = pbRate;
	mnew->bi = bi;
	return (RefTargetHandle)mnew;
	}

ParamDlg* BMTex::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) {
	BMTexDlg *dm = new BMTexDlg(hwMtlEdit, imp, this);
	dm->LoadDialog(TRUE);	
	paramDlg = dm;
	return dm;	
	}

BMTex::~BMTex() {
	DiscardTexHandle();
	FreeBitmap();
	}

void BMTex::Update(TimeValue t, Interval& valid) {		
	if (!ivalid.InInterval(t)) {
		uvGen->SetClipFlag(FALSE);

		ivalid.SetInfinite();
		uvGen->Update(t,ivalid);
		clipValid.SetInfinite();
		pblock->GetValue( PB_CLIPU, t, clipu, clipValid );
		pblock->GetValue( PB_CLIPV, t, clipv, clipValid );
		pblock->GetValue( PB_CLIPW, t, clipw, clipValid );
		pblock->GetValue( PB_CLIPH, t, cliph, clipValid );
		pblock->GetValue( PB_JITTER, t, jitter, clipValid );
		if (applyCrop) ivalid &= clipValid;
		else clipValid.SetInfinite();
		texout->Update(t,ivalid);
		}
	if (thebm) {
		if (bi.FirstFrame()!=bi.LastFrame())
			ivalid.SetInstant(t);  // force bitmap to be reloaded
		}
	else 
		ivalid.SetInstant(t);  // force bitmap to be reloaded 
	UpdtSampler();
	valid &= ivalid;
	}

RefTargetHandle BMTex::GetReference(int i) {
	switch(i) {
		case 0: return uvGen;
		case 1: return pblock;
		case 2:	return texout;
		default: return NULL;
		}
	}

int BMTex::RemapRefOnLoad(int iref) { 
	if (loadingOld) { 
		switch(iref) {
			case 0: return 0;
			case 1: return 2;
			default: assert(0); return 0;
			}
		}
	else return iref;
	}

void BMTex::SetReference(int i, RefTargetHandle rtarg) {
	switch(i) {
		case 0: uvGen = (UVGen *)rtarg; break;
		case 1: pblock = (IParamBlock *)rtarg; break;
		case 2:	texout = (TextureOutput *)rtarg; break;
		default: break;
		}
	}

	 
Animatable* BMTex::SubAnim(int i) {
	switch (i) {
		case 0: return uvGen;
		case 1: return pblock;
		case 2: return texout;
		default: assert(0); return NULL;
		}
	}

TSTR BMTex::GetFullName() {
	TSTR cnm,nm,fname;
   	GetClassName(cnm);
	GetBMName(bi,fname);
	TCHAR *s = fname.Length()>0? fname.data(): cnm.data(); 
	nm.printf(_T("%s (%s)"),GetName().data(), s);
	return nm;
	}

TSTR BMTex::SubAnimName(int i) {
	switch (i) {
		case 0: return TSTR(GetString(IDS_DS_COORDINATES));		
		case 1: return TSTR(GetString(IDS_DS_PARAMETERS));		
		case 2: return TSTR(GetString(IDS_DS_OUTPUT));		
		default: assert(0); return TSTR(_T(""));
		}
	}

RefResult BMTex::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
   PartID& partID, RefMessage message ) {
	switch (message) {
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget!=uvGen&&hTarget!=texout) {
//				if (paramDlg&&!paramDlg->isActive) 
				if (paramDlg) 
				 	paramDlg->Invalidate();
				}
			break;

		case REFMSG_UV_SYM_CHANGE:
			DiscardTexHandle();  
			break;

		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			switch (gpd->index) {
				case PB_CLIPU: 
				case PB_CLIPV: 
				case PB_CLIPW: 
				case PB_CLIPH: 
				case PB_JITTER: gpd->dim = defaultDim; break;
				}
			return REF_STOP; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			gpn->name= TSTR(GetString(name_id[gpn->index]));
			return REF_STOP; 
			}
		}
	return(REF_SUCCEED);
	}



//------------------------------------------------------------------------
// IO
//------------------------------------------------------------------------

class BMTexPostLoad : public PostLoadCallback {
	public:
		BMTex *chk;
		BMTexPostLoad(BMTex *b) {chk=b;}
		void proc(ILoad *iload) {
			chk->loadingOld = FALSE;
			if (chk->bmName.Length()>0) {
				chk->bi.SetName(chk->bmName);   // for obsolete files	
				iload->SetObsolete();
				}
			delete this;
			}
	};

#define MTL_HDR_CHUNK 0x4000
#define OLDBMTEX_NAME_CHUNK 0x5001
#define BMTEX_FILTER_CHUNK 0x5002
#define BMTEX_ALPHASOURCE_CHUNK 0x5003
#define BMTEX_NAME_CHUNK 0x5004
#define BMTEX_IO_CHUNK 0x5010
#define BMTEX_START_CHUNK 0x5011
#define BMTEX_RATE_CHUNK 0x5012
#define BMTEX_ALPHA_MONO_CHUNK 0x5013
#define BMTEX_ENDCOND_CHUNK 0x5014
#define BMTEX_ALPHA_RGB_CHUNK 0x5016
#define BMTEX_VERSOLD_CHUNK 0x5021
#define BMTEX_VERSION_CHUNK 0x5022
#define BMTEX_ALPHA_NOTPREMULT_CHUNK 0x5030
#define BMTEX_CLIP_CHUNK 0x5040
#define BMTEX_PLACE_IMAGE_CHUNK 0x5050
#define BMTEX_JITTER_CHUNK 0x5060

IOResult BMTex::Save(ISave *isave) { 
	ULONG nb;
	IOResult res;
	// Save common stuff
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();

	isave->BeginChunk(BMTEX_FILTER_CHUNK);
	isave->Write(&filterType,sizeof(filterType),&nb);			
	isave->EndChunk();

	isave->BeginChunk(BMTEX_ALPHASOURCE_CHUNK);
	isave->Write(&alphaSource,sizeof(alphaSource),&nb);			
	isave->EndChunk();

	if (startTime!=0) {
		isave->BeginChunk(BMTEX_START_CHUNK);
		isave->Write(&startTime,sizeof(startTime),&nb);			
		isave->EndChunk();
		}
	if (pbRate!=1.0) {
		isave->BeginChunk(BMTEX_RATE_CHUNK);
		isave->Write(&pbRate,sizeof(pbRate),&nb);			
		isave->EndChunk();
		}
	if (alphaAsMono) {
		isave->BeginChunk(BMTEX_ALPHA_MONO_CHUNK);
		isave->EndChunk();
		}
	if (alphaAsRGB) {
		isave->BeginChunk(BMTEX_ALPHA_RGB_CHUNK);
		isave->EndChunk();
		}
	if (!premultAlpha) {
		isave->BeginChunk(BMTEX_ALPHA_NOTPREMULT_CHUNK);
		isave->EndChunk();
		}
	if (endCond != END_LOOP) {
		isave->BeginChunk(BMTEX_ENDCOND_CHUNK);
		isave->Write(&endCond,sizeof(endCond),&nb);			
		isave->EndChunk();
		}
	if (applyCrop) {
		isave->BeginChunk(BMTEX_CLIP_CHUNK);
		isave->EndChunk();
		}
	if (placeImage) {
		isave->BeginChunk(BMTEX_PLACE_IMAGE_CHUNK);
		isave->EndChunk();
		}
	if (randPlace) {
		isave->BeginChunk(BMTEX_JITTER_CHUNK);
		isave->EndChunk();
		}

	isave->BeginChunk(BMTEX_IO_CHUNK);
	bi.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(BMTEX_VERSION_CHUNK);
	isave->EndChunk();

	return IO_OK;
	}	

IOResult BMTex::Load(ILoad *iload) { 
	ULONG nb;
	IOResult res;
	bmName.Resize(0);
	iload->RegisterPostLoadCallback(new ParamBlockPLCB(oldVersions, 3, &curVersion, this,1));
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
			case BMTEX_VERSOLD_CHUNK:
				loadingOld = TRUE;
				break;
			case BMTEX_FILTER_CHUNK:
				iload->Read(&filterType,sizeof(filterType),&nb);			
				break;
			case BMTEX_ALPHASOURCE_CHUNK:
				iload->Read(&alphaSource,sizeof(alphaSource),&nb);			
				break;
			case BMTEX_NAME_CHUNK:
				TCHAR *buf;
				if (IO_OK==iload->ReadWStringChunk(&buf)) 
					bmName = buf;					
				break;
			case BMTEX_IO_CHUNK:
				res = bi.Load(iload);
				// DS 7/18/96 - This is to handle old files 
				// that had the bi saved with start = end = 0, 
				// which now screws up animation.
				bi.SetStartFrame(BMM_UNDEF_FRAME);
				bi.SetEndFrame(BMM_UNDEF_FRAME);
				break;
			case BMTEX_START_CHUNK:
				iload->Read(&startTime,sizeof(startTime),&nb);			
				break;
			case BMTEX_RATE_CHUNK:
				iload->Read(&pbRate,sizeof(pbRate),&nb);			
				break;
			case BMTEX_ALPHA_MONO_CHUNK:
				alphaAsMono = TRUE;
				break;
			case BMTEX_ALPHA_RGB_CHUNK:
				alphaAsRGB = TRUE;
				break;
			case BMTEX_ALPHA_NOTPREMULT_CHUNK:
				premultAlpha = FALSE;
				break;
			case BMTEX_ENDCOND_CHUNK:
				iload->Read(&endCond,sizeof(endCond),&nb);			
				break;
			case BMTEX_CLIP_CHUNK:
				applyCrop = TRUE;
				break;
			case BMTEX_PLACE_IMAGE_CHUNK:
				placeImage = TRUE;
				break;
			case BMTEX_JITTER_CHUNK:
				randPlace = TRUE;
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	if (alphaSource<0||alphaSource>ALPHA_NONE) 
		alphaSource = ALPHA_FILE;
	return IO_OK;
	}

int BMTex::SetProperty(ULONG id, void *data)
	{
	switch (id) {
		case PROPID_CLEARCACHES:
			FreeBitmap();
			return 1;

		default: return 0;
		}
	}



