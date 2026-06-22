// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "EveHazeSetItem.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveHazeSetItem::EveHazeSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_hazeData( 4.0, 0.2, 2.0, 0.0 ),
	m_boneIndex( 0 )
{
}

CcpMath::AxisAlignedBox EveHazeSetItem::GetBounds() const
{
	CcpMath::AxisAlignedBox aabb( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 5.0f ) );
	aabb.Transform( TransformationMatrix( m_scaling, m_rotation, m_position ) );
	return aabb;
}

int32_t EveHazeSetItem::GetBoneIndex() const
{
	return m_boneIndex;
}
