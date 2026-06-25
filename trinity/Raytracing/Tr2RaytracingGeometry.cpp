// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2RaytracingGeometry.h"

#include "Resources/TriGeometryRes.h"

#include "Tr2Renderer.h"
#include "Shader/Tr2Shader.h"
#include "Shader/Parameter/Tr2GeometryBufferParameter.h"
#include "../Tr2RingBuffer.h"
#include "../Tr2RuntimeGpuBuffer.h"

#include "Tr2MeshArea.h"
#include "ITr2TextureProvider.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <../trinityal/dx12/Tr2BufferALDx12.h>
#endif

namespace
{
std::mutex s_geometryConstantsMutex;
}

//***********************************************************
// Tr2RaytracingPipelineStateManager
//***********************************************************


Tr2RaytracingPipelineStateManager::Tr2RaytracingPipelineStateManager() :
	m_nextName( 0 ),
	m_isDirty( true )
{
}

// remove material
bool Tr2RaytracingPipelineStateManager::AddLibrary( BlueSharedStringW& rayGenName, BlueSharedStringW& missName, Tr2Material* material, const BlueSharedString& techniqueName )
{
	if( !material )
	{
		return false;
	}
	auto shader = material->GetShaderStateInterface();
	if( !shader )
	{
		return false;
	}
	uint32_t techniqueIndex;
	if( !shader->GetTechniqueIndex( techniqueName, techniqueIndex ) )
	{
		return false;
	}
	auto& desc = shader->GetEffectDescription();
	if( desc.techniques[techniqueIndex].libraries.empty() )
	{
		return false;
	}

	AddLibrary( rayGenName, missName, desc.techniques[techniqueIndex].libraries[0] );
	return true;
}

void Tr2RaytracingPipelineStateManager::AddLibrary( BlueSharedStringW& rayGenName, BlueSharedStringW& missName, const Tr2EffectLibrary& library )
{
	auto found = m_libraries.find( library.libraryHandle );
	if( found != m_libraries.end() )
	{
		rayGenName = found->second.first;
		missName = found->second.second;
		return;
	}
	const wchar_t* names[2];
	const wchar_t* exportNamesRaw[2];
	size_t count = 0;

	if( !library.rayGenName.empty() )
	{
		rayGenName = BlueSharedStringW( GetUniqueName() );
		exportNamesRaw[count] = rayGenName.c_str();
		names[count++] = library.rayGenName.c_str();
	}
	if( !library.missName.empty() )
	{
		missName = BlueSharedStringW( GetUniqueName() );
		exportNamesRaw[count] = missName.c_str();
		names[count++] = library.missName.c_str();
	}
	m_pipelineDesc.AddShaders( count, exportNamesRaw, *Tr2EffectStateManager::GetShaderLibraryCode( library.libraryHandle ), names, library.payloadSize, library.localInput.signature );
	m_pipelineDesc.AddGlobalSignature( library.globalInput.signature );

	m_isDirty = true;

	m_libraries[library.libraryHandle] = std::make_pair( rayGenName, missName );
}

BlueSharedStringW Tr2RaytracingPipelineStateManager::AddHitGroup( const Tr2EffectLibrary& library )
{
	auto found = m_hitGroups.find( library.libraryHandle );
	if( found != end( m_hitGroups ) )
	{
		return found->second;
	}

	const wchar_t* names[3];
	std::wstring closestHitName;
	std::wstring anyHitName;
	std::wstring intersectionName;
	const wchar_t* exportNamesRaw[3];
	size_t count = 0;

	if( !library.intersectionName.empty() )
	{
		intersectionName = GetUniqueName();
		exportNamesRaw[count] = intersectionName.c_str();
		names[count++] = library.intersectionName.c_str();
	}
	if( !library.anyHitName.empty() )
	{
		anyHitName = GetUniqueName();
		exportNamesRaw[count] = anyHitName.c_str();
		names[count++] = library.anyHitName.c_str();
	}
	if( !library.closestHitName.empty() )
	{
		closestHitName = GetUniqueName();
		exportNamesRaw[count] = closestHitName.c_str();
		names[count++] = library.closestHitName.c_str();
	}

	m_pipelineDesc.AddShaders( count, exportNamesRaw, *Tr2EffectStateManager::GetShaderLibraryCode( library.libraryHandle ), names, library.payloadSize, library.localInput.signature );
	auto hitGroup = BlueSharedStringW( GetUniqueName() );
	m_pipelineDesc.AddHitGroup( hitGroup.c_str(), anyHitName.c_str(), closestHitName.c_str(), intersectionName.c_str(), library.localInput.signature );
	m_pipelineDesc.AddLocalSignature( library.localInput.signature );
	m_hitGroups[library.libraryHandle] = hitGroup;
	m_isDirty = true;
	return hitGroup;
}

Tr2RtPipelineStateAL Tr2RaytracingPipelineStateManager::GetPipelineState( Tr2RenderContext& renderContext )
{
	if( m_isDirty || !m_pipelineState.IsValid() )
	{
		if( SUCCEEDED( m_pipelineState.CreateRtPipelineState( m_pipelineDesc, renderContext.GetPrimaryRenderContext() ) ) )
		{
			m_isDirty = false;
		}
	}
	return m_pipelineState;
}

std::wstring Tr2RaytracingPipelineStateManager::GetUniqueName()
{
	return L"rtShader_" + std::to_wstring( m_nextName++ );
}

