#include "ddraw_hook.h"

std::map<LPDIRECTDRAWSURFACE, EmulatedSurfaceInfo*> EmulatedSurfaces;
std::map<LPDIRECTDRAWPALETTE, EmulatedPaletteInfo*> EmulatedPalettes;

HWND hWnd = nullptr;
DWORD oWidth = 0;
DWORD oHeight = 0;
bool fResize = false;

WNDPROC oWndProc = nullptr;
int lMouseX;
int lMouseY;

/*
int greatestCommonDenominator(int a, int b)
{
	while (b != 0)
	{
		int t = b;
		b = a % b;
		a = t;
	}

	return a;
}

void getAspect(int width, int height, int& ratioX, int& ratioY)
{
	int gcd = greatestCommonDenominator(width, height);
	ratioX = width / gcd;
	ratioY = height / gcd;
}
*/

void DoClipCursor(int w, int h)
{
	SetCursorPos(lMouseX, lMouseY);

	RECT rect;
	rect.left = 0;
	rect.right = w;
	rect.top = 0;
	rect.bottom = h;

	ClipCursor(&rect);
}

void DoUnclipCursor()
{
	POINT pt;
	GetCursorPos(&pt);
	lMouseX = pt.x;
	lMouseY = pt.y;

	ClipCursor(NULL);
	ShowWindow(hWnd, SW_MINIMIZE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if(Msg == WM_ACTIVATEAPP)
	{
		if (wParam == TRUE)
		{
			DoClipCursor(oWidth, oHeight);
		}

		if (wParam == FALSE)
		{
			DoUnclipCursor();
		}
	}

	if (Msg == WM_ACTIVATE)
	{
		if (wParam == WA_ACTIVE)
		{
			DoClipCursor(oWidth, oHeight);
		}

		if (wParam == WA_INACTIVE)
		{
			DoUnclipCursor();
		}
	}

	if (Msg == WM_SETFOCUS)
	{
		DoClipCursor(oWidth, oHeight);
	}

	if (Msg == WM_KILLFOCUS)
	{
		DoUnclipCursor();
	}

	return oWndProc(hWnd, Msg, wParam, lParam);
}

HRESULT __stdcall DDRAW_SetCooperativeLevel(IDirectDraw* This, HWND arg1, DWORD Flags)
{
	if (oWndProc == nullptr)
	{
		oWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG)WndProc);
	}

	hWnd = arg1;

	return DD_OK;
}

HRESULT __stdcall DDRAW_SetDisplayMode(IDirectDraw* This, DWORD arg1, DWORD arg2, DWORD arg3)
{
	oWidth = arg1;
	oHeight = arg2;

	POINT pt;
	GetCursorPos(&pt);
	lMouseX = pt.x;
	lMouseY = pt.y;

	DoClipCursor(oWidth, oHeight);

	MoveWindow(hWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), false);// | SWP_FRAMECHANGED);

	return DD_OK;
}

HRESULT __stdcall DDRAW_CreateSurface(IDirectDraw* This, LPDDSURFACEDESC desc, LPDIRECTDRAWSURFACE* lpSurf, IUnknown* unk)
{
	EmulatedSurfaceInfo* info = new EmulatedSurfaceInfo;
	info->surface = (uint8_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, desc->dwWidth * desc->dwHeight);
	info->converted_surface = (uint32_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, desc->dwWidth * desc->dwHeight * 4);
	info->width = desc->dwWidth;
	info->height = desc->dwHeight;
	info->pitch = desc->dwWidth;
	
	auto NewObj = (LPDIRECTDRAWSURFACE*) new long;
	*NewObj = new IDirectDrawSurface;
	(*NewObj)->lpVtbl = new IDirectDrawSurfaceVtbl;
	memset((*NewObj)->lpVtbl, 0, sizeof(IDirectDrawSurfaceVtbl));
	DDRAW_DirectDrawSurface_vTable_Hooks(NewObj);

	*lpSurf = *NewObj;

	EmulatedSurfaces[*NewObj] = info;

	return DD_OK;
}

