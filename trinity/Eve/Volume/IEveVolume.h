////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#pragma once

#ifndef IEveVolume_H
#define IEveVolume_H

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_INTERFACE( IEveVolume ) : public IRoot
{
	virtual float GetIntensity( Vector3 position ) = 0;
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform ) = 0;
	virtual Vector4 GetBoundingSphere() const = 0;
	virtual void RegisterForChanges( std::function<void()> NotifyParent ) = 0;
};
BLUE_DECLARE_IVECTOR( IEveVolume );

#endif