// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2BoneMatrixCurve.h"

// --------------------------------------------------------------------------------------
// Description:
//   Compares two matrix keys based on the key time of each key.
// Arguments:
//   context - Unused
//   a - First matrix key to compare
//   b - Second matrix key to compare
// Return Value:
//   true, if key a's time is less than key b's time
//   false, otherwise
// --------------------------------------------------------------------------------------
static bool CompareKeys( IRoot* context, Tr2MatrixKey* a, Tr2MatrixKey* b )
{
	return a->m_time < b->m_time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BoneMatrixCurve constructor.  Sets the curve to a default state.
// --------------------------------------------------------------------------------------
Tr2BoneMatrixCurve::Tr2BoneMatrixCurve( IRoot* lockobj ) :
	Tr2Curve<Tr2MatrixKey, PTr2MatrixKeyVector, Matrix>( lockobj ),
	m_cachedJoint( ~0u ),
	m_skeletonTag( ~0u )
{
	m_interpolation = SPHERICAL_LINEAR;
	Matrix m( XMMatrixIdentity() );
	m_startValue = m;
	m_endValue = m;
	m_currentValue = m;

	// use settings that make it do something even if there's no keys
	m_cycle = true;
	m_length = 1.0f;

	m_transform = XMMatrixIdentity();
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the name of the bone and resets the cached joint and skeleton tags to the
//   default value.
// Arguments:
//   bone - Name of the bone
// --------------------------------------------------------------------------------------
void Tr2BoneMatrixCurve::SetBone( const std::string& bone )
{
	m_bone = bone;
	m_cachedJoint = m_skeletonTag = ~0u;
}

// --------------------------------------------------------------------------------------
// Description:
//   Private utility function to add a matrix key at the specified time.
// Arguments:
//   time - The time at which to add the matrix key
//   value - The matrix key value
// --------------------------------------------------------------------------------------
void Tr2BoneMatrixCurve::AddKey_( float time, const Matrix& value )
{
	Tr2MatrixKeyPtr key;
	if( !key.CreateInstance() )
	{
		return;
	}

	key->m_time = time;
	key->m_value = value;
	key->m_interpolation = (Interpolation)m_interpolation;
	m_keys.Insert( -1, key );
}

// --------------------------------------------------------------------------------------
// Description:
//   Sorts the matrix keys based on key time.
// --------------------------------------------------------------------------------------
void Tr2BoneMatrixCurve::Sort( void )
{
	if( !m_keys.empty() )
	{
		if( m_keys.size() > 1 )
		{
			m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
		}
		// We might have added a key passed the length of the curve
		if( m_keys.back()->m_time > m_length )
		{
			Tr2MatrixKey* back = m_keys.back();
			float preLength = m_length;
			Matrix endValue = m_endValue;

			m_length = back->m_time;
			m_endValue = back->m_value;
			if( preLength > 0.0f )
			{
				back->m_time = preLength;
				back->m_value = endValue;
			}
		}
	}
	m_lastKey = nullptr;
	m_nextKey = nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Supposedly interpolates between two keys.  This function doesn't actually do that,
//   though.  If there is no skinned object or bone, it returns the identity matrix.
//   If there is a skinned object, this multiplies the bone transform for the cached
//   joint by the skinned object's world transform.
// Arguments:
//   out - Output matrix parameter, holds the results of the 'interpolation'
//   startKey - Unused
//   endKey - Unused
// Return Value:
//   The matrix result of the 'interpolation'
// --------------------------------------------------------------------------------------
Matrix* Tr2BoneMatrixCurve::Interpolate( Matrix* out, Tr2MatrixKey* /*startKey*/, Tr2MatrixKey* /*endKey*/ )
{
	if( !m_skinnedObject || m_bone.empty() )
	{
		*out = XMMatrixIdentity();
		return out;
	}

	if( m_skeletonTag != m_skinnedObject->GetSkeletonTag() )
	{
		m_cachedJoint = m_skinnedObject->GetBoneIndex( m_bone );
		if( m_cachedJoint != ~0u )
		{
			m_skeletonTag = m_skinnedObject->GetSkeletonTag();
		}
	}

	const Matrix* m = m_skinnedObject->GetBoneTransform( m_cachedJoint );
	if( m )
	{
		// We've got the bone in local space, now we need to multiply it
		// by our skinned object's world transform.
		*out = XMMatrixMultiply( XMMatrixMultiply( m_transform, *m ), m_skinnedObject->GetTransform() );
	}
	else
	{
		*out = XMMatrixIdentity();
	}
	return out;
}