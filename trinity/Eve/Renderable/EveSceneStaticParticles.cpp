// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "EveSceneStaticParticles.h"

#include "Include/TriMath.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2RuntimeInstanceData.h"
#include "Tr2InstancedMesh.h"
#include "Tr2Mesh.h"
#include "Eve/EveUpdateContext.h"
#include "TriFrustum.h"

const float PARTICLE_CLUSTER_MIN_SIZE = 100.0f;

// --------------------------------------------------------------------------------
// Description:
//   Constructor
// --------------------------------------------------------------------------------
EveSceneStaticParticles::EveSceneStaticParticles( IRoot* lockobj ) :
	m_minSize( 5.f ),
	m_maxSize( 200.f ),
	m_maxParticleCount( 100000 ),
	m_clusterParticleDensity( 100.f ),
	m_clusterParticleDensityAdjust( 1.f ),
	m_centerOfClusters( 0.0, 0.0, 0.0 ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_worldMatrix( IdentityMatrix() ),
	m_lastWorldMatrix( IdentityMatrix() ),
	m_estimatedSize( 0.0f ),
	m_visible( false ),
	m_center( 0.f, 0.f, 0.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveSceneStaticParticles::~EveSceneStaticParticles()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Adds a cluster of particles. Just puts it on a list, nothing is done here.
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::AddCluster( Vector3d position, float radius, Color color1, Color color2, unsigned int randomSeed )
{
	// just add it to the list, call to :: will make the actual work
	ClusterData cd;
	cd.position = position;
	cd.radius = radius;
	cd.color1 = color1;
	cd.color2 = color2;
	cd.randomSeed = randomSeed;
	m_clusters.push_back( cd );
}

// --------------------------------------------------------------------------------
// Description:
//   Remove all clusters of particles and free internal particle system data.
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::ClearClusters()
{
	// cluser list
	m_clusters.clear();
	// and the particle system data needs to go
	Tr2RuntimeInstanceData* instanceData = GetInstanceDataObject();
	if( !instanceData )
	{
		return;
	}
	instanceData->DestroyData();
}

// --------------------------------------------------------------------------------
// Description:
//   Init things once the red file is fully loaded
// --------------------------------------------------------------------------------
bool EveSceneStaticParticles::Initialize()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update. Mainly calculate a new worldmatrix based on the ever changing ego pos
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::Update( const EveUpdateContext& updateContext )
{
	// nothing to do here?
	if( Tr2Renderer::IsLowQuality() || m_clusters.empty() || !m_mesh )
	{
		return;
	}

	m_lastWorldMatrix = m_worldMatrix;
	// calc float offset from egopos to center of particles
	Vector3d offset = m_centerOfClusters - updateContext.GetOrigin();
	// build a transform matrix
	m_worldMatrix = TranslationMatrix( float( offset.x ), float( offset.y ), float( offset.z ) );

	m_center = TransformCoord( m_boundingSphere.GetXYZ(), m_worldMatrix );
}

void EveSceneStaticParticles::UpdateVisibility( const EveUpdateContext& updateContext )
{
	m_visible = false;

	// nothing to do here?
	if( Tr2Renderer::IsLowQuality() || m_clusters.empty() || !m_mesh )
	{
		return;
	}
	auto& frustum = updateContext.GetFrustum();
	m_estimatedSize = frustum.GetPixelSizeAccross( &m_boundingSphere );

	bool estimatedSizeWithinBounds = m_estimatedSize > PARTICLE_CLUSTER_MIN_SIZE * updateContext.GetLodFactor();
	bool inBoundingSphere = LengthSq( m_center - frustum.m_viewPos ) <= m_boundingSphere.w * m_boundingSphere.w;

	m_visible = inBoundingSphere || ( IsVisible( updateContext ) && estimatedSizeWithinBounds );
}

// --------------------------------------------------------------------------------
// Description:
//   The scene calls this to get the renderables
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	if( m_visible )
	{
		renderables.push_back( this );
	}
}

void EveSceneStaticParticles::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData, m_estimatedSize );
}

void EveSceneStaticParticles::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
	return;
}

Tr2PerObjectData* EveSceneStaticParticles::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveSceneStaticParticlesPerObjectData* data = accumulator->Allocate<EveSceneStaticParticlesPerObjectData>();
	if( !data )
	{
		return nullptr;
	}
	// column_major for shaders
	data->m_data.world = Transpose( m_worldMatrix );
	data->m_data.lastWorld = Transpose( m_lastWorldMatrix );
	return data;
}

bool EveSceneStaticParticles::HasTransparentBatches()
{
	return false;
}

float EveSceneStaticParticles::GetSortValue()
{
	return 0.0f;
}

bool EveSceneStaticParticles::IsVisible( const EveUpdateContext& updateContext ) const
{
	Vector4 transformedSphere( m_center, m_boundingSphere.w );
	return updateContext.GetFrustum().IsSphereVisible( &transformedSphere );
}

void EveSceneStaticParticles::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Bounding Sphere" );
}

// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this static particle system: bounding sphere
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		// draw bounding sphere
		Vector3 center = TransformCoord( m_boundingSphere.GetXYZ(), m_worldMatrix );
		renderer.DrawSphere( this, center, m_boundingSphere.w, 10, Tr2DebugRenderer::Wireframe, 0xffffff00 );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Let's iterate through the object which has been loaded via Ptyhon and
