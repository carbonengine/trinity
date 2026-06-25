// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriDebugTextRenderer_H
#define TriDebugTextRenderer_H

#include "Tr2DeviceResource.h"
#include "include/Rect.h"
#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

class TriDebugTextRenderer : public Tr2DeviceResource
{
public:
	TriDebugTextRenderer();
	~TriDebugTextRenderer();

	void Printf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, ... );
	void PrintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, ... );
	void Vprintf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, va_list args );
	void VprintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, va_list args );

	void Render( Tr2RenderContext& renderContext );
	void Clear();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
#ifdef TRINITYDEV
	void GetDescription( std::string& desc )
	{
		desc = "<TriDebugFont>";
	}
#endif

private:
	void DrawText( Tr2RenderContext& renderContext, TriDebugFont font, const char* string, const Tr2Rect& rect, uint32_t format, const Vector4& color );

private:
	struct TextEntry
	{
		TriDebugFont m_font;
		Tr2Rect m_rect;
		uint32_t m_format;
		Vector4 m_color;
		const char* m_text;
	};

	TrackableStdList<TextEntry> m_entries;

#ifdef _WIN32
	// Memory DC for GDI rendering
	HDC m_dc;
	// Offscreen GDI bitmap
	HBITMAP m_bitmap;
	// Pixel data from m_bitmap; memory managed by WinApi
	uint8_t* m_bitmapData;
	// Fonts (correspond to TriDebugFont)
	HFONT m_dcFonts[3];
	// Current m_bitmap width
	unsigned m_bitmapWidth;
	// Current m_bitmap height
	unsigned m_bitmapHeight;
#endif
	// Temporary texture
	Tr2TextureAL m_texture;
};



#endif // TriDebugTextRenderer_H