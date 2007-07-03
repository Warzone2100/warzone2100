//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "typedefs.h"
#include "debugprint.h"

#include "pcxhandler.h"


// Round the given value up to the nearest power of 2.
//
DWORD Power2(DWORD Value)
{
	if(IsPower2(Value)) {
		return Value;
	}

	for(int i=31; i>=0; i--) {
		if(Value & 1<<i) {
			break;
		}
	}

	ASSERT(i!=31);	// Overflow.

	return 1<<(i+1);
}

// Return TRUE if the given value is a power of 2.
//
BOOL IsPower2(DWORD Value)
{
	int Bits = 0;

	while(Value) {
		if(Value & 1) {
			Bits++;
		}
		Value = Value >> 1;
	}

	if(Bits != 1) {
		return FALSE;
	}

	return TRUE;
}



PCXHandler::PCXHandler(void)
{
	m_Palette=NULL;
	m_DIBBitmap=NULL;
	m_BitmapInfo=NULL;
}

PCXHandler::~PCXHandler(void)
{
//	DebugPrint("Deleted PCXHandler\n");

	if(m_Palette!=NULL) {
		delete m_Palette;
	}
	if(m_DIBBitmap!=NULL) {
		DeleteObject(m_DIBBitmap);
	}
	if(m_BitmapInfo!=NULL) {
		delete m_BitmapInfo;
	}
}

BOOL PCXHandler::ReadPCX(char *FilePath,DWORD Flags)
{
	FILE	*fid;

	fid=fopen(FilePath,"rb");
    if(fid==NULL) {
//		MessageBox( NULL, FilePath, "Unable to open file.", MB_OK );
		return FALSE;
	}
	
// Read the PCX header.
	fread(&m_Header,sizeof(PCXHeader),1,fid);

	if(m_Header.Manufacturer != 10) {
		fclose(fid);
		MessageBox( NULL, FilePath, "File is not a valid PCX.", MB_OK );
		return FALSE;
	}

	if(m_Header.NPlanes != 1) {
		MessageBox( NULL, FilePath, "Unable to load PCX. Not 256 colour.", MB_OK );
		return FALSE;
	}

// Allocate memory for the bitmap.

	LONG Height = m_Header.Window[3]+1;
	if(Flags & BMR_ROUNDUP) {
		Height = Power2(Height);
	}

	BYTE *Bitmap=new BYTE[m_Header.NPlanes*m_Header.BytesPerLine*Height];
	LONG Size=m_Header.NPlanes*m_Header.BytesPerLine*(m_Header.Window[3]+1);

	if(Bitmap==NULL) {
		fclose(fid);
		MessageBox( NULL, FilePath, "Could not allocate memory.", MB_OK );
		return FALSE;
	}

// Decode the bitmap.
	WORD	chr,cnt,i;
	LONG	decoded=0;
	BYTE	*bufr=Bitmap;

	while( (!EncodedGet(&chr, &cnt, fid)) && (decoded < Size) ) {
		for(i=0; i<cnt; i++) {
			*bufr++ = (UBYTE)chr;
			decoded++;
		}
	}

	WORD PaletteSize=0;

// If there's a palette on the end then read it.

//	DebugPrint("PCX Version %d\n",m_Header.Version);

	switch(m_Header.Version) {
		case	5:
			fseek(fid,-769,SEEK_END);
			if(getc(fid)==12) {
				PaletteSize=1<<m_Header.BitsPerPixel;
				m_Palette=new PALETTEENTRY[PaletteSize];

				for (i=0; i<PaletteSize; i++)
				{
					m_Palette[i].peRed   = (BYTE)getc(fid);
					m_Palette[i].peGreen = (BYTE)getc(fid);
					m_Palette[i].peBlue  = (BYTE)getc(fid);
					m_Palette[i].peFlags = (BYTE)0;
				}
			}
			break;
		default:
			fclose(fid);
			MessageBox( NULL, FilePath, "Version not supported.", MB_OK );
			return FALSE;
	}

	fclose(fid);
	
	m_BitmapInfo=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)+sizeof(RGBQUAD)*PaletteSize];
	m_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	m_BitmapInfo->bmiHeader.biWidth=m_Header.Window[2]+1;
	m_BitmapInfo->bmiHeader.biHeight=-Height;	//(m_Header.Window[3]+1);
	m_BitmapInfo->bmiHeader.biPlanes=1;
	m_BitmapInfo->bmiHeader.biBitCount=m_Header.BitsPerPixel*m_Header.NPlanes;
	m_BitmapInfo->bmiHeader.biCompression=BI_RGB;
	m_BitmapInfo->bmiHeader.biSizeImage=0;
	m_BitmapInfo->bmiHeader.biXPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biYPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biClrUsed=0;
	m_BitmapInfo->bmiHeader.biClrImportant=0;

	for(i=0; i<PaletteSize; i++) {
		m_BitmapInfo->bmiColors[i].rgbRed   = m_Palette[i].peRed;
		m_BitmapInfo->bmiColors[i].rgbGreen = m_Palette[i].peGreen;
		m_BitmapInfo->bmiColors[i].rgbBlue  = m_Palette[i].peBlue;
		m_BitmapInfo->bmiColors[i].rgbReserved = 0;
	}

	m_DIBBitmap=CreateDIBSection(NULL,m_BitmapInfo,DIB_RGB_COLORS,&m_DIBBits,NULL,0);

	if(m_DIBBitmap==NULL) {
		MessageBox( NULL, FilePath, "Failed to create DIB.", MB_OK );
		return FALSE;
	}

	BYTE	*Dst=(BYTE*)m_DIBBits;
	BYTE	*Src;
	WORD	j,p;

	for(j=0; j < Height; j++) {
		Src=Bitmap+j*m_Header.BytesPerLine*m_Header.NPlanes;
		for(i=0; i < m_Header.BytesPerLine; i++) {
			for(p=0; p < m_Header.NPlanes; p++) {
				*Dst=*(Src+m_Header.BytesPerLine*p);
				Dst++;
			}	
			Src++;
		}
	}

	delete Bitmap;

	return TRUE;
}


