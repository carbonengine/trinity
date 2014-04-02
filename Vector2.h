#pragma once
#ifndef VECTOR2_H
#define VECTOR2_H

#ifdef _WIN32

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 2-vector of floats.  It inherits from D3DXVECTOR2 and is
//   convertible to XMVECTOR, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector3, Vector4, Quaternion, Matrix
// --------------------------------------------------------------------------------------
struct Vector2 : public D3DXVECTOR2
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Vector2( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 2 floats
	// ----------------------------------------------------------------------------------
	explicit Vector2( const float* f ) : D3DXVECTOR2( f ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	// ----------------------------------------------------------------------------------
	Vector2( float x, float y ) : D3DXVECTOR2( x, y ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DXVECTOR2 assignment constructor
	// Arguments:
	//   other - The D3DXVECTOR2 to copy
	// ----------------------------------------------------------------------------------
	Vector2( const D3DXVECTOR2& other ) : D3DXVECTOR2( other ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment constructor
	// Arguments:
	//   other - The XMVECTOR to copy
	// ----------------------------------------------------------------------------------
	explicit Vector2( const XMVECTOR& other )
	{
		XMStoreFloat2( (XMFLOAT2*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Vector2 to copy
	// ----------------------------------------------------------------------------------
	Vector2( const Vector2& other ) : D3DXVECTOR2( other.x, other.y ) {}

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
	//   XMVECTOR constructed from the Vector2 (z- and w-components uninitialized)
	// ----------------------------------------------------------------------------------
	operator const XMVECTOR( void ) const 
	{ 
		return XMLoadFloat2( (const XMFLOAT2*)this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment operator
	// Arguments:
	//   other - the XMVECTOR to assign to this Vector2
	// Return Value:
	//   Reference to this Vector2
	// ----------------------------------------------------------------------------------
	const Vector2& operator=( const XMVECTOR& other )
	{
		XMStoreFloat2( (XMFLOAT2*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Vector2 to add to this Vector2
	// Return Value:
	//   This Vector2, after adding the other Vector2
	// ----------------------------------------------------------------------------------
	Vector2& operator+=( const Vector2& other )
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator
	// Arguments:
	//   other - The Vector2 to subtract from this Vector2
	// Return Value:
	//   This Vector2, after subtracting off the other Vector2
	// ----------------------------------------------------------------------------------
	Vector2& operator-=( const Vector2& other )
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Multiplication-assignment operator
	// Arguments:
	//   other - The Vector2 by which to multiply this Vector2 component-wise
	// Return Value:
	//   This Vector2, after multiplying by the other Vector2 component-wise
	// ----------------------------------------------------------------------------------
	Vector2& operator*=( const Vector2& other )
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Division-assignment operator
	// Arguments:
	//   other - The Vector2 by which to divide this Vector2 component-wise
	// Return Value:
	//   This Vector2, after dividing by the other Vector2 component-wise
	// ----------------------------------------------------------------------------------
	Vector2& operator/=( const Vector2& other )
	{
		x /= other.x;
		y /= other.y;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector2
	// Return Value:
	//   This Vector2, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Vector2& operator*=( float f )
	{
		x *= f;
		y *= f;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   f - The scalar by which to divide this Vector2
	// Return Value:
	//   This Vector2, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Vector2& operator/=( float f )
	{
		const float fDiv = 1.0f / f;
		x *= fDiv;
		y *= fDiv;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Vector2
	// ----------------------------------------------------------------------------------
	Vector2 operator+( void ) const 
	{
		return Vector2( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Vector2, with each component negated
	// ----------------------------------------------------------------------------------
	Vector2 operator-( void ) const 
	{ 
		return Vector2( -x, -y ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Vector2 to add to this Vector2
	// Return Value:
	//   Vector2 containing the sum of this Vector2 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector2 operator+( const Vector2& other ) const
	{
		return Vector2( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Vector2 to subtract from this Vector2
	// Return Value:
	//   Vector2 containing the result of the subtraction of the other operand from this
	//   Vector2
	// ----------------------------------------------------------------------------------
	const Vector2 operator-( const Vector2& other ) const
	{
		return Vector2( *this ) -= other;
	}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Vector2 by which to multiply this Vector2 component-wise
	// Return Value:
	//   Vector2 containing component-wise product of this Vector2 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector2 operator*( const Vector2& other ) const
	{
		return Vector2( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary division operator
	// Arguments:
	//   other - The Vector2 by which to divide this Vector2 component-wise
	// Return Value:
	//   Vector2 containing component-wise division of this Vector2 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector2 operator/( const Vector2& other ) const
	{
		return Vector2( *this ) /= other;
	}


	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector2
	// Return Value:
	//   Vector2 containing product of this Vector2 and the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector2 operator*( float f ) const
	{
		return Vector2( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Vector2
	// Return Value:
	//   Vector2 containing this Vector2 divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector2 operator/( float f ) const
	{
		return Vector2( *this ) /= f;
	}

	// Declare friend operator
	friend const Vector2 operator*( float f, const Vector2& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Vector2 with which to compare for equality
	// Return Value:
	//   true, if the two Vector2's are component-wise equal
	//   false, if the two Vector2's are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Vector2& other )
	{
		return ( x == other.x && y == other.y );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Vector2 with which to compare for inequality
	// Return Value:
	//   true, if the two Vector2's are not component-wise equal
	//   false, if the two Vector2's are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Vector2& other )
	{
		return ( x != other.x || y != other.y );
	}
};

// ----------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Vector2
//   other - The Vector2 to scale
// Return Value:
//   Vector2 containing product of the Vector2 and the scalar operand
// ----------------------------------------------------------------------------------
inline const Vector2 operator*( float f, const Vector2& other )
{
	return Vector2( other ) *= f;
}

#else

#include "CcpMath/include/Vector2.h"

#endif

#endif // VECTOR2_H