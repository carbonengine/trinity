// Copyright © 2026 CCP ehf.

#pragma once

#ifndef EveVolumeObject_h
#define EveVolumeObject_h

#include "IWorldPosition.h"
#include "IEveSpaceObject2.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

#include <ITriFunction.h>

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );
BLUE_DECLARE( EveVolumeObject );

/**
 * @class EveVolumeObject
 * @brief A standalone scene object that places a volume in the world.
 *
 * This is a trigger volume without the triggering: it owns its volumes and a destiny-ball driven
 * transform, and nothing else.
 */
BLUE_CLASS( EveVolumeObject ) :
	public IWorldPosition,
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveVolumeObject( IRoot* lockobj = NULL );
	~EveVolumeObject();

	// IEveSpaceObject2
	void UpdateSyncronous( const EveUpdateContext& updateContext ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors ) override;
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const override;
	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t ) override;
	void GetModelCenterWorldPosition( Vector3 & position ) const override;
	bool GetLocalBoundingBox( Vector3 & min, Vector3 & max ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;

	// IWorldPosition
	Vector3 GetWorldPosition() override;
	Quaternion GetWorldRotation() override;

	// IInitialize
	bool Initialize() override;

	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

private:
	/**
	 * @brief Recomputes the broad-phase bounding sphere from the volume list.
	 */
	void RebuildBoundingSphere();

	/**
	 * @brief Rebuilds the world transform from the position and rotation curves.
	 */
	void UpdateWorldTransform( Be::Time time );

	std::string m_name;
	PIEveVolumeVector m_volumes;

	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Matrix m_worldTransform;
	CcpMath::Sphere m_boundingSphere;
	bool m_enabled;
};

TYPEDEF_BLUECLASS( EveVolumeObject );

#endif
