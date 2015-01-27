#pragma once
#ifndef Tr2InteriorFlareData_H
#define Tr2InteriorFlareData_H

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorFlareData represents a per- flare image data for interior flares. It 
//   contains the data that is passed to the flare shader.
// See Also:
//   Tr2InteriorFlare
// --------------------------------------------------------------------------------------
class Tr2InteriorFlareData:
	public IRoot
{
public:
	Tr2InteriorFlareData( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

	// Proportion of vector from screen center to
	// flare position on the screen where this particular
	// flare image appears (could be negative). For example
	// (1, 1) would be flare position, (0, 0) - screen center.
	Vector2 m_positionWeight;
	// Flare size (in post-projection space)
	Vector2 m_size;
	// Offset of the flare image in texture atlas (from 0 to 1)
	Vector2 m_textureOffset;
	// Size of the flare image in texture atlas (from 0 to 1)
	Vector2 m_textureSize;
	// Rotation speed
	bool m_rotation;
	// Directional stretch
	Vector2 m_directionalStretch;
	// Edge fade
	float m_edgeFadeDistance;
	// Center fade min radius
	float m_centerFadeMinRadius;
	// Center fade max radius
	float m_centerFadeMaxRadius;
};

TYPEDEF_BLUECLASS( Tr2InteriorFlareData );
BLUE_DECLARE_VECTOR( Tr2InteriorFlareData );

#endif