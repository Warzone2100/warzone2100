#ifndef __BMPHANDLER_INCLUDED__
#define	__BMPHANDLER_INCLUDED__

class BMPHandler {
public:
	BMPHandler(void);
	~BMPHandler(void);
	BOOL ReadBMP(char *FilePath,BOOL Flip=FALSE);
	BOOL Create(int Width,int Height,void *Bits,PALETTEENTRY *Palette,int BPP=8,BOOL Is555 = FALSE);
	void Clear(void);
	void DeleteDC(void *hdc);
	void *CreateDC(void *hWnd);
	BOOL WriteBMP(char *FilePath,BOOL Flip=FALSE);
	LONG GetBitmapWidth(void) { return(m_BitmapInfo->bmiHeader.biWidth); }
	LONG GetBitmapHeight(void) { return(abs(m_BitmapInfo->bmiHeader.biHeight)); }
	WORD GetBitmapBitCount(void) { return(m_BitmapInfo->bmiHeader.biBitCount); }
	void *GetBitmapBits(void) { return(m_DIBBits); }
	HBITMAP GetBitmap(void) { return(m_DIBBitmap); }
	PALETTEENTRY *GetBitmapPaletteEntries(void) { return(m_Palette); }
protected:
	BITMAPINFO* m_BitmapInfo;
	HBITMAP	m_DIBBitmap;
	void *m_DIBBits;
 	PALETTEENTRY *m_Palette;
};

#endif