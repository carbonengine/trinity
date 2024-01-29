////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshBase.h"
#include "Resources/TriGeometryRes.h"
#include "Utilities/BoundingBox.h"


CCP_STATS_DECLARE( tr2MeshBindToRig, "Trinity/BindToRig", true, CST_COUNTER_LOW, "Number of times a mesh executed bind to a new rig" );

Tr2MeshBase::Tr2MeshBase( IRoot* lockobj ) :
	PARENTLOCK( m_opaqueAreas ),
	PARENTLOCK( m_decalAreas ),
	PARENTLOCK( m_depthAreas ),
	PARENTLOCK( m_transparentAreas ),
	PARENTLOCK( m_additiveAreas ),
	PARENTLOCK( m_pickableAreas ),
	PARENTLOCK( m_mirrorAreas ),
	PARENTLOCK( m_geometryEraserAreas ),
	PARENTLOCK( m_decalNormalAreas ),
	PARENTLOCK( m_depthNormalAreas ),
	PARENTLOCK( m_opaquePrepassAreas ),
	PARENTLOCK( m_decalPrepassAreas ),
	PARENTLOCK( m_flareAreas ),
	PARENTLOCK( m_distortionAreas ),
	m_display( true ),
	m_meshIndex( 0 ),
    m_pBoneList(NULL),
    m_numBones(0)
{
	m_opaqueAreas.SetNotify( this );
	m_decalAreas.SetNotify( this );
	m_depthAreas.SetNotify( this );
	m_transparentAreas.SetNotify( this );
	m_additiveAreas.SetNotify( this );
	m_pickableAreas.SetNotify( this );
	m_mirrorAreas.SetNotify( this );
    m_depthNormalAreas.SetNotify( this );
    m_opaquePrepassAreas.SetNotify( this );
    m_decalPrepassAreas.SetNotify( this );
    m_geometryEraserAreas.SetNotify( this );
    m_distortionAreas.SetNotify( this );
	for( int i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		m_areaLookupArray[ i ] = NULL;
	}

	m_areaLookupArray[ TRIBATCHTYPE_OPAQUE ] = &m_opaqueAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DECAL ] = &m_decalAreas;
	m_areaLookupArray[ TRIBATCHTYPE_TRANSPARENT ] = &m_transparentAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DEPTH ] = &m_depthAreas;
	m_areaLookupArray[ TRIBATCHTYPE_ADDITIVE ] = &m_additiveAreas;
	m_areaLookupArray[ TRIBATCHTYPE_PICKING ] = &m_pickableAreas;
	m_areaLookupArray[ TRIBATCHTYPE_MIRROR ] = &m_mirrorAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DECALNORMAL ] = &m_decalNormalAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DEPTHNORMAL ] = &m_depthNormalAreas;
	m_areaLookupArray[ TRIBATCHTYPE_OPAQUE_PREPASS ] = &m_opaquePrepassAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DECAL_PREPASS ] = &m_decalPrepassAreas;
	m_areaLookupArray[ TRIBATCHTYPE_GEOMETRY_ERASER ] = &m_geometryEraserAreas;
	m_areaLookupArray[ TRIBATCHTYPE_FLARE ] = &m_flareAreas;
	m_areaLookupArray[ TRIBATCHTYPE_DISTORTION ] = &m_distortionAreas;
}


Tr2MeshBase::~Tr2MeshBase()
{
	for( auto& list : m_areaLookupArray )
	{
		for( auto& area : *list )
		{
			area->RemoveOwnerMesh( this );
		}
	}
}

void Tr2MeshBase::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const IList* list
	)
{
	switch( event & BELIST_EVENTMASK )
	{
	case BELIST_INSERTED:
		if( Tr2MeshAreaPtr area = BlueCastPtr( value ) )
		{
			area->AddOwnerMesh( this );
			ReverseIndexBufferIfNeeded();
		}
		break;
	case BELIST_REMOVED:
		if( Tr2MeshAreaPtr area = BlueCastPtr( value ) )
		{
			area->RemoveOwnerMesh( this );
		}
		break;
	case BELIST_LOADFINISHED:
		for( ssize_t i = 0; i < list->GetSize(); ++i )
		{
			if( Tr2MeshAreaPtr entity = BlueCastPtr( list->GetAt( i ) ) )
			{
				entity->AddOwnerMesh( this );
			}
		}
		ReverseIndexBufferIfNeeded();
		break;
	case BELIST_UNLOADSTART:
		for( ssize_t i = 0; i < list->GetSize(); ++i )
		{
			if( Tr2MeshAreaPtr entity = BlueCastPtr( list->GetAt( i ) ) )
			{
				entity->RemoveOwnerMesh( this );
			}
		}
		break;
	default:
		break;
	}
}

