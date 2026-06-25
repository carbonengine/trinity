// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriView_h
#define TriView_h


// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed wrapper around a view matrix.
// See Also:
//   TriProjection
// --------------------------------------------------------------------------------------
BLUE_CLASS( TriView ) :
	public IRoot
{
public:
	// Constructor
	TriView( void );

	EXPOSE_TO_BLUE();

	// Set the view matrix
	void SetTransform( const Matrix& m );
	// Get the view matrix
	const Matrix& GetTransform() const;

	// Construct a look-at matrix
	void SetLookAtPosition( const Vector3& eye, const Vector3& at, const Vector3& up );

private:
	// The wrapped view matrix
	Matrix m_transform;
};

TYPEDEF_BLUECLASS( TriView );

#endif