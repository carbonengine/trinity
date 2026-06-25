// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Vector4d_H
#define Vector4d_H

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 4-vector of doubles.  Unlike Vector4, it does not
//   inherit from a D3DXMath base class (because none of them use doubles).  It provides
//   the standard set of constructors, arithmetic, assignment, comparison, and
//   conversion operators.  Vector4d is used in EVE in some situations which require
//   double precision.
// See Also:
//   Vector2, Vector3, Vector4, Quaternion, Matrix
// --------------------------------------------------------------------------------------
struct Vector4d
{
public:
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor, initializes to zero vector.
	// ----------------------------------------------------------------------------------
	Vector4d( void ) :
		x( 0.0 ), y( 0.0 ), z( 0.0 ), w( 0.0 )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Double-array constructor
	// Arguments:
	//   d - An array of 3 doubles
	// ----------------------------------------------------------------------------------
	explicit Vector4d( const double* d )
	{
		x = d[0];
		y = d[1];
		z = d[2];
		w = d[3];
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 3 floats
	// ----------------------------------------------------------------------------------
	explicit Vector4d( const float* f )
	{
		x = f[0];
		y = f[1];
		z = f[2];
		w = f[3];
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Double component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	//   z - The z-component value to initialize
	//   w - The w-component value to initialize
	// ---------------------------------------------------------------------------------
	Vector4d( double _x, double _y, double _z, double _w ) :
		x( _x ), y( _y ), z( _z ), w( _w )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Vector4d to copy
	// ----------------------------------------------------------------------------------
	Vector4d( const Vector4d& other ) :
		x( other.x ), y( other.y ), z( other.z ), w( other.w )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   constructor from float vector
	// Arguments:
	//   other - The Vector4 to copy
	// ----------------------------------------------------------------------------------
	Vector4d( const Vector4& other ) :
		x( other.x ), y( other.y ), z( other.z ), w( other.w )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   constructor from vector3d and double
	// Arguments:
	//   other - The Vector3d to copy
	//   _w - The double w value
	// ----------------------------------------------------------------------------------
	Vector4d( const Vector3d& other, double _w ) :
		x( other.x ), y( other.y ), z( other.z ), w( _w )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   constructor from float vector3 and float
	// Arguments:
	//   other - The Vector3 to copy
	//   _w - The float w value
	// ----------------------------------------------------------------------------------
	Vector4d( const Vector3& other, float _w ) :
		x( other.x ), y( other.y ), z( other.z ), w( _w )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Double-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the vector
	// ----------------------------------------------------------------------------------
	operator double*( void )
	{
		return &x;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const double-array conversion operator
	// Return Value:
	//   Pointer to the x-component of the vector
	// ----------------------------------------------------------------------------------
	operator const double*( void ) const
	{
		return &x;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Conversion function to produce a Vector4 from this Vector4d
	// Return Value:
	//   The Vector4 produced by the conversion
	// ----------------------------------------------------------------------------------
	const Vector4 AsVector4( void ) const
	{
		return Vector4( static_cast<float>( x ),
						static_cast<float>( y ),
						static_cast<float>( z ),
						static_cast<float>( w ) );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Vector4d to add to this Vector4d
	// Return Value:
	//   This Vector4d, after adding the other Vector4d
	// ----------------------------------------------------------------------------------
	Vector4d& operator+=( const Vector4d& other )
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
	//   other - The Vector4d to subtract from this Vector4d
	// Return Value:
	//   This Vector4d, after subtracting off the other Vector4d
	// ----------------------------------------------------------------------------------
	Vector4d& operator-=( const Vector4d& other )
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
	//   other - The Vector4d by which to multiply this Vector4d component-wise
	// Return Value:
	//   This Vector4d, after multiplying by the other Vector4d component-wise
	// ----------------------------------------------------------------------------------
	Vector4d& operator*=( const Vector4d& other )
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
	//   d - The scalar by which to multiply this Vector4d
	// Return Value:
	//   This Vector4d, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Vector4d& operator*=( double d )
	{
		x *= d;
		y *= d;
		z *= d;
		w *= d;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   d - The scalar by which to divide this Vector4d
	// Return Value:
	//   This Vector4d, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Vector4d& operator/=( double d )
	{
		const double dDiv = 1.0 / d;
		x *= dDiv;
		y *= dDiv;
		z *= dDiv;
		w *= dDiv;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator for Vector4 right-hand side
	// Arguments:
	//   other - The Vector4 to add to this Vector4d
	// Return Value:
	//   This Vector4d, after adding the other Vector4
	// ----------------------------------------------------------------------------------
	Vector4d& operator+=( const Vector4& other )
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator for Vector4 right-hand side
	// Arguments:
	//   other - The Vector4 to subtract from this Vector4d
	// Return Value:
	//   This Vector4d, after subtracting off the other Vector4
	// ----------------------------------------------------------------------------------
	Vector4d& operator-=( const Vector4& other )
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Vector4d
	// ----------------------------------------------------------------------------------
	Vector4d operator+( void ) const
	{
		return Vector4d( *this );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Vector4d, with each component negated
	// ----------------------------------------------------------------------------------
	Vector4d operator-( void ) const
	{
		return Vector4d( -x, -y, -z, -w );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Vector3d to add to this Vector4d
	// Return Value:
	//   Vector4d containing the sum of this Vector4d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector4d operator+( const Vector4d& other ) const
	{
		return Vector4d( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Vector4d to subtract from this Vector4d
	// Return Value:
	//   Vector4d containing the result of the subtraction of the other operand from this
	//   Vector4d
	// ----------------------------------------------------------------------------------
	const Vector4d operator-( const Vector4d& other ) const
	{
		return Vector4d( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator for Vector4 second operand
	// Arguments:
	//   other - The Vector4 to add to this Vector4d
	// Return Value:
	//   Vector4d containing the sum of this Vector4d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector4d operator+( const Vector4& other ) const
	{
		return Vector4d( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator for Vector4 second operand
	// Arguments:
	//   other - The Vector4 to subtract from this Vector4d
	// Return Value:
	//   Vector4d containing the result of the subtraction of the other operand from this
	//   Vector4d
	// ----------------------------------------------------------------------------------
	const Vector4d operator-( const Vector4& other ) const
	{
		return Vector4d( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Vector3d by which to multiply this Vector4d component-wise
	// Return Value:
	//   Vector4d containing component-wise product of this Vector4d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector4d operator*( const Vector4d& other ) const
	{
		return Vector4d( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   d - The scalar by which to multiply this Vector4d
	// Return Value:
	//   Vector4d containing product of this Vector3d and the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector4d operator*( double d ) const
	{
		return Vector4d( *this ) *= d;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   d - The scalar by which to divide this Vector4d
	// Return Value:
	//   Vector4d containing this Vector4d divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector4d operator/( double d ) const
	{
		return Vector4d( *this ) /= d;
	}

	// Declare friend operators
	friend const Vector4d operator*( double d, const Vector4d& other );
	friend const Vector4d operator+( const Vector4& a, const Vector4d& b );
	friend const Vector4d operator-( const Vector4& a, const Vector4d& b );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Vector4d with which to compare for equality
	// Return Value:
	//   true, if the two Vector4d's are component-wise equal
	//   false, if the two Vector4d's are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Vector4d& other )
	{
		return ( x == other.x && y == other.y && z == other.z && w == other.w );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Vector4d with which to compare for inequality
	// Return Value:
	//   true, if the two Vector4d's are not component-wise equal
	//   false, if the two Vector4d's are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Vector4d& other )
	{
		return ( x != other.x || y != other.y || z != other.z || w != other.w );
	}

	// The x-component of the vector
	double x;
	// The y-component of the vector
	double y;
	// The z-component of the vector
	double z;
	// The w-component of the vector
	double w;
};

// ----------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   d - The scalar by which to multiply the Vector4d
//   other - The Vector4d to scale
// Return Value:
//   Vector4d containing product of the Vector4d and the scalar operand
// ----------------------------------------------------------------------------------
inline const Vector4d operator*( double d, const Vector4d& other )
{
	return Vector4d( other ) *= d;
}

// ----------------------------------------------------------------------------------
// Description
//   Global binary addition operator for Vector4 first operand
// Arguments:
//   a - The first Vector4 operand
//   b - The second Vector4d operand
// Return Value:
//   Vector4d containing sum of the Vector4 and the Vector4d
// ----------------------------------------------------------------------------------
inline const Vector4d operator+( const Vector4& a, const Vector4d& b )
{
	return Vector4d( b ) += a;
}

// ----------------------------------------------------------------------------------
// Description
//   Global binary subtraction operator for Vector4 first operand
// Arguments:
//   a - The first Vector4 operand
//   b - The second Vector4d operand
// Return Value:
//   Vector4d containing subtraction of the Vector4d from the Vector4
// ----------------------------------------------------------------------------------
inline const Vector4d operator-( const Vector4& a, const Vector4d& b )
{
	return Vector4d( b ) -= a;
}

// --------------------------------------------------------------------------------------
inline double Dot( const Vector4d& v1, const Vector4d& v2 )
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}




#endif