void Tr2MeshBase::ReverseIndexBufferIfNeeded()
{
	for( auto& list : m_areaLookupArray )
	{
		for( auto& area : *list )
		{
			if( area->IsReversed() )
			{
				ReverseIndexBuffers();
				return;
			}
		}
	}
}

void Tr2MeshBase::ReverseIndexBuffers()
{
	auto geometry = GetGeometryResource();
	if( geometry && geometry->IsGood() )
	{
		if( auto mesh = geometry->GetMeshData( m_meshIndex ) )
		{
			if( !mesh->m_reversedIndicesValid )
			{
				USE_MAIN_THREAD_RENDER_CONTEXT();
				geometry->ReverseIndexBuffer( *mesh, renderContext );
			}
			for( auto lod : mesh->m_lods )
			{
				if( auto meshLod = geometry->GetMeshData( unsigned( lod.meshIndex ) ) )
				{
					if( !meshLod->m_reversedIndicesValid )
					{
						USE_MAIN_THREAD_RENDER_CONTEXT();
						geometry->ReverseIndexBuffer( *meshLod, renderContext );
					}
				}
			}
		}
	}
}

unsigned int Tr2MeshBase::FindJoint( const std::string* boneList, const int numBones, const char* name ) const
{
	if( boneList && name )
	{
		for( int ix = 0; ix < numBones; ++ix )
		{
			if( strcmp( name, boneList[ix].c_str() ) == 0 )
			{
				return ix;
			}
		}
	}
	return 0xffffffff;
}

bool Tr2MeshBase::BindToRig( const std::string* boneList, const int numBones, TriGeometryResSkeletonData* renderRig, bool forceRebind )
{
	CCP_STATS_ZONE( "Tr2Mesh::BindToRig" );

	CCP_STATS_INC( tr2MeshBindToRig );

	forceRebind |= m_forcedRebind;
	m_forcedRebind = false;

	if( !GetGeometryResource() || !GetGeometryResource()->IsPrepared() ) 
	{
		return false;
	}

	if( (m_pBoneList == boneList) && (m_renderRig == renderRig) && (m_numBones == numBones) && !forceRebind )
	{
		return true;
	}

	TriGeometryResMeshData* meshData = GetGeometryResource()->GetMeshData( m_meshIndex );
	if( !meshData )
	{
		// resource is prepard but mesh doesn't exist, this can happen for ragdoll dummy boxshapes.
		// don't trigger continuous rebindings, return true.
		return true;
	}

	// keep this array here as member
	unsigned int n = (unsigned int)meshData->m_jointBindings.size();
	m_jointMappingAnimRig.resize( n );

	for( unsigned int j = 0; j < m_jointMappingAnimRig.size(); ++j )
	{
		const char* name = meshData->m_jointBindings[j].m_name.c_str();

		m_jointMappingAnimRig[j] = FindJoint( boneList, numBones, name );

		while (m_jointMappingAnimRig[j] == 0xffffffff)
		{
			unsigned int renderIndex = renderRig->FindJoint(name);
			if( renderIndex != 0xffffffff)
			{
				unsigned int parentIndex = renderRig->m_joints[renderIndex].m_parentJoint;
				if( parentIndex != 0xffffffff )
				{
					const char* parentName = renderRig->m_joints[parentIndex].m_name.c_str();
					m_jointMappingAnimRig[j] = FindJoint( boneList, numBones, parentName );
					name = parentName;
				} else {
					CCP_LOGWARN( "Resource %s - attempted to bind a joint that was not found on the render rig: %s", meshData->m_name.c_str(), name );
					break;
				}
			} else {
				CCP_LOGWARN( "Resource %s - attempted to bind a joint that was not found on the render rig: %s", meshData->m_name.c_str(), name );
				break;
			}
		}
	}

	for( int i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_areaLookupArray[ i ] )
		{
			for( PTr2MeshAreaVector::iterator it = m_areaLookupArray[ i ]->begin(); it != m_areaLookupArray[ i ]->end(); ++it )
			{
				(*it)->SetJointMappingAnimRig( &m_jointMappingAnimRig[0] );
				(*it)->SetJointCount( n );
			}
		}
	}

	// set this so we only do this once!
	m_pBoneList = boneList;
	m_renderRig = renderRig;
	m_numBones = numBones;

	return true;
}

