////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#pragma once

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_INTERFACE( IEveVolume ) : public IRoot
{
	virtual float GetIntensity( Vector3 position ) = 0;
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Color& baseColor = 0xFFFFFFFF) = 0;
	virtual const CcpMath::Sphere GetBoundingSphere() const = 0;
};
BLUE_DECLARE_IVECTOR( IEveVolume );
