////////////////////////////////////////////////////////////
//
//    Created:   Januaru 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"

#include "EveSpriteLineSetItem.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSpriteLineSetItem::EveSpriteLineSetItem( IRoot* lockobj ) :
	m_isCircle( false ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_spacing( 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_blinkRate( 0.1f ),
	m_blinkPhase( 0.f ),
	m_blinkPhaseShift( 0.f ),
	m_minScale( 1.f ),
	m_maxScale( 10.f ),
	m_falloff( 0.f ),
	m_boneIndex( 0 )
{
}

CcpMath::Sphere EveSpriteLineSetItem::GetBounds() const
{
	if( m_isCircle == false )
	{
		Matrix m = RotationMatrix( m_rotation );
		Vector3 dir = TransformNormal( Vector3( 1.f, 0.f, 0.f ), m );

		size_t numOfSprites = size_t( m_scaling.x );
		auto endPos = m_position + m_spacing * dir * float( numOfSprites );
		return CcpMath::Sphere( ( m_position + endPos ) * 0.5f, numOfSprites * m_spacing * 0.5f );
	}
	else
	{
		return CcpMath::Sphere( m_position, std::max( m_scaling.x, m_scaling.y ) );
	}
}

int32_t EveSpriteLineSetItem::GetBoneIndex() const
{
	return m_boneIndex;
}
