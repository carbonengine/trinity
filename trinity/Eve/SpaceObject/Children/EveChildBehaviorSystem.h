#pragma once
#ifndef EveChildBehaviorSystem_H
#define EveChildBehaviorSystem_H

#include "Eve/EveUpdateContext.h"
#include "Behaviors/BehaviorGroup.h"
#include "Behaviors/SplineTunnelGroup.h"
#include "Tr2DeviceResource.h"
#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "TriRenderBatch.h"
#include "ITr2Renderable.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"

BLUE_DECLARE( EveChildBehaviorSystem );
BLUE_DECLARE( SplineTunnelGroup );
BLUE_DECLARE_VECTOR( SplineTunnelGroup );
BLUE_DECLARE( BehaviorGroup );
BLUE_DECLARE_VECTOR( BehaviorGroup );

class TriBehaviorSystemInstancingBatch;

BLUE_CLASS( EveChildBehaviorSystem ) :
	public IEveSpaceObjectChild,
	public Tr2DeviceResource,
	public ITr2Renderable,
	public IInitialize,
	public IListNotify,
	public EveChildTransform,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveChildBehaviorSystem( IRoot* lockobj = nullptr );
	~EveChildBehaviorSystem();

	enum RenderType {
		RENDER_SHIP,
		RENDER_BOOSTER,

		COUNT
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// EveChildMesh
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );

	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	// IInitialize
	bool Initialize();

	void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList
	);

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2DeviceResource
	void ReleaseResources( TriStorage s ) {}

	const char* GetName() const;
	void SetName( const char* name );

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;

	Matrix GetWorldTransform();

	void ChangeBufferVertexCount();
	const std::vector<SplineTunnel>* GetTunnels() const;
	SplineTunnelGroupVector* GetSplineTunnels();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InstanceData
	bool IsInstanceDataReady() const;
	unsigned int GetInstanceBufferCount() const;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const;
	unsigned int GetInstanceBufferVertexCount( unsigned int bufferIndex ) const;
	void GetVertexBuffer( unsigned int bufferIndex, Tr2BufferAL& buffer, unsigned& stride );
	bool GetInstanceBufferBoundingBox( unsigned int bufferIndex, Vector3& minBounds, Vector3& maxBounds ) const;
	bool HasTransparentBatches();
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void Draw( TriBehaviorSystemInstancingBatch*, Tr2RenderContext& renderContext, int count, unsigned int vertexDecl,
			  int groupIndex, RenderType renderType );

	std::vector<std::pair<int, int>> GetVertexElementAddedThroughCode() const;

private:

	bool OnPrepareResources();
	void PassInVertexesToBehaviorGroups();
	void PassInTunnelFunctionsToBehaviorGroups();
	char* m_name;
	bool m_behaviorGroupLoaded;
	bool m_behaviorGroupLoadedForTunnel;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );
	
	void UpdateTunnelRegistry();

	/////////////////////////////////////////////////////////////////////////////////////
	// EveChildBehaviorSystem
	void UpdateAgents( const float deltaTime );
	void UpdateBuffer( Tr2RenderContext& renderContext );
	void GetGroupBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType,
						 const Tr2PerObjectData* perObjectData,
						 Tr2MeshPtr mesh, BehaviorGroup* group );
	void GetGroupBoosterBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType,
								 const Tr2PerObjectData* perObjectData, BehaviorGroup* group );

	void DrawMeshes( TriBehaviorSystemInstancingBatch* batch, Tr2RenderContext& renderContext, int count,
		unsigned int vertexDecl, int groupIndex );
	void DrawBoosters( TriBehaviorSystemInstancingBatch* batch, Tr2RenderContext& renderContext, int count,
		unsigned int vertexDecl, int groupIndex );

	std::vector<uint32_t> m_offsets;

	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;

	bool m_display;
	bool m_isVisible;

	// Instance data as vertex buffer
	Tr2BufferAL m_vertexBuffer;
	//number of locations in memory between beginnings of successive array elements
	unsigned const m_stride;
	// Number of instances
	unsigned m_vertexCount;

	PSplineTunnelGroupVector m_splineTunnels;
	std::vector<SplineTunnel> m_tunnels;
	PBehaviorGroupVector m_behaviorGroups;
};

TYPEDEF_BLUECLASS( EveChildBehaviorSystem );

#endif