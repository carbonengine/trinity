////////////////////////////////////////////////////////////
//
//    Created:   November 2014
//    Copyright: CCP 2014
//

#pragma once
#ifndef EveSceneStaticParticles_H
#define EveSceneStaticParticles_H

#include "Utilities/Vector3d.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
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
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveSceneStaticParticles(IRoot* lockobj = NULL);
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
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
    virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	// update & render
	void Update( EveUpdateContext& updateContext );
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables );

	// manage clusters
	void AddCluster( Vector3d position, float radius, Color color1, Color color2, unsigned int randomSeed );
	void ClearClusters();
	void Rebuild();

private:
	// helper functions to access the right places in the loaded data
	Tr2RuntimeInstanceData* GetInstanceDataObject();
	Tr2InstancedMesh* GetInstanceMeshObject();

	// general data of this whole system
	float m_minSize;
	float m_maxSize;
	size_t m_maxParticleCount;
	float m_clusterParticleDensity;
	float m_clusterParticleDensityAdjust;

	// keep a list of all clusers we have
	std::vector<ClusterData> m_clusters;

	// data for positioning
	Vector3d m_centerOfClusters;
	Matrix m_worldMatrix;

	// bounding sphere
	Vector4 m_boundingSphere;

	// the actual rendering object
	EveTransformPtr m_transform;
};

TYPEDEF_BLUECLASS( EveSceneStaticParticles );
BLUE_DECLARE_VECTOR( EveSceneStaticParticles );

#endif