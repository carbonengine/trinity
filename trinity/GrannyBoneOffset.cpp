// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#ifndef GAMEWORLD_64

#include "GrannyBoneOffset.h"

GrannyBoneOffset::GrannyBoneOffset( IRoot* /*lockobj*/ ) :
	m_transforms( "GrannyBoneOffset/m_transforms" ), m_riggedTransforms( "GrannyBoneOffset/m_riggedTransforms" )
{
}

GrannyBoneOffset::~GrannyBoneOffset()
{
}

bool GrannyBoneOffset::Initialize()
{
	return true;
}

bool GrannyBoneOffset::HaveTransforms() const
{
	return !m_transforms.empty();
}

// Description:
//  check if the number of bones has changed since the last rebind.
// Arguments:
//   numBones - number of bones in the current animation rig.
bool GrannyBoneOffset::NeedRebind( unsigned numBones ) const
{
	// yes if we have transforms and the skeleton rig doesn't match up
	return m_riggedTransforms.size() != numBones && HaveTransforms();
}

void GrannyBoneOffset::ClearRigBindings()
{
	m_riggedTransforms.clear();
}

void GrannyBoneOffset::ClearTransforms()
{
	m_transforms.clear();
	ClearRigBindings();
}

// Description:
//	Apply stored offset and/or rotation to specific bone whose rig index is joint.
// Arguments:
//  targetMatrix - destination for final collapsed bone transform, 16 floats
//  joint - index in the animation rig of the joint whose transform we're computing
//  boneMatrix - local transform of the joint, taken from the animation data, 16 floats
//  parentMatrix - collapsed parent transform of the joint, 16 floats
// Return Value:
//	true if there was an offset/rotation, and targetMatrix has been filled in properly.
//  false otherwise and then it's up to the caller to make sure targetMatrix is set up.
bool GrannyBoneOffset::Apply( float* targetMatrix, unsigned joint, const float* boneMatrix, const float* parentMatrix )
{
	if( const float* const t = m_riggedTransforms[joint] )
	{
		// apply the rotation in bone space, so eg. the head tilts instead of swerving around the origin in bind pose space
		// so, pre-multiply, before the animation transform
		float m[16];

		for( unsigned j = 0; j != 3; ++j ) // 3 rows only
		{
			for( unsigned i = 0; i != 4; ++i )
			{
				float f = 0;
				for( unsigned k = 0; k != 4; ++k )
				{
					f += t[j * 4 + k] * boneMatrix[k * 4 + i];
				}
				m[j * 4 + i] = f;
			}
		}

		m[12] = boneMatrix[12] + t[12];
		m[13] = boneMatrix[13] + t[13];
		m[14] = boneMatrix[14] + t[14];
		m[15] = 1;

		for( unsigned j = 0; j != 4; ++j )
		{
			for( unsigned i = 0; i != 4; ++i )
			{
				float f = 0;
				for( unsigned k = 0; k != 4; ++k )
				{
					f += m[j * 4 + k] * parentMatrix[k * 4 + i];
				}
				targetMatrix[j * 4 + i] = f;
			}
		}
		return true;
	}

	return false;
}

// Description:
//	Map bone strings to joint indices using this skeleton
// Arguments:
//  bones - pointer to exactly numBones strings
//  numBones - number of bones in this rig
void GrannyBoneOffset::BindToRig( const std::string* bones, size_t numBones )
{
	if( !bones || !numBones )
	{
		return;
	}

	m_riggedTransforms.resize( numBones, NULL );
	for( size_t i = 0; i != numBones; ++i )
	{
		TransformsMap::const_iterator it = m_transforms.find( bones[i] );
		if( it != m_transforms.end() )
		{
			m_riggedTransforms[i] = it->second.m;
		}
	}
}

// Description:
//	Set or replace the additional rotation for this bone.
// Arguments:
//	bone - Name of the bone in the animation rig.  If it doesn't exist (at all, or in current lod) then that's fine.
//  r, i, j, k - Rotation quaternion; applied before the animation's transform.
// SeeAlso:
//	SetOffset
void GrannyBoneOffset::SetRotation( const std::string& bone, float r, float i, float j, float k )
{
	if( bone.empty() )
	{
		return;
	}

	float* m = m_transforms[bone].m;

	Matrix mq;
	Quaternion q;
	q.x = r;
	q.y = i;
	q.z = j;
	q.w = k;
	mq = RotationMatrix( q );

	memcpy( m, &mq.m[0][0], 64 );

	ClearRigBindings();
}

// Description:
//	Set or replace the additional translation for this bone.
// Arguments:
//	bone - Name of the bone in the animation rig.  If it doesn't exist (at all, or in current lod) then that's fine.
//  x, y, z - Displacement; applied in the local coordinate frame of the parent bone, after the animation's transform.
// SeeAlso:
//	SetRotation
void GrannyBoneOffset::SetOffset( const std::string& bone, float x, float y, float z )
{
	if( bone.empty() )
	{
		return;
	}

	TransformsMap::iterator it = m_transforms.find( bone );
	if( it == m_transforms.end() )
	{
		float* m = m_transforms[bone].m;
		memset( m, 0, 16 * 4 );
		m[0] = m[5] = m[10] = m[15] = 1.0f;
		it = m_transforms.find( bone );
	}

	float* m = m_transforms[bone].m;
	m[12] = x;
	m[13] = y;
	m[14] = z;

	ClearRigBindings();
}

bool GrannyBoneOffset::ApplyToLocal( unsigned joint, Quaternion& rotation, Vector3& position ) const
{
	if( const float* const t = m_riggedTransforms[joint] )
	{
		Matrix matrix = *reinterpret_cast<const Matrix*>( t );
		Vector3 offsetScale;
		Quaternion offsetRotation;
		Vector3 offsetPosition;
		Decompose( offsetScale, offsetRotation, offsetPosition, matrix );

		rotation = offsetRotation * rotation;
		position = position + offsetPosition;
		return true;
	}

	return false;
}

#endif /* GAMEWORLD_64 */