CcpMath::AxisAlignedBox Tr2MeshBase::GetBounds( const Matrix* boneTransforms ) const
{
	if( boneTransforms )
	{
		if( auto geometry = GetGeometryResource() )
		{
			TriGeometryResMeshData* meshData = geometry->GetMeshData( m_meshIndex );
			if( meshData && !m_jointMappingAnimRig.empty() )
			{
				auto aabb = CcpMath::AxisAlignedBox();
				for( size_t i = 0; i < m_jointMappingAnimRig.size(); ++i )
				{
					auto& joint = meshData->m_jointBindings[i];
					auto& m = boneTransforms[m_jointMappingAnimRig[i]];

					CcpMath::AxisAlignedBox( joint.m_obbMin, joint.m_obbMax ).EnumerateVertices( [&]( const Vector3& vtx ) {
						aabb.IncludePoint( TransformCoord( vtx, m ) );
					} );
				}
				return m_boundsAdjustment.AdjustBounds( aabb );
			}
		}
	}
	return m_cachedBounds;
}

CcpMath::AxisAlignedBox Tr2MeshBase::GetAreaBounds( unsigned int areaIx, const Matrix* boneTransforms ) const
{
	CcpMath::AxisAlignedBox areaAabb;
	auto geometry = GetGeometryResource();
	if( !geometry || !geometry->GetAreaBoundingBox( m_meshIndex, areaIx, areaAabb.m_min, areaAabb.m_max ) )
	{
		return CcpMath::AxisAlignedBox();
	}

	if( boneTransforms )
	{
		TriGeometryResMeshData* meshData = geometry->GetMeshData( m_meshIndex );
		if( meshData && !m_jointMappingAnimRig.empty() )
		{
			auto aabb = CcpMath::AxisAlignedBox();
			for( size_t i = 0; i < m_jointMappingAnimRig.size(); ++i )
			{
				auto& joint = meshData->m_jointBindings[i];
				auto& m = boneTransforms[m_jointMappingAnimRig[i]];

				if( auto box = Intersection( areaAabb, CcpMath::AxisAlignedBox( joint.m_obbMin, joint.m_obbMax ) ) )
				{
					box.EnumerateVertices( [&]( const Vector3& vtx ) {
						aabb.IncludePoint( TransformCoord( vtx, m ) );
					} );
				}
			}
			areaAabb = aabb;
		}
	}
	return m_boundsAdjustment.AdjustBounds( areaAabb );
}

void Tr2MeshBase::CacheBounds()
{
	auto geometry = GetGeometryResource();
	if( !geometry || !geometry->GetBoundingBox( m_meshIndex, m_cachedBounds.m_min, m_cachedBounds.m_max ) )
	{
		m_cachedBounds = CcpMath::AxisAlignedBox();
		return;
	}

	m_cachedBounds = m_boundsAdjustment.AdjustBounds( m_cachedBounds );
}