// ***************** Tr2RaytracingMesh *****************

Tr2RaytracingMesh::Tr2RaytracingMesh() :
	m_skinnedVertices( nullptr ),
	m_meshIndex( 0 ),
	m_boneOffset( 0 ),
	m_skinnedVertexOffset( 0 ),
	m_morphAnimationDataOffset( 0 ),
	m_morphAnimationDataCount( 0 ),
	m_isDirty( true ),
	m_screenSize( 0.f ),
	m_lodIndex( 0 )
{
}

void Tr2RaytracingMesh::UpdateRtMesh( TriGeometryRes* geometry, uint32_t meshIndex, float screenSize )
{
	if( m_geometry != geometry || m_meshIndex != meshIndex )
	{
		//Geometry has changed, invalidate everything.
		m_geometry = geometry;
		m_meshIndex = meshIndex;
		m_screenSize = screenSize;
		if( m_geometry && m_geometry->IsGood() )
		{
			m_lodIndex = m_geometry->GetLodIndexForScreenSize( m_meshIndex, m_screenSize );
		}
		else
		{
			m_lodIndex = -1;
		}

		m_transforms.clear();

		m_isDirty = true;
	}
	else if( m_screenSize != screenSize || m_lodIndex == -1 )
	{
		m_screenSize = screenSize;

		//screen size has changed, so check if we've changed to a different LOD
		auto lodIndex = -1;
		if( m_geometry && m_geometry->IsGood() )
		{
			lodIndex = m_geometry->GetLodIndexForScreenSize( m_meshIndex, m_screenSize );
		}

		m_isDirty |= lodIndex != m_lodIndex;
		m_lodIndex = lodIndex;
	}
}

bool Tr2RaytracingMesh::SetBoneTransforms( size_t count, const Float4x3* transforms, uint32_t offset )
{
	auto newSize = count * 3 * 4;
	if( m_transforms.size() != newSize )
	{
		m_transforms.resize( newSize );
		m_isDirty = true;
	}

	if( newSize > 0 && memcmp( m_transforms.data(), transforms, newSize * sizeof( float ) ) != 0 )
	{
		memcpy( m_transforms.data(), transforms, count * sizeof( Float4x3 ) );
		m_isDirty = true;
	}

	m_boneOffset = offset;

	return m_isDirty;
}

bool Tr2RaytracingMesh::SetMorphAnimations( size_t count, const Tr2MorphTargetAnimationData* morphTargets, uint32_t morphTargetAnimationDataOffset )
{
	auto newSize = count * sizeof( Tr2MorphTargetAnimationData );
	if( m_morphAnimationDatas.size() != newSize )
	{
		m_morphAnimationDatas.resize( newSize );
		m_isDirty = true;
	}

	if( newSize > 0 && memcmp( m_morphAnimationDatas.data(), morphTargets, newSize ) != 0 )
	{
		memcpy( m_morphAnimationDatas.data(), morphTargets, newSize );
		m_isDirty = true;
	}

	m_morphAnimationDataOffset = morphTargetAnimationDataOffset;
	m_morphAnimationDataCount = uint32_t( count );

	return m_isDirty;
}

bool Tr2RaytracingMesh::IsGood() const
{
	return m_geometry && m_geometry->IsGood() && GetCurrentLodData();
}

bool Tr2RaytracingMesh::IsGoodForArea( uint32_t area ) const
{
	if( !m_geometry || !m_geometry->IsGood() )
	{
		return false;
	}
	auto data = GetCurrentLodData();
	if( !data )
	{
		return false;
	}
	return area < data->m_areas.size();
}


bool Tr2RaytracingMesh::GetAndResetDirtyFlag()
{
	if( m_isDirty )
	{
		m_isDirty = false;
		return true;
	}
	return false;
}

void Tr2RaytracingMesh::MarkDirty()
{
	m_isDirty = true;
}

TriGeometryResLodData* Tr2RaytracingMesh::GetCurrentLodData() const
{
	return m_geometry->GetMeshLod( m_meshIndex, m_lodIndex );
}

TriGeometryResLodData* Tr2RaytracingMesh::GetHighestLodData() const
{
	return m_geometry->GetMeshLod( m_meshIndex, 0 );
}

uint32_t Tr2RaytracingMesh::GetTransformOffset() const
{
	return m_boneOffset;
}

const Tr2BufferAL* Tr2RaytracingMesh::GetSkinnedVertexBuffer() const
{
	return m_skinnedVertices;
}

void Tr2RaytracingMesh::SetSkinnedVertices( const Tr2BufferAL& buffer, uint32_t offset )
{
	m_skinnedVertices = &buffer;
	m_skinnedVertexOffset = offset;
}

uint32_t Tr2RaytracingMesh::GetSkinnedVertexOffset() const
{
	return m_skinnedVertexOffset;
}

const Tr2BufferAL& Tr2RaytracingMesh::GetVertexBuffer() const
{
	return GetCurrentLodData()->m_vertexAllocation.GetBuffer();
}

const Tr2BufferAL& Tr2RaytracingMesh::GetIndexBuffer() const
{
	return GetCurrentLodData()->m_indexAllocation.GetBuffer();
}

// ***************** Tr2RaytracingMeshArea *****************

Tr2RaytracingMeshArea::Tr2RaytracingMeshArea( uint32_t index ) :
	m_areaIndex( index ),
	m_blasOutdated( true )
{
}

