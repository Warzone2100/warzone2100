#ifndef __DIBDRAW_INCLUDED__
#define __DIBDRAW_INCLUDED__

#define MAGIC 29121967	// For debuging.


class CDIBDraw {
public:
	CDIBDraw(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555=FALSE);
	CDIBDraw(void *hWnd,DWORD Width,DWORD Height,BOOL Is555=FALSE);
	CDIBDraw(void *hWnd,BOOL Is555=FALSE);
	~CDIBDraw(void);
	void CreateDIB(void *hWnd,DWORD Width,DWORD Height,DWORD Planes,DWORD BitsPerPixel,BOOL Is555=FALSE);
	void ClearDIB(void);
	void SwitchBuffers(void);
	void SwitchBuffers(void *hdc);
	void SwitchBuffers(int SrcOffX,int SrcOffY);
	void *GetDIBBits(void);
	DWORD GetDIBWidth(void);
	DWORD GetDIBHeight(void);
	void *GetDIBDC(void);
	void *GetDIBWindow(void);
	DWORD GetDIBValue(DWORD r,DWORD g,DWORD b);
	void PutDIBPixel(DWORD Value,DWORD x,DWORD y);
	void PutDIBFatPixel(DWORD Value,DWORD x,DWORD y);
protected:
	BOOL m_Is555;
	DWORD m_Valid;
	HWND m_hWnd;
	BITMAPINFO *m_BitmapInfo;
	void *m_DIBBits;
	HBITMAP m_hDIB;
	HDC m_hdcDIB;
	DWORD m_Width;
	DWORD m_Height;
};

#endif