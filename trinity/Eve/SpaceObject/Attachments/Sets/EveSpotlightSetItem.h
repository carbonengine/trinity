// Copyright © 2012 CCP ehf.

#pragma once
#ifndef EveSpotlightSetItem_H
#define EveSpotlightSetItem_H



BLUE_DECLARE( EveSpotlightSetItem );
BLUE_DECLARE_VECTOR( EveSpotlightSetItem );

// --------------------------------------------------------------------------------
// Description:
//   This class holds individual spotlight data, such as transform, colors and scales.
// SeeAlso:
//   EveSpotlightSet, EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpotlightSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSpotlightSetItem( IRoot* lockobj = NULL );

	CcpMath::AxisAlignedBox GetBounds() const;
	int32_t GetBoneIndex() const;

	// data
	BlueSharedString m_name;
	Matrix m_transform;

	// 1st value is the uniform glow scale the 2nd and 3rd are the flare scale in 2D
	Vector3 m_spriteScale;

	Color m_coneColor;
	Color m_spriteColor;
	Color m_flareColor;

	// animation granny parent bone index
	int32_t m_boneIndex;

	// booster gain (aka ship speed) dependent
	bool m_boosterGainInfluence;
};

TYPEDEF_BLUECLASS( EveSpotlightSetItem );

#endif // EveSpotlightSetItem_H