/*
* Suggestion from DXR specs https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#general-tips-for-building-acceleration-structures
----------------------------------------------------------------------------------------------------
PREFER_ FAST_ TRACE	||  PREFER_ FAST_ BUILD	||	ALLOW_ UPDATE	||	EXAMPLE
------------------		--------------------	----------------   -----------
no					||		yes				||		no			||	Fully dynamic geometry like particles, destruction,
																		changing prim counts or moving wildly (explosions etc),
																		where per-frame rebuild is required.
---------------------------------------------------------------------------------------------------
no					||		yes				||		yes			||	Lower LOD dynamic objects, unlikely to be hit by too many rays but
																	still need to be refitted per frame to be correct.
---------------------------------------------------------------------------------------------------
yes					||		no				||		no			||	Default choice for static level geometry
----------------------------------------------------------------------------------------------------
yes					||		no				||		yes			||	Hero character, high-LOD dynamic objects that are expected to be hit by a
																	significant number of rays.
----------------------------------------------------------------------------------------------------

For compaction, a general rule is: compact for static geometry, for fully dynamic geometry there's no benefit from compaction
*/

const Tr2RtBottomLevelAccelerationStructureAL& Tr2RaytracingMeshArea::BuildBlas( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext )
{
	auto lod = mesh.GetCurrentLodData();
	if( !lod || m_areaIndex >= lod->m_areas.size() )
	{
		return m_blas;
	}
	if( lod->m_areas[m_areaIndex].m_isSkinned || lod->m_areas[m_areaIndex].m_isMorphed )
	{
		if( m_blas.IsValid() && !m_blasOutdated )
		{
			return m_blas; //Nothing to do, we're up to date and valid
		}

		auto skinnedVB = mesh.GetSkinnedVertexBuffer();
		if( !skinnedVB )
		{
			return m_blas; //No skinned vertices, can't build BLAS
		}

		//At this point we know that the BLAS is either invalid or dirty. We at least need to update it.
		bool update = true;
		bool rebuild = false;

		if( !m_blas.IsValid() )
		{
			//the BLAS isn't valid, so we need to rebuild it.
			update = false;
			rebuild = true;
		}

		Tr2RtGeometryAL geometry;
		geometry.positions = Tr2RtPositionStreamAL(
			*skinnedVB,
			3 * sizeof( float ),
			lod->m_vertexCount,
			mesh.GetSkinnedVertexOffset(),
			0,
			Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
		geometry.indices = Tr2RtIndicesStreamAL( lod->m_indexAllocation.GetBuffer(), lod->m_indexAllocation.GetStride(), lod->m_indexAllocation.GetStartIndex() + lod->m_areas[m_areaIndex].m_firstIndex, lod->m_areas[m_areaIndex].m_primitiveCount * 3 );

		if( update )
		{
			if( FAILED( m_blas.Update( geometry, renderContext.GetPrimaryRenderContext() ) ) )
			{
				//This will fail gracefully whenever the LOD changes, so fall back to rebuild in this case.
				//CCP_LOGERR( "Failed to update BLAS, recreating it instead" );
				rebuild = true; //update failed, so rebuild it instead
			}
		}

		if( rebuild )
		{
			CCP_STATS_ZONE( "BLAS rebuild" );

			auto capacity = geometry;
			auto highestLod = mesh.GetHighestLodData();

			if( highestLod != lod )
			{
				// provide the capacity for the BLAS to grow into
				capacity.positions = Tr2RtPositionStreamAL(
					geometry.positions.m_vertexBuffer,
					3 * sizeof( float ),
					highestLod->m_vertexCount,
					mesh.GetSkinnedVertexOffset(),
					geometry.positions.m_positionOffset,
					Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
				capacity.indices = Tr2RtIndicesStreamAL( highestLod->m_indexAllocation.GetBuffer(), highestLod->m_indexAllocation.GetStride(), highestLod->m_indexAllocation.GetStartIndex() + highestLod->m_areas[m_areaIndex].m_firstIndex, highestLod->m_areas[m_areaIndex].m_primitiveCount * 3 );
			}

			if( FAILED( m_blas.Create( geometry, capacity, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_BUILD | Tr2RtBuildFlags::ALLOW_UPDATE, renderContext.GetPrimaryRenderContext() ) ) )
			{
				CCP_LOGERR( "Failed to create BLAS!" );
			}
		}

		m_blasOutdated = false;

		return m_blas;
	}

	if( !lod->m_areas[m_areaIndex].m_staticBlas.IsValid() )
	{
		auto vertexStream = Tr2RtPositionStreamAL(
			lod->m_vertexAllocation.GetBuffer(),
			lod->m_vertexAllocation.GetStride(),
			lod->m_vertexCount,
			lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
			0,
			Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
		auto indexStream = Tr2RtIndicesStreamAL(
			lod->m_indexAllocation.GetBuffer(),
			lod->m_indexAllocation.GetStride(),
			lod->m_indexAllocation.GetStartIndex() + lod->m_areas[m_areaIndex].m_firstIndex,
			lod->m_areas[m_areaIndex].m_primitiveCount * 3 );

		lod->m_areas[m_areaIndex].m_staticBlas.Create(
			{ vertexStream, indexStream },
			Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY,
			Tr2RtBuildFlags::PREFER_FAST_TRACE,
			renderContext.GetPrimaryRenderContext() );
	}
	return lod->m_areas[m_areaIndex].m_staticBlas;
}

const Tr2ConstantBufferAL* Tr2RaytracingMeshArea::GetGeometryConstants( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext ) const
{
	TriGeometryResLodData* lod = mesh.GetCurrentLodData();
	if( !lod || m_areaIndex >= lod->m_areas.size() )
	{
		return nullptr; //No mesh data or area index out of bounds
	}
	std::scoped_lock lock( s_geometryConstantsMutex );

	if( !lod->m_areas[m_areaIndex].m_rtGeometryConstants.IsValid() )
	{
		if( SUCCEEDED( lod->m_areas[m_areaIndex].m_rtGeometryConstants.Create( sizeof( TriRtGeometryConstants ), renderContext.GetPrimaryRenderContext() ) ) )
		{
			TriRtGeometryConstants* data;
			if( SUCCEEDED( lod->m_areas[m_areaIndex].m_rtGeometryConstants.Lock( (void**)&data, renderContext ) ) )
			{
				*data = TriRtGeometryConstants{};

				Tr2VertexDefinition def;
				Tr2EffectStateManager::GetVertexDeclarationElements( lod->m_mesh->m_vertexDeclarationHandle, def );
				for( auto it = begin( def.m_items ); it != end( def.m_items ); ++it )
				{
					if( it->m_usage == Tr2VertexDefinition::POSITION && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->positionOffset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->positionType = it->m_dataType;

						uint32_t type = data->positionType;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT32_3,
							"position type has to be FLOAT32_3!" );
					}

					if( it->m_usage == Tr2VertexDefinition::NORMAL && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->normalOffset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->normalType = it->m_dataType;

						uint32_t type = data->normalType;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_3 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_3,
							"normal type has to be FLOAT16_3 or FLOAT32_3!" );
					}

					if( it->m_usage == Tr2VertexDefinition::TANGENT && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->tangentOffset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->tangentType = it->m_dataType;

						uint32_t type = data->tangentType;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_3 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_3 ||
								type == Tr2VertexDefinition::DataType::UBYTE_4_NORM ||
								type == Tr2VertexDefinition::DataType::USHORT_4_NORM ||
								type == Tr2VertexDefinition::DataType::SHORT_4_NORM,
							"tangent type has to be FLOAT16_3 or FLOAT32_3 or UBYTE_4_NORM or USHORT_4_NORM or SHORT_4_NORM!" );
					}

					if( it->m_usage == Tr2VertexDefinition::BITANGENT && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->bitangentOffset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->bitangentType = it->m_dataType;

						uint32_t type = data->bitangentType;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_3 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_3,
							"bitangent type has to be FLOAT16_3 or FLOAT32_3!" );
					}

					if( it->m_usage == Tr2VertexDefinition::TEXCOORD && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->texCoord0Offset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->texCoord0Type = it->m_dataType;

						uint32_t type = data->texCoord0Type;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_2 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_2,
							"texCoord0 type has to be FLOAT16_2 or FLOAT32_2!" );
					}

					if( it->m_usage == Tr2VertexDefinition::TEXCOORD && it->m_usageIndex == 1 && it->m_stream == 0 )
					{
						data->texCoord1Offset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->texCoord1Type = it->m_dataType;

						uint32_t type = data->texCoord1Type;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_2 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_2,
							"texCoord1 type has to be FLOAT16_2 or FLOAT32_2!" );
					}

					if( it->m_usage == Tr2VertexDefinition::TEXCOORD && it->m_usageIndex == 2 && it->m_stream == 0 )
					{
						data->texCoord2Offset = it->m_offset + lod->m_vertexAllocation.GetOffset();
						data->texCoord2Type = it->m_dataType;

						uint32_t type = data->texCoord2Type;
						CCP_ASSERT_M(
							type == Tr2VertexDefinition::DataType::FLOAT16_2 ||
								type == Tr2VertexDefinition::DataType::FLOAT32_2,
							"texCoord2 type has to be FLOAT16_2 or FLOAT32_2!" );
					}

					// skipping blending data because we get gpu skinned meshes as input (see SkinVertices.fx), so it shouldn't be required for raytracing shaders
				}

				data->vertexBufferId = lod->m_vertexAllocation.GetBuffer().GetSrvIndexInHeap();
				data->vertexBufferStride = lod->m_vertexAllocation.GetStride();
				data->indexBufferId = lod->m_indexAllocation.GetBuffer().GetSrvIndexInHeap();
				data->indexBufferStride = lod->m_indexAllocation.GetStride();
				data->indexOffset = lod->m_areas[m_areaIndex].m_firstIndex * lod->m_indexAllocation.GetStride() + lod->m_indexAllocation.GetOffset();

				lod->m_areas[m_areaIndex].m_rtGeometryConstants.Unlock( renderContext );
			}
		}
	}
	else
	{
		TriRtGeometryConstants* data;
		if( SUCCEEDED( lod->m_areas[m_areaIndex].m_rtGeometryConstants.Lock( (void**)&data, renderContext ) ) )
		{
			data->vertexBufferId = lod->m_vertexAllocation.GetBuffer().GetSrvIndexInHeap();
			data->vertexBufferStride = lod->m_vertexAllocation.GetStride();
			data->indexBufferId = lod->m_indexAllocation.GetBuffer().GetSrvIndexInHeap();
			data->indexBufferStride = lod->m_indexAllocation.GetStride();
			data->indexOffset = lod->m_areas[m_areaIndex].m_firstIndex * lod->m_indexAllocation.GetStride() + lod->m_indexAllocation.GetOffset();
			lod->m_areas[m_areaIndex].m_rtGeometryConstants.Unlock( renderContext );
		}
	}
	return &lod->m_areas[m_areaIndex].m_rtGeometryConstants;
}

