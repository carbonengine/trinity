// Copyright © 2026 CCP ehf.

#pragma once

#ifndef EveTriggerVolume_h
#define EveTriggerVolume_h

#include "IWorldPosition.h"
#include "IEveSpaceObject2.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

#include <ITriFunction.h>

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );
BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );
BLUE_DECLARE( EveTriggerVolume );

/**
 * @class EveTriggerVolume
 * @brief A volume that triggers a Python callback when a tracked position enters or exits it.
 *
 */
BLUE_CLASS( EveTriggerVolume ) :
	public IWorldPosition,
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveTriggerVolume( IRoot* lockobj = NULL );
	~EveTriggerVolume();

	/**
	 * @brief Sets the callable invoked on enter/exit transitions.
	 *
	 * @param callback Callable or None.
	 */
	void SetCallback( const BlueScriptCallback& callback );

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

	/**
	 * @brief Returns the highest intensity any enabled volume in the list gives the position.
	 * @param volumes The volumes to evaluate.
	 * @param position The position to evaluate, in object space.
	 */
	static float GetMaxIntensity( const PIEveVolumeVector& volumes, const Vector3& position );

	/**
	 * @brief Evaluates whether the tracked position is inside the volumes and fires the callback on transitions.
	 */
	void UpdateTriggerState( const EveUpdateContext& updateContext );

	/**
	 * @brief Invokes the stored callback.
	 * @param entered True if the tracked position entered the volume, false if it exited.
	 */
	void InvokeCallback( bool entered );

	std::string m_name;
	PIEveVolumeVector m_volumes;
	PIEveVolumeVector m_exclusionVolumes;
	PTr2ExternalParameterVector m_externalParameters;

	CcpMath::Sphere m_boundingSphere;

	ITriVectorFunctionPtr m_trackedPosition;

	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Matrix m_worldTransform;

	float m_enterThreshold;
	bool m_isInside;
	float m_currentIntensity;

	BlueScriptCallback m_callback;
};

TYPEDEF_BLUECLASS( EveTriggerVolume );

#endif
