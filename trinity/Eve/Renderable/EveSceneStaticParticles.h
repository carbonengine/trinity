// Copyright © 2014 CCP ehf.

#pragma once
#ifndef EveSceneStaticParticles_H
#define EveSceneStaticParticles_H

#include "Utilities/Vector3d.h"
#include "ITr2Renderable.h"
#include "Eve/IEveSpaceObject2.h"
#include "Tr2PersistentPerObjectData.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveTransform );
BLUE_DECLARE( Tr2RuntimeInstanceData );
BLUE_DECLARE( Tr2InstancedMesh );

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSceneStaticParticles ) :
	public IInitialize,
	public ITr2DebugRenderable,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	EveSceneStaticParticles( IRoot* lockobj = NULL );
	~EveSceneStaticParticles();

	// structs
	struct ClusterData
	{
		Vector3d position;
		float radius;
		Color color1;
		Color color2;
		unsigned int randomSeed;
	};

	struct ParticleBufferItem
	{
		Vector3 position;
		float size;
		Color color;
	};

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize );
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual bool IsVisible( const EveUpdateContext& updateContext ) const override;

	// update & render
	void Update( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext );
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables );

	// manage clusters
	void AddCluster( Vector3d position, float radius, Color color1, Color color2, unsigned int randomSeed );
	void ClearClusters();
	void Rebuild();

private:
	// helper functions to access the right places in the loaded data
	Tr2RuntimeInstanceData* GetInstanceDataObject();

	// general data of this whole system
	float m_minSize;
	float m_maxSize;
	size_t m_maxParticleCount;
	float m_clusterParticleDensity;
	float m_clusterParticleDensityAdjust;
	float m_estimatedSize;

	// keep a list of all clusers we have
	std::vector<ClusterData> m_clusters;

	// data for positioning
	Vector3d m_centerOfClusters;
	Matrix m_worldMatrix;
	Matrix m_lastWorldMatrix;
	Vector3 m_center;

	// bounding sphere
	Vector4 m_boundingSphere;

	// the actual rendering object
	Tr2InstancedMeshPtr m_mesh;
	bool m_visible;
};

class EveSceneStaticParticlesPerObjectData : public Tr2PerObjectData
{
public:
	void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const override
	{
		FillAndSetConstants( *buffers[Tr2RenderContextEnum::VERTEX_SHADER],
							 &m_data,
							 sizeof( m_data ),
							 Tr2RenderContextEnum::VERTEX_SHADER,
							 Tr2Renderer::GetPerObjectVSStartRegister(),
							 renderContext );
	}

	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override
	{
		writer.SetPerObjectData( Tr2RenderContextEnum::VERTEX_SHADER, &m_data, sizeof( m_data ) );
	}

	struct Data
	{
		Matrix world;
		Matrix lastWorld;
	} m_data;
};

TYPEDEF_BLUECLASS( EveSceneStaticParticles );
BLUE_DECLARE_VECTOR( EveSceneStaticParticles );

#endif