#include "StdAfx.h"
#include "Tr2CpuSkinnedModel.h"
#include "Tr2DynamicMesh.h"
#include "Resources/TriGeometryRes.h"

Matrix* TriMatCopy( Matrix* pOut, const float* pMatrix3x4 )
{
	// copy content from 3x4 matrix
    pOut->_11 = *pMatrix3x4++;
    pOut->_21 = *pMatrix3x4++;
    pOut->_31 = *pMatrix3x4++;
    pOut->_41 = *pMatrix3x4++;
    pOut->_12 = *pMatrix3x4++;
    pOut->_22 = *pMatrix3x4++;
    pOut->_32 = *pMatrix3x4++;
    pOut->_42 = *pMatrix3x4++;
    pOut->_13 = *pMatrix3x4++;
    pOut->_23 = *pMatrix3x4++;
    pOut->_33 = *pMatrix3x4++;
    pOut->_43 = *pMatrix3x4++;
	// defaults for the other 4 floats
	pOut->_44 = 1.f;
	pOut->_14 = pOut->_24 = pOut->_34 = 0.f;

	return pOut;
}

// ------------------------------------------------------------------------------------------------------
Tr2CpuSkinnedModel::Tr2CpuSkinnedModel( IRoot* lockobj ) : 
	Tr2SkinnedModel( lockobj )
{
}

// ------------------------------------------------------------------------------------------------------
Tr2CpuSkinnedModel::~Tr2CpuSkinnedModel()
{
}

