////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshBase.h"
#include "Resources/TriGeometryRes.h"

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
	m_meshIndex( 0 ),
	m_areBoundsValid( false ),
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
}

void Tr2MeshBase::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const IList* theList
	)
{
	m_areasChanged = true;
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
		const char* name = meshData->m_jointBindings[j].c_str();

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

float Tr2MeshBase::CalcMeshSortValue( const Matrix& worldTransform )
{
	if( !m_areBoundsValid )
	{
		return FLT_MAX;
	}

    Vector3 center;
    D3DXVec3TransformCoord( &center, (Vector3*)&m_boundingSphere, &worldTransform );

	Vector3	d = center - Tr2Renderer::GetViewPosition();
    float distSq = D3DXVec3LengthSq( &d );

    return distSq;
}

bool Tr2MeshBase::GetBoundingBox( Vector3& min, Vector3& max ) const
{
    if( !m_areBoundsValid )
    {
        return false;
    }

	min = m_minBounds;
	max = m_maxBounds;
	return true;
}

bool Tr2MeshBase::GetBoundingSphere( Vector4& sphere )
{
	if( !m_areBoundsValid )
	{
		return false;
	}

	sphere = m_boundingSphere;

	return true;
}

bool Tr2MeshBase::GetAreaBoundingBox( unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	// Bail out if we don't have a geometry resource
	if( !GetGeometryResource() )
	{
		return false;
	}

	// Get the bounding box from the geometry resource
	return GetGeometryResource()->GetAreaBoundingBox( m_meshIndex, areaIx, min, max );
}

bool Tr2MeshBase::GetAreaBasis( unsigned int areaIx, Vector3& pointOnTriangle, Vector3& edge1, Vector3& edge2 ) const
{
	// Bail out if we don't have a geometry resource
	if( !GetGeometryResource() )
	{
		return false;
	}

	// Get the bounding box from the geometry resource
	return GetGeometryResource()->GetAreaBasis( m_meshIndex, areaIx, pointOnTriangle, edge1, edge2 );
}

void Tr2MeshBase::GetBatches( ITriRenderBatchAccumulator* batches, 
	const Tr2MeshAreaVector* areas, 
	const Tr2PerObjectData* data,
	ITr2MeshBatchCallback* callback ) const
{
	if( IsHidden() )
	{
		return;
	}

	for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		ITr2ShaderMaterial* shadMat = area->GetMaterialInterface();

		if( area->IsHidden())
		{
			continue;
		}

		if( !shadMat )
		{
			continue;
		}

		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( shadMat );
			batch->SetPerObjectData( data );
			batch->SetGeometryResource( GetGeometryResource() );
			batch->SetMeshParameters( m_meshIndex, area->GetIndex(), area->GetCount(), area->GetReversed() );

			if( callback )
			{
				if( !callback->ProcessBatch( area, batch ) )
				{
					continue;
				}
			}

			batches->Commit( batch );
		}
	}
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
		TriRenderBatchAreaBlock ab( (*a)->GetIndex(), (*a)->GetCount() );
		collector.push_back( ab );
	}
}

