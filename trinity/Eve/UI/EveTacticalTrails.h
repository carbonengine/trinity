// Copyright © 2025 CCP ehf.

#pragma once

#include "../IEveSpaceObject2.h"
#include "../../ITr2Renderable.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );

BLUE_CLASS( EveTacticalTrails ) :
	public IEveSpaceObject2,
	public ITr2Renderable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveTacticalTrails( IRoot* lockobj = nullptr );

	void RegisterObject( ITriVectorFunction * object );
	void UnregisterObject( ITriVectorFunction * object );

	/////////////////////////////////////////////////////////////////////////////////////
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

	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;

protected:
	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

private:
	struct Postion
	{
		Vector3d position;
		Be::Time time;
	};

	struct TrackedObject
	{
		BlueWeakRef<ITriVectorFunction> ball;
		std::vector<Postion> positions;
	};

	void UpdateGraphicsState( Be::Time now );

	Tr2EffectPtr m_trailEffect;
	Tr2BufferAL m_vertexBuffer;
	unsigned m_vertexDeclHandle = 0xffffffff;

	std::vector<TrackedObject> m_trackedObjects;
	ITriVectorFunctionPtr m_egoBall;
	Vector3d m_egoBallPosition = { 0, 0, 0 };
	uint32_t m_segmentCount = 0;
	float m_fadeOutTime = 5;
};

TYPEDEF_BLUECLASS( EveTacticalTrails );
