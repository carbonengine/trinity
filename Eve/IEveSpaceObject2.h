#ifndef IEveSpaceObject2_H
#define IEveSpaceObject2_H

#include "EveLODHelper.h"

struct ITr2Renderable;
struct ViewDistanceInfo;
class TriFrustum;
class Tr2RenderContext;
class EveUpdateContext;
class Tr2QuadRenderer;
class Tr2LightManager;

BLUE_INTERFACE( IEveSpaceObject2 ) : public IRoot
{
	virtual void UpdateSyncronous( EveUpdateContext& updateContext ) = 0;
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext ) = 0;
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext ) = 0;
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;

	// This version of the function should perform an update on the model / ball position
	virtual void GetModelCenterWorldPosition( Vector3 &position, Be::Time t ) = 0;

	// This version of the function should not update the object
	virtual void GetCurrentModelCenterWorldPosition( Vector3 &position ) = 0;

	// If possible, return an AABB in local coordinates
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) = 0;
	// Get the local to world transform
	virtual void GetLocalToWorldTransform( Matrix &transform ) const = 0;

	// Registers an object and its attachments with the quad renderer
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}
	// Adds quads from space object and its attachments to the quad renderer. ATTENTION: this function is called in-parallel
	virtual void AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}

	virtual void GetLights( Tr2LightManager& lightManager ) const {}
};

BLUE_DECLARE_IVECTOR( IEveSpaceObject2 );

#endif