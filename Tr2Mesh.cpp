#include "StdAfx.h"
#include "Tr2Mesh.h"
#include "Tr2Effect.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2LowLevelShader.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2ShaderMaterial );

CCP_STATS_DECLARE( tr2MeshBindToRig, "Trinity/BindToRig", true, CST_COUNTER_LOW, "Number of times a mesh executed bind to a new rig" );

Tr2Mesh::Tr2Mesh( IRoot* lockobj ) : 
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
	PARENTLOCK( m_lodResources ),
	m_meshIndex( 0 ),
	m_deferGeometryLoad( false ),
	m_immutable( false ),
	m_computeAccess( false ),
	m_areBoundsValid( false ),
	m_isLoading( false ),
    m_pBoneList(NULL),
    m_numBones(0),
	m_resourceLoadCbId( 0 ),
	m_resourcePrepCbId( 0 ),
	m_selectedLod( TR2_LOD_UNSPECIFIED )
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

	m_isBindPending = false;
}

Tr2Mesh::~Tr2Mesh()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}

	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}

	m_geometryPreparedCallbacks.clear();
}


// ---------------------------------------------------------------
bool Tr2Mesh::Initialize()
{
	if( !m_deferGeometryLoad )
	{
		InitializeGeometryResource();
	}

	return true;
}

// ---------------------------------------------------------------
void Tr2Mesh::AddGeometryPreparedCallback( const BlueScriptCallback& callback )
{
	CCP_ASSERT(callback);

	if (callback)
	{
		m_geometryPreparedCallbacks.push_back( callback );
	}
}

void Tr2Mesh::StaticResourceLoadFinished( void* pContext )
{
	Tr2Mesh* pThis = ( Tr2Mesh* )pContext;
	BeResMan->AddToQueue( BRMQ_MAIN, StaticResourcePrepFinished, pContext, 0, &pThis->m_resourcePrepCbId );
	pThis->m_resourceLoadCbId = 0;
}

void Tr2Mesh::StaticResourcePrepFinished( void* pContext )
{
	Tr2Mesh* pThis = ( Tr2Mesh* )pContext;
	pThis->m_resourcePrepCbId = 0;
	pThis->m_isLoading = false;

	// Kick off a low-level shader bind if one is pending
	if( pThis->m_isBindPending )
	{
		pThis->BindLowLevelShaders( pThis->m_pendingBindSituationFlags, 
			pThis->m_pendingDefaultOverride, pThis->m_pendingVariableStore );
	}


}

void Tr2Mesh::DoPrepCallbacks()
{
	CCP_ASSERT( GetGeometryResource() );
	CCP_ASSERT( GetGeometryResource()->IsPrepared() );
	CCP_ASSERT( !GetGeometryResource()->IsLoading() );

	//	Call the python callback if there is one
	for ( auto it = m_geometryPreparedCallbacks.begin(); it!=m_geometryPreparedCallbacks.end(); ++it )
	{

		auto& callback = ( *it );

		CCP_ASSERT(callback);

		if (callback)
		{
			callback.CallVoid();
		}
	}

	//	We've done all the callbacks so clear the list
	//	Note that it is a sin to add more callbacks DURING a callback!
	m_geometryPreparedCallbacks.clear();
}
// ---------------------------------------------------------------
bool Tr2Mesh::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_meshResPath ) )
	{
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_deferGeometryLoad ) )
	{
		if( !m_deferGeometryLoad && !m_geometryResource )
		{
			Initialize();
		}
	}

	return true;
}

void Tr2Mesh::OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		)
{
	m_forcedRebind = true;	// still up to the caller to actually reach out to BindToRig though..
}

float Tr2Mesh::CalcMeshSortValue( const Matrix& worldTransform )
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

bool Tr2Mesh::GetBoundingBox( Vector3& min, Vector3& max ) const
{
    if( !m_areBoundsValid )
    {
        return false;
    }

	min = m_minBounds;
	max = m_maxBounds;
	return true;
}

