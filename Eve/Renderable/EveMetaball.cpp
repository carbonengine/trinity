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
	m_isolevel( 1.0 ),
	m_gooValue( 1.0 )
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
	if( !m_effect )
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
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( Vertex ) );
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
	if( !m_effect )
	{
		return;
	}
	if( m_sourceItems.empty() )
	{
		return;
	}

	// only opaque for now
	if( batchType != TRIBATCHTYPE_OPAQUE )
	{
		return;
	}

	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
bool EveMetaball::HasTransparentBatches()
{
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
	EveBasicPerObjectData* data = accumulator->Allocate<EveBasicPerObjectData>();

	if( !data )
	{
		return nullptr;
	}

	// only two matrices for this one
	D3DXMatrixTranspose( &data->m_world, &m_worldTransform );
	D3DXMatrixInverse( &data->m_worldInverseTranspose, NULL, &m_worldTransform );

	return data;
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
			tvd.Add( tvd.FLOAT32_4, tvd.BLENDWEIGHTS );
			tvd.Add( tvd.UBYTE_4, tvd.BLENDINDICES );
			tvd.Add( tvd.FLOAT32_2, tvd.TEXCOORD );
			tvd.Add( tvd.FLOAT32_3, tvd.NORMAL );
			tvd.Add( tvd.FLOAT32_3, tvd.TANGENT );
			tvd.Add( tvd.FLOAT32_3, tvd.BITANGENT );
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

	std::vector<Triangle> allTriangles;
	allTriangles.reserve( ( m_triangleCount != 0 ) ? m_triangleCount : 10000 );

	Vector3 steps;
	D3DXVec3Subtract( &steps, &m_maxBounds, &m_minBounds );
	D3DXVec3Scale( &steps, &steps, 1.0f / m_boxSize );
	
	int stepsX = int(steps.x);
	int stepsY = int(steps.y);
	int stepsZ = int(steps.z);

	// Cache 2 layers in the grid.
	const int cacheSize = ( stepsY + 1) * ( stepsZ + 1) * 2;
	CellCorner* cachedCornerValues = new CellCorner[ cacheSize ];

	int nextIsoCacheLayer = 0;
	bool firstRun = true;

	int startX = (int)floor( m_minBounds.x / m_boxSize );

	for( int x = 0; x < stepsX; ++x )
	{
		// { previous bottom (now top), new bottom layer }
		int isoCacheLayers[2] = { (nextIsoCacheLayer + 1 ) % 2, nextIsoCacheLayer };
		nextIsoCacheLayer = isoCacheLayers[0];
		{
			CCP_STATS_ZONE( "Collect ISO values" );

			int stepsYPP = stepsY + 1;
			int stepsZPP = stepsZ + 1;
			int stepsYZPP = stepsYPP * stepsZPP;

			// Fill the cache. Both layers are filled in the first run.
			for( int r = 0; r < 1 + firstRun; ++r )
			{
				int cacheBase = isoCacheLayers[1 - r] * stepsYZPP;
				int xpp = x + 1 - r;

				for( int y = 0; y < stepsYPP; ++y )
				{
					int cacheBaseY = cacheBase + y * stepsZPP;

					for( int z = 0; z < stepsZPP; ++z )
					{
						Vector3 coordinate = Vector3( (float)xpp, (float)y, (float)z );
						CellCorner values;
						GetCornerValues( coordinate, &values);
						cachedCornerValues[ cacheBaseY + z ] = values;
					}
				}
			}	
		} // CCP_STATS_ZONE

		// Go over iso values and triangulate
		{
			CCP_STATS_ZONE( "Triangulate" );
			for( int y = 0; y < stepsY; ++y )
			{
				for( int z = 0; z < stepsZ; ++z )
				{
					Cell cell;
					unsigned int mask = 0;

					// for each gridpoint
					for( int i = 0; i < 8; ++i )
					{
						Vector3 cellCornerPos = Vector3((float)x, (float)y, (float)z) + s_vertexOffsetTable[ i ];
						D3DXVec3Scale( &cell.position, &cellCornerPos, m_boxSize );
						cell.position += m_minBounds;
						// Sample iso value
						int cacheBase = isoCacheLayers[ (int)s_vertexOffsetTable[ i ].x ] * (stepsY + 1) * (stepsZ + 1);
						int cacheBaseY = cacheBase + (int)cellCornerPos.y * ( stepsZ + 1 );
						float value = cachedCornerValues[ cacheBaseY + (int)cellCornerPos.z ].value;
						Vector3 normal = cachedCornerValues[ cacheBaseY + (int)cellCornerPos.z ].normal;

						cell.value[i] = value;
						cell.normal[i] = normal;
						if( value > m_isolevel )
						{
							mask |= 1 << i;
						}
					}
					cell.mask = mask;
				
					int nTriangles = 0;
					Triangle triangles[5];
					Triangulate( cell, triangles, nTriangles );

					for( int i = 0; i < nTriangles; ++i)
					{
						allTriangles.push_back( triangles[i] );
					}
				}
			}
		}// CCP_STATS_ZONE

		// First run is over
		firstRun = false;
	}

	// now we know the triangle count
	m_triangleCount = (unsigned int)allTriangles.size();
	
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
	CR_RETURN( m_vertexBuffer.Create( 3 * m_triangleCount * sizeof( Vertex ), USAGE_CPU_WRITE, nullptr, renderContext ) );
	Vertex* vertexBuffer;
	CR_RETURN( m_vertexBuffer.Lock( vertexBuffer, LOCK_WRITEONLY, renderContext ) );
	for( unsigned int i = 0; i < allTriangles.size(); ++i )
	{
		vertexBuffer[ 3 * i + 0 ].position = allTriangles[ i ].position[0];
		vertexBuffer[ 3 * i + 0 ].normal = allTriangles[ i ].normal[0];
		vertexBuffer[ 3 * i + 1 ].position = allTriangles[ i ].position[2];
		vertexBuffer[ 3 * i + 1 ].normal = allTriangles[ i ].normal[1];
		vertexBuffer[ 3 * i + 2 ].position = allTriangles[ i ].position[1];
		vertexBuffer[ 3 * i + 2 ].normal = allTriangles[ i ].normal[2];
	}
	m_vertexBuffer.Unlock( renderContext );

}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::GetCornerValues( Vector3 coordinate, CellCorner* values )
{
	values->value = 0.0f;
	values->normal = Vector3( 0.0f, 0.0f, 0.0f );

	for( int p = 0; p < 8; ++p )
{
	//Traverse all metball items
	for( auto it = m_sourceItems.begin(); it != m_sourceItems.end(); ++it )
	{
		EveMetaballItemPtr item = (*it);
			values->position = ( coordinate + s_vertexOffsetTable[p] ) * m_boxSize + m_minBounds;
			Vector3 v = values->position - item->GetPosition();
			float radius = item->GetRadius();
			float lengthSq = D3DXVec3LengthSq( &v );
			float length = D3DXVec3Length( &v );

			// normal
			Vector3 normalInfluence;
			D3DXVec3Scale( &normalInfluence, &v, radius * m_gooValue / lengthSq);
			values->normal += normalInfluence;
			// iso value
			values->value += pow( item->GetRadius() / length, m_gooValue );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
void EveMetaball::Triangulate(Cell cell, Triangle *triangles, int &nTriangles)
{
	const int* edgesList = s_triTable[cell.mask];

	int triIdx = 0;
	while( edgesList[ 3 * triIdx ] != -1 )
	{
		int edgeIdx0 = edgesList[ 3 * triIdx + 0 ];
		int edgeIdx1 = edgesList[ 3 * triIdx + 1 ];
		int edgeIdx2 = edgesList[ 3 * triIdx + 2 ];

		float edge0StartValue = cell.value[ s_edgeToVertsTable[edgeIdx0][0] ];
		float edge0EndValue = cell.value[ s_edgeToVertsTable[edgeIdx0][1] ];
		float edge1StartValue = cell.value[ s_edgeToVertsTable[edgeIdx1][0] ];
		float edge1EndValue = cell.value[ s_edgeToVertsTable[edgeIdx1][1] ];
		float edge2StartValue = cell.value[ s_edgeToVertsTable[edgeIdx2][0] ];
		float edge2EndValue = cell.value[ s_edgeToVertsTable[edgeIdx2][1] ];

		float fscale0 = ( m_isolevel - edge0StartValue ) / ( edge0EndValue - edge0StartValue );
		float fscale1 = ( m_isolevel - edge1StartValue ) / ( edge1EndValue - edge1StartValue );
		float fscale2 = ( m_isolevel - edge2StartValue ) / ( edge2EndValue - edge2StartValue );

		Vector3 offset0, offset1, offset2;
		D3DXVec3Lerp( &offset0, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx0][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx0][1]], fscale0 );
		D3DXVec3Lerp( &offset1, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx1][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx1][1]], fscale1 );
		D3DXVec3Lerp( &offset2, &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx2][0]], &s_vertexOffsetTable[s_edgeToVertsTable[edgeIdx2][1]], fscale2 );

		Triangle tri;
		tri.position[0] = cell.position + m_boxSize * offset0;
		tri.position[1] = cell.position + m_boxSize * offset1;
		tri.position[2] = cell.position + m_boxSize * offset2;
		
		D3DXVec3Lerp( &tri.normal[0], &cell.normal[s_edgeToVertsTable[edgeIdx0][0]], &cell.normal[s_edgeToVertsTable[edgeIdx0][1]], fscale0 );
		D3DXVec3Lerp( &tri.normal[1], &cell.normal[s_edgeToVertsTable[edgeIdx1][0]], &cell.normal[s_edgeToVertsTable[edgeIdx1][1]], fscale1 );
		D3DXVec3Lerp( &tri.normal[2], &cell.normal[s_edgeToVertsTable[edgeIdx2][0]], &cell.normal[s_edgeToVertsTable[edgeIdx2][1]], fscale2 );

		D3DXVec3Normalize( &tri.normal[0], &tri.normal[0]);
		D3DXVec3Normalize( &tri.normal[1], &tri.normal[1]);
		D3DXVec3Normalize( &tri.normal[2], &tri.normal[2]);

		triangles[triIdx] = tri;
		++triIdx;
	}
	nTriangles = triIdx;
}







