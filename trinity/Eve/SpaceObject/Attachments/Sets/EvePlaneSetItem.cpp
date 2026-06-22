// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "EvePlaneSetItem.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EvePlaneSetItem::EvePlaneSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_layer1Transform( 1.f, 1.f, 0.f, 0.f ),
	m_layer2Transform( 1.f, 1.f, 0.f, 0.f ),
	m_layer1Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_boneIndex( 0 ),
	m_maskAtlasID( 0 ),
	m_blinkData( 1.f, 0.f, 1.f, 0.f )
{
}

CcpMath::AxisAlignedBox EvePlaneSetItem::GetBounds() const
{
	CcpMath::AxisAlignedBox aabb( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) );
	aabb.Transform( TransformationMatrix( m_scaling, m_rotation, m_position ) );
	return aabb;
}

int32_t EvePlaneSetItem::GetBoneIndex() const
{
	return m_boneIndex;
}
