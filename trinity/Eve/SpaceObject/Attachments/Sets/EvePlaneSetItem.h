////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EvePlaneSetItem_H
#define EvePlaneSetItem_H

#include "ITr2GeometryProvider.h"

// forwards
BLUE_DECLARE( EvePlaneSetItem );
BLUE_DECLARE_VECTOR( EvePlaneSetItem );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EvePlaneSetItem ) :
	public IRoot
	
{
public:
	EXPOSE_TO_BLUE();

	EvePlaneSetItem( IRoot* lockobj = NULL );

	CcpMath::AxisAlignedBox GetBounds() const;
	int32_t GetBoneIndex() const;
	
	// name
	BlueSharedString m_name;
	// positional attributes
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;
	// appearance
	Color m_color;
	Vector4 m_layer1Transform;
	Vector4 m_layer2Transform;
	// animation
	Vector4 m_layer1Scroll;
	Vector4 m_layer2Scroll;
	// animation granny parent bone index
	int32_t m_boneIndex;
	// supports atlasing
	uint32_t m_maskAtlasID;
	// Blink data - [rate, phase, dutyCycle, blinkMode]
	Vector4 m_blinkData;
};

TYPEDEF_BLUECLASS( EvePlaneSetItem );

#endif // EvePlaneSetItem_H