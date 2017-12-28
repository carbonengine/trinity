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


// --------------------------------------------------------------------------------------
inline Quaternion IdentityQuaternion()
{
	return Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
}

// --------------------------------------------------------------------------------------
inline float LengthSq( const Quaternion& q )
{
	return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

// --------------------------------------------------------------------------------------
inline float Length( const Quaternion& q )
{
	return sqrt( LengthSq( q ) );
}

// --------------------------------------------------------------------------------------
inline Quaternion Normalize( const Quaternion& q )
{
	float l = 1.0f / Length( q );
	return q * l;
}

// --------------------------------------------------------------------------------------
inline Quaternion Inverse( const Quaternion& q )
{
	float l = 1.0f / LengthSq( q );
	return Quaternion( -q.x * l, -q.y * l, -q.z * l, q.w * l );
}

// --------------------------------------------------------------------------------------
inline Quaternion Conjugate( const Quaternion& q )
{
	Quaternion out;
	out.x = -q.x;
	out.y = -q.y;
	out.z = -q.z;
	out.w = q.w;
	return out;
}

// --------------------------------------------------------------------------------------
inline Quaternion Exp( const Quaternion& q )
{
	float norm = sqrt( q.x * q.x + q.y * q.y + q.z * q.z );
	Quaternion out;
	if( norm )
	{
		out.x = sin( norm ) * q.x / norm;
		out.y = sin( norm ) * q.y / norm;
		out.z = sin( norm ) * q.z / norm;
		out.w = cos( norm );
	}
	else
	{
		out.x = 0.0f;
		out.y = 0.0f;
		out.z = 0.0f;
		out.w = 1.0f;
	}
	return out;


}

// --------------------------------------------------------------------------------------
inline float Dot( const Quaternion& q1, const Quaternion& q2 )
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

// --------------------------------------------------------------------------------------
inline Quaternion RotationQuaternion( const Matrix& m )
{
	Quaternion out;
	int i, maxi;
	float maxdiag, S, trace;

	trace = m.m[0][0] + m.m[1][1] + m.m[2][2] + 1.0f;
	if( trace > 1.0f )
	{
		out.x = ( m.m[1][2] - m.m[2][1] ) / ( 2.0f * sqrt( trace ) );
		out.y = ( m.m[2][0] - m.m[0][2] ) / ( 2.0f * sqrt( trace ) );
		out.z = ( m.m[0][1] - m.m[1][0] ) / ( 2.0f * sqrt( trace ) );
		out.w = sqrt( trace ) / 2.0f;
		return out;
	}
	maxi = 0;
	maxdiag = m.m[0][0];
	for( i = 1; i<3; i++ )
	{
		if( m.m[i][i] > maxdiag )
		{
			maxi = i;
			maxdiag = m.m[i][i];
		}
	}
	switch( maxi )
	{
	case 0:
		S = 2.0f * sqrt( 1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2] );
		out.x = 0.25f * S;
		out.y = ( m.m[0][1] + m.m[1][0] ) / S;
		out.z = ( m.m[0][2] + m.m[2][0] ) / S;
		out.w = ( m.m[1][2] - m.m[2][1] ) / S;
		break;
	case 1:
		S = 2.0f * sqrt( 1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2] );
		out.x = ( m.m[0][1] + m.m[1][0] ) / S;
		out.y = 0.25f * S;
		out.z = ( m.m[1][2] + m.m[2][1] ) / S;
		out.w = ( m.m[2][0] - m.m[0][2] ) / S;
		break;
	case 2:
		S = 2.0f * sqrt( 1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1] );
		out.x = ( m.m[0][2] + m.m[2][0] ) / S;
		out.y = ( m.m[1][2] + m.m[2][1] ) / S;
		out.z = 0.25f * S;
		out.w = ( m.m[0][1] - m.m[1][0] ) / S;
		break;
	}
	return out;
}

// --------------------------------------------------------------------------------------
inline Quaternion RotationQuaternion( float yaw, float pitch, float roll )
{
	float sinYaw = sin( yaw / 2.0f );
	float cosYaw = cos( yaw / 2.0f );
	float sinPitch = sin( pitch / 2.0f );
	float cosPitch = cos( pitch / 2.0f );
	float sinRoll = sin( roll / 2.0f );
	float cosRoll = cos( roll / 2.0f );

	Quaternion out;
	out.x = sinYaw * cosPitch * sinRoll + cosYaw * sinPitch * cosRoll;
	out.y = sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll;
	out.z = cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll;
	out.w = cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll;
	return out;
}

// --------------------------------------------------------------------------------------
inline Quaternion RotationQuaternion( const Vector3& axis, float angle )
{
	Vector3 temp = Normalize( axis );
	Quaternion out;
	out.x = sin( angle / 2.0f ) * temp.x;
	out.y = sin( angle / 2.0f ) * temp.y;
	out.z = sin( angle / 2.0f ) * temp.z;
	out.w = cos( angle / 2.0f );
	return out;
}

// --------------------------------------------------------------------------------------
inline Quaternion Slerp( const Quaternion& q1, const Quaternion& q2, float t )
{
	float epsilon = 1.0f;
	float temp = 1.0f - t;
	float u = t;
	float dot = Dot( q1, q2 );
	if( dot < 0.0f )
	{
		epsilon = -1.0f;
		dot = -dot;
	}
	if( 1.0f - dot > 0.001f )
	{
		float theta = acos( dot );
		temp = sin( theta * temp ) / sin( theta );
		u = sin( theta * u ) / sin( theta );
	}
	Quaternion out;
	out.x = temp * q1.x + epsilon * u * q2.x;
	out.y = temp * q1.y + epsilon * u * q2.y;
	out.z = temp * q1.z + epsilon * u * q2.z;
	out.w = temp * q1.w + epsilon * u * q2.w;
	return out;
}

// --------------------------------------------------------------------------------------
inline std::pair<Vector3, float> GetAxisAngle( const Quaternion& q )
{
	return std::make_pair( Vector3( q.x, q.y, q.z ), acos( q.w ) );
}

#endif