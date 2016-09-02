#include "StdAfx.h"
#include "Tr2Mesh.h"
#include "Shader/Tr2Effect.h"
#include "Resources/TriGeometryRes.h"
#include "Shader/Tr2ShaderMaterial.h"
#include "Tr2LowLevelShader.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2ShaderMaterial );

Tr2Mesh::Tr2Mesh( IRoot* lockobj ) : 
	PARENTLOCK( m_lodResources ),
	m_deferGeometryLoad( false ),
	m_immutable( false ),
	m_computeAccess( false ),
	m_isLoading( false ),
	m_resourceLoadCbId( 0 ),
	m_resourcePrepCbId( 0 ),
	m_selectedLod( TR2_LOD_UNSPECIFIED )
{
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

namespace {

struct ImmutableHelper : IBlueResManNotifications
{
	bool m_immutable;
	bool m_computeAccess;

	ImmutableHelper( bool immutable, bool computeAccess ) 
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
	}
}

void Tr2Mesh::ReleaseCachedData( BlueAsyncRes* p )
{
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
			fx->RebuildCachedData();
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

TriGeometryRes* Tr2Mesh::GetGeometryResource() const
{
	return m_geometryResource;
}