BOOL PCXHandler::Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette)
{
	m_Header.Manufacturer = 10;
	m_Header.Version = 5;
	m_Header.Encoding = 1;
	m_Header.BitsPerPixel = 8;
	m_Header.Window[0] = 0;
	m_Header.Window[1] = 0;
	m_Header.Window[2] = Width-1;
	m_Header.Window[3] = Height-1;
	m_Header.HRes = 150;
	m_Header.VRes = 150;
	memset(m_Header.Colormap,0,48*sizeof(BYTE));
	m_Header.Reserved = 0;
	m_Header.NPlanes = 1;
	m_Header.BytesPerLine = Width;
	m_Header.PaletteInfo = 1;
	memset(m_Header.Filler,0,58*sizeof(BYTE));

	int PaletteSize=1<<m_Header.BitsPerPixel;
	m_Palette = new PALETTEENTRY[PaletteSize];

	for (int i=0; i<PaletteSize; i++) {
		m_Palette[i] = Palette[i];
	}

	m_BitmapInfo=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)+sizeof(RGBQUAD)*PaletteSize];
	m_BitmapInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	m_BitmapInfo->bmiHeader.biWidth=m_Header.Window[2]+1;
	m_BitmapInfo->bmiHeader.biHeight=-Height;
	m_BitmapInfo->bmiHeader.biPlanes=1;
	m_BitmapInfo->bmiHeader.biBitCount=m_Header.BitsPerPixel*m_Header.NPlanes;
	m_BitmapInfo->bmiHeader.biCompression=BI_RGB;
	m_BitmapInfo->bmiHeader.biSizeImage=0;
	m_BitmapInfo->bmiHeader.biXPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biYPelsPerMeter=0;
	m_BitmapInfo->bmiHeader.biClrUsed=0;
	m_BitmapInfo->bmiHeader.biClrImportant=0;

	for(i=0; i<PaletteSize; i++) {
		m_BitmapInfo->bmiColors[i].rgbRed   = m_Palette[i].peRed;
		m_BitmapInfo->bmiColors[i].rgbGreen = m_Palette[i].peGreen;
		m_BitmapInfo->bmiColors[i].rgbBlue  = m_Palette[i].peBlue;
		m_BitmapInfo->bmiColors[i].rgbReserved = 0;
	}

	m_DIBBitmap=CreateDIBSection(NULL,m_BitmapInfo,DIB_RGB_COLORS,&m_DIBBits,NULL,0);

	if(m_DIBBitmap==NULL) {
		MessageBox( NULL, "Error", "Failed to create DIB.", MB_OK );
		return FALSE;
	}

	BYTE	*Dst=(BYTE*)m_DIBBits;
	BYTE	*Src;
	WORD	j,p;

	for(j=0; j < Height; j++) {
		Src=((BYTE*)Bits)+j*m_Header.BytesPerLine*m_Header.NPlanes;
		for(i=0; i < m_Header.BytesPerLine; i++) {
			for(p=0; p < m_Header.NPlanes; p++) {
				*Dst=*(Src+m_Header.BytesPerLine*p);
				Dst++;
			}	
			Src++;
		}
	}

	return TRUE;
}


