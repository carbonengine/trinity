// Copyright © 2026 CCP ehf.

#pragma once

#ifndef EveVolumeObject_h
#define EveVolumeObject_h

#include "IWorldPosition.h"
#include "IEveSpaceObject2.h"
#include "EveEntity.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2

#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

#include <ITriFunction.h>
#include <ITr2VolumeObject.h>

BLUE_DECLARE( EveBoxVolume );
BLUE_DECLARE_INTERFACE( ITr2VolumeObject );
BLUE_DECLARE( EveVolumeObject );

/**
 * @class EveVolumeObject
 * @brief A standalone scene object that places a volume in the world and publishes its shape.
 *
 *
 * The shape is pushed every frame it changes, the scene origin follows the ego ball and so
 * moves the volume whenever the player moves. The consumer has to stay in step with everything else
 * placed in the scene. The consumer only holds the shape while this object is in a scene.
 */

BLUE_CLASS( EveVolumeObject ) :
	public IWorldPosition,
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2DebugRenderable,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

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

	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

private:
	/**
	 * @brief Recomputes the broad-phase bounding sphere from the volume list.
	 */
	void RebuildBoundingSphere();

	/**
	 * @brief Rebuilds the world transform from the position and rotation curves.
	 */
	void UpdateWorldTransform( Be::Time time );

	/**
	 * @brief Picks the box volume that defines the shape and listens for edits to it.
	 */
	void ResolveBoxVolume();

	/**
	 * @brief Pushes the enabled state and, when it changed, the unit box world transform to the consumer.
	 */
	void PushToConsumer();

	/**
	 * @brief Sends the transform now and records what was sent.
	 */
	void SendTransform( const Matrix& unitBoxToWorld );

	/**
	 * @brief Takes the shape back from the consumer it was pushed to; the next push sends it again.
	 */
	void RemoveFromConsumer();

	/**
	 * @brief True when two transforms are close enough that a consumer would not notice the difference.
	 */
	static bool TransformsNearlyEqual( const Matrix& a, const Matrix& b );

	std::string m_name;
	PIEveVolumeVector m_volumes;

	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Matrix m_worldTransform;
	CcpMath::Sphere m_boundingSphere;
	bool m_enabled;

	/// Whoever the scene file attached to receive the shape.
	ITr2VolumeObjectPtr m_consumer;
	/// The consumer the shape was pushed to.
	ITr2VolumeObjectPtr m_pushedConsumer;
	EveBoxVolumePtr m_boxVolume;
	uint32_t m_boxChangeCallbackID;
	bool m_warnedAboutVolumes;

	Matrix m_lastSentTransform;
	bool m_hasSentTransform;
	bool m_lastSentEnabled;
	bool m_transformDirty;
	/// Set when leaving a scene with a shape out, so registering again.
	bool m_resendOnRegister;
};

TYPEDEF_BLUECLASS( EveVolumeObject );

#endif
