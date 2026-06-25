// Copyright © 2012 CCP ehf.

#pragma once
#ifndef EveMissile_H
#define EveMissile_H


#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveMissileWarhead );
BLUE_DECLARE_VECTOR( EveMissileWarhead );

// --------------------------------------------------------------------------------
// Description:
//   This class holds an Eve missile, which corresponds to a missile's destinyball.
//   An EveMissile can hold mutliple warheads, for example when the player groups
//   modules and only one destiny ball is used, but we want to see all the
//   individual missiles.
//   It derives from EveSpaceObject2, just like ships or stations.
// SeeAlso:
//   EveMissileWarhead, EveSpaceObject2
// --------------------------------------------------------------------------------
class EveMissile : public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveMissile( IRoot* lockobj = NULL );
	~EveMissile();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - overriding EveSpaceObject2 implementations
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget - overriding EveSpaceObject2 implementations
	virtual void RebuildCachedData( BlueAsyncRes* p );


	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	// start this whole MIRV
	void Start( const Vector3& shipVelocity, float estimatedFlyingTime );

	// rebuild bounding sphere based on all warheads of this MIRV
	bool RebuildMissileBoundingSphere();

private:
	//  a list of all sub-missiles
	PEveMissileWarheadVector m_warheads;

	// enable control of warheads
	bool m_updateWarheads;

	// start data from ship
	Vector3 m_inheritedStartVelocity;

	// current data on the MIRV
	Vector3 m_inheritedVelocity;

	// timing
	float m_time;
	float m_estimatedTotalAliveTime;
	float m_lastValidSpeed;

	// the target
	ITriTargetablePtr m_target;
	float m_targetRadius;

	// explosion callback function
	BlueScriptCallback m_callback;
};

TYPEDEF_BLUECLASS( EveMissile );

#endif // EveMissile_H