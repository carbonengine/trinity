// Copyright © 2017 CCP ehf.

#pragma once
#ifndef EveHazeSetItem_H
#define EveHazeSetItem_H

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveHazeSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveHazeSetItem( IRoot* lockobj = NULL );

	CcpMath::AxisAlignedBox GetBounds() const;
	int32_t GetBoneIndex() const;

	// name
	BlueSharedString m_name;
	// positional attributes
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;
	// appearance
	Vector4 m_hazeData;
	Color m_color;
	// animation granny parent bone index
	int32_t m_boneIndex;
};

TYPEDEF_BLUECLASS( EveHazeSetItem );
BLUE_DECLARE_VECTOR( EveHazeSetItem );

#endif // EveHazeSetItem_H