BOOL PCXHandler::WritePCX(char *FilePath)
{
	FILE *fid;

	fid=fopen(FilePath,"wb");
    if(fid==NULL) {
		MessageBox( NULL, FilePath, "Unable to create file.", MB_OK );
		return FALSE;
	}

// Write the PCX header.
	fwrite(&m_Header,sizeof(PCXHeader),1,fid);

// Encode and write the body.
	BYTE *Ptr = (BYTE*)m_DIBBits;
	for(int i=0; i<GetBitmapHeight(); i++) {
		EncodeLine(Ptr,(WORD)GetBitmapWidth(),fid);
		Ptr += GetBitmapWidth();
	}

// Write the palette.
	putc(12,fid);
	int PaletteSize=1<<m_Header.BitsPerPixel;
	for (i=0; i<PaletteSize; i++)
	{
		putc(m_Palette[i].peRed,fid);
		putc(m_Palette[i].peGreen,fid);
		putc(m_Palette[i].peBlue,fid);
	}
	
	fclose(fid);

	return TRUE;
}


/* This procedure reads one encoded block from the image file and
stores a count and data byte. Result:
    0 = valid data stored
    EOF = out of data in file
int *pbyt;     where to place data
int *pcnt;     where to place count
FILE *fid;     image file handle
*/
WORD PCXHandler::EncodedGet(WORD *pbyt, WORD *pcnt, FILE *fid)
{
	WORD i;

  *pcnt = 1;     /* safety play */

  i=getc(fid);
  if(feof(fid)) {
	  return(1);
  }

  if(0xc0 == (0xc0 & i)) {
    *pcnt = 0x3f&i;
	  i=getc(fid);
	  if(feof(fid)) {
		  return(1);
	  }
  }
  *pbyt = i;

  return(0);
}


/* This subroutine encodes one scanline and writes it to a file
unsigned char *inBuff;  pointer to scanline data
int inLen;              length of raw scanline in bytes
FILE *fp;               file to be written to
*/
WORD PCXHandler::EncodeLine(UBYTE *inBuff, WORD inLen, FILE *fid)
{  /* returns number of bytes written into outBuff, 0 if failed */

  UBYTE thisone, last;
  WORD srcIndex, i;

  register WORD total;
  register UBYTE runCount; /* max single runlength is 63 */

  total = 0;
  last = *(inBuff);
  runCount = 1;

  for (srcIndex = 1; srcIndex < inLen; srcIndex++)
  {
    thisone = *(++inBuff);
    if (thisone == last)
    {
      runCount++;  /* it encodes */
      if (runCount == 63)
      {
        if (!(i=EncodedPut(last, runCount, fid)))
          return(0);
        total += i;
        runCount = 0;
      }
    }
    else
    {  /* thisone != last */
      if (runCount)
      {
        if (!(i=EncodedPut(last, runCount, fid)))
          return(0);
        total += i;
      }
      last = thisone;
      runCount = 1;
    }
  } /* endloop */

  if (runCount)
  {  /* finish up */
    if (!(i=EncodedPut(last, runCount, fid)))
      return(0);
    return(total + i);
  }
  return(total);
}



/* subroutine for writing an encoded byte pair 
(or single byte  if it doesn't encode) to a file
unsigned char byt, cnt;
FILE *fid;
*/
WORD PCXHandler::EncodedPut(UBYTE byt, UBYTE cnt, FILE *fid) /* returns count of bytes written, 0 if err */
{
  if(cnt)
  {
    if((cnt==1) && (0xc0 != (0xc0&byt)))
    {
      if(EOF == putc((int)byt, fid))
        return(0); /* disk write error (probably full) */
      return(1);
    }
    else
   {
     if(EOF == putc((int)0xC0 | cnt, fid))
       return(0);  /* disk write error */
     if(EOF == putc((int)byt, fid))
       return(0);  /* disk write error */
     return(2);
    }
  }
  return(0);
}