// ------------------------------------------------------------------------------------------------------
void Tr2CpuSkinnedModel::deform( const float* deformMatrices, unsigned int numOfMatrices )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// deform each mesh of this model
	for( unsigned int m = 0; m < GetNumOfMeshes(); ++m )
	{
		Tr2DynamicMeshPtr mesh = dynamic_cast<Tr2DynamicMesh*>( static_cast<Tr2Mesh*>( GetMesh( m ) ) );
		if( mesh )
		{
			TriGeometryRes* geomRes = mesh->GetGeometryResource();
			if( geomRes )
			{
				TriGeometryResMeshData* resMeshData = geomRes->GetMeshData( mesh->GetMeshIndex() );
				if( resMeshData )
				{
					Tr2VertexBufferAL& vertexBuffer = mesh->GetDynamicVertexBuffer();
					if( vertexBuffer.IsValid() )
					{
						TriGeometryResVertexData* pVtxData = resMeshData->m_pVertexData;
						if( pVtxData )
						{
							uint8_t* pVideoMemBuffer;
							if( vertexBuffer.Lock( pVideoMemBuffer, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) == S_OK && pVideoMemBuffer )
							{
								memcpy( pVideoMemBuffer, pVtxData->m_pBuffer, mesh->GetDynamicVertexBufferSizeInBytes() );

								// setup pointer to all important data within the vertex
								uint8_t* boneIndex0 = pVtxData->m_pBoneIndex0;
								uint8_t* boneIndex1 = pVtxData->m_pBoneIndex1;
								uint8_t* boneIndex2 = pVtxData->m_pBoneIndex2;
								uint8_t* boneIndex3 = pVtxData->m_pBoneIndex3;
								uint8_t* boneWeight0 = pVtxData->m_pBoneWeight0;
								uint8_t* boneWeight1 = pVtxData->m_pBoneWeight1;
								uint8_t* boneWeight2 = pVtxData->m_pBoneWeight2;
								uint8_t* boneWeight3 = pVtxData->m_pBoneWeight3;
								uint8_t* vtxSrcPosition = pVtxData->m_pSrcPosition;
								uint8_t* vtxSrcNormal = pVtxData->m_pSrcNormal;
								uint8_t* vtxSrcTangent = pVtxData->m_pSrcTangent;
								uint8_t* vtxSrcBinormal = pVtxData->m_pSrcBinormal;
								uint8_t* vtxDstPosition = pVideoMemBuffer + pVtxData->m_dstPositionOffset;
								uint8_t* vtxDstNormal = pVideoMemBuffer + pVtxData->m_dstNormalOffset;
								uint8_t* vtxDstTangent = pVideoMemBuffer + pVtxData->m_dstTangentOffset;
								uint8_t* vtxDstBinormal = pVideoMemBuffer + pVtxData->m_dstBinormalOffset;
								// now deform all verts
								for( unsigned int v = 0; v < resMeshData->m_vertexCount; ++v )
								{
									// convert matrix, cause is provided in 3x4 form, we need 4x4 to use d3dxmath
									Matrix m0, m1, m2, m3;
									TriMatCopy( &m0, &deformMatrices[3 * 4 * mesh->GetJointMapping( *boneIndex0 )] );
									TriMatCopy( &m1, &deformMatrices[3 * 4 * mesh->GetJointMapping( *boneIndex1 )] );
									TriMatCopy( &m2, &deformMatrices[3 * 4 * mesh->GetJointMapping( *boneIndex2 )] );
									TriMatCopy( &m3, &deformMatrices[3 * 4 * mesh->GetJointMapping( *boneIndex3 )] );

									// build deform matrix
									Matrix skinMatrix = ( (float)(*boneWeight0) / 255.f ) * m0;
									if( *boneWeight1 )
									{
										skinMatrix += ( (float)(*boneWeight1) / 255.f ) * m1;
									}
									if( *boneWeight2 )
									{
										skinMatrix += ( (float)(*boneWeight2) / 255.f ) * m2;
									}
									if( *boneWeight3 )
									{
										skinMatrix += ( (float)(*boneWeight3) / 255.f ) * m3;
									}

									// "fix" these values: somehow the matrices from gameworld have these components all wrong...
									skinMatrix._44 = 1.f;
									skinMatrix._14 = skinMatrix._24 = skinMatrix._34 = 0.f;

									// matrix is ready, now transform
									D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( vtxDstPosition ), reinterpret_cast<Vector3*>( vtxSrcPosition ), &skinMatrix );
									D3DXVec3TransformNormal( reinterpret_cast<Vector3*>( vtxDstNormal ), reinterpret_cast<Vector3*>( vtxSrcNormal ), &skinMatrix );
									D3DXVec3TransformNormal( reinterpret_cast<Vector3*>( vtxDstTangent ), reinterpret_cast<Vector3*>( vtxSrcTangent ), &skinMatrix );
									D3DXVec3TransformNormal( reinterpret_cast<Vector3*>( vtxDstBinormal ), reinterpret_cast<Vector3*>( vtxSrcBinormal ), &skinMatrix );

									// next vertex
									boneIndex0 += resMeshData->m_bytesPerVertex;
									boneIndex1 += resMeshData->m_bytesPerVertex;
									boneIndex2 += resMeshData->m_bytesPerVertex;
									boneIndex3 += resMeshData->m_bytesPerVertex;
									boneWeight0 += resMeshData->m_bytesPerVertex;
									boneWeight1 += resMeshData->m_bytesPerVertex;
									boneWeight2 += resMeshData->m_bytesPerVertex;
									boneWeight3 += resMeshData->m_bytesPerVertex;
									vtxSrcPosition += resMeshData->m_bytesPerVertex;
									vtxSrcNormal += resMeshData->m_bytesPerVertex;
									vtxSrcTangent += resMeshData->m_bytesPerVertex;
									vtxSrcBinormal += resMeshData->m_bytesPerVertex;
									vtxDstPosition += resMeshData->m_bytesPerVertex;
									vtxDstNormal += resMeshData->m_bytesPerVertex;
									vtxDstTangent += resMeshData->m_bytesPerVertex;
									vtxDstBinormal += resMeshData->m_bytesPerVertex;
								}

								vertexBuffer.Unlock( renderContext );
							}
						}
					}
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2CpuSkinnedModel::GetBatchesForAreaDynamic( Tr2MeshAreaVector* areas, Tr2DynamicMesh* mesh, ITriRenderBatchAccumulator* batches, const Matrix* pm, const Tr2PerObjectData* data )
{
	if( !mesh->GetDisplay() )
	{
		return;
	}

	TriGeometryRes* geomRes = mesh->GetGeometryResource();
	if( !geomRes )
	{
		return;
	}
	int meshIx = mesh->GetMeshIndex();

	TriGeometryResMeshData* meshData = geomRes->GetMeshData( meshIx );
	if( !meshData )
	{
		return;
	}

	for( PTr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		auto material = area->GetMaterialInterface();

		if( !area->GetDisplay() || !material )
		{
			continue;
		}
		TriDynamicGeometryBatch* batch = batches->Allocate<TriDynamicGeometryBatch>();

		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( material );
			batch->SetPerObjectData( data );
			batch->SetGeometryResource( geomRes );
			batch->SetDynamicVertexBuffer( mesh->GetDynamicVertexBuffer() );
			batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount(), area->GetReversed() );
				
			batches->Commit( batch );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2CpuSkinnedModel::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Matrix& m, const Tr2PerObjectData* data )
{
	Matrix* pm = batches->Allocate<Matrix>();

	CCP_ASSERT_M( pm, "No memory available for opaque batches." );
	if ( pm == NULL )
	{
		return;
	}
	*pm = m;

	for( PTr2MeshVector::iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2DynamicMesh* mesh = dynamic_cast<Tr2DynamicMesh*>( *meshIt );
		if( mesh )
		{
			Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );
			if( areas )
			{
				GetBatchesForAreaDynamic( areas, mesh, batches, pm, data );
			}
		}
	}
}
