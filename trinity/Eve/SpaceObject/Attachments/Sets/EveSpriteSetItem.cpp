#include "StdAfx.h"
#include "EveSpriteSetItem.h"


EveSpriteSetItem::EveSpriteSetItem( IRoot* lockobj ) :
	m_position( 0.0f, 0.0f, 0.0f ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_warpColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_blinkRate( 0.1f ),
	m_blinkPhase( 0.0f ),
	m_minScale( 1.0f ),
	m_maxScale( 10.0f ),
	m_falloff( 0.f ),
	m_boneIndex( 0 )
{
}

EveSpriteSetItem& EveSpriteSetItem::operator=( const EveSpriteSetItem& other )
{
	m_name = other.m_name;
	m_position = other.m_position;
	m_blinkRate = other.m_blinkRate;
	m_blinkPhase = other.m_blinkPhase;
	m_minScale = other.m_minScale;
	m_maxScale = other.m_maxScale;
	m_falloff = other.m_falloff;
	m_color = other.m_color;
	m_boneIndex = other.m_boneIndex;

	return *this;
}

CcpMath::Sphere EveSpriteSetItem::GetBounds() const
{
	return CcpMath::Sphere( m_position, m_maxScale );
}

int32_t EveSpriteSetItem::GetBoneIndex() const
{
	return m_boneIndex;
}