// ***************** Tr2RaytracingGeometry *****************
Tr2RaytracingGeometry::Tr2RaytracingGeometry()
{
	m_skinVerticesEffect.CreateInstance();
	m_skinVerticesEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/skinvertices.fx" );
	m_skinVerticesEffect->SetParameter( m_inVertexBufferTechniqueName, static_cast<ITr2GpuBuffer*>( nullptr ) );
	m_skinVerticesEffect->SetParameter( m_outVertexBufferTechniqueName, static_cast<ITr2GpuBuffer*>( nullptr ) );
}

Tr2BufferAL* Tr2RaytracingGeometry::GetGpuBuffer( unsigned )
{
	return const_cast<Tr2BufferAL*>( &m_tlas.GetBuffer() );
}

void Tr2RaytracingGeometry::BeginSceneUpdate()
{
	m_geometryData.clear();
	m_usedResources.Clear();
}


void Tr2RaytracingGeometry::EndSceneUpdate( Tr2RenderContext& renderContext, int32_t numRaycasters, Tr2RtShaderTableDescriptionAL** shaderTableDescs, Tr2RaytracingPipelineStateManager** pipelineManagers )
{
	for( auto& threadLocalData : m_threadLocalGeometryData )
	{
		m_geometryData.insert( end( m_geometryData ), begin( threadLocalData ), end( threadLocalData ) );
		threadLocalData.clear();
	}
	for( auto& threadLocalData : m_threadLocalUsedResources )
	{
		m_usedResources.Add( threadLocalData );
		threadLocalData.Clear();
	}

	PrepareShaderTableDescription( renderContext, numRaycasters, shaderTableDescs, pipelineManagers );
	TransformMeshes( renderContext );
	BuildAccelerationStructures( renderContext );
}

