#include "StdAfx.h"
#include "EveSpaceObjectDecal.h"
#include "Tr2Renderer.h"
#include "TriRenderBatch.h"
#include "TriPoolAllocator.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Mesh.h"
#include "Tr2DebugRenderer.h"
#include "Resources/TriGeometryRes.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/MatrixUtils.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

static BlueStructureDefinition s_eveSpaceObjectDecalIndexDef[] =
{ 
	{ "index",	Be::UINT32_1,	0 }, 
	{0} 
};

// ------------------------------------------------------------------------------------------------------
EveSpaceObjectDecal::EveSpaceObjectDecal( IRoot* lockobj ) :
	m_display( true ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_parentBoneIndex( -1 ),
	m_rebuildIndexBuffer( false ),
	m_decalPrimitiveCount( 0 ),
	PARENTLOCK( m_indices )
{
	// init
	PrepareResources();
	memset( &m_parentData, 0, sizeof( ParentData ) );
	D3DXMatrixIdentity( &m_decalMatrix );
	D3DXMatrixIdentity( &m_invDecalMatrix );
	D3DXMatrixIdentity( &m_parentData.transform );
	D3DXMatrixIdentity( &m_parentBoneMatrix );
	m_indices.SetStructureDefinition( s_eveSpaceObjectDecalIndexDef );
}

// ------------------------------------------------------------------------------------------------------
EveSpaceObjectDecal::~EveSpaceObjectDecal()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::UpdateDecalMatrix()
{
	// update decal matrix
	D3DXMatrixTransformation( &m_decalMatrix, NULL, NULL, &m_scaling, NULL, &m_rotation, &m_position );
	// and calc the inverse right away, is needed for intersection
	D3DXMatrixInverse( &m_invDecalMatrix, NULL, &m_decalMatrix );
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::Initialize()
{
	UpdateDecalMatrix();

	PrepareResources();

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_position ) || IsMatch( value, m_rotation )  || IsMatch( value, m_scaling ) )
	{
		// update the decal matrix
		UpdateDecalMatrix();
		// and rebuild the index buffer, cause decal volume has changed size and might hit new triangles
		if( m_indices.empty() )
		{
			m_rebuildIndexBuffer = true;
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::ReleaseResources( TriStorage s )
{
	if( s & m_indexBuffer.GetMemoryClass() )
	{
		m_indexBuffer.Destroy();
	}
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::OnPrepareResources()
{
	// create new index buffer
	if( !m_indexBuffer.IsValid() )
	{
		m_rebuildIndexBuffer = true;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::GetRenderables( TriGeometryRes* geomRes, std::vector<ITr2Renderable*>& renderables, const ParentData* parentData )
{
	if( !geomRes || !m_decalEffect )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	if( !m_indices.empty() )
	{
		if( m_rebuildIndexBuffer )
		{
			CreateStaticIndexBuffer();
		}
		if( geomRes != m_baseGeometryResource )
		{
			m_baseGeometryResource = geomRes;
		}
	}
	else
	{
		// if we get a new mesh, we must re-build the index buffer. This should be avoided cause it is slow!
		if( geomRes != m_baseGeometryResource )
		{
			m_rebuildIndexBuffer = true;
			m_baseGeometryResource = geomRes;
		}

		if( m_rebuildIndexBuffer )
		{
			CreateDecalIndexBuffer( geomRes );
		}
	}
	if( !m_indexBuffer.IsValid() )
	{
		return;
	}

	if( !m_decalPrimitiveCount )
	{
		return;
	}

	renderables.push_back( this );

	// store the parent transform etc. for later use
	m_parentData = *parentData;
}

// --------------------------------------------------------------------------------
// Description:
//   decals are always transparent
// --------------------------------------------------------------------------------
bool EveSpaceObjectDecal::HasTransparentBatches()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Todo
// --------------------------------------------------------------------------------
void EveSpaceObjectDecal::GetBatches( ITriRenderBatchAccumulator* batches, 
									  TriBatchType batchType, 
									  const Tr2PerObjectData* perObjectData )
{
	if( batchType != TRIBATCHTYPE_DECAL )
	{
		return;
	}

	if( !m_baseGeometryResource || !m_decalEffect )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	if( !m_indexBuffer.IsValid() )
	{
		return;
	}

	if( !m_decalPrimitiveCount )
	{
		return;
	}

	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_decalEffect );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
float EveSpaceObjectDecal::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveSpaceObjectDecal::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveDecalPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	// world matrix
	D3DXMatrixTranspose( &perObjectData->m_worldMatrix, &m_parentData.transform );
	// inv world matrix
	D3DXMatrixInverse( &perObjectData->m_invWorldMatrix, NULL, &perObjectData->m_worldMatrix );

	// decal matrix (both nrm and inv)
	D3DXMatrixTranspose( &perObjectData->m_decalMatrix, &m_decalMatrix );
	D3DXMatrixTranspose( &perObjectData->m_invDecalMatrix, &m_invDecalMatrix );

	// matrix from possible bone animation of parent
	D3DXMatrixTranspose( &perObjectData->m_parentBoneMatrix, &m_parentBoneMatrix );

	// clip sphere data from parent
	perObjectData->m_shipData = m_parentData.shipData;
	perObjectData->m_clipData1 = m_parentData.clipData;
	perObjectData->m_clipData2 = m_parentData.clipDataEx;

	// display data
	perObjectData->m_displayData = Vector4( (float)m_parentData.displayCounter, 0.f, 0.f, 0.f );

	if( m_parentData.shLighting )
	{
		memcpy( perObjectData->m_shLightingCoefficients, m_parentData.shLighting, sizeof( perObjectData->m_shLightingCoefficients ) );
	}
	else
	{
		memset( perObjectData->m_shLightingCoefficients, 0, sizeof( perObjectData->m_shLightingCoefficients ) );
	}

	return perObjectData;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( !m_baseGeometryResource )
	{
		return;
	}

	if( !m_baseGeometryResource->IsGood() )
	{
		return;
	}

	if( m_baseGeometryResource->GetMeshCount() < 1 )
	{
		return;
	}

	const TriGeometryResMeshData* meshData = m_baseGeometryResource->GetMeshData( 0 );
	if( !meshData )
	{
		return;
	}

	if( !m_indexBuffer.IsValid() )
	{
		return;
	}

	if( !meshData->m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( !m_decalPrimitiveCount )
	{
		return;
	}

	// render
	renderContext.m_esm.ApplyVertexDeclaration( meshData->m_vertexDeclaration );
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( meshData->m_vertexCount, 0, m_decalPrimitiveCount );
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& worldMatrix ) const
{
	Matrix worldDecalMatrix;

	D3DXMatrixMultiply( &worldDecalMatrix, &m_parentBoneMatrix, &worldMatrix );
	D3DXMatrixMultiply( &worldDecalMatrix, &m_decalMatrix, &worldDecalMatrix );
	renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
	renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Solid, 0 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Here the parent has it's chance to give in a bone matrix, if the parent is
//   animated.
// Arguments:
//   bonesMatrices - a pointer to the bone-matrix array of the parent
//   bonesMatricesCount - the max numbers of bones in this array
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetBoneMatrix( const granny_matrix_3x4* bonesMatrices, int bonesMatricesCount )
{
	// sanity
	if( m_parentBoneIndex >= bonesMatricesCount )
	{
		return;
	}

	// -1 mean disabled
	if( m_parentBoneIndex == -1 )
	{
		return;
	}

	// keep it
	TriMatrixCopyFrom3x4( &m_parentBoneMatrix, &bonesMatrices[ m_parentBoneIndex ] );
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the main shader of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetEffect( Tr2EffectPtr newEffect )
{
	m_decalEffect = newEffect;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the center position of this decal
// --------------------------------------------------------------------------------------
const Vector3& EveSpaceObjectDecal::GetPosition() const
{
	return m_position;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the center position of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetPosition( const Vector3& pos )
{
	m_position = pos;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the rotation of this decal
// --------------------------------------------------------------------------------------
const Quaternion& EveSpaceObjectDecal::GetRotation() const
{
	return m_rotation;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the rotation of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetRotation( const Quaternion& rot )
{
	m_rotation = rot;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the scaling of this decal
// --------------------------------------------------------------------------------------
const Vector3& EveSpaceObjectDecal::GetScaling() const
{
	return m_scaling;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the scaling of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetScaling( const Vector3& sc )
{
	m_scaling = sc;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the boneindex of this decal
// --------------------------------------------------------------------------------------
int EveSpaceObjectDecal::GetBoneIndex() const
{
	return m_parentBoneIndex;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the boneindex of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetBoneIndex( int idx )
{
	m_parentBoneIndex = idx;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::CreateDecalIndexBuffer( TriGeometryResPtr geomRes )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	BeTimer timer;

	// is loaded and good?
	if( !geomRes )
	{
		return;
	}

	if( !geomRes->IsGood() )
	{
		return;
	}

	if( geomRes->GetMeshCount() < 1 )
	{
		return;
	}

	/*const*/ TriGeometryResMeshData* meshData = geomRes->GetMeshData( 0 );
	if( !meshData )
	{
		return;
	}

	if( !meshData->m_vertexBuffer.IsValid() || !meshData->m_indexBuffer.IsValid() )
	{
		return;
	}

	// absolutly make sure we release old one!
	m_indexBuffer.Destroy();
	
	// lock existing buffers, we do this only once and it is managed, so this call is ok
	unsigned char* originalVertices = NULL;
	unsigned char* originalIndices = NULL;

	if( FAILED( meshData->m_vertexBuffer.Lock( originalVertices, LOCK_READONLY, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { meshData->m_vertexBuffer.Unlock( renderContext ); } );


	if( FAILED( meshData->m_indexBuffer.Lock( 0, 0, reinterpret_cast<void**>( &originalIndices ), LOCK_READONLY, renderContext) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { meshData->m_indexBuffer.Unlock( renderContext ); } );

	// correct source pointers
	const unsigned int* indices32 = reinterpret_cast<const unsigned int*>( originalIndices );
	const unsigned short* indices16 = reinterpret_cast<const unsigned short*>( originalIndices );
	const Vector3* positions = reinterpret_cast<const Vector3*>( originalVertices );
	// collect indices for decal geometry
	std::vector<unsigned int> decalIndices32;
	std::vector<unsigned short> decalIndices16;
	if( meshData->m_indexBuffer.Is16Bit() )
	{
		decalIndices16.reserve( meshData->m_primitiveCount * 3 );
	}
	else
	{
		decalIndices32.reserve( meshData->m_primitiveCount * 3 );
	}

	// is vertex compressed to 16bit floats?
	bool vertexPositionCompressed = false;
	Tr2VertexDefinition vd;
	if( Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, vd ) )
	{
		if( auto pos = vd.Find( vd.POSITION ) )
		{
			vertexPositionCompressed = pos->m_dataType == vd.FLOAT16_4;
		}
	}
			

	Vector3 aabbMin( -1.f, -1.f, -1.f );
	Vector3 aabbMax( 1.f, 1.f, 1.f );
	BoundingBoxTransform( aabbMin, aabbMax, m_decalMatrix );

	for( unsigned int t = 0; t < meshData->m_primitiveCount; ++t )
	{
		// get triangle indices
		unsigned int index0 = meshData->m_indexBuffer.Is16Bit() ? indices16[0] : indices32[0];
		unsigned int index1 = meshData->m_indexBuffer.Is16Bit() ? indices16[1] : indices32[1];
		unsigned int index2 = meshData->m_indexBuffer.Is16Bit() ? indices16[2] : indices32[2];
		// do real collision detection here
		if( vertexPositionCompressed )
		{
			Vector3 pos[3];
			D3DXFloat16To32Array( reinterpret_cast<float*>( &pos[0] ), reinterpret_cast<const D3DXFLOAT16*>( originalVertices + index0 * meshData->m_bytesPerVertex ), 3 );
			D3DXFloat16To32Array( reinterpret_cast<float*>( &pos[1] ), reinterpret_cast<const D3DXFLOAT16*>( originalVertices + index1 * meshData->m_bytesPerVertex ), 3 );
			D3DXFloat16To32Array( reinterpret_cast<float*>( &pos[2] ), reinterpret_cast<const D3DXFLOAT16*>( originalVertices + index2 * meshData->m_bytesPerVertex ), 3 );
			if( IntersectTriangleAABB( pos, pos + 1, pos + 2, aabbMin, aabbMax ) )
			{
				if( IntersectTriangleOrientedBox( pos, pos + 1, pos + 2, m_invDecalMatrix ) )
				{
					if( meshData->m_indexBuffer.Is16Bit() )
					{
						decalIndices16.push_back( index0 );
						decalIndices16.push_back( index1 );
						decalIndices16.push_back( index2 );
					}
					else
					{
						decalIndices32.push_back( index0 );
						decalIndices32.push_back( index1 );
						decalIndices32.push_back( index2 );
					}
				}
			}
		}
		else
		{
			Vector3* v0 = reinterpret_cast<Vector3*>( originalVertices + index0 * meshData->m_bytesPerVertex );
			Vector3* v1 = reinterpret_cast<Vector3*>( originalVertices + index1 * meshData->m_bytesPerVertex );
			Vector3* v2 = reinterpret_cast<Vector3*>( originalVertices + index2 * meshData->m_bytesPerVertex );

			if( IntersectTriangleAABB( v0, v1, v2, aabbMin, aabbMax ) )
			{
				if( IntersectTriangleOrientedBox( v0, v1, v2, m_invDecalMatrix ) )
				{
					if( meshData->m_indexBuffer.Is16Bit() )
					{
						decalIndices16.push_back( index0 );
						decalIndices16.push_back( index1 );
						decalIndices16.push_back( index2 );
					}
					else
					{
						decalIndices32.push_back( index0 );
						decalIndices32.push_back( index1 );
						decalIndices32.push_back( index2 );
					}
				}
			}
		}
		// next!
		indices16 += 3;
		indices32 += 3;
	}

	// primitive count and index size
	unsigned int decalIdxBufferSize = 0;
	unsigned decalIndexCount = 0;
	const void* decalIdxSrc = NULL;
	if( meshData->m_indexBuffer.Is16Bit() )
	{
		m_decalPrimitiveCount = unsigned( decalIndices16.size() / 3 );
		decalIdxBufferSize = unsigned( decalIndices16.size() * sizeof( unsigned short ) );
		decalIndexCount = unsigned( decalIndices16.size() );
		decalIdxSrc = &decalIndices16[0];
	}
	else
	{
		m_decalPrimitiveCount = unsigned( decalIndices32.size() / 3 );
		decalIdxBufferSize = unsigned( decalIndices32.size() * sizeof(unsigned int) );
		decalIndexCount = unsigned( decalIndices32.size() );
		decalIdxSrc = &decalIndices32[0];
	}

	// dont create empty buffer
	if( decalIdxBufferSize )
	{
		if( SUCCEEDED( m_indexBuffer.Create(	decalIndexCount, 
												USAGE_IMMUTABLE | USAGE_HINT_MANAGED, 
												meshData->m_indexBuffer.GetIBBitcount(), 
												decalIdxSrc, 
												renderContext ) ) )
		{
			m_rebuildIndexBuffer = false;
		}
	}
	else
	{
		m_rebuildIndexBuffer = false;
	}
}

void EveSpaceObjectDecal::SetIndices( const uint32_t* indices, size_t count )
{
	m_indices.clear();
	if( !indices || !count )
	{
		return;
	}
	for( size_t i = 0; i < count; ++i )
	{
		m_indices.Append( indices + i );
	}
}

void EveSpaceObjectDecal::CreateStaticIndexBuffer()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( SUCCEEDED( m_indexBuffer.Create( uint32_t( m_indices.size() ), USAGE_IMMUTABLE | USAGE_HINT_MANAGED, IB_32BIT, &m_indices[0], renderContext ) ) )
	{
		m_rebuildIndexBuffer = false;
		m_decalPrimitiveCount = unsigned( m_indices.size() / 3 );
	}
}

bool EveSpaceObjectDecal::HasStaticIndexBuffer() const
{
	return !m_indices.empty();
}

void EveSpaceObjectDecal::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_ATTACHMENTS ) == 0 )
	{
		return;
	}
	GetBatches( batches, TRIBATCHTYPE_DECAL, perObjectData );
}


// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveDecalPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	// add up constant count, see EveDecalPerObjectData
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_worldMatrix, 5 * 64, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_displayData, ( 4 + Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT ) * 16, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}






