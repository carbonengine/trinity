// Copyright © 2026 Fenris Creations ehf.

#pragma once

#include "Eve/EveComponentRegistry.h"

class TriFrustum;
class Tr2QuadRenderer;
struct ITr2Renderable;

// Renderables that reach outside their owner's bounds (f.ex. a turret firing effect hitting its target).
// The scene collects them directly, so they draw whether the owner is visible, culled or impostored.
// Each one does its own frustum test.
struct IEveUnboundedRenderable
{
	virtual void GetUnboundedRenderables( std::vector<ITr2Renderable*>& renderables ) = 0;
	virtual void AddUnboundedQuads( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const = 0;
};

REGISTER_COMPONENT_TYPE( "UnboundedRenderable", IEveUnboundedRenderable );