bool Tr2Mesh::GetAreaBoundingBox( unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	// Bail out if we don't have a geometry resource
	if( !m_geometryResource )
	{
		return false;
	}

	// Get the bounding box from the geometry resource
	return m_geometryResource->GetAreaBoundingBox( m_meshIndex, areaIx, min, max );
}

bool Tr2Mesh::GetAreaBasis( unsigned int areaIx, Vector3& pointOnTriangle, Vector3& edge1, Vector3& edge2 ) const
{
	// Bail out if we don't have a geometry resource
	if( !m_geometryResource )
	{
		return false;
	}

	// Get the bounding box from the geometry resource
	return m_geometryResource->GetAreaBasis( m_meshIndex, areaIx, pointOnTriangle, edge1, edge2 );
}

bool Tr2Mesh::GetBoundingSphere( Vector4& sphere )
{
	if( !m_areBoundsValid )
	{
		return false;
	}

	sphere = m_boundingSphere;

	return true;
}

bool Tr2Mesh::BindToRig( const std::string* boneList, const int numBones, TriGeometryResSkeletonData* renderRig, bool forceRebind )
{
	CCP_STATS_ZONE( "Tr2Mesh::BindToRig" );

	CCP_STATS_INC( tr2MeshBindToRig );

	forceRebind |= m_forcedRebind;
	m_forcedRebind = false;

	if( !m_geometryResource || !m_geometryResource->IsPrepared() ) 
	{
		return false;
	}

	if( (m_pBoneList == boneList) && (m_renderRig == renderRig) && (m_numBones == numBones) && !forceRebind )
	{
		return true;
	}

	TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( m_meshIndex );
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

unsigned int Tr2Mesh::FindJoint( const std::string* boneList, const int numBones, const char* name ) const
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

void Tr2Mesh::SetGeometryRes( TriGeometryRes* res )
{
	// Remove existing callback setup if any, set new geometry resource and attach callback
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	m_geometryResource = res;

	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Set a new geometry path from the outside. This will trigger an initialize of
//   the new geometry resource!
// Arguments:
//   path - gr2 res path
// --------------------------------------------------------------------------------------
void Tr2Mesh::SetMeshResPath( const char* path )
{
	m_meshResPath = path;

	// trigger change, this will automatically be triggered when set through python
	OnModified( (Be::Var*)&m_meshResPath );
}

void Tr2Mesh::GetBatches( ITriRenderBatchAccumulator* batches, 
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
			batch->SetGeometryResource( m_geometryResource );
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

namespace {

struct ImmutableHelper : IBlueResManNotifications
{
	bool m_immutable;
	bool m_computeAccess;

	ImmutableHelper( unsigned immutable, unsigned computeAccess ) 
	: m_immutable( immutable ) 
	, m_computeAccess( computeAccess )
	{}

	void OnResourceCreated( void* res )
	{
		static_cast<TriGeometryRes*>(res)->m_immutable = m_immutable;
		static_cast<TriGeometryRes*>(res)->m_computeAccess = m_computeAccess;
	}
};

}

void Tr2Mesh::InitializeGeometryResource()
{
	TriGeometryResPtr res;

	if( !m_meshResPath.empty() )
	{
		ImmutableHelper helper( m_immutable, m_computeAccess );
		BeResMan->GetResource( m_meshResPath.c_str(), m_geomResourceEx.c_str(), res, &helper );
		m_isLoading = true;

		if( m_resourceLoadCbId )
		{
			BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
			m_resourceLoadCbId = 0;
		}
		if( m_resourcePrepCbId )
		{
			BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
			m_resourcePrepCbId = 0;
		}

		BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
	}

	SetGeometryRes( res );
}

void Tr2Mesh::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		m_areBoundsValid = true;

		if( !m_geometryResource->GetBoundingBox( m_meshIndex, m_minBounds, m_maxBounds ) )
		{
			m_minBounds = Vector3( 0.0f, 0.0f, 0.0f );
			m_maxBounds = Vector3( 0.0f, 0.0f, 0.0f );
		}

		// Todo: Geometry files should have this in them - do an offline process to calculate
		// proper bounding spheres. Until then, approximate with a sphere around the bounding box
		//if( !m_geometryResource->GetBoundingSphere( m_meshIndex, m_boundingSphereCenter, m_boundingSphereRadius ) )
		//{
		//	return;
		//}
		Vector3 d = m_maxBounds - m_minBounds;
		m_boundingSphere = Vector4( ( m_minBounds + m_maxBounds ) * 0.5f, D3DXVec3Length( &d ) * 0.5f );

		m_areBoundsValid = true;

		//	 Call our resource callbacks if we have them
		DoPrepCallbacks();
	}
}

void Tr2Mesh::ReleaseCachedData( BlueAsyncRes* p )
{
}

// -------------------------------------------------------------
// Description:
//   Gets the mesh area vector, depending on the batch type 
//	 requested. Defaults to NULL if there is no vector for the given batch type.
// Arguments:
//	 areaType - the TriBatchType as enumerated in ITr2Renderable
// -------------------------------------------------------------
Tr2MeshAreaVector* Tr2Mesh::GetAreas( TriBatchType areaType )
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
const Tr2MeshAreaVector* Tr2Mesh::GetAreas( TriBatchType areaType ) const
{
	return m_areaLookupArray[ areaType ];
}

// -------------------------------------------------------------
// Description:
//   Put the very basic info of a mesharea (block) into the provided
//   vector.
// Arguments:
//	 areaBlockVector - vector to appen the block to
//   areaType - only append area blocks from this block
// -------------------------------------------------------------
void Tr2Mesh::CollectAreaBlocks( std::vector<TriRenderBatchAreaBlock>& collector, TriBatchType areaType ) const
{
	const Tr2MeshAreaVector* areas = GetAreas( areaType );
	for( auto a = areas->begin(); a != areas->end(); ++a )
	{
		TriRenderBatchAreaBlock ab( (*a)->GetIndex(), (*a)->GetCount() );
		collector.push_back( ab );
	}
}

// Takes an area vector, and walks it, rebinding all meshes 
void Tr2Mesh::BindAreaShaders( Tr2MeshAreaVector* areas, 
							   const std::vector<unsigned int>& engineFlags,
							   bool overrideDefaultSituation,
							   Tr2VariableStore* variableStore )
{
	// Initialize base situation
	Tr2ShaderSituation baseSituation( engineFlags );

	typedef Tr2MeshAreaVector::iterator MeshAreaIteratorType;
	MeshAreaIteratorType endOfAreas = areas->end();

	// Extend the situation with the vertex format
	if( TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( m_meshIndex ) )
	{
		Tr2VertexDefinition vd;
		Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, vd );
		baseSituation.AddVertexFormat( vd );
	}

	for( MeshAreaIteratorType areaIt = areas->begin(); areaIt != endOfAreas; ++areaIt )
	{
		if( Tr2ShaderMaterial* mat = dynamic_cast<Tr2ShaderMaterial*>( ( *areaIt )->GetMaterialInterface() ) )
		{
			Tr2ShaderSituation currentSituation = baseSituation;
			mat->ApplyMaterialToSituation( currentSituation, overrideDefaultSituation );
			mat->SetVariableStore( variableStore );

			mat->BindLowLevelShader(currentSituation);
		}
		else if( Tr2Effect* fx = dynamic_cast<Tr2Effect*>( ( *areaIt )->GetMaterialInterface() ) )
		{
			fx->SetVariableStore( variableStore );
			fx->RebuildCachedData( fx->GetEffectRes() );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Queries whether this Tr2Mesh is waiting to bind low-level shaders (because the
//   geometry resource hasn't fully loaded yet).
// Return Value:
//   true if the Tr2Mesh has a pending low-level shader bind
//   false if the Tr2Mesh does not have a pending low-level shader bind
// See Also:
//   ExecutePendingLowLevelShaderBind, BindLowLevelShaders
// --------------------------------------------------------------------------------------
bool Tr2Mesh::HasPendingLowLevelShaderBind( void ) const
{
	return m_isBindPending;
}

// --------------------------------------------------------------------------------------
// Description:
//   If the Tr2Mesh has a pending low-level shader bind, attempt to bind low-level shaders.
//   Has no effect if the geometry resource is not fully loaded.
// See Also:
//   HasPendingLowLevelShaderBind, BindLowLevelShaders
// --------------------------------------------------------------------------------------
void Tr2Mesh::ExecutePendingLowLevelShaderBind( void )
{
	BindLowLevelShaders( m_pendingBindSituationFlags, m_pendingDefaultOverride );
}

void Tr2Mesh::BindLowLevelShaders( const std::vector<unsigned int>& engineFlags,
								   bool overrideDefaultSituation,
								   Tr2VariableStore* variableStore )
{
    // Note: this should be IsGood, not IsPrepared, but for Tr2SkinnedModels, the 
	// TriGeometryRes's don't return true for IsGood, despite being valid.  Probably has
	// something to do with mesh setup in Paperdoll.
	if( m_geometryResource && m_geometryResource->IsGood() && Tr2Renderer::IsResourceCreationAllowed() )
	{
		for( unsigned int i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
		{
			Tr2MeshAreaVector* areas = GetAreas( (TriBatchType)i );
			if( areas )
			{
				BindAreaShaders( areas, engineFlags, overrideDefaultSituation, variableStore );
			}
		}
		m_isBindPending = false;
		m_pendingVariableStore = NULL;
	}
	else
	{
		m_isBindPending = true;
		m_pendingDefaultOverride = overrideDefaultSituation;
		m_pendingBindSituationFlags = engineFlags;
		m_pendingVariableStore = variableStore;
	}
}

void Tr2Mesh::PySetGeometryRes( TriGeometryRes* geometryRes )
{
	SetMeshResPath( "" );
	SetGeometryRes( geometryRes );
}

int Tr2Mesh::GetAreasCount() const
{
	return TRIBATCHTYPE_COUNT_OF_BATCH_TYPES;
}

void Tr2Mesh::UnloadWhenUnreferenced()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}

	for( int i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_areaLookupArray[ i ] )
		{
			for( PTr2MeshAreaVector::iterator it = m_areaLookupArray[ i ]->begin(); it != m_areaLookupArray[ i ]->end(); ++it )
			{
				if( *it )
				{
					auto material = (*it)->GetMaterialInterface();
					if( material )
					{
						material->UnloadResources();
					}
				}
			}
		}
	}
}

void Tr2Mesh::ReloadWhenReferenced()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	bool requiresLoading = false;
	for( int i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_areaLookupArray[ i ] )
		{
			for( PTr2MeshAreaVector::iterator it = m_areaLookupArray[ i ]->begin(); it != m_areaLookupArray[ i ]->end(); ++it )
			{
				if( *it )
				{
					auto material = (*it)->GetMaterialInterface();
					if( material )
					{
						if( !material->LoadResources() )
						{
							requiresLoading = true;
						}
					}
				}
			}
		}
	}

	if( requiresLoading )
	{
		if( m_resourceLoadCbId )
		{
			BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
			m_resourceLoadCbId = 0;
		}
		if( m_resourcePrepCbId )
		{
			BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
			m_resourcePrepCbId = 0;
		}

		m_isLoading = true;
		BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
	}
}

void Tr2Mesh::SelectLod( Tr2Lod lod )
{
	if( m_selectedLod == lod )
	{
		return;
	}

	m_selectedLod = lod;
	for( auto it = m_lodResources.begin(); it != m_lodResources.end(); ++it )
	{
		(*it)->SelectLod( lod );
	}
}

void Tr2Mesh::AddLodResource( Tr2LodResource* lr )
{
	m_lodResources.Append( lr );
}

void Tr2Mesh::RemoveLodResource( Tr2LodResource* lr )
{
	auto key = m_lodResources.FindKey( lr );
	if( key != -1 )
	{
		m_lodResources.Remove( key );
	}
}

void Tr2Mesh::ClearLodResources()
{
	m_lodResources.Remove( -1 );
}




