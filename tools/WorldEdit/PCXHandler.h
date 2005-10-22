#ifndef __PSXHANDLER_INCLUDED__
#define	__PSXHANDLER_INCLUDED__

BOOL IsPower2(DWORD Value);
DWORD Power2(DWORD Value);

struct PCXHeader {
	BYTE	Manufacturer;   // 1   Constant Flag  10 = ZSoft .PCX
	BYTE	Version;        // 1   Version information:
	                        // 0 = Version 2.5
	                        // 2 = Version 2.8 w/palette information
	                        // 3 = Version 2.8 w/o palette information
	                        // 5 = Version 3.0
	BYTE	Encoding;       // 1   1 = .PCX run length encoding
	BYTE	BitsPerPixel;	// 1   Number of bits/pixel per plane
	WORD	Window[4];      // 8   Picture Dimensions 
	                        // (Xmin, Ymin) - (Xmax - Ymax)
	                        // in pixels, inclusive
	WORD	HRes;           // 2   Horizontal Resolution of creating device
	WORD	VRes;           // 2   Vertical Resolution of creating device
	BYTE	Colormap[48];   // 48  Color palette setting, see text
	BYTE	Reserved;       // 1
	BYTE	NPlanes;        // 1   Number of color planes
	WORD	BytesPerLine;	// 2   Number of bytes per scan line per 
	                        // color plane (always even for .PCX files)
	WORD	PaletteInfo;	// 2   How to interpret palette - 1 = color/BW,
	                        // 2 = grayscale
	BYTE	Filler[58];     // 58  blank to fill out 128 byte header
};


#define BMR_ROUNDUP	1

class PCXHandler {
public:
	PCXHandler(void);
	~PCXHandler(void);
	BOOL Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette);
	BOOL ReadPCX(char *FilePath,DWORD Flags = 0);
	BOOL WritePCX(char *FilePath);
	LONG GetBitmapWidth(void) { return(m_BitmapInfo->bmiHeader.biWidth); }
	LONG GetBitmapHeight(void) { return(abs(m_BitmapInfo->bmiHeader.biHeight)); }
	WORD GetBitmapBitCount(void) { return(m_BitmapInfo->bmiHeader.biBitCount); }
	void *GetBitmapBits(void) { return(m_DIBBits); }
	HBITMAP GetBitmap(void) { return(m_DIBBitmap); }
	PALETTEENTRY *GetBitmapPaletteEntries(void) { return(m_Palette); }
protected:
	PCXHeader	m_Header;
	BITMAPINFO* m_BitmapInfo;
	HBITMAP	m_DIBBitmap;
	void *m_DIBBits;
 	PALETTEENTRY *m_Palette;
	WORD EncodedGet(WORD *pbyt, WORD *pcnt, FILE *fid);
	WORD EncodeLine(UBYTE *inBuff, WORD inLen, FILE *fp);
	WORD EncodedPut(UBYTE byt, UBYTE cnt, FILE *fid);
};

#endif
