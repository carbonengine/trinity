// Copyright © 2023 CCP ehf.

#ifndef IEveSpaceObject2_H
#define IEveSpaceObject2_H

#include "EveLODHelper.h"
#include "Eve/EveUpdateContext.h"

struct ITr2Renderable;
struct ViewDistanceInfo;
struct EveSpaceObjectVSData;
struct EveSpaceObjectPSData;
class TriFrustum;
class Tr2RenderContext;
class EveUpdateContext;
class Tr2QuadRenderer;
class Tr2LightManager;
class Tr2ImpostorManager;

BLUE_INTERFACE( IEveSpaceObject2 ) :
	public IRoot
{

	// public struct to pass this info to children
	struct ParentData
	{
		Matrix transform = IdentityMatrix();
		uint32_t killCount = 0;
		Vector4 shipData = Vector4( 0, 0, 0, 0 );
		Vector3 clipSphereCenter = Vector3( 0, 0, 0 );
		float clipRadiusSq = 0;
		float clipRadius2Sq = 0;
		float clipFactor = 0;
		float clipFactor2 = 0;
		const Vector4* shLighting = nullptr;
		Vector4 customData = Vector4( 0, 0, 0, 0 );
	};

	virtual void UpdateSyncronous( const EveUpdateContext& updateContext ) = 0;
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext ) = 0;
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors ) = 0;
	virtual bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const = 0;

	// This version of the function should perform an update on the model / ball position
	virtual void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t ) = 0;

	// This version of the function should not update the object
	virtual void GetModelCenterWorldPosition( Vector3 & position ) const = 0;

	// If possible, return an AABB in local coordinates
	virtual bool GetLocalBoundingBox( Vector3 & min, Vector3 & max ) = 0;
	// Get the local to world transform
	virtual void GetLocalToWorldTransform( Matrix & transform ) const = 0;

	virtual void GetWorldVelocity( Vector3 & velocity ) const
	{
		velocity = Vector3( 0, 0, 0 );
	}

	// Registers an object and its attachments with the quad renderer
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer )
	{
	}
	// Adds quads from space object and its attachments to the quad renderer. ATTENTION: this function is called in-parallel
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
	{
	}

	virtual void GetPerObjectStructs( EveSpaceObjectVSData & vsData, EveSpaceObjectPSData & psData ) const
	{
	}

	virtual bool IsPickable() const
	{
		return true;
	}

	virtual bool IsAudioOccluder() const
	{
		return false;
	}

	virtual void SetProceduralContainerVariable( const char* name, float value )
	{
	}

	virtual void GetParentData( IEveSpaceObject2::ParentData * pd ) const {};
};

BLUE_DECLARE_IVECTOR( IEveSpaceObject2 );

#endif