// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "include/TriColor.h"
#include "include/TriVector.h"
#include "include/TriMath.h"

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
static void HSVtoRGB(
	float* r,
	float* g,
	float* b,
	float h,
	float s,
	float v )
{
	int i;
	float f, p, q, t;
	if( s == 0 )
	{
		s = 0.000000001f;
		// achromatic (grey)
		//*r = *g = *b = v;
		//return;
	}
	h /= 60; // sector 0 to 5
	i = (int)floor( h );
	f = h - i; // factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i )
	{
	case 0:
		*r = v;
		*g = t;
		*b = p;
		break;
	case 1:
		*r = q;
		*g = v;
		*b = p;
		break;
	case 2:
		*r = p;
		*g = v;
		*b = t;
		break;
	case 3:
		*r = p;
		*g = q;
		*b = v;
		break;
	case 4:
		*r = t;
		*g = p;
		*b = v;
		break;
	default: // case 5:
		*r = v;
		*g = p;
		*b = q;
		break;
	}
}


//---------------------------------------------------------------------------
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
#define c_MAX( a, b, c ) a > b ? ( a > c ? a : c ) : b > c ? b : \
															 c;
#define c_MIN( a, b, c ) a < b ? ( a < c ? a : c ) : b < c ? b : \
															 c;
static void RGBtoHSV(
	float r,
	float g,
	float b,
	float* h,
	float* s,
	float* v )
{
	float min, max, delta;
	min = c_MIN( r, g, b );
	max = c_MAX( r, g, b );
	*v = max; // v
	delta = max - min;

	if( delta == 0.0 )
	{
		*h = 0.0;
		*s = 0.0;
		return;
	}
	else if( max != 0 )
	{
		*s = delta / max; // s
	}
	else
	{
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}

	if( r == max )
		*h = ( g - b ) / delta; // between yellow & magenta
	else if( g == max )
		*h = 2 + ( b - r ) / delta; // between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta; // between magenta & cyan
	*h *= 60; // degrees
	if( *h < 0 )
		*h += 360;
}


/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

#if BLUE_WITH_PYTHON
void TriColor::Destroy()
{
	//delete this;
}


PyObject* TriColor::GetAttr(
	const char* name,
	bool* handled )
{
	return 0;
}


bool TriColor::SetAttr(
	const char* name,
	PyObject* v,
	bool* handled )
{
	return true;
}


PyObject* TriColor::Repr(
	bool* handled )
{
	*handled = true;
	char buf[120];
	sprintf_s( buf, "<Instance of 'TriColor' (r=%0.2f g=%0.2f b=%0.2f a=%0.2f)>", r, g, b, a );
	return ToPython( buf );
}


/////////////////////////////////////////////////////////////////////////////////////////
// TriColor
/////////////////////////////////////////////////////////////////////////////////////////

TriColor::TriColor( IRoot* lockobj ) :
	::Color( 0.0f, 0.0f, 0.0f, 0.0f )
{
}


TriColor::~TriColor()
{
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriColor Impl
/////////////////////////////////////////////////////////////////////////////////////////

void TriColor::SetRGB(
	float _r,
	float _g,
	float _b,
	float _a )
{
	r = _r;
	g = _g;
	b = _b;

	if( _a != TRICOLOR_NOALPHA )
		a = _a;
}


void TriColor::SetHSV(
	float h,
	float s,
	float v,
	float _a )
{
	HSVtoRGB( &r, &g, &b, h, s, v );

	if( _a != TRICOLOR_NOALPHA )
		a = _a;
}


void TriColor::SetVector(
	const Vector3* in,
	float alpha )
{
	TriColorFromVector( this, in, alpha );
}

void TriColor::GetHSV(
	float* h,
	float* s,
	float* v,
	float* _a )
{
	RGBtoHSV( r, g, b, h, s, v );

	if( _a )
		*_a = a;
}

void TriColor::SetColor(
	const ::Color* ar )
{
	r = ar->r;
	g = ar->g;
	b = ar->b;

	if( ar->a != TRICOLOR_NOALPHA )
		a = ar->a;
}


const ::Color* TriColor::GetColor() const
{
	return this;
}


::Color* TriColor::CopyColor(
	::Color* in ) const
{
	// would this work?
	//return &(*in = *this);

	*in = *this;
	return in;
}


::Color* TriColor::Color()
{
	return this;
}

void TriColor::Scale(
	float s )
{
	*static_cast<::Color*>( this ) = *this * s;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Python thunkers for ITriColor interface
/////////////////////////////////////////////////////////////////////////////////////////
void TriColor::Py__init__( float _r, float _g, float _b, float _a )
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}

void TriColor::PySetRGB( float r, float g, float b, Be::Optional<float> a )
{
	SetRGB( r, g, b, a.IsAssigned() ? float( a ) : TRICOLOR_NOALPHA );
}


void TriColor::PySetHSV( float hue, float saturation, float value, Be::Optional<float> a )
{
	SetHSV( hue, saturation, value, a.IsAssigned() ? float( a ) : TRICOLOR_NOALPHA );
}

void TriColor::PySetVector( ITriVector* vector, float alpha )
{
	SetVector( vector->GetVector(), alpha );
}

Vector3 TriColor::PyGetHSV()
{
	Vector3 hsv;
	GetHSV( &hsv.x, &hsv.y, &hsv.z, nullptr );
	return hsv;
}

void TriColor::PyFromInt( int32_t color )
{
	// For legacy reasons, Python expects a signed integer value
	uint32_t unsignedColor = static_cast<uint32_t>( color );
	::Color t( unsignedColor );
	r = t.r;
	g = t.g;
	b = t.b;
	a = t.a;
}

int32_t TriColor::PyAsInt()
{
	return static_cast<int32_t>( *this );
}


PyObject* TriColor::PyAdd(
	PyObject* args )
{
	Py_INCREF( Py_None );
	return Py_None;
}

PyObject* TriColor::PyLerp(
	PyObject* args )
{
	Py_INCREF( Py_None );
	return Py_None;
}

#endif