HRESULT __stdcall DDRAW_CreateClipper(IDirectDraw* This, DWORD arg1, LPDIRECTDRAWCLIPPER* arg2, IUnknown* arg3)
{
	auto NewObj = (LPDIRECTDRAWCLIPPER*) new long;
	*NewObj = new IDirectDrawClipper;
	(*NewObj)->lpVtbl = new IDirectDrawClipperVtbl;
	memset((*NewObj)->lpVtbl, 0, sizeof(IDirectDrawClipperVtbl));
	DDRAW_DirectDrawClipper_vTable_Hooks(NewObj);

	*arg2 = *NewObj;

	return DD_OK;
}

HRESULT __stdcall DDRAW_CreatePalette(IDirectDraw* This, DWORD arg1, LPPALETTEENTRY arg2, LPDIRECTDRAWPALETTE* arg3, IUnknown* arg4)
{
	EmulatedPaletteInfo* info = new EmulatedPaletteInfo;
	info->palette = (uint32_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 256 * 4);

	auto NewObj = (LPDIRECTDRAWPALETTE*) new long;
	*NewObj = new IDirectDrawPalette;
	(*NewObj)->lpVtbl = new IDirectDrawPaletteVtbl;
	memset((*NewObj)->lpVtbl, 0, sizeof(IDirectDrawPaletteVtbl));
	DDRAW_DirectDrawPalette_vTable_Hooks(NewObj);

	*arg3 = *NewObj;

	EmulatedPalettes[*NewObj] = info;

	return DD_OK;
}

HRESULT __stdcall DDRAW_QueryInterface(IDirectDraw* This, const IID& riid, LPVOID* ppvObj)
{
	*ppvObj = This;

	return DD_OK;
}

