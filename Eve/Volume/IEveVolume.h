#pragma once

#ifndef IEveVolume_H
#define IEveVolume_H

#include "StdAfx.h"

class Tr2DebugRenderer;

BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_INTERFACE( IEveVolume ) : public IRoot
{
	virtual float GetIntensity( Vector3 cameraPosition ) = 0;
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform ) = 0;
	virtual Vector4 GetBoundingSphere() const = 0;
	virtual void RegisterForChanges( std::function<void()> NotifyParent ) = 0;
};
BLUE_DECLARE_IVECTOR( IEveVolume );

#endif