void Tr2RaytracingGeometry::PrepareShaderTableDescription( Tr2RenderContext& renderContext, int32_t numRaycasters, Tr2RtShaderTableDescriptionAL** shaderTableDescs, Tr2RaytracingPipelineStateManager** pipelineManagers )
{
	CCP_ASSERT_M( numRaycasters > 0, "numRaycasters has to be greater than zero! Why are you preparing shader tables when you have no raycasters?" );

	GPU_REGION( renderContext, "PrepareShaderTableDescription" );
	CCP_STATS_ZONE( __FUNCTION__ );
	for( int32_t i = 0; i < numRaycasters; i++ )
	{
		*shaderTableDescs[i] = Tr2RtShaderTableDescriptionAL();
		shaderTableDescs[i]->Reserve( m_geometryData.size() );
	}

	std::vector<std::map<uint32_t, std::pair<BlueSharedStringW, uint32_t>>> seenLibraries;
	seenLibraries.resize( numRaycasters );

	uint32_t materialIndex = 0;
	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
		Tr2RtLocalMaterialDescriptionAL material;
		if( !it->material )
		{
			continue;
		}
		auto shader = it->material->GetShaderStateInterface();
		if( !shader )
		{
			continue;
		}
		uint32_t techniqueIndex;
		if( !shader->GetTechniqueIndex( m_rtShadowTechniqueName, techniqueIndex ) )
		{
			continue;
		}
		auto& desc = shader->GetEffectDescription();
		if( desc.techniques[techniqueIndex].libraries.empty() )
		{
			continue;
		}
		auto& lib = desc.techniques[techniqueIndex].libraries[0];
		if( !lib.anyHitName.empty() )
		{
			it->isTransparent = true;
		}

		if( it->perObjectData )
		{
			material.SetConstants( Tr2Renderer::GetPerObjectPSStartRegister(), *it->perObjectData );
		}
		if( it->vertexBufferData )
		{
			material.SetConstants( Tr2Renderer::GetPerObjectRTVertexBufferDataRegister(), *it->vertexBufferData );
		}
		it->material->ApplyMaterialDataForRtMaterial( techniqueIndex, material, renderContext );

		bool consumedMaterialIndex = false;
		for( int32_t i = 0; i < numRaycasters; i++ )
		{
			auto foundLib = seenLibraries[i].find( lib.libraryHandle );
			if( foundLib == end( seenLibraries[i] ) )
			{
				auto hitGroupName = pipelineManagers[i]->AddHitGroup( lib );
				seenLibraries[i][lib.libraryHandle] = std::make_pair( hitGroupName, materialIndex );
				shaderTableDescs[i]->AddHitGroup( hitGroupName.c_str(), material );
				it->materialIndex = materialIndex;
				consumedMaterialIndex = true;
			}
			else if( !lib.localInput.signature.registers.empty() )
			{
				shaderTableDescs[i]->AddHitGroup( foundLib->second.first.c_str(), material );
				it->materialIndex = materialIndex;
				consumedMaterialIndex = true;
			}
			else
			{
				it->materialIndex = foundLib->second.second;
			}
		}
		if( consumedMaterialIndex )
		{
			materialIndex++;
		}
	}
}

