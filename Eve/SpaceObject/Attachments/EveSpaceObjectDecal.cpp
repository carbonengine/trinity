#include "StdAfx.h"
#include "EveSpaceObjectDecal.h"
#include "Tr2Renderer.h"
#include "TriRenderBatch.h"
#include "TriPoolAllocator.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Mesh.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2RuntimeInstanceData.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/MatrixUtils.h"
#include "TriFrustum.h"
#include "Tr2InstancedMesh.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
CCP_STATS_DECLARE( decalDPCount, "Trinity/EveSpaceObject2/DecalDPCount", true, CST_COUNTER_LOW, "Number of decals rendered" );

extern float g_eveSpaceSceneLODFactor;

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
	m_rebuildIndexBuffers( false ),
	m_geometryLodIndex( -1 ),
	m_isVisible( 0 ),
	m_decalMatrix( IdentityMatrix() ),
	m_invDecalMatrix( IdentityMatrix() ),
	m_parentBoneMatrix( IdentityMatrix() ),
	m_minScreenSize( 0 ),
	m_vertexDeclarationOverride( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_instanceData( nullptr ),
	m_minBounds( 0, 0, 0 ),
	m_maxBounds( 0, 0, 0 ),
	m_isGeometryFrozen( false )
{
	// init
	PrepareResources();
	memset( &m_parentData, 0, sizeof( IEveSpaceObject2::ParentData ) );
	m_parentData.transform = IdentityMatrix();
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
	m_decalMatrix = TransformationMatrix( m_scaling, m_rotation, m_position );
	// and calc the inverse right away, is needed for intersection
	m_invDecalMatrix = Inverse( m_decalMatrix );
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
		if( !HasStaticIndexBuffers() )
		{
			m_rebuildIndexBuffers = true;
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::ReleaseResources( TriStorage )
{
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::OnPrepareResources()
{
	// create new index buffer
	if( !m_indexBuffer.IsValid() )
	{
		m_rebuildIndexBuffers = true;
	}
	return true;
}

void EveSpaceObjectDecal::UpdateVisibility( const TriFrustum& frustum, const IEveSpaceObject2::ParentData* parentData )
{
	m_isVisible = 0;

	if( !m_display || !m_decalEffect )
	{
		return;
	}

	if( m_minScreenSize > 0 )
	{
		Matrix worldDecalMatrix = m_parentBoneMatrix * parentData->transform; 

		Vector3 min( -1, -1, -1 );
		Vector3 max( 1, 1, 1 );
		bool isInstanced = m_instanceData != nullptr;
		
		// are we using instance magic?
		if( isInstanced )
		{
			// then we need to use the geometry mesh bounding box as the sphere
			min = XMVector3Transform( m_minBounds, m_parentBoneMatrix * parentData->transform );
			max = XMVector3Transform( m_maxBounds, m_parentBoneMatrix * parentData->transform );

			if( BoundingBoxIsInside( min, max, frustum.m_viewPos ) )
			{
				m_isVisible = 1;
				m_parentData = *parentData;
				return;
			}
			else if( !frustum.IsBoxVisible( min, max) )
			{
				m_isVisible = 0;
				return;
			}
						
			Vector3 closest = ClosestPointToBoundingBox( min, max, frustum.m_viewPos );
			worldDecalMatrix = TranslationMatrix( closest - frustum.m_viewPos ) * worldDecalMatrix;
		}

		worldDecalMatrix = m_decalMatrix * worldDecalMatrix;
		BoundingBoxTransform( min, max, worldDecalMatrix );
		Vector3 sphereCenter( ( min + max ) / 2 );
		float sphereRadius( Length( min - max ) / 2 );

		auto pixelSize = frustum.GetPixelSizeAccrossEst( sphereCenter, sphereRadius );
			
		float modifiedMinScreen = m_minScreenSize * g_eveSpaceSceneLODFactor;
		if( pixelSize < modifiedMinScreen )
		{
			m_isVisible = 0;
			return;
		}
		
		m_isVisible = std::min( ( pixelSize - modifiedMinScreen ) / ( modifiedMinScreen * 0.5f ), 1.f );
	}
	else
	{
		m_isVisible = 1;
	}
	m_parentData = *parentData;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::GetRenderables( std::vector<ITr2Renderable*>& renderables, TriGeometryRes* geomRes, float screensize )
{
	if( m_isVisible <= 0 || !geomRes )
	{
		return;
	}

	m_geometryLodIndex = m_isGeometryFrozen ? -1 : geomRes->GetLodIndexForScreenSize( 0, screensize );

	if( HasStaticIndexBuffers() )
	{
		if( m_rebuildIndexBuffers )
		{
			CreateStaticIndexBuffers();
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
			m_rebuildIndexBuffers = true;
			m_baseGeometryResource = geomRes;
		}

		if( m_rebuildIndexBuffers )
		{
			CreateDecalIndexBuffers( geomRes );
		}
	}

	int index = m_geometryLodIndex + 1;
	if( index >= m_indexBuffers.size() ||
		!m_indexBuffer.IsValid() ||
		!m_indexBuffers[index].m_primitiveCount )
	{
		return;
	}

	renderables.push_back( this );
}

void EveSpaceObjectDecal::GetInstancedRenderables( std::vector<ITr2Renderable*>& renderables, const Tr2InstancedMesh* instancedMesh )
{
	GetRenderables( renderables, instancedMesh->GetGeometryResource(), std::numeric_limits<float>::max() );
	m_instanceData = instancedMesh->GetInstanceGeometryResource();
	m_vertexDeclarationOverride = instancedMesh->GetVertexDeclaration();
	instancedMesh->GetBoundingBox( m_minBounds, m_maxBounds );
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
									  const Tr2PerObjectData* perObjectData,
									  Tr2RenderReason reason )
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

	int index = m_geometryLodIndex + 1;
	if( index >= m_indexBuffers.size() ||
		!m_indexBuffer.IsValid() ||
		!m_indexBuffers[index].m_primitiveCount )
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
    perObjectData->m_vsData.m_worldMatrix = Transpose( m_parentData.transform );
	// inv world matrix
    perObjectData->m_vsData.m_invWorldMatrix = Inverse( perObjectData->m_vsData.m_worldMatrix );

	// decal matrix (both nrm and inv)
    perObjectData->m_vsData.m_decalMatrix = Transpose( m_decalMatrix );
    perObjectData->m_vsData.m_invDecalMatrix = Transpose( m_invDecalMatrix );

	// matrix from possible bone animation of parent
    perObjectData->m_vsData.m_parentBoneMatrix = Transpose( m_parentBoneMatrix );
    perObjectData->m_vsData.m_invParentBoneMatrix = Inverse( Transpose( m_parentBoneMatrix ) );

	// clip sphere data from parent
    perObjectData->m_psData.m_shipData = m_parentData.shipData;
    perObjectData->m_psData.m_clipData = m_parentData.clipData;

	// display data
    perObjectData->m_psData.m_displayData = Vector4( (float)m_parentData.killCount, m_isVisible, 0.f, 0.f );

	if( m_parentData.shLighting )
	{
		memcpy( perObjectData->m_psData.m_shLightingCoefficients, m_parentData.shLighting, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
	}
	else
	{
		memset( perObjectData->m_psData.m_shLightingCoefficients, 0, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
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

	int index = m_geometryLodIndex + 1;
	if( index >= m_indexBuffers.size() ||
		!m_indexBuffer.IsValid() ||
		!m_indexBuffers[index].m_primitiveCount )
	{
		return;
	}

	const TriGeometryResMeshData* meshData = m_geometryLodIndex < 0 ? m_baseGeometryResource->GetMeshData( 0 ) : m_baseGeometryResource->GetMeshDataLod( 0, m_geometryLodIndex );
	if( !meshData )
	{
		return;
	}

	if( !meshData->m_vertexBuffer.IsValid() )
	{
		return;
	}

	CCP_STATS_INC( decalDPCount );

	auto declaration = m_vertexDeclarationOverride != Tr2EffectStateManager::UNINITIALIZED_DECLARATION ? m_vertexDeclarationOverride : meshData->m_vertexDeclaration;
	
	// render
	renderContext.m_esm.ApplyVertexDeclaration( declaration );
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	renderContext.SetTopology( TOP_TRIANGLES );


	if( m_instanceData )
	{
		ITr2InstanceData::InstanceData data = m_instanceData->GetInstanceData( 0, std::numeric_limits<float>::max() );

		renderContext.m_esm.ApplyStreamSource( 1, data.buffer, 0, data.stride );

		renderContext.DrawIndexedInstanced( meshData->m_vertexCount, m_indexBuffers[index].m_startIndex, m_indexBuffers[index].m_primitiveCount, data.count );	
	}
	else
	{
		renderContext.DrawIndexedPrimitive( meshData->m_vertexCount, m_indexBuffers[index].m_startIndex, m_indexBuffers[index].m_primitiveCount );	
	}
}

void EveSpaceObjectDecal::CopyFrom( EveSpaceObjectDecal *object )
{
	m_display = object->m_display;
	m_position = object->m_position;
	m_rotation = object->m_rotation;
	m_scaling = object->m_scaling;
	m_parentBoneIndex = object->m_parentBoneIndex;
	m_rebuildIndexBuffers = object->m_rebuildIndexBuffers;
	m_geometryLodIndex = object->m_geometryLodIndex;
	m_decalMatrix = object->m_decalMatrix;
	m_invDecalMatrix = object->m_invDecalMatrix;
	m_decalEffect = object->m_decalEffect;
	m_minScreenSize = object->m_minScreenSize;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix ) const
{
	Matrix worldDecalMatrix = m_parentBoneMatrix * worldMatrix;
	if( m_instanceData != nullptr )
	{
		if( Tr2RuntimeInstanceDataPtr runtimeInstanceData = BlueCastPtr(m_instanceData) )
		{
			// do some magic for instanced meshes that have decals!
			const char* data = reinterpret_cast<const char*>( runtimeInstanceData->GetData() );
			if( data == 0 )
			{
				return;
			}
			unsigned int stride = runtimeInstanceData->GetStride();

			// need vertex declaration to get offset of position element in the vertex
			Tr2VertexDefinition decl;
			if( Tr2EffectStateManager::GetVertexDeclarationElements( runtimeInstanceData->GetInstanceBufferVertexDeclaration( 0 ), decl ) )
			{
				auto transform0 = decl.Find( decl.TEXCOORD, 0 );
				auto transform1 = decl.Find( decl.TEXCOORD, 1 );
				auto transform2 = decl.Find( decl.TEXCOORD, 2 );

				for( uint32_t p = 0; p < runtimeInstanceData->GetCount(); ++p )
				{
					// get the transposed instanced values and detranspose them!
					const Vector4* transform0Value = (const Vector4*)&data[p * stride + transform0->m_offset];
					const Vector4* transform1Value = (const Vector4*)&data[p * stride + transform1->m_offset];
					const Vector4* transform2Value = (const Vector4*)&data[p * stride + transform2->m_offset];
					Matrix m = {
						transform0Value->x,
						transform1Value->x,
						transform2Value->x,
						0.0,
						transform0Value->y,
						transform1Value->y,
						transform2Value->y,
						0.0,
						transform0Value->z,
						transform1Value->z,
						transform2Value->z,
						0.0,
						transform0Value->w,
						transform1Value->w,
						transform2Value->w,
						1.0,
					};

					auto instancedDecalMatrix = m_decalMatrix * m * worldDecalMatrix;

					renderer.DrawBox( this, instancedDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
					renderer.DrawBox( this, instancedDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Solid, 0 );
				}

			}
		}
	}
	else 
	{
		worldDecalMatrix = m_decalMatrix * worldDecalMatrix;
		renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
		renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Solid, 0 );
	}
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

void EveSpaceObjectDecal::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_decalEffect )
	{
		m_decalEffect->SetOption( name, value );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the state for the geometry of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetHighDetailDecalState( bool isFrozen )
{
	m_isGeometryFrozen = isFrozen;
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
void EveSpaceObjectDecal::CreateDecalIndexBuffers( TriGeometryResPtr geomRes )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// is loaded and good?
	if( !geomRes )
	{
		return;
	}

	if( !geomRes->IsGood() )
	{
		return;
	}

	if( !geomRes->GetMeshCount() )
	{
		return;
	}

	m_indexBuffers.clear();

	std::vector<uint32_t> allIndices;

	TriGeometryResMeshData* originalMeshData = m_baseGeometryResource->GetMeshData( 0 );
	for( int i = -1; i < static_cast<int>( originalMeshData->m_lods.size() ); i++ )
	{
		DecalIndexBuffer buffer;

		TriGeometryResMeshData* meshData = i == -1 ? originalMeshData : m_baseGeometryResource->GetMeshDataLod( 0, i );

		if( !meshData )
		{
			return;
		}

		if( !meshData->m_vertexBuffer.IsValid() || !meshData->m_indexBuffer.IsValid() )
		{
			return;
		}

		// lock existing buffers, we do this only once and it is managed, so this call is ok
		const unsigned char* originalVertices = NULL;
		const unsigned char* originalIndices = NULL;

		if( FAILED( meshData->m_vertexBuffer.MapForReading( originalVertices, renderContext ) ) )
		{
			return;
		}
		ON_BLOCK_EXIT( [&] { meshData->m_vertexBuffer.UnmapForReading( renderContext ); } );


		if( FAILED( meshData->m_indexBuffer.MapForReading( originalIndices, renderContext ) ) )
		{
			return;
		}
		ON_BLOCK_EXIT( [&] { meshData->m_indexBuffer.UnmapForReading( renderContext ); } );

		// correct source pointers
		const unsigned int* indices32 = reinterpret_cast<const unsigned int*>( originalIndices );
		const unsigned short* indices16 = reinterpret_cast<const unsigned short*>( originalIndices );
		// collect indices for decal geometry
		std::vector<unsigned int> decalIndices32;
		std::vector<unsigned short> decalIndices16;
		bool is16bit = meshData->m_indexBuffer.GetDesc().stride == 2;
		if( is16bit )
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
			unsigned int index0 = is16bit ? indices16[0] : indices32[0];
			unsigned int index1 = is16bit ? indices16[1] : indices32[1];
			unsigned int index2 = is16bit ? indices16[2] : indices32[2];
			// do real collision detection here
			if( vertexPositionCompressed )
			{
				Vector3 pos[3];
				pos[0] = *reinterpret_cast<const Vector3_16*>( originalVertices + index0 * meshData->m_bytesPerVertex );
				pos[1] = *reinterpret_cast<const Vector3_16*>( originalVertices + index1 * meshData->m_bytesPerVertex );
				pos[2] = *reinterpret_cast<const Vector3_16*>( originalVertices + index2 * meshData->m_bytesPerVertex );
				if( IntersectTriangleAABB( pos, pos + 1, pos + 2, aabbMin, aabbMax ) )
				{
					if( IntersectTriangleOrientedBox( pos, pos + 1, pos + 2, m_invDecalMatrix ) )
					{
						if( is16bit )
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
				const Vector3* v0 = reinterpret_cast<const Vector3*>( originalVertices + index0 * meshData->m_bytesPerVertex );
				const Vector3* v1 = reinterpret_cast<const Vector3*>( originalVertices + index1 * meshData->m_bytesPerVertex );
				const Vector3* v2 = reinterpret_cast<const Vector3*>( originalVertices + index2 * meshData->m_bytesPerVertex );

				if( IntersectTriangleAABB( v0, v1, v2, aabbMin, aabbMax ) )
				{
					if( IntersectTriangleOrientedBox( v0, v1, v2, m_invDecalMatrix ) )
					{
						if( is16bit )
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
		buffer.m_startIndex = uint32_t( allIndices.size() );
		if( is16bit )
		{
			buffer.m_primitiveCount = unsigned( decalIndices16.size() / 3 );
			allIndices.insert( end( allIndices ), begin( decalIndices16 ), end( decalIndices16 ) );
		}
		else
		{
			buffer.m_primitiveCount = unsigned( decalIndices32.size() / 3 );
			allIndices.insert( end( allIndices ), begin( decalIndices32 ), end( decalIndices32 ) );
		}

		m_indexBuffers.push_back( buffer );
	}

	// dont create empty buffer
	if( !allIndices.empty() )
	{
		if( !SUCCEEDED( m_indexBuffer.Create(
				4,
				uint32_t( allIndices.size() ),
				Tr2GpuUsage::INDEX_BUFFER,
				Tr2CpuUsage::NONE,
				allIndices.data(),
				renderContext ) ) )
		{
			return;
		}
	}

	m_rebuildIndexBuffers = false;
}

void EveSpaceObjectDecal::SetIndices( const std::vector<std::vector<uint32_t>>& indices )
{
	m_rebuildIndexBuffers = true;
	m_indexBuffers.clear();

	for( const auto& index : indices )
	{
		m_indexBuffers.push_back( { index } );
	}
}

std::vector<std::vector<uint32_t>> EveSpaceObjectDecal::GetStaticIndexBuffers() const
{
	std::vector<std::vector<uint32_t>> buffers;
	for( const auto& buffer : m_indexBuffers )
	{
		buffers.push_back( buffer.m_indices );
	}
	return buffers;
}

void EveSpaceObjectDecal::SetMinScreenSize( float minScreenSize )
{
	m_minScreenSize = minScreenSize;
}

void EveSpaceObjectDecal::CreateStaticIndexBuffers()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	m_rebuildIndexBuffers = false;

	uint32_t total = 0;
	for( auto& buffer : m_indexBuffers )
	{
		buffer.m_startIndex = total;
		buffer.m_primitiveCount = uint32_t( buffer.m_indices.size() / 3 );
		total += uint32_t( buffer.m_indices.size() );
	}

	std::vector<uint32_t> indices;
	indices.resize( total );
	
	for( auto& buffer : m_indexBuffers )
	{
		std::copy( begin( buffer.m_indices ), end( buffer.m_indices ), begin( indices ) + buffer.m_startIndex );
	}
	if( !SUCCEEDED( m_indexBuffer.Create( 4, uint32_t( indices.size() ), Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, &indices[0], renderContext ) ) )
	{
		m_rebuildIndexBuffers = true;
	}
}

bool EveSpaceObjectDecal::HasStaticIndexBuffers() const
{
	for( const auto& buffer : m_indexBuffers )
	{
		if( !buffer.m_indices.empty() )
		{
			return true;
		}
	}

	return false;
}

void EveSpaceObjectDecal::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_ATTACHMENTS ) == 0 )
	{
		return;
	}
	GetBatches( batches, TRIBATCHTYPE_DECAL, perObjectData );
}

std::vector<uint32_t> EveSpaceObjectDecal::GetDecalPrimitiveCounts() const
{
	std::vector<uint32_t> counts;
	for( auto& buffer : m_indexBuffers )
	{
		counts.push_back( buffer.m_primitiveCount );
	}
	return counts;
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveDecalPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	// add up constant count, see EveDecalPerObjectData
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_vsData, sizeof(DecalVSPerObjectData), VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_psData, sizeof(DecalPSPerObjectData), PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}