HRESULT __stdcall DDRAW_EnumDisplayModes(IDirectDraw* This, DWORD arg1, LPDDSURFACEDESC description, LPVOID context, LPDDENUMMODESCALLBACK func)
{
	//int ratioX;
	//int ratioY;

	if (description == nullptr)
	{
		description = new DDSURFACEDESC;

		//getAspect(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), ratioX, ratioY);

		/*
		int MaxModes = 98;
		for (int MUL = ratioY; (640 + ratioX * MUL < GetSystemMetrics(SM_CXSCREEN)) && (480 + ratioY * MUL < GetSystemMetrics(SM_CYSCREEN)) && MaxModes > 0; MUL += ratioY)
		{
			description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
			description->dwWidth = 640 + ratioX * MUL;
			description->dwHeight = 480 + ratioY * MUL;
			description->dwMipMapCount = 0;

			func(description, context);
			MaxModes--;
		}
		*/

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 640;
		description->dwHeight = 480;
		description->dwMipMapCount = 0;
		func(description, context);

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 800;
		description->dwHeight = 600;
		description->dwMipMapCount = 0;
		func(description, context);

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 1024;
		description->dwHeight = 768;
		description->dwMipMapCount = 0;
		func(description, context);

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 1280;
		description->dwHeight = 1024;
		description->dwMipMapCount = 0;
		func(description, context);

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 1600;
		description->dwHeight = 1200;
		description->dwMipMapCount = 0;
		func(description, context);

		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwWidth = 1920;
		description->dwHeight = 1080;
		description->dwMipMapCount = 0;
		func(description, context);

		description->dwWidth = GetSystemMetrics(SM_CXSCREEN);
		description->dwHeight = GetSystemMetrics(SM_CYSCREEN);
		description->ddpfPixelFormat.dwRGBBitCount = 8; // Fake
		description->dwMipMapCount = 0;

		func(description, context);

		free(description);
	}

	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_GetAttachedSurface(IDirectDrawSurface* This, LPDDSCAPS arg1, LPDIRECTDRAWSURFACE* arg2)
{
	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_SetClipper(IDirectDrawSurface* This, LPDIRECTDRAWCLIPPER arg1)
{
	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_SetPalette(IDirectDrawSurface* This, LPDIRECTDRAWPALETTE arg1)
{
	if (EmulatedPalettes.find((LPDIRECTDRAWPALETTE)arg1) == EmulatedPalettes.end())
	{
		return DD_FALSE;
	}
	else
	{
		if (EmulatedSurfaces.find((LPDIRECTDRAWSURFACE)This) == EmulatedSurfaces.end())
		{
			return DD_FALSE;
		}
		else
		{
			auto info = EmulatedSurfaces.at((LPDIRECTDRAWSURFACE)This);
			auto info2 = EmulatedPalettes.at((LPDIRECTDRAWPALETTE)arg1);
			info->palette = info2->palette;

			return DD_OK;
		}
	}

	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_Lock(IDirectDrawSurface* This, LPRECT arg1, LPDDSURFACEDESC arg2, DWORD arg3, HANDLE arg4)
{
	EmulatedSurfaceInfo* getInfo;

	if (EmulatedSurfaces.find((LPDIRECTDRAWSURFACE)This) == EmulatedSurfaces.end())
	{
		return DD_FALSE;
	}
	else
	{
		getInfo = EmulatedSurfaces[(LPDIRECTDRAWSURFACE)This];
		arg2->dwSize = sizeof(DDSURFACEDESC);
		arg2->lpSurface = getInfo->surface;
		arg2->dwWidth = getInfo->width;
		arg2->dwHeight = getInfo->height;
		arg2->lPitch = getInfo->pitch;

		return DD_OK;
	}
}

HRESULT __stdcall DDRAW_Surface_Unlock(IDirectDrawSurface* This, LPVOID arg1)
{
	EmulatedSurfaceInfo* getInfo;
	getInfo = EmulatedSurfaces[(LPDIRECTDRAWSURFACE)This];
	uint8_t* surf = (uint8_t*)getInfo->surface;
	uint32_t* conv_surf = (uint32_t*)getInfo->converted_surface;
	auto height = getInfo->height;
	auto width = getInfo->width;
	auto pitch = getInfo->pitch;
	uint32_t* palette = getInfo->palette;

	for (int y = 0; y < (int)height; y++)
	{
		for (int x = 0; x < (int)width; x++)
		{
			uint8_t index = surf[y * pitch + x];
			conv_surf[y * width + x] = palette[index];
		}
	}

	BITMAPINFO* bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = width;
	bmi->bmiHeader.biHeight = -(int)height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 32;
	bmi->bmiHeader.biCompression = BI_BITFIELDS;
	DWORD* masks = (DWORD*)((BYTE*)bmi + sizeof(BITMAPINFOHEADER));
	masks[0] = 0x00FF0000;
	masks[1] = 0x0000FF00;
	masks[2] = 0x000000FF;

	HDC dc = GetDC(hWnd);
	StretchDIBits(dc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, oWidth, oHeight, conv_surf, bmi, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(hWnd, dc);

	free(bmi);

	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_IsLost(IDirectDrawSurface* This)
{
	return DD_OK;
}

HRESULT __stdcall DDRAW_Surface_Blt(IDirectDrawSurface* This, LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDBLTFX arg5)
{
	return DD_OK;
}

HRESULT __stdcall DDRAW_Clipper_SetHwnd(IDirectDrawClipper* This, DWORD arg1, HWND arg2)
{
	hWnd = arg2;

	return DD_OK;
}

HRESULT __stdcall DDRAW_Palette_SetEntries(IDirectDrawPalette* This, DWORD arg1, DWORD arg2, DWORD arg3, LPPALETTEENTRY arg4)
{
	if (EmulatedPalettes.find((LPDIRECTDRAWPALETTE)This) == EmulatedPalettes.end())
	{
		return DD_FALSE;
	}
	else
	{
		auto info = EmulatedPalettes.at((LPDIRECTDRAWPALETTE)This);

		for (int i = 0; i < 256; i++)
		{
			uint8_t r, g, b;
			r = arg4[i].peRed;
			g = arg4[i].peGreen;
			b = arg4[i].peBlue;

			uint32_t conv_color = (b << 0) | (g << 8) | (r << 16);
			info->palette[i] = conv_color;
		}
	}

	return DD_OK;
}

ULONG __stdcall DDRAW_Palette_Release(IDirectDrawPalette* This)
{
	auto pal = EmulatedPalettes[This];

	if (pal != nullptr)
	{
		if (pal->palette != nullptr)
		{
			HeapFree(GetProcessHeap(), 0, pal->palette);
		}
	}
	
	return DD_OK;
}

ULONG __stdcall DDRAW_Surface_Release(IDirectDrawSurface* This)
{
	auto surf = EmulatedSurfaces[This];

	if (surf != nullptr)
	{
		if (surf->surface != nullptr)
		{
			HeapFree(GetProcessHeap(), 0, surf->surface);
		}

		if (surf->converted_surface != nullptr)
		{
			HeapFree(GetProcessHeap(), 0, surf->converted_surface);
		}
	}

	return DD_OK;
}

ULONG __stdcall DDRAW_Clipper_Release(IDirectDrawClipper* This)
{
	return DD_OK;
}

ULONG __stdcall DDRAW_Release(IDirectDraw* This)
{
	return DD_OK;
}

void DDRAW_DirectDraw_vTable_Hooks(LPDIRECTDRAW* lplpDD)
{
	typedef HRESULT(__stdcall* DDRAW_SCL)(IDirectDraw* This, HWND hWnd, DWORD Flags);
	DDRAW_SCL ddraw_scl = DDRAW_SetCooperativeLevel;
	(*lplpDD)->lpVtbl->SetCooperativeLevel = ddraw_scl;

	typedef HRESULT(__stdcall* DDRAW_SDM)(IDirectDraw* This, DWORD, DWORD, DWORD);
	DDRAW_SDM ddraw_sdm = DDRAW_SetDisplayMode;
	(*lplpDD)->lpVtbl->SetDisplayMode = ddraw_sdm;

	typedef HRESULT(__stdcall* DDRAW_CS)(IDirectDraw* This, LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, IUnknown*);
	DDRAW_CS ddraw_cs = DDRAW_CreateSurface;
	(*lplpDD)->lpVtbl->CreateSurface = ddraw_cs;

	typedef HRESULT(__stdcall* DDRAW_CC)(IDirectDraw* This, DWORD arg1, LPDIRECTDRAWCLIPPER* arg2, IUnknown* arg3);
	DDRAW_CC ddraw_cc = DDRAW_CreateClipper;
	(*lplpDD)->lpVtbl->CreateClipper = ddraw_cc;

	typedef HRESULT(__stdcall* DDRAW_CP)(IDirectDraw* This, DWORD arg1, LPPALETTEENTRY arg2, LPDIRECTDRAWPALETTE* arg3, IUnknown* arg4);
	DDRAW_CP ddraw_cp = DDRAW_CreatePalette;
	(*lplpDD)->lpVtbl->CreatePalette = ddraw_cp;

	typedef ULONG(__stdcall* DDRAW_R)(IDirectDraw* This);
	DDRAW_R ddraw_r = DDRAW_Release;
	(*lplpDD)->lpVtbl->Release = ddraw_r;

	typedef HRESULT(__stdcall* DDRAW_QI)(IDirectDraw* This, const IID &riid, LPVOID* ppvObj);
	DDRAW_QI ddraw_qi = DDRAW_QueryInterface;
	(*lplpDD)->lpVtbl->QueryInterface = ddraw_qi;

	typedef HRESULT(__stdcall* DDRAW_EDM)(IDirectDraw* This, DWORD arg1, LPDDSURFACEDESC arg2, LPVOID arg3, LPDDENUMMODESCALLBACK arg4);
	DDRAW_EDM ddraw_edm = DDRAW_EnumDisplayModes;
	(*lplpDD)->lpVtbl->EnumDisplayModes = ddraw_edm;
}

void DDRAW_DirectDrawSurface_vTable_Hooks(LPDIRECTDRAWSURFACE* lplpDDSurf)
{
	typedef HRESULT(__stdcall* DDRAW_S_GAS)(IDirectDrawSurface* This, LPDDSCAPS arg1, LPDIRECTDRAWSURFACE* arg2);
	DDRAW_S_GAS ddraw_s_gas = DDRAW_Surface_GetAttachedSurface;
	(*lplpDDSurf)->lpVtbl->GetAttachedSurface = ddraw_s_gas;

	typedef HRESULT(__stdcall* DDRAW_S_SC)(IDirectDrawSurface* This, LPDIRECTDRAWCLIPPER arg1);
	DDRAW_S_SC ddraw_s_sc = DDRAW_Surface_SetClipper;
	(*lplpDDSurf)->lpVtbl->SetClipper = ddraw_s_sc;

	typedef HRESULT(__stdcall* DDRAW_S_SP)(IDirectDrawSurface* This, LPDIRECTDRAWPALETTE arg1);
	DDRAW_S_SP ddraw_s_sp = DDRAW_Surface_SetPalette;
	(*lplpDDSurf)->lpVtbl->SetPalette = ddraw_s_sp;

	typedef HRESULT(__stdcall* DDRAW_S_L)(IDirectDrawSurface* This, LPRECT arg1, LPDDSURFACEDESC arg2, DWORD arg3, HANDLE arg4);
	DDRAW_S_L ddraw_s_l = DDRAW_Surface_Lock;
	(*lplpDDSurf)->lpVtbl->Lock = ddraw_s_l;

	typedef HRESULT(__stdcall* DDRAW_S_U)(IDirectDrawSurface* This, LPVOID arg1);
	DDRAW_S_U ddraw_s_u = DDRAW_Surface_Unlock;
	(*lplpDDSurf)->lpVtbl->Unlock = ddraw_s_u;

	typedef HRESULT(__stdcall* DDRAW_S_IL)(IDirectDrawSurface* This);
	DDRAW_S_IL ddraw_s_il = DDRAW_Surface_IsLost;
	(*lplpDDSurf)->lpVtbl->IsLost = ddraw_s_il;
	
	typedef ULONG(__stdcall* DDRAW_S_R)(IDirectDrawSurface* This);
	DDRAW_S_R ddraw_s_r = DDRAW_Surface_Release;
	(*lplpDDSurf)->lpVtbl->Release = ddraw_s_r;

	typedef HRESULT(__stdcall* DDRAW_S_B)(IDirectDrawSurface* This, LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDBLTFX arg5);
	DDRAW_S_B ddraw_s_b = DDRAW_Surface_Blt;
	(*lplpDDSurf)->lpVtbl->Blt = ddraw_s_b;
}

void DDRAW_DirectDrawClipper_vTable_Hooks(LPDIRECTDRAWCLIPPER* lplpDDClip)
{
	typedef HRESULT(__stdcall* DDRAW_C_SH)(IDirectDrawClipper* This, DWORD arg1, HWND arg2);
	DDRAW_C_SH ddraw_s_sh = DDRAW_Clipper_SetHwnd;
	(*lplpDDClip)->lpVtbl->SetHWnd = ddraw_s_sh;

	typedef ULONG(__stdcall* DDRAW_C_R)(IDirectDrawClipper* This);
	DDRAW_C_R ddraw_c_r = DDRAW_Clipper_Release;
	(*lplpDDClip)->lpVtbl->Release = ddraw_c_r;
}

void DDRAW_DirectDrawPalette_vTable_Hooks(LPDIRECTDRAWPALETTE* lplpDDPal)
{
	typedef HRESULT(__stdcall* DDRAW_P_SE)(IDirectDrawPalette* This, DWORD arg1, DWORD arg2, DWORD arg3, LPPALETTEENTRY arg4);
	DDRAW_P_SE ddraw_p_se = DDRAW_Palette_SetEntries;
	(*lplpDDPal)->lpVtbl->SetEntries = ddraw_p_se;

	typedef ULONG(__stdcall* DDRAW_X_R)(IDirectDrawPalette* This);
	DDRAW_X_R ddraw_x_r = DDRAW_Palette_Release;
	(*lplpDDPal)->lpVtbl->Release = ddraw_x_r;
}