namespace
{
struct SkinningShaderCBuffer
{
	uint32_t vertexCount;
	uint32_t stride;
	uint32_t positionOffset;
	uint32_t boneOffset;
	uint32_t boneWeightsOffset;
	uint32_t transformOffset;
	uint32_t inVB;
	uint32_t outVB;
	uint32_t outVBOffset;
	uint32_t morphAnimationDataOffset;
	uint32_t morphAnimationDataCount;
	uint32_t morphTargetPositionOffset;
	uint32_t morphTargetStride;
	uint32_t morphTargetSize;
	uint32_t bakedMorphTargetPositionOffset;
	uint32_t _padding;
};
}

void Tr2RaytracingGeometry::TransformMeshes( Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "TransformMeshes" );
	CCP_STATS_ZONE( __FUNCTION__ );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	renderContext.PushDisableUAVBarriersDx12();
	ON_BLOCK_EXIT( [&] { renderContext.PopDisableUAVBarriersDx12(); } );
#endif

	for( auto& geom : m_geometryData )
	{
		if( geom.materialIndex == INVALID_MATERIAL )
		{
			continue;
		}
		if( geom.area->IsBlasOutdated() )
		{
			geom.mesh->m_isDirty = true;
		}
	}

	std::vector<GeometryData*> outdatedMeshes;
	outdatedMeshes.reserve( m_geometryData.size() );

	{
		CCP_STATS_ZONE( "Prepare BoneTransformBuffer" );
		Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( renderContext );
	}
	{
		CCP_STATS_ZONE( "Prepare MorphTargetAnimationDataBuffer" );
		Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( renderContext );
	}

	uint32_t skinnedVertexCount = 0;

	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
		if( it->materialIndex == INVALID_MATERIAL )
		{
			continue;
		}

		Tr2RaytracingMesh* mesh = it->mesh;
		TriGeometryResLodData* lod = mesh->GetCurrentLodData();
		if( lod && ( lod->m_areas[it->area->GetAreaIndex()].m_isSkinned || lod->m_areas[it->area->GetAreaIndex()].m_isMorphed ) )
		{
			if( !mesh->GetAndResetDirtyFlag() )
			{
				continue;
			}

			outdatedMeshes.push_back( &( *it ) );

			skinnedVertexCount += lod->m_vertexCount;
		}
	}

	if( outdatedMeshes.empty() )
	{
		return;
	}

	if( !m_skinnedVertices.IsValid() || m_skinnedVertices.GetDesc().count < 3 * skinnedVertexCount )
	{
		const uint32_t vertexSize = sizeof( float );
		m_skinnedVertices.Create( Tr2BufferDescriptionAL( vertexSize, 3 * skinnedVertexCount, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ ), nullptr, renderContext.GetPrimaryRenderContext() );
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_skinnedVertices.TrinityALImpl_GetObject()->GetGpuResource();
	barrier.Transition.StateBefore = m_skinnedVertices.TrinityALImpl_GetObject()->GetDefaultState();
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	renderContext.ResourceBarrierDx12( barrier );
	renderContext.FlushBarriersDx12();

	std::swap( barrier.Transition.StateBefore, barrier.Transition.StateAfter );

	ON_BLOCK_EXIT( [&] { renderContext.ResourceBarrierDx12( barrier ); } );
#endif

	CTr2RuntimeGpuBuffer inVB;
	CTr2RuntimeGpuBuffer outVB;

	Tr2GeometryBufferParameterPtr inVbParam = BlueCastPtr( m_skinVerticesEffect->GetResourceByName( m_inVertexBufferTechniqueName.c_str() ) );
	Tr2GeometryBufferParameterPtr outVbParam = BlueCastPtr( m_skinVerticesEffect->GetResourceByName( m_outVertexBufferTechniqueName.c_str() ) );

	if( !outdatedMeshes.empty() )
	{
		CCP_STATS_ZONE( "Dispatch" );

		auto perObjVSRegister = Tr2Renderer::GetPerObjectVSStartRegister();

		// compute shader desc
		auto shader = m_skinVerticesEffect->GetShaderStateInterface();
		if( !shader )
		{
			return;
		}
		auto& shaderDesc = shader->GetEffectDescription();
		// the shader only has one pass
		auto& pass = shaderDesc.techniques[0].passes[0];
		auto& stage = pass.stageInputs[Tr2RenderContextEnum::COMPUTE_SHADER];
		if( !stage.m_exists )
		{
			return;
		}

		bool firstIteration = true;
		if( !m_skinVerticesData.IsValid() )
		{
			m_skinVerticesData.Create( uint32_t( sizeof( SkinningShaderCBuffer ) ), renderContext.GetPrimaryRenderContext() );
		}

#if TRINITY_PLATFORM != TRINITY_DIRECTX12
		outVB.m_buffer = m_skinnedVertices;
		outVbParam->SetGpuBuffer( &outVB );
#endif
		uint32_t outOffset = 0;


		for( auto it = begin( outdatedMeshes ); it != end( outdatedMeshes ); ++it )
		{
			Tr2RaytracingMesh* mesh = ( *it )->mesh;
			TriGeometryResLodData* lod = mesh->GetCurrentLodData();
			if( !lod )
			{
				return;
			}

			auto vertexCount = lod->m_vertexCount;

			SkinningShaderCBuffer* constData;
			m_skinVerticesData.Lock( (void**)&constData, renderContext );

			constData->vertexCount = vertexCount;
			constData->stride = lod->m_vertexAllocation.GetStride() / 4;
			auto offsets = FindOffsets( lod->m_mesh->m_vertexDeclarationHandle );
			constData->positionOffset = offsets.positionOffset / 4 + lod->m_vertexAllocation.GetOffset() / 4;
			constData->boneOffset = offsets.boneOffset / 4 + lod->m_vertexAllocation.GetOffset() / 4;
			constData->boneWeightsOffset = offsets.boneWeightsOffset == 0xffffffff ? offsets.boneWeightsOffset : offsets.boneWeightsOffset / 4 + lod->m_vertexAllocation.GetOffset() / 4;
			constData->transformOffset = mesh->GetTransformOffset();
			constData->inVB = lod->m_vertexAllocation.GetBuffer().GetSrvIndexInHeap();
			constData->outVB = m_skinnedVertices.GetUavIndexInHeap();
			constData->outVBOffset = outOffset * 3;
			constData->morphAnimationDataOffset = 0;
			constData->morphAnimationDataCount = 0;
			constData->morphTargetPositionOffset = 0;
			constData->morphTargetStride = 0;
			constData->morphTargetSize = 0;
			constData->bakedMorphTargetPositionOffset = std::numeric_limits<uint32_t>::max();
			if( lod->m_morphTargetAllocation.IsValid() )
			{
				auto morphOffsets = FindOffsets( lod->m_morphVertexDeclaration );
				constData->morphAnimationDataOffset = mesh->m_morphAnimationDataOffset;
				constData->morphAnimationDataCount = mesh->m_morphAnimationDataCount;
				constData->morphTargetPositionOffset = ( lod->m_morphTargetAllocation.GetOffset() + sizeof( TriMorphTargetGeometryConstants ) + morphOffsets.positionOffset ) >> 2;
				constData->morphTargetStride = lod->m_bytesPerMorphTargetVertex >> 2;
				constData->morphTargetSize = ( lod->m_bytesPerMorphTargetVertex * vertexCount ) >> 2;
				constData->bakedMorphTargetPositionOffset = ( *it )->bakedMorphOffset;
			}
			m_skinVerticesData.Unlock( renderContext );

			renderContext.SetConstants( m_skinVerticesData, Tr2RenderContextEnum::COMPUTE_SHADER, perObjVSRegister );

#if TRINITY_PLATFORM != TRINITY_DIRECTX12
			inVB.m_buffer = lod->m_vertexAllocation.GetBuffer();
			inVbParam->SetGpuBuffer( &inVB );
#endif
			// cheat a bit instead of calling RunComputeShader() to make things more performant
			if( firstIteration )
			{
				firstIteration = false;

				// ApplyAllStateForPass(0, i)
				renderContext.m_esm.ApplyShaderProgram( pass.shaderProgram );
				renderContext.m_esm.ApplyRenderStates( pass.renderStates );

				// for future performance:
				// to speed things up is we could skip the ApplyMaterialDataForPass()
				// and instead shove the two buffers in via m_descriptorCache in renderContextDx12
				// this would save some performance but cost us having ugly code
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
				m_skinVerticesEffect->ApplyMaterialDataForPass( 0, 0, renderContext );
#endif
			}
#if TRINITY_PLATFORM != TRINITY_DIRECTX12
			m_skinVerticesEffect->ApplyMaterialDataForPass( 0, 0, renderContext );
#endif
			renderContext.RunComputeShader( ( vertexCount + 63 ) / 64, 1, 1 );

			mesh->SetSkinnedVertices( m_skinnedVertices, outOffset );

			outOffset += lod->m_vertexCount;
		}
	}
#if TRINITY_PLATFORM != TRINITY_DIRECTX12
	renderContext.SetResourceSet( Tr2ResourceSetAL() );
	inVbParam->SetGpuBuffer( static_cast<ITr2GpuBuffer*>( nullptr ) );
	outVbParam->SetGpuBuffer( static_cast<ITr2GpuBuffer*>( nullptr ) );
#endif
}

