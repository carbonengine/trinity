#pragma once
#ifndef VECTOR3_H
#define VECTOR3_H

#ifdef _WIN32

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 3-vector of floats.  It inherits from D3DXVECTOR3 and is
//   convertible to XMVECTOR, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector2, Vector4, Quaternion, Matrix
// --------------------------------------------------------------------------------------
struct Vector3 : public D3DXVECTOR3
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Vector3( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 3 floats
	// ----------------------------------------------------------------------------------
	explicit Vector3( const float* f ) : D3DXVECTOR3( f ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float16 array constructor
	// Arguments:
	//   f - An array of 3 float16
	// ----------------------------------------------------------------------------------
	explicit Vector3( CONST D3DXFLOAT16 * f )
		:D3DXVECTOR3( f )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	//   z - The z-component value to initialize
	// ----------------------------------------------------------------------------------
	Vector3( float x, float y, float z ) : D3DXVECTOR3( x, y, z ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DVECTOR assignment constructor
	// Arguments:
	//   other - The D3DVECTOR to copy
	// ----------------------------------------------------------------------------------
	Vector3( const D3DVECTOR& other ) : D3DXVECTOR3( other ) {}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment constructor
	// Arguments:
	//   other - The XMVECTOR to copy
	// ----------------------------------------------------------------------------------
	explicit Vector3( const XMVECTOR& other )
	{
		XMStoreFloat3( (XMFLOAT3*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Vector3 to copy
	// ----------------------------------------------------------------------------------
	Vector3( const Vector3& other ) : 
		D3DXVECTOR3( other.x, other.y, other.z ) {}

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
	//   XMVECTOR constructed from the Vector3 (w-component uninitialized)
	// ----------------------------------------------------------------------------------
	operator const XMVECTOR( void ) const 
	{
		return XMLoadFloat3( (const XMFLOAT3*)this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment operator
	// Arguments:
	//   other - the XMVECTOR to assign to this Vector3
	// Return Value:
	//   Reference to this Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& operator=( const XMVECTOR& other )
	{
		XMStoreFloat3( (XMFLOAT3*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Vector3 to add to this Vector3
	// Return Value:
	//   This Vector3, after adding the other Vector3
	// ----------------------------------------------------------------------------------
	Vector3& operator+=( const Vector3& other )
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator
	// Arguments:
	//   other - The Vector3 to subtract from this Vector3
	// Return Value:
	//   This Vector3, after subtracting off the other Vector3
	// ----------------------------------------------------------------------------------
	Vector3& operator-=( const Vector3& other )
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Multiplication-assignment operator
	// Arguments:
	//   other - The Vector3 by which to multiply this Vector3 component-wise
	// Return Value:
	//   This Vector3, after multiplying by the other Vector3 component-wise
	// ----------------------------------------------------------------------------------
	Vector3& operator*=( const Vector3& other )
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector3
	// Return Value:
	//   This Vector3, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Vector3& operator*=( float f )
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   f - The scalar by which to divide this Vector3
	// Return Value:
	//   This Vector3, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Vector3& operator/=( float f )
	{
		const float fDiv = 1.0f / f;
		x *= fDiv;
		y *= fDiv;
		z *= fDiv;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Vector3
	// ----------------------------------------------------------------------------------
	Vector3 operator+( void ) const 
	{ 
		return Vector3( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Vector3, with each component negated
	// ----------------------------------------------------------------------------------
	Vector3 operator-( void ) const 
	{ 
		return Vector3( -x, -y, -z ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Vector3 to add to this Vector3
	// Return Value:
	//   Vector3 containing the sum of this Vector3 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector3 operator+( const Vector3& other ) const
	{
		return Vector3( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Vector3 to subtract from this Vector3
	// Return Value:
	//   Vector3 containing the result of the subtraction of the other operand from this
	//   Vector3
	// ----------------------------------------------------------------------------------
	const Vector3 operator-( const Vector3& other ) const
	{
		return Vector3( *this ) -= other;
	}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Vector3 by which to multiply this Vector3 component-wise
	// Return Value:
	//   Vector3 containing component-wise product of this Vector3 and the other operand
	// ----------------------------------------------------------------------------------
	const Vector3 operator*( const Vector3& other ) const
	{
		return Vector3( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Vector3
	// Return Value:
	//   Vector3 containing product of this Vector3 and the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector3 operator*( float f ) const
	{
		return Vector3( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Vector3
	// Return Value:
	//   Vector3 containing this Vector3 divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector3 operator/( float f ) const
	{
		return Vector3( *this ) /= f;
	}

	// Declare friend operator
	friend const Vector3 operator*( float f, const Vector3& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Vector3 with which to compare for equality
	// Return Value:
	//   true, if the two Vector3's are component-wise equal
	//   false, if the two Vector3's are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Vector3& other )
	{
		return ( x == other.x && y == other.y && z == other.z );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Vector3 with which to compare for inequality
	// Return Value:
	//   true, if the two Vector3's are not component-wise equal
	//   false, if the two Vector3's are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Vector3& other )
	{
		return ( x != other.x || y != other.y || z != other.z );
	}
};

// ----------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Vector3
//   other - The Vector3 to scale
// Return Value:
//   Vector3 containing product of the Vector3 and the scalar operand
// ----------------------------------------------------------------------------------
inline const Vector3 operator*( float f, const Vector3& other )
{
	return Vector3( other ) *= f;
}

#else

#include "CcpMath/include/Vector3.h"

#endif

inline bool IsMatch( Be::Var* value, const Vector3& t )
{
	return (Be::Var*)&t == value;
}

#endif // VECTOR3_H