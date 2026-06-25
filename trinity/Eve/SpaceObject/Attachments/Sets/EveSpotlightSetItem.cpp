// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "EveSpotlightSetItem.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to invalid/empty
// --------------------------------------------------------------------------------
EveSpotlightSetItem::EveSpotlightSetItem( IRoot* lockobj ) :
	m_spriteScale( 1.0f, 1.0f, 1.0f ),
	m_coneColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_spriteColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_flareColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_boneIndex( 0 ),
	m_boosterGainInfluence( false ),
	m_transform( IdentityMatrix() )
{
}

CcpMath::AxisAlignedBox EveSpotlightSetItem::GetBounds() const
{
	CcpMath::AxisAlignedBox aabb( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) );
	aabb.Transform( m_transform );
	return aabb;
}

int32_t EveSpotlightSetItem::GetBoneIndex() const
{
	return m_boneIndex;
}