void Tr2RaytracingGeometry::BuildAccelerationStructures( Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "BuildAccelerationStructures" );
	CCP_STATS_ZONE( __FUNCTION__ );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	renderContext.FlushBarriersDx12();
#endif

	static std::vector<Tr2RtInstanceAL> instances;
	instances.clear();
	Tr2RtBottomLevelAccelerationStructureAL invalidBlas;
	if( instances.capacity() < m_geometryData.size() )
	{
		instances.reserve( m_geometryData.size() );
	}
	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
		if( it->materialIndex == INVALID_MATERIAL )
		{
			continue;
		}

		Tr2RtInstanceAL instance;
		instance.flags = it->isTransparent ? Tr2RtInstanceAL::FORCE_NON_OPAQUE : Tr2RtInstanceAL::NONE;
		instance.materialIndex = it->materialIndex;
		instance.blas = it->area->BuildBlas( *it->mesh, renderContext ).TrinityALImpl_GetObject();
		if( !instance.blas || instance.blas == invalidBlas.TrinityALImpl_GetObject() )
		{
			continue;
		}

		if( it->worldTransforms )
		{
			for( uint32_t i = 0; i < it->instanceCount; ++i )
			{
				memcpy( instance.transform, it->worldTransforms + i, sizeof( Float4x3 ) );
				instances.push_back( instance );
			}
		}
		else
		{
			auto m = Transpose( it->worldTransform );
			memcpy( instance.transform[0], &m.GetX(), 4 * sizeof( float ) );
			memcpy( instance.transform[1], &m.GetY(), 4 * sizeof( float ) );
			memcpy( instance.transform[2], &m.GetZ(), 4 * sizeof( float ) );
			instances.push_back( instance );
		}
	}

	{
		CCP_STATS_ZONE( "TLAS update" );
		if( instances.empty() )
		{
			m_tlas = Tr2RtTopLevelAccelerationStructureAL();
		}
		else if( FAILED( m_tlas.Update( instances.size(), instances.data(), renderContext ) ) )
		{
			CCP_STATS_ZONE( "TLAS create" );
			m_tlas.Create( instances.size(), instances.data(), Tr2RtBuildFlags::PREFER_FAST_TRACE, renderContext.GetPrimaryRenderContext() );
		}
	}
}

