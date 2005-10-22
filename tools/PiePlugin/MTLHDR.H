/**********************************************************************
 *<
	FILE: mtlhdr.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MTLHDR__H
#define __MTLHDR__H

#ifdef BLD_MTL
#define MtlExport __declspec( dllexport )
#else
#define MtlExport __declspec( dllimport )
#endif

#include "max.h"
#include "imtl.h"
#include "texutil.h"

extern ClassDesc* GetStdMtlDesc();
extern ClassDesc* GetBMTexDesc();
extern ClassDesc* GetTexmapsDesc();
extern ClassDesc* GetOldTexmapsDesc();
extern ClassDesc* GetCMtlDesc();
extern ClassDesc* GetCheckerDesc();
extern ClassDesc* GetMixDesc();
extern ClassDesc* GetMarbleDesc();
extern ClassDesc* GetMaskDesc();
extern ClassDesc* GetTintDesc();
extern ClassDesc* GetNoiseDesc();
extern ClassDesc* GetMultiDesc();
extern ClassDesc* GetDoubleSidedDesc();
extern ClassDesc* GetMixMatDesc();
extern ClassDesc* GetACubicDesc();
extern ClassDesc* GetMirrorDesc();
extern ClassDesc* GetGradientDesc();
extern ClassDesc* GetCompositeDesc();
extern ClassDesc* GetMatteDesc();
extern ClassDesc* GetRGBMultDesc();
extern ClassDesc* GetOutputDesc();
extern ClassDesc* GetFalloffDesc();
extern ClassDesc* GetVColDesc();
extern ClassDesc* GetPartBlurDesc();
extern ClassDesc* GetPlateDesc();
extern ClassDesc* GetPartAgeDesc();

TCHAR *GetString(int id);

#endif
