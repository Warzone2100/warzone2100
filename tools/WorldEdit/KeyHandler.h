
#ifndef __INCLUDED_KEYHANDLER__
#define __INCLUDED_KEYHANDLER__

#define KEYTABSIZE	256

class CKeyHandler {
public:
	CKeyHandler(void);
	void InitKeyTable(void);
	void HandleKeyDown(UINT VKey);
	void HandleKeyUp(UINT VKey);
	BOOL GetKeyState(UINT VKey) { return m_KeyTable[VKey]; }
protected:
	BOOL m_KeyTable[KEYTABSIZE];
};

#endif
