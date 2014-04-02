#pragma once
#ifndef QUATERNION_H
#define QUATERNION_H

#ifdef _WIN32

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a quaternion.  It inherits from D3DXQUATERNION and is
//   convertible to XMVECTOR, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector2, Vector3, Vector4, Matrix
// --------------------------------------------------------------------------------------
struct Quaternion :  public D3DXQUATERNION
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Quaternion( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 4 floats
	// ----------------------------------------------------------------------------------
	Quaternion( const float* f ) : D3DXQUATERNION( f ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	//   z - The z-component value to initialize
	//   w - The w-component value to initialize
	// ----------------------------------------------------------------------------------
	Quaternion( float x, float y, float z, float w ) :
		D3DXQUATERNION( x, y, z, w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DXQUATERNION assignment constructor
	// Arguments:
	//   other - The D3DXQUATERNION to copy
	// ----------------------------------------------------------------------------------
	Quaternion( const D3DXQUATERNION& other ) :
		D3DXQUATERNION( other.x, other.y, other.z, other.w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment constructor
	// Arguments:
	//   other - The XMVECTOR to copy
	// ----------------------------------------------------------------------------------
	explicit Quaternion( const XMVECTOR& other )
	{
		XMStoreFloat4( (XMFLOAT4*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Quaternion to copy
	// ----------------------------------------------------------------------------------
	Quaternion( const Quaternion& other ) : 
		D3DXQUATERNION( other.x, other.y, other.z, other.w ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the Quaternion
	// ----------------------------------------------------------------------------------
	operator float*( void ) 
	{ 
		return &x; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const float-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the Quaternion
	// ----------------------------------------------------------------------------------
	operator const float*( void ) const 
	{ 
		return &x; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const XMVECTOR conversion operator
	// Return Value:
	//   XMVECTOR constructed from the Quaternion
	// ----------------------------------------------------------------------------------
	operator const XMVECTOR( void ) const 
	{
		return XMLoadFloat4( (const XMFLOAT4*)this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMVECTOR assignment operator
	// Arguments:
	//   other - the XMVECTOR to assign to this Quaternion
	// Return Value:
	//   Reference to this Quaternion
	// ----------------------------------------------------------------------------------
	const Quaternion& operator=( const XMVECTOR& other )
	{
		XMStoreFloat4( (XMFLOAT4*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Quaternion to add to this Quaternion
	// Return Value:
	//   This Quaternion, after adding the other Quaternion
	// ----------------------------------------------------------------------------------
	Quaternion& operator+=( const Quaternion& other )
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
	//   other - The Quaternion to subtract from this Quaternion
	// Return Value:
	//   This Quaternion, after subtracting off the other Quaternion
	// ----------------------------------------------------------------------------------
	Quaternion& operator-=( const Quaternion& other )
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
	//   other - The Quaternion by which to multiply this Quaternion
	// Return Value:
	//   This Quaternion, after multiplying by the other Quaternion
	// ----------------------------------------------------------------------------------
	Quaternion& operator*=( const Quaternion& other )
	{
		D3DXQuaternionMultiply( this, this, &other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Quaternion
	// Return Value:
	//   This Quaternion, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Quaternion& operator*=( float f )
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
	//   f - The scalar by which to divide this Quaternion
	// Return Value:
	//   This Quaternion, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Quaternion& operator/=( float f )
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
	//   A copy of this Quaternion
	// ----------------------------------------------------------------------------------
	Quaternion operator+( void ) const 
	{ 
		return Quaternion( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Quaternion, with each component negated
	// ----------------------------------------------------------------------------------
	Quaternion operator-( void ) const 
	{ 
		return Quaternion( -x, -y, -z, -w ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Quaternion to add to this Quaternion
	// Return Value:
	//   Quaternion containing the sum of this Quaternion and the other operand
	// ----------------------------------------------------------------------------------
	const Quaternion operator+( const Quaternion& other ) const
	{
		return Quaternion( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Quaternion to subtract from this Quaternion
	// Return Value:
	//   Quaternion containing the result of the subtraction of the other operand from 
	//   this Quaternion
	// ----------------------------------------------------------------------------------
	const Quaternion operator-( const Quaternion& other ) const
	{
		return Quaternion( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Quaternion by which to multiply this Quaternion
	// Return Value:
	//   Quaternion containing the product of this Quaternion and the other operand
	// ----------------------------------------------------------------------------------
	const Quaternion operator*( const Quaternion& other ) const
	{
		return Quaternion( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Quaternion
	// Return Value:
	//   Quaternion containing product of this Quaternion and the scalar operand
	// ----------------------------------------------------------------------------------
	const Quaternion operator*( float f ) const
	{
		return Quaternion( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Quaternion
	// Return Value:
	//   Quaternion containing this Quaternion divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Quaternion operator/( float f ) const
	{
		return Quaternion( *this ) /= f;
	}

	// Declare friend operator
	friend const Quaternion operator*( float f, const Quaternion& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Quaternion with which to compare for equality
	// Return Value:
	//   true, if the two Quaternions are component-wise equal
	//   false, if the two Quaternions are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Quaternion& other )
	{
		return ( x == other.x && y == other.y && z == other.z && w == other.w );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Quaternion with which to compare for inequality
	// Return Value:
	//   true, if the two Quaternions are not component-wise equal
	//   false, if the two Quaternions are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Quaternion& other )
	{
		return ( x != other.x || y != other.y || z != other.z || w != other.w );
	}
};

// --------------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Quaternion
//   other - The Quaternion to scale
// Return Value:
//   Quaternion containing product of the Quaternion and the scalar operand
// --------------------------------------------------------------------------------------
inline const Quaternion operator*( float f, const Quaternion& other )
{
	return Quaternion( other ) *= f;
}

#else
#include "CcpMath/include/Quaternion.h"
#endif

inline bool IsMatch( Be::Var* value, const Quaternion& t )
{
	return (Be::Var*)&t == value;
}

#endif