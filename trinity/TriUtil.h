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

#define D3DPERF_EVENT(n)
#define D3DPERF_EVENTC(n,c)
#define D3DPERF_EVENT1(n,a1)
#define D3DPERF_MARKER(n)
#define D3DPERF_MARKERC(n,c)


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

inline bool IsMatch( Be::Var* value, const Vector3& t )
{
	return ( Be::Var* )&t == value;
}

// --------------------------------------------------------------------------------------
inline bool IsMatch( Be::Var* value, const Vector4& t )
{
	return ( Be::Var* )&t == value;
}

// --------------------------------------------------------------------------------------
inline bool IsMatch( Be::Var* value, const Matrix& t )
{
	return ( Be::Var* )&t == value;
}

// --------------------------------------------------------------------------------------
inline bool IsMatch( Be::Var* value, const Color& t )
{
	return ( Be::Var* )&t == value;
}

// --------------------------------------------------------------------------------------
inline bool IsMatch( Be::Var* value, const Quaternion& t )
{
	return ( Be::Var* )&t == value;
}

// --------------------------------------------------------------------------------------
template<>
inline void BlueGetNullValue( Quaternion& resultRef )
{
	resultRef = IdentityQuaternion();
}

#endif //defined TRIUTIL_H
