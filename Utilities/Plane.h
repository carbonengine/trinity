#pragma once

#include "Vector3.h"

struct Plane
{
	Plane();
	Plane( float a, float b, float c, float d );

	// assignment operators
	Plane& operator *= ( float );
	Plane& operator /= ( float );

	// unary operators
	Plane operator + () const;
	Plane operator - () const;

	// binary operators
	Plane operator * ( float ) const;
	Plane operator / ( float ) const;

	bool operator == ( const Plane& ) const;
	bool operator != ( const Plane& ) const;

	float a, b, c, d;
};


// --------------------------------------------------------------------------------------
inline Plane::Plane()
{
}

// --------------------------------------------------------------------------------------
inline Plane::Plane( float a_, float b_, float c_, float d_ )
	:a( a_ ),
	b( b_ ),
	c( c_ ),
	d( d_ )
{
}

// --------------------------------------------------------------------------------------
inline Plane& Plane::operator *= ( float scale )
{
	a *= scale;
	b *= scale;
	c *= scale;
	d *= scale;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Plane& Plane::operator /= ( float div )
{
	float scale = 1.0f / div;
	a *= scale;
	b *= scale;
	c *= scale;
	d *= scale;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Plane Plane::operator + () const
{
	return *this;
}

// --------------------------------------------------------------------------------------
inline Plane Plane::operator - () const
{
	return Plane( -a, -b, -c, -d );
}

// --------------------------------------------------------------------------------------
inline Plane Plane::operator * ( float scale ) const
{
	return Plane( a * scale, b * scale, c * scale, d * scale );
}

// --------------------------------------------------------------------------------------
inline Plane Plane::operator / ( float div ) const
{
	float scale = 1.0f / div;
	return Plane( a * scale, b * scale, c * scale, d * scale );
}

// --------------------------------------------------------------------------------------
inline bool Plane::operator == ( const Plane& other ) const
{
	return a == other.a && b == other.b && c == other.c && d == other.d;
}

// --------------------------------------------------------------------------------------
inline bool Plane::operator != ( const Plane& other ) const
{
	return a != other.a || b != other.b || c != other.c || d != other.d;
}


// --------------------------------------------------------------------------------------
inline Plane operator * ( float scale, const Plane& plane )
{
	return plane * scale;
}

// --------------------------------------------------------------------------------------
inline std::pair<bool, Vector3> IntersectLine(
	const Plane& p,
	const Vector3& v1,
	const Vector3& v2 )
{
	Vector3 direction = v2 - v1;
	Vector3 normal;
	normal.x = p.a;
	normal.y = p.b;
	normal.z = p.c;

	float dot = Dot( normal, direction );
	if( dot == 0.0f )
	{
		return std::make_pair( false, Vector3( 0, 0, 0 ) );
	}
	float temp = ( p.d + Dot( normal, v1 ) ) / dot;
	return std::make_pair( true, Vector3(
		v1.x - temp * direction.x,
		v1.y - temp * direction.y,
		v1.z - temp * direction.z ) );
}

// --------------------------------------------------------------------------------------
inline Plane Normalize( const Plane& plane )
{
	float l = 1.0f / Length( reinterpret_cast<const Vector3&>( plane ) );
	return Plane(
		plane.a * l,
		plane.b * l,
		plane.c * l,
		plane.d * l );
}

// --------------------------------------------------------------------------------------
inline float DotCoord( const Plane& p, const Vector3& v )
{
	return p.a * v.x + p.b * v.y + p.c * v.z + p.d;
}

// --------------------------------------------------------------------------------------
inline float DotNormal( const Plane& p, const Vector3& v )
{
	return p.a * v.x + p.b * v.y + p.c * v.z;
}

