// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveSpriteSetItem_H
#define EveSpriteSetItem_H


BLUE_DECLARE( EveSpriteSetItem );
BLUE_DECLARE_VECTOR( EveSpriteSetItem );

BLUE_CLASS( EveSpriteSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSpriteSetItem( IRoot* lockobj = NULL );

	EveSpriteSetItem& operator=( const EveSpriteSetItem& other );

	CcpMath::Sphere GetBounds() const;
	int32_t GetBoneIndex() const;

	// data
	BlueSharedString m_name;
	Vector3 m_position;
	float m_blinkRate;
	float m_blinkPhase;
	float m_minScale;
	float m_maxScale;
	float m_falloff;
	Color m_color;
	Color m_warpColor;
	int32_t m_boneIndex;
};

TYPEDEF_BLUECLASS( EveSpriteSetItem );

#endif // EveSpriteSetItem_H