bool Tr2MeshBase::GetBoundingBox( Vector3& min, Vector3& max ) const
{
	if( auto aabb = GetBounds() )
	{
		min = aabb.m_min;
		max = aabb.m_max;
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
bool Tr2MeshBase::GetDisplay() const
{
	return m_display;
}

Tr2RenderBatch CreateGeometryBatch( TriGeometryResMeshData* mesh, Tr2MeshArea* area, const Tr2PerObjectData* data )
{
	Tr2RenderBatch batch;

	if( !area->GetDisplay() )
	{
		return batch;
	}

	auto shadMat = area->GetMaterialInterface();
	if( !shadMat || !shadMat->GetShaderStateInterface() )
	{
		return batch;
	}

	auto primCount = GetPrimitiveCount( *mesh, area->GetIndex(), area->GetCount() );
	auto& meshArea = mesh->m_areas[area->GetIndex()];

	if( !primCount )
	{
		return batch;
	}

	if( area->GetReversed() && !mesh->m_reversedIndicesValid )
	{
		return batch;
	}

	batch.SetMaterial( shadMat );
	batch.SetGeometry( mesh->m_vertexDeclaration, mesh->m_vertexAllocation, mesh->m_indexAllocation );

	batch.SetPerObjectData( data );

	auto& indices = area->GetReversed() ? mesh->m_reversedIndexAllocation : mesh->m_indexAllocation;
	uint32_t startIndex;
	if( area->GetReversed() )
	{
		startIndex = indices.GetStartIndex() + mesh->m_primitiveCount * 3 - meshArea.m_firstIndex - primCount * 3;
	}
	else
	{
		startIndex = indices.GetStartIndex() + meshArea.m_firstIndex;
	}

	batch.SetDrawIndexedInstanced(
		primCount * 3,
		1,
		startIndex,
		mesh->m_vertexAllocation.GetOffset() / mesh->m_vertexAllocation.GetStride(),
		0 );
	return batch;
}



void Tr2MeshBase::GetBatches( ITriRenderBatchAccumulator* batches, 
	const Tr2MeshAreaVector* areas, 
	const Tr2PerObjectData* data,
	float screenSize ) const
{
	if( !GetDisplay() )
	{
		return;
	}

	auto geometry = GetGeometryResource();
	if( !geometry || !geometry->IsGood() )
	{
		return;
	}
	auto mesh = geometry->GetMeshData( m_meshIndex, screenSize );
	if( !mesh )
	{
		return;
	}

	for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
	{
		if( auto batch = CreateGeometryBatch( mesh, *it, data ) )
		{
			batch.SetPickingData( m_meshIndex, ( *it )->GetIndex() );
			batches->Commit( batch );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Returns the respath to the currently used geometry. Might
//   be LOD based.
// -------------------------------------------------------------
const wchar_t* Tr2MeshBase::GetGeometryResPath() const
{
	const TriGeometryRes* currentRes = GetGeometryResource();
	if( !currentRes )
	{
		return L"";
	}
	return currentRes->GetPath();
}

// -------------------------------------------------------------
// Description:
//   Gets the mesh area vector, depending on the batch type 
//	 requested. Defaults to NULL if there is no vector for the given batch type.
// Arguments:
//	 areaType - the TriBatchType as enumerated in ITr2Renderable
// -------------------------------------------------------------
Tr2MeshAreaVector* Tr2MeshBase::GetAreas( TriBatchType areaType )
{
	return m_areaLookupArray[ areaType ];
}

// -------------------------------------------------------------
// Description:
//   Gets the mesh area vector, depending on the batch type 
//	 requested. Defaults to NULL if there is no vector for the given batch type.
// Arguments:
//	 areaType - the TriBatchType as enumerated in ITr2Renderable
// -------------------------------------------------------------
const Tr2MeshAreaVector* Tr2MeshBase::GetAreas( TriBatchType areaType ) const
{
	return m_areaLookupArray[ areaType ];
}

// -------------------------------------------------------------
// Description:
//   Put the very basic info of a mesharea (block) into the provided
//   vector.
// Arguments:
//	 areaBlockVector - vector to append the block to
//   areaType - only append area blocks from this block
// -------------------------------------------------------------
void Tr2MeshBase::CollectAreaBlocks( std::vector<TriRenderBatchAreaBlock>& collector, TriBatchType areaType ) const
{
	const Tr2MeshAreaVector* areas = GetAreas( areaType );
	for( auto a = areas->begin(); a != areas->end(); ++a )
	{
		if( areaType == TRIBATCHTYPE_OPAQUE && !( *a )->IsCastingShadows() )
		{
			// skip non-shadow casting areas when collecting areas for overlay rendering: such areas may cause problems
			// (for example scaffolding + build effect)
			continue;
		}
		TriRenderBatchAreaBlock ab( (*a)->GetIndex(), (*a)->GetCount() );
		collector.push_back( ab );
	}
}

void Tr2MeshBase::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	const auto length = TRIBATCHTYPE_COUNT_OF_BATCH_TYPES;
	for ( auto i = 0; i < length; ++i )
	{
		const auto tribatchType = static_cast<TriBatchType>( i );
		const auto area = GetAreas( tribatchType );
		for ( auto itInner = area->begin(); itInner != area->end(); ++itInner )
		{
			const Tr2MeshArea *area = *itInner;
			const auto material = area->GetMaterialInterface();
			if ( nullptr != material )
			{
				material->SetOption( name, value );
			}
		}
	}
}

void Tr2MeshBase::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Mesh Bounds" );
}

void Tr2MeshBase::RenderDebugInfo( const Matrix& worldTransform, ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Mesh Bounds" ) )
	{
		auto bounds = GetBounds( nullptr );
		renderer.DrawBox( this, worldTransform, bounds.m_min, bounds.m_max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xffaa8800, 0x22aa8800 ) );
	}
}

std::vector<Tr2MeshAreaPtr> Tr2MeshBase::GetAllAreas() const
{
	std::vector<Tr2MeshAreaPtr> areas;
	for( auto areaType : m_areaLookupArray )
	{
		if( areaType )
		{
			areas.insert( areas.end(), begin( *areaType ), end( *areaType ) );
		}
	}
	return areas;
}

Tr2MaterialBoundsAdjustment Tr2MeshBase::GetMaterialBoundsAdjustment() const
{
	return m_boundsAdjustment;
}

void Tr2MeshBase::SetMaterialBoundsAdjustment( const Tr2MaterialBoundsAdjustment& adjustment )
{
	m_boundsAdjustment = adjustment;
	CacheBounds();
}

void Tr2MeshBase::UseWithScreenSize( float screenSize, float worldRadius ) const
{
	if (auto geometry = GetGeometryResource() )
	{
		if( auto mesh = geometry->GetMeshData( m_meshIndex ) )
		{
			for( auto areaType : m_areaLookupArray )
			{
				if( areaType )
				{
					for( auto& area : *areaType )
					{
						if( area && area->GetMaterialInterface() )
						{
							area->GetMaterialInterface()->UsedWithScreenSize( screenSize, worldRadius, mesh->m_uvDensities );
						}
					}
				}
			}
		}
	}
}