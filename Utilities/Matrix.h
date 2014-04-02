#pragma once
#ifndef MATRIX_H
#define MATRIX_H

#ifdef _WIN32

#include "Vector3.h"
#include "Vector4.h"

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 4x4 float matrix.  It inherits from D3DXMATRIX and is
//   convertible to XMMATRIX, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector2, Vector3, Vector4, Quaternion
// --------------------------------------------------------------------------------------
struct Matrix : public D3DXMATRIX
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Matrix( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 16 floats
	// ----------------------------------------------------------------------------------
	explicit Matrix( const float* f ) : D3DXMATRIX( f ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor (arguments omitted from docstring)
	// ----------------------------------------------------------------------------------
	Matrix( float _11, float _12, float _13, float _14,
		    float _21, float _22, float _23, float _24,
		    float _31, float _32, float _33, float _34,
			float _41, float _42, float _43, float _44 ) :
		D3DXMATRIX( _11, _12, _13, _14, 
		            _21, _22, _23, _24, 
					_31, _32, _33, _34, 
					_41, _42, _43, _44 ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DMATRIX assignment constructor
	// Arguments:
	//   other - The D3DMATRIX to copy
	// ----------------------------------------------------------------------------------
	Matrix( const D3DMATRIX& other ) : D3DXMATRIX( other ){}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   XMMATRIX assignment constructor
	// Arguments:
	//   other - The XMMATRIX to copy
	// ----------------------------------------------------------------------------------
	explicit Matrix( const XMMATRIX& other )
	{
		XMStoreFloat4x4( (XMFLOAT4X4*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Matrix to copy
	// ----------------------------------------------------------------------------------
	Matrix( const Matrix& other ) :
		D3DXMATRIX( other._11, other._12, other._13, other._14,
			        other._21, other._22, other._23, other._24,
					other._31, other._32, other._33, other._34,
					other._41, other._42, other._43, other._44 ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Access grant operator
	// Arguments:
	//   row - The row of the element to access
	//   col - The column of the element to access
	// Return Value:
	//   Reference to the element in position m[row][col]
	// ----------------------------------------------------------------------------------
	float& operator()( unsigned int row, unsigned int col ) 
	{ 
		return m[row][col]; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const access grant operator
	// Arguments:
	//   row - The row of the element to access
	//   col - The column of the element to access
	// Return Value:
	//   Copy of the element in position m[row][col]
	// ----------------------------------------------------------------------------------
	float operator()( unsigned int row, unsigned int col ) const 
	{ 
		return m[row][col]; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array conversion operator
	// Return Value:
	//   Pointer to the _11-component of the Matrix
	// ----------------------------------------------------------------------------------
	operator float*( void ) 
	{ 
		return &_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const float-array conversion operator
	// Return Value:
	//   Pointer to the _11-component of the Matrix
	// ----------------------------------------------------------------------------------
	operator const float*( void ) const 
	{ 
		return &_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const XMMATRIX conversion operator
	// Return Value:
	//   XMMATRIX constructed from the Matrix
	// ----------------------------------------------------------------------------------
	operator XMMATRIX() const 
	{ 
		return XMLoadFloat4x4( (const XMFLOAT4X4*)this );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMMATRIX assignment operator
	// Arguments:
	//   other - the XMMATRIX to assign to this Matrix
	// Return Value:
	//   Reference to this Matrix
	// ----------------------------------------------------------------------------------
	const Matrix& operator=( const XMMATRIX& other )
	{
		XMStoreFloat4x4( (XMFLOAT4X4*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Matrix to add to this Matrix
	// Return Value:
	//   This Matrix, after adding the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator+=( const Matrix& other )
	{
		_11 += other._11;
		_12 += other._12;
		_13 += other._13;
		_14 += other._14;

		_21 += other._21;
		_22 += other._22;
		_23 += other._23;
		_24 += other._24;

		_31 += other._31;
		_32 += other._32;
		_33 += other._33;
		_34 += other._34;

		_41 += other._41;
		_42 += other._42;
		_43 += other._43;
		_44 += other._44;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator
	// Arguments:
	//   other - The Matrix to subtract from this Matrix
	// Return Value:
	//   This Matrix, after subtracting off the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator-=( const Matrix& other )
	{
		_11 += other._11;
		_12 += other._12;
		_13 += other._13;
		_14 += other._14;

		_21 += other._21;
		_22 += other._22;
		_23 += other._23;
		_24 += other._24;

		_31 += other._31;
		_32 += other._32;
		_33 += other._33;
		_34 += other._34;

		_41 += other._41;
		_42 += other._42;
		_43 += other._43;
		_44 += other._44;

		return *this;
	}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Multiplication-assignment operator
	// Arguments:
	//   other - The Matrix by which to multiply this Matrix
	// Return Value:
	//   This Matrix, after multiplying by the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator*=( const Matrix& other )
	{
		D3DXMatrixMultiply( this, this, &other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Matrix
	// Return Value:
	//   This Matrix, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Matrix& operator*=( float f )
	{
		_11 *= f;
		_12 *= f;
		_13 *= f;
		_14 *= f;

		_21 *= f;
		_22 *= f;
		_23 *= f;
		_24 *= f;

		_31 *= f;
		_32 *= f;
		_33 *= f;
		_34 *= f;

		_41 *= f;
		_42 *= f;
		_43 *= f;
		_44 *= f;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   f - The scalar by which to divide this Matrix
	// Return Value:
	//   This Matrix, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Matrix& operator/=( float f )
	{
		const float fDiv = 1.0f / f;

		_11 *= fDiv;
		_12 *= fDiv;
		_13 *= fDiv;
		_14 *= fDiv;

		_21 *= fDiv;
		_22 *= fDiv;
		_23 *= fDiv;
		_24 *= fDiv;

		_31 *= fDiv;
		_32 *= fDiv;
		_33 *= fDiv;
		_34 *= fDiv;

		_41 *= fDiv;
		_42 *= fDiv;
		_43 *= fDiv;
		_44 *= fDiv;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Matrix
	// ----------------------------------------------------------------------------------
	Matrix operator+( void ) const 
	{ 
		return Matrix( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Matrix, with each component negated
	// ----------------------------------------------------------------------------------
	Matrix operator-( void ) const
	{
		return Matrix( -_11, -_12, -_13, -_14,
			           -_21, -_22, -_23, -_24,
					   -_31, -_32, -_33, -_34,
					   -_41, -_42, -_43, -_44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Matrix to add to this Matrix
	// Return Value:
	//   Matrix containing the sum of this Matrix and the other operand
	// ----------------------------------------------------------------------------------
	const Matrix operator+( const Matrix& other ) const
	{
		return Matrix( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Matrix to subtract from this Matrix
	// Return Value:
	//   Matrix containing the result of the subtraction of the other operand from 
	//   this Matrix
	// ----------------------------------------------------------------------------------
	const Matrix operator-( const Matrix& other ) const
	{
		return Matrix( *this ) -= other;
	}


	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Matrix by which to multiply this Matrix
	// Return Value:
	//   Matrix containing the product of this Matrix and the other operand
	// ----------------------------------------------------------------------------------
	const Matrix operator*( const Matrix& other ) const
	{
		return Matrix( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Matrix
	// Return Value:
	//   Matrix containing product of this Matrix and the scalar operand
	// ----------------------------------------------------------------------------------
	const Matrix operator*( float f ) const
	{
		return Matrix( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Matrix
	// Return Value:
	//   Matrix containing this Matrix divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Matrix operator/( float f ) const
	{
		return Matrix( *this ) /= f;
	}

	// Declare friend operator
	friend const Matrix operator*( float f, const Matrix& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Matrix with which to compare for equality
	// Return Value:
	//   true, if the two Matrices are component-wise equal
	//   false, if the two Matrices are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Matrix& other ) const
	{
		return ( 
			_11 == other._11 && _12 == other._12 && _13 == other._13 && _14 == other._14 &&
			_21 == other._21 && _22 == other._22 && _23 == other._23 && _24 == other._24 &&
			_31 == other._31 && _32 == other._32 && _33 == other._33 && _34 == other._34 &&
			_41 == other._41 && _42 == other._42 && _43 == other._43 && _44 == other._44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Matrix with which to compare for inequality
	// Return Value:
	//   true, if the two Matrices are not component-wise equal
	//   false, if the two Matrices are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Matrix& other ) const
	{
		return ( 
			_11 != other._11 || _12 != other._12 || _13 != other._13 || _14 != other._14 ||
			_21 != other._21 || _22 != other._22 || _23 != other._23 || _24 != other._24 ||
			_31 != other._31 || _32 != other._32 || _33 != other._33 || _34 != other._34 ||
			_41 != other._41 || _42 != other._42 || _43 != other._43 || _44 != other._44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const translation getter.  Gets the translational component of the Matrix as a
	//   reference to a const Vector3.
	// Return Value:
	//   Const reference to the fourth row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetTranslation( void ) const 
	{ 
		return *(const Vector3*)&_41; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//  Translation getter.  Gets the translational component of the Matrix as a
	//   reference to a Vector3.
	// Return Value:
	//   Reference to the fourth row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetTranslation( void ) 
	{ 
		return *(Vector3*)&_41; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//  Translation setter.  Sets the translational component of the Matrix with a
	//   Vector3.
	// ----------------------------------------------------------------------------------
	void SetTranslation( const Vector3* translation ) 
	{ 
		*((Vector3*)&_41) = *translation;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const x-axis getter.  Gets the x-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the first row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetX( void ) const 
	{ 
		return *(const Vector3*)&_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   X-axis getter.  Gets the x-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the first row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetX( void ) 
	{ 
		return *(Vector3*)&_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const y-axis getter.  Gets the y-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the second row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetY( void ) const 
	{ 
		return *(const Vector3*)&_21; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Y-axis getter.  Gets the y-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the second row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetY( void ) 
	{ 
		return *(Vector3*)&_21; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const z-axis getter.  Gets the z-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the third row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetZ( void ) const 
	{ 
		return *(const Vector3*)&_31; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Z-axis getter.  Gets the z-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the third row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetZ( void ) 
	{ 
		return *(Vector3*)&_31; 
	}
};

// --------------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Matrix
//   other - The Matrix to scale
// Return Value:
//   Matrix containing product of the Matrix and the scalar operand
// --------------------------------------------------------------------------------------
inline const Matrix operator*( float f, const Matrix& other )
{
	return Matrix( other ) *= f;
}

inline Vector4 operator*( Vector4 p, const Matrix& m )
{
	return Vector4( 
		p.x * m._11 + p.y * m._21 + p.z * m._31 + p.w * m._41,
		p.x * m._12 + p.y * m._22 + p.z * m._32 + p.w * m._42,
		p.x * m._13 + p.y * m._23 + p.z * m._33 + p.w * m._43,
		p.x * m._14 + p.y * m._24 + p.z * m._34 + p.w * m._44
		);
}

inline Vector4 operator*( const Matrix& m, Vector4 p )
{
	return Vector4( 
		m._11 * p.x + m._12 * p.y + m._13 * p.z + m._14 * p.w,
		m._21 * p.x + m._22 * p.y + m._23 * p.z + m._24 * p.w,
		m._31 * p.x + m._32 * p.y + m._33 * p.z + m._34 * p.w,
		m._41 * p.x + m._42 * p.y + m._43 * p.z + m._44 * p.w
		);
}

#else

#include "CcpMath/include/Matrix.h"

#endif

inline bool IsMatch( Be::Var* value, const Matrix& t )
{
	return (Be::Var*)&t == value;
}

#endif // MATRIX_H