////////////////////////////////////////////////////////////
//
//    Created:   Januaru 2016
//    Copyright: CCP 2016
//
#pragma once
#ifndef EveSpriteLineSetItem_H
#define EveSpriteLineSetItem_H

// forwards
BLUE_DECLARE( EveSpriteLineSetItem );
BLUE_DECLARE_VECTOR( EveSpriteLineSetItem );

// --------------------------------------------------------------------------------
// Description:
//   This class holds all the necesarry data for one line set, rendered and
//   maneged by EveLineSet
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpriteLineSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSpriteLineSetItem( IRoot* lockobj = NULL );

	CcpMath::Sphere GetBounds() const;
	int32_t GetBoneIndex() const;

	// data
	BlueSharedString m_name;
	bool m_isCircle;
	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scaling;
	float m_spacing;
	float m_blinkRate;
	float m_blinkPhase;
	float m_blinkPhaseShift;
	float m_minScale;
	float m_maxScale;
	float m_falloff;
	Color m_color;
	int32_t m_boneIndex;
};

TYPEDEF_BLUECLASS( EveSpriteLineSetItem );

#endif // EveSpriteLineSetItem_H