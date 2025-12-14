

#pragma once

#define CINTERFACE
#include "pch.h"



//#define _NO_COM
//#include "ddraw.h"



struct EmulatedSurfaceInfo
{
	DWORD width;
	DWORD height;
	DWORD pitch;
	void* surface;
	void* converted_surface;
	uint32_t* palette;
};


struct EmulatedPaletteInfo
{
	uint32_t* palette;
};




struct EnumDisplayModesContext
{
	DWORD width;
	DWORD height;
	DWORD mipMapCount;
};





//HRESULT __stdcall SetDisplayMode(IDirectDraw* This, DWORD, DWORD, DWORD);



void DDRAW_DirectDraw_vTable_Hooks(LPDIRECTDRAW* lplpDD);
void DDRAW_DirectDrawSurface_vTable_Hooks(LPDIRECTDRAWSURFACE* lplpDDSurf);
void DDRAW_DirectDrawClipper_vTable_Hooks(LPDIRECTDRAWCLIPPER* lplpDDClip);
void DDRAW_DirectDrawPalette_vTable_Hooks(LPDIRECTDRAWPALETTE* lplpDDPal);




