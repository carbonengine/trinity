////////////////////////////////////////////////////////////
//
//    Created:   January 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveMetaball.h"

#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Tr2PerObjectData.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"

#include "Eve/EveConstantBufferFormats.h"
#include "Eve/EveTransform.h"
#include "Eve/Renderable/EveMetaballItem.h"
#include "Eve/Renderable/EveMetaballTables.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveMetaball::EveMetaball( IRoot* lockobj ) :
	PARENTLOCK( m_sourceItems ),
	m_display( true ),
	m_boxSize( 10.f ),
	m_boundingSphere( 0.f, 0.f, 0.f, -1.f ),
	m_minBounds( 0.f, 0.f, 0.f ),
	m_maxBounds( 0.f, 0.f, 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_triangleCount( 0 ),
	m_isoValue( 1.0 ),
	m_gooValue( 1.0 ),
	m_cellCounter( 0 )
{
	// 0
	D3DXMatrixIdentity( &m_worldTransform );
}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveMetaball::~EveMetaball()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   REset things once the red file is fully loaded
// --------------------------------------------------------------------------------
bool EveMetaball::Initialize()
{
	PrepareResources();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::UpdateSyncronous( EveUpdateContext& updateContext )
{
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	// really?
	if( !m_display )
	{
		return;
	}

	if( m_sourceItems.empty() )
	{
		return;
	}

	// use parent as world
	m_worldTransform = parentTransform;

	Vector4 boundingSphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, boundingSphere );

	// cull!
	if( frustum.IsSphereVisible( &boundingSphere ) )
	{
		renderables.push_back( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
bool EveMetaball::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	// if we don't have sources, there is boundingsphere
	if( m_sourceItems.empty() )
	{
		return false;
	}

	// build a sphere around the box we maintain
	BoundingSphereFromBox( sphere, m_minBounds, m_maxBounds );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{
}


// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
EveMetaball::Cell* EveMetaball::CheckCell( int x, int y, int z, int* sharedVerts, Cell* fromCell )
{
	Cell* cell = new Cell();
	cell->mask = 0;
	cell->x = x;
	cell->y = y;
	cell->z = z;

	// Update the counter for unique cell calculations
	m_cellCounter++;

	for( int p = 0; p < 8; ++p )
	{
		if( sharedVerts[p] >= 0 && fromCell)
		{
			cell->value[p] = fromCell->value[sharedVerts[p]];
			cell->position[p] = fromCell->position[sharedVerts[p]];
		}
		else
		{
			Vector3 coordinate = Vector3( (float)x, (float)y, (float)z );
			D3DXVec3Add( &coordinate, &coordinate, &s_vertexOffsetTable[p] );
			Vector3 position;
			D3DXVec3Scale( &position, &coordinate, m_boxSize );
			D3DXVec3Add( &position, &position, &m_minBounds );
			cell->position[p] = position;

			float value = GetGridValue( position );
			cell->value[p] = value;
		}
		
		if( cell->value[p] > m_isoValue )
		{
			cell->mask |= 1 << p;
		}
	}

	if( cell->mask != 0 && cell->mask != 255 )
	{
		return cell;
	}

	delete cell;
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::RecursiveCheckCell( int x, int y, int z, int* sharedVerts, Cell* cell )
{
	if( x < 0 || x >= m_gridSizeX )
	{
		return;
	}
	if( y < 0 || y >= m_gridSizeY )
	{
		return;
	}
	if( z < 0 || z >= m_gridSizeZ )
	{
		return;
	}

	// Check whether current cell exists in cache
	unsigned int key = z + ( y * ( m_gridSizeZ ) ) + ( x * ( m_gridSizeZ ) * ( m_gridSizeY ) );
	auto it =  m_cellCache.find(key);
	if( it != m_cellCache.end() )
	{
		return;
	}

	Cell* newCell = CheckCell( x, y, z, sharedVerts, cell );

	// cache it
	m_cellCache[key] = newCell;

	if( newCell )
	{
		if( newCell->mask & s_x1Mask )
		{
			RecursiveCheckCell( x - 1, y, z, s_xPosVerts, newCell );
		}
		if( newCell->mask & s_x0Mask )
		{
			RecursiveCheckCell( x + 1, y, z, s_xNegVerts, newCell );
		}
		if( newCell->mask & s_y1Mask )
		{
			RecursiveCheckCell( x, y - 1, z, s_yPosVerts, newCell );
		}
		if( newCell->mask & s_y0Mask )
		{
			RecursiveCheckCell( x, y + 1, z, s_yNegVerts, newCell );
		}
		if( newCell->mask & s_z1Mask )
		{
			RecursiveCheckCell( x, y, z - 1, s_zPosVerts, newCell );
		}
		if( newCell->mask & s_z0Mask )
		{
			RecursiveCheckCell( x, y, z + 1, s_zNegVerts, newCell );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::March()
{
	m_cellCounter = 0;

	if( m_sourceItems.size() == 0 )
	{
		return;
	}
	EveMetaballItem* firstBall = (EveMetaballItem*)m_sourceItems.GetAt( 0 );

	Vector3 startPosition = firstBall->GetPosition() - m_minBounds;
	D3DXVec3Scale( &startPosition, &startPosition, 1.0f / m_boxSize );
	int x = (int)( startPosition.x + 0.5f );
	int y = (int)( startPosition.y + 0.5f );
	int z = (int)( startPosition.z + 0.5f );

	while( z >= 0 )
	{
		RecursiveCheckCell( x, y, z, s_zPosVerts, nullptr );
		z--;
	}
}


// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( !m_vertexBuffer.IsValid() || !m_indexBuffer.IsValid() )
	{
		return;
	}
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( EveMetaball::Vertex ) );
//	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	renderContext.SetTopology( TOP_TRIANGLES );
//	renderContext.DrawIndexedPrimitive( 8, 0, m_triangleCount );
	renderContext.DrawPrimitive( 0, m_triangleCount );
}

// -------------------------------------------------------------
// Description:
//   Creates render batches for line set.
// Arguments:
//   batches - Render batch accumulator
//   batchType - Type of batches to gather
//   data - Per-object data
// -------------------------------------------------------------
void EveMetaball::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	// really?
	if( !m_display )
	{
		return;
	}

	if( m_sourceItems.empty() )
	{
		return;
	}

	if( batchType == TRIBATCHTYPE_ADDITIVE && m_additiveEffect )
	{
		TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
		if( batch )
		{
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( m_additiveEffect );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}
	}
	if( batchType == TRIBATCHTYPE_TRANSPARENT && m_transparentEffect )
	{
		TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
		if( batch )
		{
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( m_transparentEffect );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}
	}
	if( batchType == TRIBATCHTYPE_DISTORTION && m_distortionEffect )
	{
		TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
		if( batch )
		{
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( m_distortionEffect );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
bool EveMetaball::HasTransparentBatches()
{
	if( m_transparentEffect )
	{
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
float EveMetaball::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveMetaball::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataWithPersistentBuffers<EveMetaball>* data = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveMetaball>>();

	if( !data )
	{
		return nullptr;
	}

	data->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return data;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
uint32_t EveMetaball::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return 16;
	}
	else
	{
		return 64 + 16;	// m_vsWorldMatrix + m_vsSpaceObjectData
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::VERTEX_SHADER)
	{
		D3DXMatrixTranspose( reinterpret_cast<Matrix*>( data ), &m_worldTransform );
	}
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Releases vertex/index buffer
//   and vertex declaration.
// Arguments:
//   s - Type of resources to release
// -------------------------------------------------------------
void EveMetaball::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer.Destroy();
	m_indexBuffer.Destroy();
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Creates vertex/index buffer
//   and vertex declaration.
// -------------------------------------------------------------
bool EveMetaball::OnPrepareResources()
{
	// create vertex decl
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		static Tr2VertexDefinition s_metaBallDataVertexDecl;
		if( s_metaBallDataVertexDecl.empty() )
		{
			Tr2VertexDefinition& tvd = s_metaBallDataVertexDecl;

			tvd.Add( tvd.FLOAT32_3, tvd.POSITION );
			tvd.Add( tvd.FLOAT32_3, tvd.NORMAL );
		}

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_metaBallDataVertexDecl );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	UpdateBuffers();

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// create index buffer
	CR_RETURN_VAL( m_indexBuffer.Create( 36, USAGE_CPU_WRITE, IB_32BIT, nullptr, renderContext ), false );
	unsigned int* indexBuffer;
	CR_RETURN_VAL( m_indexBuffer.Lock( indexBuffer, LOCK_WRITEONLY, renderContext ), false );
	indexBuffer[ 0 ] = 0; indexBuffer[ 1 ] = 1; indexBuffer[ 2 ] = 2;
	indexBuffer[ 3 ] = 2; indexBuffer[ 4 ] = 1; indexBuffer[ 5 ] = 3;
	m_indexBuffer.Unlock( renderContext );
	
	return true;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	uint32_t color = 0xff0000ff;
	Tr2Renderer::DrawBox( m_minBounds, m_maxBounds, color );

	for( auto it = m_sourceItems.begin(); it != m_sourceItems.end(); ++it )
	{
		EveMetaballItemPtr item = (*it);
		Tr2Renderer::DrawSphere( item->GetPosition(), item->GetRadius(), 8, 0xffff00ff );
	}
}


// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::AddToBoundingBox( EveMetaballItemPtr item )
{
	Vector4 sphere = Vector4( item->GetPosition(), item->GetRadius() );
	BoundingBoxUpdate( m_minBounds, m_maxBounds, sphere );
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::UpdateBuffers()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// no update if no data
	if( m_sourceItems.empty() )
	{
		return;
	}

	// calc total size of max box around the whole metaball
	BoundingBoxInitialize( m_minBounds, m_maxBounds );
	for( auto it = m_sourceItems.begin(); it != m_sourceItems.end(); ++it )
	{
		AddToBoundingBox( *it );
	}

	Vector3 totalSize = m_maxBounds - m_minBounds;

	m_triangles.clear();
	m_triangles.reserve( ( m_triangleCount != 0 ) ? m_triangleCount : 10000 );

	Vector3 steps;
	D3DXVec3Subtract( &steps, &m_maxBounds, &m_minBounds );
	D3DXVec3Scale( &steps, &steps, 1.0f / m_boxSize );
	
	m_gridSizeX = (int)(steps.x) + 1;
	m_gridSizeY = (int)(steps.y) + 1;
	m_gridSizeZ = (int)(steps.z) + 1;

	m_cellCache.clear();

	{
		CCP_STATS_ZONE( "March" );
		March(); // caches cells.
	}
	{
		CCP_STATS_ZONE( "Triangulate" );
		for( auto it = m_cellCache.begin(); it != m_cellCache.end(); ++it )
		{
			Cell* cell = (*it).second;
			if( cell )
			{
				Triangulate( cell );
			}
			delete cell;
		}
	} // CCP_STATS_ZONE

	// now we know the triangle count
	m_triangleCount = (unsigned int)m_triangles.size();
	
	if ( m_triangleCount == 0 )
	{
		return;
	}

	// fill the vertex buffer
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// could be an update!
	if( m_vertexBuffer.IsValid() )
	{
		m_vertexBuffer.Destroy();
	}

	// create vertex buffer
	CR_RETURN( m_vertexBuffer.Create( 3 * m_triangleCount * sizeof( EveMetaball::Vertex ), USAGE_CPU_WRITE, nullptr, renderContext ) );
	EveMetaball::Vertex* vertexBuffer;
	CR_RETURN( m_vertexBuffer.Lock( vertexBuffer, LOCK_WRITEONLY, renderContext ) );
	for( unsigned int i = 0; i < m_triangles.size(); ++i )
	{
		vertexBuffer[ 3 * i + 0 ].position = m_triangles[ i ].position[0];
		vertexBuffer[ 3 * i + 0 ].normal = m_triangles[ i ].normal[0];
		vertexBuffer[ 3 * i + 1 ].position = m_triangles[ i ].position[2];
		vertexBuffer[ 3 * i + 1 ].normal = m_triangles[ i ].normal[2];
		vertexBuffer[ 3 * i + 2 ].position = m_triangles[ i ].position[1];
		vertexBuffer[ 3 * i + 2 ].normal = m_triangles[ i ].normal[1];
	}
	m_vertexBuffer.Unlock( renderContext );

}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
float EveMetaball::GetGridValue( Vector3 position )
{
	float value = 0.0f;

	//Traverse all metball items
	for( auto it = m_sourceItems.begin(); it != m_sourceItems.end(); ++it )
	{
		EveMetaballItemPtr item = (*it);
		Vector3 v = position - item->GetPosition();
		float length = D3DXVec3Length( &v );

		// iso value
		value += pow( item->GetRadius() / length, m_gooValue );
		
	}
	return value;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::Triangulate( const Cell* cell )
{
	const int* edgesList = s_triTable[cell->mask];
	int index = 0;
	while( edgesList[ 3 * index ] != -1 )
	{
		int edgeIdx0 = edgesList[ 3 * index + 0 ];
		int edgeIdx1 = edgesList[ 3 * index + 1 ];
		int edgeIdx2 = edgesList[ 3 * index + 2 ];

		float edge0StartValue = cell->value[ s_edgeToVertsTable[edgeIdx0][0] ];
		float edge0EndValue = cell->value[ s_edgeToVertsTable[edgeIdx0][1] ];
		float edge1StartValue = cell->value[ s_edgeToVertsTable[edgeIdx1][0] ];
		float edge1EndValue = cell->value[ s_edgeToVertsTable[edgeIdx1][1] ];
		float edge2StartValue = cell->value[ s_edgeToVertsTable[edgeIdx2][0] ];
		float edge2EndValue = cell->value[ s_edgeToVertsTable[edgeIdx2][1] ];

		float fscale0 = ( m_isoValue - edge0StartValue ) / ( edge0EndValue - edge0StartValue );
		float fscale1 = ( m_isoValue - edge1StartValue ) / ( edge1EndValue - edge1StartValue );
		float fscale2 = ( m_isoValue - edge2StartValue ) / ( edge2EndValue - edge2StartValue );

		Vector3 offset0, offset1, offset2;
		D3DXVec3Lerp( &offset0, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx0][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx0][1]], fscale0 );
		D3DXVec3Lerp( &offset1, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx1][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx1][1]], fscale1 );
		D3DXVec3Lerp( &offset2, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx2][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx2][1]], fscale2 );

		Triangle tri;
		tri.position[0] = cell->position[3] + m_boxSize * offset0; // (0,0,0) has cellcorner index 3
		tri.position[1] = cell->position[3] + m_boxSize * offset1;
		tri.position[2] = cell->position[3] + m_boxSize * offset2;

		tri.normal[0] = CalculateNormals( tri.position[0] );
		tri.normal[1] = CalculateNormals( tri.position[1] );
		tri.normal[2] = CalculateNormals( tri.position[2] );

		m_triangles.push_back(tri);
		index++;
	}
}

Vector3 EveMetaball::CalculateNormals( Vector3 position )
{
	Vector3 normal;
	normal.x = 0.0f;
	normal.y = 0.0f;
	normal.z = 0.0f;

	for( auto it = m_sourceItems.begin(); it != m_sourceItems.end(); ++it )
	{
		EveMetaballItemPtr item = (*it);
		Vector3 v = position - item->GetPosition();

		float radiusSq = item->GetRadius();
		radiusSq *= radiusSq;
		float distSq = D3DXVec3LengthSq( &v );
		D3DXVec3Scale( &v, &v, radiusSq / ( distSq * distSq ) );

		normal += v;
	}
	D3DXVec3Normalize( &normal, &normal );
	return normal;
}





