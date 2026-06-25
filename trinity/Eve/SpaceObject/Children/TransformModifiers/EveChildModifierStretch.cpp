// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierStretch.h"
#include "Include/TriMath.h"

EveChildModifierStretch::EveChildModifierStretch( IRoot* lockobj ) :
	m_destPosition( 0, 0, 0 )
{
}

EveChildModifierStretch::~EveChildModifierStretch()
{
}

void EveChildModifierStretch::SetDest( ITriVectorFunction* dest )
{
	m_dest = dest;
}

void EveChildModifierStretch::SetDestPosition( Vector3 destPosition )
{
	m_destPosition = destPosition;
}

Matrix EveChildModifierStretch::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	Vector3 start, diff, dir;
	Vector3 end = m_destPosition;

	Vector3 sourceTranslation, sourceScale;
	Quaternion sourceRotation;

	Decompose( sourceScale, sourceRotation, sourceTranslation, transform );
	start = transform.GetTranslation();

	Be::Time now = BeOS->GetCurrentFrameTime();
	if( m_dest )
	{
		m_dest->GetValueAt( &end, now );
	}

	diff = end - start;
	dir = Normalize( diff );
	Quaternion rotation = IdentityQuaternion();
	TriQuaternionArcFromForward( &rotation, &dir );

	float length = Length( diff );
	Vector3 scale = Vector3( sourceScale.x, sourceScale.y, length );

	return RotationMatrix( sourceRotation ) * TransformationMatrix( scale, rotation, start + diff / 2.0f );
}