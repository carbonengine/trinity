// Copyright © 2020 CCP ehf.

#pragma once

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

// Debug colors shared by the volume shapes. The outline stays faintly visible where the volume is hidden
// behind other geometry, and the fill is a faint tint that also carries through to whatever is inside it.
inline Tr2DebugColor EveVolumeDebugColor( const Color& color )
{
	Color occluded = color;
	occluded.a *= 0.25f;
	return Tr2DebugColor( color, occluded );
}

inline Tr2DebugColor EveVolumeFillColor( const Color& color )
{
	Color fill = color;
	fill.a *= 0.15f;
	return EveVolumeDebugColor( fill );
}

// The debug renderer culls back faces, so a solid shape is invisible from inside. Drawing it a second time
// with this transform mirrors it, which reverses the winding so the inner faces render as well: together the
// two draws are a two-sided solid.
inline Matrix EveVolumeInsideOut( const Matrix& transform )
{
	return ScalingMatrix( -1.0f, 1.0f, 1.0f ) * transform;
}

BLUE_INTERFACE( IEveVolume ) :
	public IRoot
{
	virtual float GetIntensity( Vector3 position ) = 0;
	virtual uint32_t RegisterForChanges( const std::function<void()>& callBack ) = 0; // returns the callbackID
	virtual void UnregisterForChanges( uint32_t callbackID ) = 0;
	// GeneratePointsFromOuterVolume : returns N points in volume with directions facing the closest outward surface
	virtual void GeneratePointsInVolume( std::vector<Vector3> & points, size_t howManyToAdd, bool excludeInnerVolume, float fallOffFactor ) = 0;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Color& baseColor = 0xFFFFFFFF ) = 0;
	virtual const CcpMath::Sphere GetBoundingSphere() const = 0;
};
BLUE_DECLARE_IVECTOR( IEveVolume );
