#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "typedefs.h"
#include "DebugPrint.h"

#include "KeyHandler.h"

CKeyHandler::CKeyHandler(void)
{
	InitKeyTable();
}

void CKeyHandler::InitKeyTable(void)
{
	for(int i = 0; i<KEYTABSIZE; i++) {
		m_KeyTable[i] = 0;
	}
}

void CKeyHandler::HandleKeyDown(UINT VKey)
{
	if(VKey < KEYTABSIZE) {
//		DebugPrint("KeyDown %d\n",VKey);
		m_KeyTable[VKey] = 1;
	}
}

void CKeyHandler::HandleKeyUp(UINT VKey)
{
	if(VKey < KEYTABSIZE) {
		m_KeyTable[VKey] = 0;
//		DebugPrint("KeyUp %d\n",VKey);
	}
}

