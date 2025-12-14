#define CINTERFACE
#include "pch.h"

BOOL DDRAW_Loaded = false;
HMODULE DDRAW = 0;
FARPROC DDRAW_DirectDrawCreate = 0;

HRESULT WINAPI DirectDrawCreate(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
{
	auto NewObj = (LPDIRECTDRAW*) new long;
	*NewObj = new IDirectDraw;
	(*NewObj)->lpVtbl = new IDirectDrawVtbl;
	memset((*NewObj)->lpVtbl, 0, sizeof(IDirectDrawVtbl));
	DDRAW_DirectDraw_vTable_Hooks(NewObj);

	*lplpDD = *NewObj;

	return DD_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{

	}

	return TRUE;
}