//   find the particle instance data
// --------------------------------------------------------------------------------
Tr2RuntimeInstanceData* EveSceneStaticParticles::GetInstanceDataObject()
{
	// anything there at all?
	if( !m_mesh )
	{
		return nullptr;
	}

	ITr2InstanceData* instanceData = m_mesh->GetInstanceGeometryResource();

	// is instance data of correct type
	Tr2RuntimeInstanceDataPtr dataPtr;
	if( !instanceData->QueryInterface( BlueInterfaceIID<Tr2RuntimeInstanceData>(), (void**)&dataPtr ) )
	{
		CCP_LOGERR( "EveSceneStaticParticles: instance data is not of correct type!" );
		return nullptr;
	}

	return dataPtr;
}

// --------------------------------------------------------------------------------
// Description:
//   Distributes particles in the clusters.
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::Rebuild()
{
	if( !m_mesh )
	{
		return;
	}

	// this object must have been set via python
	Tr2RuntimeInstanceData* instanceData = GetInstanceDataObject();
	if( !instanceData )
	{
		return;
	}

	// setup particle system
	Tr2VertexDefinition particleBufferVtxDef;
	particleBufferVtxDef.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	particleBufferVtxDef.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::TEXCOORD, 0 );
	particleBufferVtxDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 1 );
	instanceData->SetLayout( particleBufferVtxDef );

	// need total radius and a center for all clusters
	m_centerOfClusters = Vector3d( 0.0, 0.0, 0.0 );
	for( auto it = m_clusters.begin(); it != m_clusters.end(); ++it )
	{
		const ClusterData* clusterData = &( *it );
		m_centerOfClusters += Vector3d( (double)clusterData->position.x, (double)clusterData->position.y, (double)clusterData->position.z );
	}
	m_centerOfClusters /= (double)m_clusters.size();

	// need total size of particle buffer
	size_t particleBufferSize = 0;
	for( auto it = m_clusters.begin(); it != m_clusters.end(); ++it )
	{
		const ClusterData* clusterData = &( *it );
		particleBufferSize += size_t( m_clusterParticleDensity * clusterData->radius );
	}

	// are we too big? Too many particles? Do we need to adjust?
	m_clusterParticleDensityAdjust = 1.f;
	if( particleBufferSize > m_maxParticleCount )
	{
		m_clusterParticleDensityAdjust = float( m_maxParticleCount ) / float( particleBufferSize );
	}

	// count particle buffer size again, this time with adjustment
	particleBufferSize = 0;
	for( auto it = m_clusters.begin(); it != m_clusters.end(); ++it )
	{
		const ClusterData* clusterData = &( *it );
		particleBufferSize += size_t( m_clusterParticleDensityAdjust * m_clusterParticleDensity * clusterData->radius );
	}

	// alloc big buffer in the particle system
	ParticleBufferItem* currentParticleBufferItem = static_cast<ParticleBufferItem*>( instanceData->GetData( (unsigned int)particleBufferSize ) );

	// run over all the clusters and build
	for( auto it = m_clusters.begin(); it != m_clusters.end(); ++it )
	{
		const ClusterData* clusterData = &( *it );

		int particlesPerCluster = int( m_clusterParticleDensityAdjust * m_clusterParticleDensity * clusterData->radius );

		// relative position to cluster center in float position
		Vector3d d = Vector3d( (double)clusterData->position.x, (double)clusterData->position.y, (double)clusterData->position.z ) - m_centerOfClusters;
		Vector3 clusterPosRelativeToCenter = Vector3( (float)d.x, (float)d.y, (float)d.z );

		// seed any randomness
		if( clusterData->randomSeed > 0 )
		{
			TriRandomSeed( clusterData->randomSeed );
		}

		// calc all of the particles
		for( int i = 0; i < particlesPerCluster; ++i )
		{
			// position
			float deviation = std::min( clusterData->radius, 2000.f );
			Vector3 pos( TriFloatRandomGauss( 0.f, 2.f * deviation ), TriFloatRandomGauss( 0.f, deviation ), TriFloatRandomGauss( 0.f, 2.f * deviation ) );
			Vector3 nrm = Normalize( pos );
			currentParticleBufferItem->position = clusterPosRelativeToCenter + pos + clusterData->radius * nrm;

			// color (the alpha of the color is the seed)
			currentParticleBufferItem->color = Lerp( clusterData->color1, clusterData->color2, TriFloatRandom01() );
			currentParticleBufferItem->color.a = float( i ) / float( particlesPerCluster );

			// size
			currentParticleBufferItem->size = TriFloatRandom01() * std::min( clusterData->radius / 10.f, m_maxSize ) + m_minSize;

			// next item in buffer
			++currentParticleBufferItem;
		}
	}

	// finish up the instance buffer
	instanceData->UpdateBoundingBox();
	instanceData->UpdateData();
	Vector3 bbmin, bbmax;
	instanceData->GetBoundingBox( bbmin, bbmax );
	m_mesh->SetBoundingBox( bbmin, bbmax );

	// calculate a rough bounding sphere
	BoundingSphereFromBox( m_boundingSphere, bbmin, bbmax );
}
