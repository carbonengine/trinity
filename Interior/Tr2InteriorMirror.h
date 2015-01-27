#pragma once

#ifndef Tr2InteriorMirror_H
#define Tr2InteriorMirror_H

#include "Vector3.h"

// Forward declarations
struct ITr2Renderable;
class Tr2InteriorPlaceable;

class Tr2InteriorMirror
{
public:
	Tr2InteriorMirror();
	virtual ~Tr2InteriorMirror();

	// Get the mesh index
	int GetMeshIndex( void ) const;
	// Get the mesh area index
	int GetAreaIndex( void ) const;
	// Get the warp matrix for the front of the mirror
	const Matrix& GetWarpMatrixFront( void ) const;
	// Get the warp matrix for the 'back' of the mirror
	const Matrix& GetWarpMatrixBack( void ) const;
	// Get the transformation matrix for the mirror
	const Matrix& GetTransformMatrix( void ) const;

	// Set the hosting placeable
	void SetPlaceable( Tr2InteriorPlaceable* placeable );
	// Get the hosting placeable
	Tr2InteriorPlaceable* GetPlaceable( void ) const;

	// Get the mirror index
	int GetMirrorIndex( void ) const;
	// Set the mirror index
	void SetMirrorIndex( int index );

	// Set the mesh index
	void SetMeshIndex( int index );
	// Set the area index
	void SetAreaIndex( int index );
	// Set the warp matrix for the front of the mirror
	void SetWarpMatrixFront( const Matrix& warpMatrix );
	// Set the warp matrix for the back of the mirror
	void SetWarpMatrixBack( const Matrix& warpMatrix );
	// Set the transformation matrix for the mirror
	void SetTransformMatrix( const Matrix& transformMatrix );
	// Set the mirror bounding box
	void SetBoundingBox( const Vector3& minBounds, const Vector3& maxBounds );
protected:
	// Warp matrix for the front of the mirror
	Matrix m_warpMatrixFront;
	// Warp matrix for the 'back' of the mirror
	Matrix m_warpMatrixBack;

	// Transformation matrix for the mirror
	Matrix m_transformMatrix;
	
	// Mesh index
	int m_meshIndex;
	// Area index
	int m_areaIndex;
	// Mirror index
	int m_mirrorIndex;

	// Bounding box for mirror model
	Vector3 m_minBounds;
	Vector3 m_maxBounds;

	// The hosting placeable
	Tr2InteriorPlaceable* m_placeable;
};
TYPEDEF_BLUECLASS( Tr2InteriorMirror );

#endif