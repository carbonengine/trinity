// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveTrailsSet.h"

#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "Resources/TriGeometryRes.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveTrailsSet::EveTrailsSet( IRoot* lockobj ) :
	m_display( true ),
	m_trailVertexDeclElementCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_fadeSpeed( 1.f )
{
	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveTrailsSet::~EveTrailsSet()
{
	Cleanup();

	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveTrailsSet::Initialize()
{
	// geom path is here, so load it
	InitializeGeometryResource();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Load the geometry resource, might be a re-load
// SeeAlso:
//   IBlueResource, TriGeometryRes
// --------------------------------------------------------------------------------
void EveTrailsSet::InitializeGeometryResource()
{
	// Remove existing callback setup if any
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}

	// old geometry resource is gone, do some cleanup
	Cleanup();

	if( !m_geometryResPath.empty() )
	{
		// get new geometry resource
		BeResMan->GetResource( m_geometryResPath, "", BlueInterfaceIID<TriGeometryRes>(), (void**)&m_geometryResource );
	}

	// attach callback, so ::RebuildCachedData() will be called when it finished loading
	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Free all of the stuff, we allocated
// --------------------------------------------------------------------------------
void EveTrailsSet::Cleanup()
{
	// vertex decl element array is no longer valid
	m_trailVertexDeclElementCount = 0;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the main effect of this set from the outside
// --------------------------------------------------------------------------------
void EveTrailsSet::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set a new geometry path from the outside. This will trigger an initialize of
//   the new geometry resource!
// Arguments:
//   path - gr2 res path
// --------------------------------------------------------------------------------------
void EveTrailsSet::SetMeshResPath( const char* path )
{
	m_geometryResPath = path;

	// trigger change, this will automatically be triggered when set through python
	OnModified( (Be::Var*)&m_geometryResPath );
}

// --------------------------------------------------------------------------------
// Description:
//   This gets called when the geometry data has finished loading, so we have a
//   whole lot of init's to do:
//   - take the original vertex declaration, extend by additional members for the 2nd
//     stream, and create a new one
//   - grab bounding sphere from mesh
// SeeAlso:
//   TriGeometryResMeshData, Tr2EffectStateManager
// --------------------------------------------------------------------------------
void EveTrailsSet::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		// finished loading the turret geometry resource, so grab vertex decl and bounding sphere
		if( m_geometryResource->GetMeshCount() )
		{
			const TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( 0 );
			if( meshData )
			{
				// gemoetry's original vertex-decl must exist
				if( meshData->m_vertexDeclarationHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
				{
					// get gemoetry's original vertex-decl...
					if( Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclarationHandle, m_trailVertexDecl ) )
					{
						// ...expand it with instances stream elements...
						auto& item = m_trailVertexDecl.Add( m_trailVertexDecl.FLOAT32_4, m_trailVertexDecl.TEXCOORD, 1, 1, 1 );
						item.m_offset = 0;

						// ...and create new vertex dcel again
						m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( m_trailVertexDecl );

						m_trailVertexDeclElementCount = (unsigned int)m_trailVertexDecl.m_items.size();
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Zero some memebers and free granny's animation data, cause the granny file
//   is no longer valid.
// --------------------------------------------------------------------------------
void EveTrailsSet::ReleaseCachedData( BlueAsyncRes* p )
{
	// mem release
	Cleanup();
}

// --------------------------------------------------------------------------------
// Description:
//   If someone changed some data of the boosters we must re-create
// --------------------------------------------------------------------------------
bool EveTrailsSet::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_geometryResPath ) )
	{
		// new gr2 file specified -> reload!
		InitializeGeometryResource();
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Not much at the moment...
// --------------------------------------------------------------------------------
void EveTrailsSet::Update( Be::Time t )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Clear all the individual boosters this set was holding so far.
// --------------------------------------------------------------------------------
void EveTrailsSet::Clear()
{
	// clear everything
	m_trailData.clear();

	// also release the resources
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new individual booster to this at a specific position/orientation.
//   First we add it to the internal booster list and then we must update the
//   lensflares handled in the EveSpriteSet
// Arguments:
//   localMatrix - position/orientation of single turret in object-space
//   size - a size indicator, coming from the size of the booster
// SeeAlso:
//   EveSpriteSet
// --------------------------------------------------------------------------------
void EveTrailsSet::Add( const Matrix* localMatrix, float size )
{
	SingleTrailData data;
	data.transform = *localMatrix;
	data.size = size;
	// keep it in our list of boosters
	m_trailData.push_back( data );
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EveTrailsSet::ReleaseResources( TriStorage s )
{
	g_sharedBuffer.Free( m_instanceBuffer );
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering
// --------------------------------------------------------------------------------
bool EveTrailsSet::OnPrepareResources()
{
	// already loaded?
	if( m_trailVertexDeclElementCount )
	{
		// create vertex decl
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( m_trailVertexDecl );
			if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
			{
				return false;
			}
		}
	}
	// now build the "instance" buffer, which depends on the actual number of booster, this set currently holds
	InitializeInstanceBuffer();

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild the instance vertex buffer. Must even re-create it because the
//   number of individual boosters might has changed.
// --------------------------------------------------------------------------------
void EveTrailsSet::InitializeInstanceBuffer()
{
	// get rid of old one
	g_sharedBuffer.Free( m_instanceBuffer );

	// something there?
	if( m_trailData.empty() )
	{
		return;
	}

	// how many indiviual trails are in this set?
	unsigned int trailCount = (unsigned int)m_trailData.size();

	// create and fill with star-shape's position and some random-value
	std::vector<InstanceVertex> verts( trailCount );
	for( unsigned int i = 0; i < trailCount; ++i )
	{
		verts[i].transform = Vector4( m_trailData[i].transform._41, m_trailData[i].transform._42, m_trailData[i].transform._43, m_trailData[i].size );
	}
	USE_MAIN_THREAD_RENDER_CONTEXT();


	CR_RETURN( g_sharedBuffer.Allocate(
		sizeof( InstanceVertex ),
		trailCount,
		&verts[0],
		renderContext,
		m_instanceBuffer ) );
}

// --------------------------------------------------------------------------------
// Description:
//   The way of rendering in trinity: provide a batch for the complete boosterset
//   and pass this call to the lensflares. This will be handled in EveSpriteSet
// SeeAlso:
//   EveSpriteSet, TriForwardingBatch
// --------------------------------------------------------------------------------
void EveTrailsSet::GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData )
{
	if( !m_display )
	{
		return;
	}
	if( !m_geometryResource || !m_geometryResource->IsGood() )
	{
		return;
	}
	if( !m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}
	auto meshData = m_geometryResource->GetMeshData( 0 );
	if( !meshData )
	{
		return;
	}

	auto& lodData = meshData->m_lods[0];
	if( !lodData->m_allocationsValid )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_effect );
	batch.SetPerObjectData( perObjectData );
	batch.SetGeometry( m_vertexDeclHandle, lodData->m_vertexAllocation, m_instanceBuffer, lodData->m_indexAllocation );
	batch.SetDrawIndexedInstanced(
		lodData->m_primitiveCount * 3,
		uint32_t( m_trailData.size() ),
		lodData->m_indexAllocation.GetStartIndex(),
		lodData->m_vertexAllocation.GetOffset() / lodData->m_vertexAllocation.GetStride(),
		m_instanceBuffer.GetOffset() / m_instanceBuffer.GetStride() );
	accumulator->Commit( batch );
}