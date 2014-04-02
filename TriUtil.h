/* 
	*************************************************************************************

	TriUtil.h

	Created:   March 2005
	OS:        Win32
	Project:   Trinity

	Description:   

		Utility classes for various Trinity stuff


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2005

	*************************************************************************************
*/
#pragma once

#ifndef TRIUTIL_H
#define TRIUTIL_H

#if (TRINITY_PLATFORM == TRINITY_DIRECTX9) && defined(_WIN32)
	#if defined(_DEBUG) || defined(TRINITYDEV)
		#define D3D_DEBUG_INFO
		//#define taskletProfiling //profile?
		#define D3DPERF
	#endif
#endif


#ifdef D3DPERF

//Helper class for d3d events
class D3DPERF_Event
{
public:
	D3DPERF_Event(LPCWSTR name, D3DCOLOR col = 0) {D3DPERF_BeginEvent(col, name);}
	D3DPERF_Event(D3DCOLOR col, const wchar_t *format, ...) {
		va_list va;
		va_start(va, format);
		BeginEvent(format, va, col);
		va_end(va);
	}
	~D3DPERF_Event() {D3DPERF_EndEvent();}

private:
	static void BeginEvent(const wchar_t *format, va_list va, D3DCOLOR col = 0)
	{
		wchar_t *buf=0, mybuf[64];
		size_t s = _vscwprintf(format, va)+1;
		if (s <= _countof(mybuf)) {
			buf = mybuf;
			s = _countof(mybuf);
		} else
			buf = CCP_NEW("BeginEvent/buf") wchar_t[s];
		vswprintf(buf, s, format, va);
		D3DPERF_BeginEvent(col, buf);
		if (buf != mybuf)
			CCP_DELETE [] buf;
	}
};

class D3DPERF_Marker
{
public:
	D3DPERF_Marker(LPCWSTR name, D3DCOLOR col = 0) {D3DPERF_SetMarker(col, name);}
	D3DPERF_Marker(D3DCOLOR col, const wchar_t *format, ...) {
		va_list va;
		va_start(va, format);
		SetMarker(format, va, col);
		va_end(va);
	}
private:
	static void SetMarker(const wchar_t *format, va_list va, D3DCOLOR col = 0)
	{
		wchar_t *buf=0, mybuf[64];
		size_t s = _vscwprintf(format, va)+1;
		if (s <= _countof(mybuf)) {
			buf = mybuf;
			s = _countof(mybuf);
		} else
			buf = CCP_NEW("SetMarker/buf") wchar_t[s];
		vswprintf(buf, s, format, va);
		D3DPERF_SetMarker(col, buf);
		if (buf != mybuf)
			CCP_DELETE [] buf;
	}	
};

#endif

//ease-of-use macros
#ifdef D3DPERF
#define D3DPERF_EVENT(n) D3DPERF_Event _event((n))
#define D3DPERF_EVENTC(n, c) D3DPERF_Event _event((n), (c))
#define D3DPERF_EVENT1(n, a1) D3DPERF_Event _event(0, (n), (a1))
#define D3DPERF_MARKER(n) D3DPERF_Marker _marker((n))
#define D3DPERF_MARKERC(n, c) D3DPERF_Marker _marker((n), (c))
#else
#define D3DPERF_EVENT(n)
#define D3DPERF_EVENTC(n,c)
#define D3DPERF_EVENT1(n,a1)
#define D3DPERF_MARKER(n)
#define D3DPERF_MARKERC(n,c)
#endif

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// Use this nifty class template to create a singleton that never executes the destructor
// on the templated type - thus allowing the object to remain intact until the program is
// completely dead.
template<typename T> class NeverEndingSingleton
{
private:
	char m_buffer[sizeof(T)];
	T* m_instance;

public:
	NeverEndingSingleton()
	{
		// Placement new to initialize instance in the memory (we don't want to involve the 
		// dynamic memory manager)
		m_instance = new( m_buffer ) T;
	}

	~NeverEndingSingleton()
	{
		// Intentionally blank.  We want the memory to be unmodified and don't want any tear-down.
		// This object is supposed to live forever!
	}

	T& GetInstance()
	{
		return *m_instance;
	}
private:
	NeverEndingSingleton( const NeverEndingSingleton& ) /* = delete */;
	NeverEndingSingleton& operator=( const NeverEndingSingleton& ) /* = delete */;
};

// -------------------------------------------------------------
// Description:
//   Converts value from linear color space to gamma 2.2 color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in linear color space
// Return value:
//   value in gamma 2.2 color space
// -------------------------------------------------------------
inline float TriLinearToGamma( float value )
{
	return pow( value, 0.454545f );
}