void Tr2RaytracingGeometry::AddGeometry( Tr2RaytracingMesh& mesh, Tr2RaytracingMeshArea& area, Tr2Material* material, const Tr2ConstantBufferAL* perObjectData, const Tr2ConstantBufferAL* vertexBufferData, const Matrix& worldTransform, uint32_t bakedMorphOffset )
{
	if( !mesh.IsGoodForArea( area.GetAreaIndex() ) )
	{
		return;
	}

	GeometryData obj;
	obj.mesh = &mesh;
	obj.area = &area;
	obj.material = material;
	obj.perObjectData = perObjectData;
	obj.vertexBufferData = vertexBufferData;
	obj.worldTransform = worldTransform;
	obj.materialIndex = INVALID_MATERIAL;
	obj.isTransparent = false;
	obj.bakedMorphOffset = bakedMorphOffset;
	m_threadLocalGeometryData.local().push_back( obj );
}

void Tr2RaytracingGeometry::AddGeometry( Tr2RaytracingMesh& mesh, Tr2RaytracingMeshArea& area, Tr2Material* material, const Tr2ConstantBufferAL* perObjectData, const Tr2ConstantBufferAL* vertexBufferData, const Float4x3* worldTransforms, size_t instanceCount, uint32_t bakedMorphOffset )
{
	if( !mesh.IsGoodForArea( area.GetAreaIndex() ) )
	{
		return;
	}

	GeometryData obj;
	obj.mesh = &mesh;
	obj.area = &area;
	obj.material = material;
	obj.perObjectData = perObjectData;
	obj.vertexBufferData = vertexBufferData;
	obj.worldTransforms = worldTransforms;
	obj.instanceCount = uint32_t( instanceCount );
	obj.materialIndex = INVALID_MATERIAL;
	obj.isTransparent = false;
	obj.bakedMorphOffset = bakedMorphOffset;
	m_threadLocalGeometryData.local().push_back( obj );
}

bool Tr2RaytracingGeometry::HasGeometry() const
{
	return m_tlas.IsValid();
}

Tr2RtTopLevelAccelerationStructureAL Tr2RaytracingGeometry::GetTLAS() const
{
	return m_tlas;
}

const Tr2BindlessResourcesAL& Tr2RaytracingGeometry::GetBindlessResources() const
{
	return m_usedResources;
}

void Tr2RaytracingGeometry::AddBindlessResources( const Tr2MeshAreaVector& areas, const Tr2RaytracingMesh& rtMesh )
{
	auto& m_usedResources = m_threadLocalUsedResources.local();
	for( auto& area : areas )
	{
		uint32_t techniqueIndex;
		if( !area->GetMaterialInterface() || !area->GetMaterialInterface()->GetShaderStateInterface() )
		{
			continue;
		}
		if( area->GetMaterialInterface()->GetShaderStateInterface()->GetTechniqueIndex( m_rtShadowTechniqueName, techniqueIndex ) )
		{
			area->GetMaterialInterface()->GetUsedBindlessTextures( techniqueIndex, m_usedResources );
		}
	}
	m_usedResources.Add( rtMesh.GetVertexBuffer() );
	m_usedResources.Add( rtMesh.GetIndexBuffer() );
}

Tr2RaytracingGeometry::VtxOffsets Tr2RaytracingGeometry::FindOffsets( unsigned declHandle )
{
	auto found = m_offsets.find( declHandle );
	if( found != m_offsets.end() )
	{
		return found->second;
	}
	VtxOffsets offsets = { 0, 0, 0xffffffff };
	Tr2VertexDefinition def;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( declHandle, def ) )
	{
		return VtxOffsets();
	}
	for( auto it = begin( def.m_items ); it != end( def.m_items ); ++it )
	{
		if( it->m_usage == Tr2VertexDefinition::POSITION && it->m_usageIndex == 0 )
		{
			offsets.positionOffset = it->m_offset;
		}
		else if( it->m_usage == Tr2VertexDefinition::BLENDINDICES && it->m_usageIndex == 0 )
		{
			offsets.boneOffset = it->m_offset;
		}
		else if( it->m_usage == Tr2VertexDefinition::BLENDWEIGHTS && it->m_usageIndex == 0 )
		{
			offsets.boneWeightsOffset = it->m_offset;
		}
	}
	m_offsets[declHandle] = offsets;
	return offsets;
}

void Tr2RaytracingGeometry::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_skinVerticesData = Tr2ConstantBufferAL();
	}
}
