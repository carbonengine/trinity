#pragma once
#ifndef VECTOR4_H
#define VECTOR4_H

#ifdef _WIN32

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 4-vector of floats.  It inherits from D3DXVECTOR4 and is
//   convertible to XMVECTOR, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector2, Vector3, Quaternion, Matrix
// --------------------------------------------------------------------------------------
struct Vector4 : public D3DXVECTOR4
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Vector4( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 4 floats
	// ----------------------------------------------------------------------------------
	explicit Vector4( const float* f ) : D3DXVECTOR4( f ) {}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	//   z - The z-component value to initialize
	//   w - The w-component value to initialize
	// ----------------------------------------------------------------------------------
	Vector4( float x, float y, float z, float w ) :
		D3DXVECTOR4( x, y, z, w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DVECTOR assignment constructor
	// Arguments:
	//   other - The D3DVECTOR to copy x-, y-, and z-components
	//   w - The w component to copy
	// ----------------------------------------------------------------------------------
	Vector4( const D3DVECTOR& xyz, float w ) : D3DXVECTOR4( xyz, w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment constructor
	// Arguments:
	//   other - The XMVECTOR to copy
	// ----------------------------------------------------------------------------------
	explicit Vector4( const XMVECTOR& other )
	{
		XMStoreFloat4( (XMFLOAT4*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Vector4 to copy
	// ----------------------------------------------------------------------------------
	Vector4( const Vector4& other ) : 
		D3DXVECTOR4( other.x, other.y, other.z, other.w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the vector
	// ----------------------------------------------------------------------------------
	operator float*( void ) 
	{ 
		return &x; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const float-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the vector
	// ----------------------------------------------------------------------------------
	operator const float*( void ) const 
	{ 
		return &x; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const XMVECTOR conversion operator
	// Return Value:
	//   XMVECTOR constructed from the Vector4
	// ----------------------------------------------------------------------------------
	operator const XMVECTOR( void ) const 
	{
		return XMLoadFloat4( (const XMFLOAT4*)this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment operator
	// Arguments:
	//   other - the XMVECTOR to assign to this Vector4
	// Return Value:
	//   Reference to this Vector4
	// ----------------------------------------------------------------------------------
	const Vector4& operator=( const XMVECTOR& other )
	{
		XMStoreFloat4( (XMFLOAT4*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Vector4 to add to this Vector4
	// Return Value:
	//   This Vector4, after adding the other Vector4
	// ----------------------------------------------------------------------------------
	Vector4& operator+=( const Vector4& other )
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator
	// Arguments:
	//   other - The Vector4 to subtract from this Vector4
	// Return Value:
	//   This Vector4, after subtracting off the other Vector4
	// ----------------------------------------------------------------------------------
	Vector4& operator-=( const Vector4& other )
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Multiplication-assignment operator
	// Arguments:
	//   other - The Vector4 by which to multiply this Vector4 component-wise
	// Return Value:
	//   This Vector4, after multiplying by the other Vector4 component-wise
	// ----------------------------------------------------------------------------------
	Vector4& operator*=( const Vector4& other )
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector4
	// Return Value:
	//   This Vector4, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Vector4& operator*=( float f )
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   f - The scalar by which to divide this Vector4
	// Return Value:
	//   This Vector4, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Vector4& operator/=( float f )
	{
		const float fDiv = 1.0f / f;
		x *= fDiv;
		y *= fDiv;
		z *= fDiv;
		w *= fDiv;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Vector4
	// ----------------------------------------------------------------------------------
	Vector4 operator+( void ) const 
	{ 
		return Vector4( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Vector4, with each component negated
	// ----------------------------------------------------------------------------------
	Vector4 operator-( void ) const 
	{ 
		return Vector4( -x, -y, -z, -w ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Vector4 to add to this Vector4
	// Return Value:
	//   Vector4 containing the sum of this Vector4 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector4 operator+( const Vector4& other ) const
	{
		return Vector4( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Vector4 to subtract from this Vector4
	// Return Value:
	//   Vector4 containing the result of the subtraction of the other operand from this
	//   Vector4
	// ----------------------------------------------------------------------------------
	const Vector4 operator-( const Vector4& other ) const
	{
		return Vector4( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Vector4 by which to multiply this Vector4 component-wise
	// Return Value:
	//   Vector4 containing component-wise product of this Vector4 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector4 operator*( const Vector4& other ) const
	{
		return Vector4( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector4
	// Return Value:
	//   Vector4 containing product of this Vector4 and the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector4 operator*( float f ) const
	{
		return Vector4( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Vector4
	// Return Value:
	//   Vector4 containing this Vector4 divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector4 operator/( float f ) const
	{
		return Vector4( *this ) /= f;
	}

	// Declare friend operator
	friend const Vector4 operator*( float f, const Vector4& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Vector4 with which to compare for equality
	// Return Value:
	//   true, if the two Vector4's are component-wise equal
	//   false, if the two Vector4's are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Vector4& other )
	{
		return ( x == other.x && y == other.y && z == other.z && w == other.w );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Vector4 with which to compare for inequality
	// Return Value:
	//   true, if the two Vector4's are not component-wise equal
	//   false, if the two Vector4's are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Vector4& other )
	{
		return ( x != other.x || y != other.y || z != other.z || w != other.w );
	}
};

// ----------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Vector4
//   other - The Vector4 to scale
// Return Value:
//   Vector4 containing product of the Vector4 and the scalar operand
// ----------------------------------------------------------------------------------
inline const Vector4 operator*( float f, const Vector4& other )
{
	return Vector4( other ) *= f;
}

#else

#include "CcpMath/include/Vector4.h"

#endif

inline bool IsMatch( Be::Var* value, const Vector4& t )
{
	return (Be::Var*)&t == value;
}

#endif // VECTOR4_H