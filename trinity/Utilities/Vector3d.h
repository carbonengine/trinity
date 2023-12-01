#pragma once
#ifndef Vector3d_H
#define Vector3d_H

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 3-vector of doubles.  Unlike Vector3, it does not
//   inherit from a D3DXMath base class (because none of them use doubles).  It provides
//   the standard set of constructors, arithmetic, assignment, comparison, and 
//   conversion operators.  Vector3d is used in EVE in some situations which require
//   double precision.
// See Also:
//   Vector2, Vector3, Vector4, Quaternion, Matrix
// --------------------------------------------------------------------------------------
struct Vector3d
{
public:
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor, initializes to zero vector.
	// ----------------------------------------------------------------------------------
	Vector3d( void ) : x( 0.0 ), y( 0.0 ), z( 0.0 ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Double-array constructor
	// Arguments:
	//   d - An array of 3 doubles
	// ----------------------------------------------------------------------------------
	explicit Vector3d( const double* d )
	{
		x = d[0];
		y = d[1];
		z = d[2];
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 3 floats
	// ----------------------------------------------------------------------------------
	explicit Vector3d( const float* f )
	{
		x = f[0];
		y = f[1];
		z = f[2];
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Double component constructor
	// Arguments:
	//   x - The x-component value to initialize
	//   y - The y-component value to initialize
	//   z - The z-component value to initialize
	// ---------------------------------------------------------------------------------
	Vector3d( double _x, double _y, double _z ) : x( _x ), y( _y ), z( _z ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Vector3d to copy
	// ----------------------------------------------------------------------------------
	Vector3d( const Vector3d& other ) : x( other.x ), y( other.y ), z( other.z ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   constructor from float vector
	// Arguments:
	//   other - The Vector3 to copy
	// ----------------------------------------------------------------------------------
	Vector3d( const Vector3& other ) : x( other.x ), y( other.y ), z( other.z ) {}

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
	//   Conversion function to produce a Vector3 from this Vector3d
	// Return Value:
	//   The Vector3 produced by the conversion
	// ----------------------------------------------------------------------------------
	const Vector3 AsVector3( void ) const
	{
		return Vector3( static_cast<float>( x ),
			static_cast<float>( y ),
			static_cast<float>( z ) );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Vector3d to add to this Vector3d
	// Return Value:
	//   This Vector3d, after adding the other Vector3d
	// ----------------------------------------------------------------------------------
	Vector3d& operator+=( const Vector3d& other )
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
	//   other - The Vector3d to subtract from this Vector3d
	// Return Value:
	//   This Vector3d, after subtracting off the other Vector3d
	// ----------------------------------------------------------------------------------
	Vector3d& operator-=( const Vector3d& other )
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
	//   other - The Vector3d by which to multiply this Vector3d component-wise
	// Return Value:
	//   This Vector3d, after multiplying by the other Vector3d component-wise
	// ----------------------------------------------------------------------------------
	Vector3d& operator*=( const Vector3d& other )
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
	//   d - The scalar by which to multiply this Vector3d
	// Return Value:
	//   This Vector3d, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Vector3d& operator*=( double d )
	{
		x *= d;
		y *= d;
		z *= d;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   d - The scalar by which to divide this Vector3d
	// Return Value:
	//   This Vector3d, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Vector3d& operator/=( double d )
	{
		const double dDiv = 1.0 / d;
		x *= dDiv;
		y *= dDiv;
		z *= dDiv;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator for Vector3 right-hand side
	// Arguments:
	//   other - The Vector3 to add to this Vector3d
	// Return Value:
	//   This Vector3d, after adding the other Vector3
	// ----------------------------------------------------------------------------------
	Vector3d& operator+=( const Vector3& other )
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator for Vector3 right-hand side
	// Arguments:
	//   other - The Vector3 to subtract from this Vector3d
	// Return Value:
	//   This Vector3d, after subtracting off the other Vector3
	// ----------------------------------------------------------------------------------
	Vector3d& operator-=( const Vector3& other )
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Vector3d
	// ----------------------------------------------------------------------------------
	Vector3d operator+( void ) const
	{
		return Vector3d( *this );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Vector3d, with each component negated
	// ----------------------------------------------------------------------------------
	Vector3d operator-( void ) const
	{
		return Vector3d( -x, -y, -z );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Vector3d to add to this Vector3d
	// Return Value:
	//   Vector3d containing the sum of this Vector3d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector3d operator+( const Vector3d& other ) const
	{
		return Vector3d( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Vector3d to subtract from this Vector3d
	// Return Value:
	//   Vector3d containing the result of the subtraction of the other operand from this
	//   Vector3d
	// ----------------------------------------------------------------------------------
	const Vector3d operator-( const Vector3d& other ) const
	{
		return Vector3d( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator for Vector3 second operand
	// Arguments:
	//   other - The Vector3 to add to this Vector3d
	// Return Value:
	//   Vector3d containing the sum of this Vector3d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector3d operator+( const Vector3& other ) const
	{
		return Vector3d( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator for Vector3 second operand
	// Arguments:
	//   other - The Vector3 to subtract from this Vector3d
	// Return Value:
	//   Vector3d containing the result of the subtraction of the other operand from this
	//   Vector3d
	// ----------------------------------------------------------------------------------
	const Vector3d operator-( const Vector3& other ) const
	{
		return Vector3d( *this ) -= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Vector3d by which to multiply this Vector3d component-wise
	// Return Value:
	//   Vector3d containing component-wise product of this Vector3d and the other operand
	// ----------------------------------------------------------------------------------
	const Vector3d operator*( const Vector3d& other ) const
	{
		return Vector3d( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   d - The scalar by which to multiply this Vector3d
	// Return Value:
	//   Vector3d containing product of this Vector3d and the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector3d operator*( double d ) const
	{
		return Vector3d( *this ) *= d;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   d - The scalar by which to divide this Vector3d
	// Return Value:
	//   Vector3d containing this Vector3d divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Vector3d operator/( double d ) const
	{
		return Vector3d( *this ) /= d;
	}

	// Declare friend operators
	friend const Vector3d operator*( double d, const Vector3d& other );
	friend const Vector3d operator+( const Vector3& a, const Vector3d& b );
	friend const Vector3d operator-( const Vector3& a, const Vector3d& b );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Vector3d with which to compare for equality
	// Return Value:
	//   true, if the two Vector3d's are component-wise equal
	//   false, if the two Vector3d's are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Vector3d& other )
	{
		return ( x == other.x && y == other.y && z == other.z );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Vector3d with which to compare for inequality
	// Return Value:
	//   true, if the two Vector3d's are not component-wise equal
	//   false, if the two Vector3d's are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Vector3d& other )
	{
		return ( x != other.x || y != other.y || z != other.z );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Computes the length of the Vector3d
	// Return Value:
	//   The length of the Vector3d, as a double
	// ----------------------------------------------------------------------------------
	double Length( void ) const
	{
		return sqrt( x*x + y*y + z*z );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Computes the squared length of the Vector3d
	// Return Value:
	//   The squared length of the Vector3d, as a double
	// ----------------------------------------------------------------------------------
	double LengthSq( void ) const
	{
		return x*x + y*y + z*z;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Normalizes the Vector3d and returns a reference.  If the Vector3d has zero
	//   length, it returns without normalizing.
	// Return Value:
	//   The normalized vector
	// ----------------------------------------------------------------------------------
	Vector3d& Normalize( void )
	{
		double norm = Length();
		if( norm == 0.0 )
		{
			return *this;
		}

		norm = 1.0 / norm;

		x *= norm;
		y *= norm;
		z *= norm;
		return *this;
	};

	// ----------------------------------------------------------------------------------
	// Description
	//   Computes in-place cross-product of this Vector3d and another one.
	// Arguments:
	//   other - The second operand in the cross product
	// Return Value:
	//   Reference to this Vector3d, containing the cross product with another Vector3d.
	// ----------------------------------------------------------------------------------
	Vector3d& Cross( const Vector3d& other )
	{
		double xt = y * other.z - z * other.y;
		double yt = z * other.x - x * other.z;
		double zt = x * other.y - y * other.x;

		x = xt;
		y = yt;
		z = zt;
		return *this;
	};

	double Dot( const Vector3d& other )
	{
		return x*other.x + y*other.y + z*other.z;
	};

	// The x-component of the vector
	double x;
	// The y-component of the vector
	double y;
	// The z-component of the vector
	double z;
};

// ----------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   d - The scalar by which to multiply the Vector3d
//   other - The Vector3d to scale
// Return Value:
//   Vector3d containing product of the Vector3 and the scalar operand
// ----------------------------------------------------------------------------------
inline const Vector3d operator*( double d, const Vector3d& other )
{
	return Vector3d( other ) *= d;
}

// ----------------------------------------------------------------------------------
// Description
//   Global binary addition operator for Vector3 first operand
// Arguments:
//   a - The first Vector3 operand
//   b - The second Vector3d operand
// Return Value:
//   Vector3d containing sum of the Vector3 and the Vector3d
// ----------------------------------------------------------------------------------
inline const Vector3d operator+( const Vector3& a, const Vector3d& b )
{
	return Vector3d( b ) += a;
}

// ----------------------------------------------------------------------------------
// Description
//   Global binary subtraction operator for Vector3 first operand
// Arguments:
//   a - The first Vector3 operand
//   b - The second Vector3d operand
// Return Value:
//   Vector3d containing subtraction of the Vector3d from the Vector3
// ----------------------------------------------------------------------------------
inline const Vector3d operator-( const Vector3& a, const Vector3d& b )
{
	return Vector3d( b ) -= a;
}

// --------------------------------------------------------------------------------------
inline double Dot( const Vector3d& v1, const Vector3d& v2 )
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// --------------------------------------------------------------------------------------
inline Vector3d Cross( const Vector3d& v1, const Vector3d& v2 )
{
	return Vector3d( v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x );
}

// --------------------------------------------------------------------------------------
inline double LengthSq( const Vector3d& v )
{
	return Dot( v, v );
}

// --------------------------------------------------------------------------------------
inline double Length( const Vector3d& v )
{
	return std::sqrt( LengthSq( v ) );
}



#endif