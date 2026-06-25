// Copyright © 2020 CCP ehf.

#pragma once

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

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
