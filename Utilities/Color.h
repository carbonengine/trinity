#pragma once
#ifndef COLOR_H
#define COLOR_H

#ifdef _WIN32

struct Color : public D3DXCOLOR
{
	Color() {}
	Color( uint32_t argb );
	Color( const float * );
	Color( const D3DXFLOAT16* pf );
	Color( const D3DCOLORVALUE& c );
	Color( float r, float g, float b, float a );
	Color( const D3DXCOLOR& c );
};

inline Color::Color( uint32_t dw )
{
	const float f = 1.0f / 255.0f;
	r = f * (float) (unsigned char) (dw >> 16);
	g = f * (float) (unsigned char) (dw >>  8);
	b = f * (float) (unsigned char) (dw >>  0);
	a = f * (float) (unsigned char) (dw >> 24);
}

inline Color::Color( const float* pf )
{
	r = pf[0];
	g = pf[1];
	b = pf[2];
	a = pf[3];
}

inline Color::Color( const D3DXFLOAT16* pf )
{
	D3DXFloat16To32Array(&r, pf, 4);
}

inline Color::Color( const D3DCOLORVALUE& c )
{
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

inline Color::Color( float fr, float fg, float fb, float fa )
{
	r = fr;
	g = fg;
	b = fb;
	a = fa;
}

inline Color::Color( const D3DXCOLOR& c )
{
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

#else

#include "CcpMath/include/Color.h"

#endif

inline bool IsMatch( Be::Var* value, const Color& t )
{
	return (Be::Var*)&t == value;
}

#endif