// -------------------------------------------------------------
// Description:
//   Converts value from linear color space to gamma 2.2 color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in linear color space
// Return value:
//   value in gamma 2.2 color space
// -------------------------------------------------------------
inline Vector2 TriLinearToGamma( const Vector2 &value )
{
	return Vector2( XMVectorPow( value, XMVectorReplicate( 0.454545f ) ) );
}

// -------------------------------------------------------------
// Description:
//   Converts value from linear color space to gamma 2.2 color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in linear color space
// Return value:
//   value in gamma 2.2 color space
// -------------------------------------------------------------
inline Vector3 TriLinearToGamma( const Vector3 &value )
{
	return Vector3( XMVectorPow( value, XMVectorReplicate( 0.454545f ) ) );
}

// -------------------------------------------------------------
// Description:
//   Converts value from linear color space to gamma 2.2 color
//   space. Does not check if the value is denormalized. W 
//   component is left unmodified.
// Arguments:
//   value - value in linear color space
// Return value:
//   value in gamma 2.2 color space
// -------------------------------------------------------------
inline Vector4 TriLinearToGamma( const Vector4 &value )
{
	Vector4 result = Vector4( XMVectorPow( value, Vector4( 0.454545f, 0.454545f, 0.454545f, 1.f ) ) );
	// Copy W component to guarantee that it is not changed by power function
	result.w = value.w;
	return result;
}

// -------------------------------------------------------------
// Description:
//   Converts value from linear color space to gamma 2.2 color
//   space. Does not check if the value is denormalized. Alpha
//   component is left unmodified.
// Arguments:
//   value - value in linear color space
// Return value:
//   value in gamma 2.2 color space
// -------------------------------------------------------------
inline Color TriLinearToGamma( const Color &value )
{
	Vector4 result = Vector4( XMVectorPow( XMLoadFloat4( (XMFLOAT4*)&value ), Vector4( 0.454545f, 0.454545f, 0.454545f, 1.f ) ) );
	// Copy alpha component to guarantee that it is not changed by power function
	result.w = value.a;
	return Color( result.x, result.y, result.z, result.w );
}

// -------------------------------------------------------------
// Description:
//   Converts value from gamma 2.2 color space to linear color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in gamma 2.2 color space
// Return value:
//   value in linear color space
// -------------------------------------------------------------
inline float TriGammaToLinear( float value )
{
	return pow( value, 2.2f );
}

// -------------------------------------------------------------
// Description:
//   Converts value from gamma 2.2 color space to linear color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in gamma 2.2 color space
// Return value:
//   value in linear color space
// -------------------------------------------------------------
inline Vector2 TriGammaToLinear( const Vector2 &value )
{
	return Vector2( XMVectorPow( value, XMVectorReplicate( 2.2f ) ) );
}

// -------------------------------------------------------------
// Description:
//   Converts value from gamma 2.2 color space to linear color
//   space. Does not check if the value is denormalized.
// Arguments:
//   value - value in gamma 2.2 color space
// Return value:
//   value in linear color space
// -------------------------------------------------------------
inline Vector3 TriGammaToLinear( const Vector3 &value )
{
	return Vector3( XMVectorPow( value, XMVectorReplicate( 2.2f ) ) );
}

// -------------------------------------------------------------
// Description:
//   Converts value from gamma 2.2 color space to linear color
//   space. Does not check if the value is denormalized. W 
//   component is left unmodified.
// Arguments:
//   value - value in gamma 2.2 color space
// Return value:
//   value in linear color space
// -------------------------------------------------------------
inline Vector4 TriGammaToLinear( const Vector4 &value )
{
	Vector4 result = Vector4( XMVectorPow( value, XMVectorReplicate( 2.2f ) ) );
	result.w = value.w;
	return result;
}

// -------------------------------------------------------------
// Description:
//   Converts value from gamma 2.2 color space to linear color
//   space. Does not check if the value is denormalized. Alpha
//   component is left unmodified.
// Arguments:
//   value - value in gamma 2.2 color space
// Return value:
//   value in linear color space
// -------------------------------------------------------------
inline Color TriGammaToLinear( const Color &value )
{
	Vector4 result = Vector4( XMVectorPow( XMLoadFloat4( (XMFLOAT4*)&value ), XMVectorReplicate( 2.2f ) ) );
	result.w = value.a;
	return Color( result.x, result.y, result.z, result.w );
}

#endif //defined TRIUTIL_H
