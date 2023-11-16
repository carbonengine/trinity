#pragma once

#ifndef Tr2BoneMatrixCurve_h
#define Tr2BoneMatrixCurve_h

#include "include/Tr2Curve.h"
#include "Tr2SkinnedObject.h"

// --------------------------------------------------------------------------------------
// Description:
//   This is a Matrix interpolation key for a bone matrix curve.
// See Also:
//   Tr2Key, Matrix, Tr2BoneMatrixCurve
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2MatrixKey ):
	public IRoot,
	public Tr2Key<Matrix>
{
public:
	// Contructor
	Tr2MatrixKey( IRoot* lockobj = NULL ) {}

	// Blue exposure
	EXPOSE_TO_BLUE();
};
BLUE_DECLARE_VECTOR( Tr2MatrixKey );

// --------------------------------------------------------------------------------------
// Description:
//   This is an interpolation curve for skinning matrices.
// See Also:
//   Tr2MatrixKey
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2BoneMatrixCurve ):
	public Tr2Curve<Tr2MatrixKey, PTr2MatrixKeyVector, Matrix>
{
public:
	// Constructor
	Tr2BoneMatrixCurve( IRoot* lockobj = NULL );

	// Blue exposure
	EXPOSE_TO_BLUE();

	// Sorts the matrix keys
	void Sort( void );
	// Gets the world-transformed bone matrix from the skinned object
	Matrix* Interpolate( Matrix* out, Tr2MatrixKey* startKey, Tr2MatrixKey* endKey );

	// Pointer to the skinned object
	Tr2SkinnedObjectPtr	m_skinnedObject;

	// Sets the name of the target bone
	void SetBone( const std::string& bone );
	// Gets the name of the target bone
	const std::string& GetBone() const { return m_bone; }

private:
	std::string			m_bone;
	unsigned			m_cachedJoint;
	unsigned			m_skeletonTag;
	// Additional transform in bone's local space
	Matrix				m_transform;

	void AddKey_( float time, const Matrix& value );
};

TYPEDEF_BLUECLASS( Tr2BoneMatrixCurve );
TYPEDEF_BLUECLASS( Tr2MatrixKey );
#endif //Tr2BoneMatrixCurve_h
