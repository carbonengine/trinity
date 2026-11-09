// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2RaytracingGeometry.h"

#include "Resources/TriGeometryRes.h"

#include "Tr2Renderer.h"
#include "Shader/Tr2Shader.h"
#include "../Tr2RingBuffer.h"

#include "Tr2MeshArea.h"
#include "ITr2TextureProvider.h"

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
	m_meshIndex( 0 ),
	m_boneOffset( 0 ),
	m_morphAnimationDataOffset( 0 ),
	m_morphAnimationDataCount( 0 ),
	m_isDirty( true ),
	m_screenSize( 0.f ),
	m_lodIndex( 0 )
{
}

void Tr2RaytracingMesh::UpdateRtMesh( Tr2GeometryProvider* geometry, uint32_t meshIndex, float screenSize )
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
	m_blasOutdated( true ),
	m_transformMaterial( nullptr ),
	m_transformTechniqueIndex( 0 ),
	m_transformedVertices( nullptr ),
	m_transformedVertexOffset( 0 )
{
}

void Tr2RaytracingMeshArea::SetVertexTransform( const Tr2Material* material, uint32_t techniqueIndex )
{
	if( m_transformMaterial != material || m_transformTechniqueIndex != techniqueIndex )
	{
		// another shader writes the vertices, or the area switches to or from the shared static BLAS
		m_transformMaterial = material;
		m_transformTechniqueIndex = techniqueIndex;
		m_blasOutdated = true;
	}
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
	// skinned, morphed or displaced, see Tr2RaytracingVertexTransforms
	if( m_transformMaterial )
	{
		if( m_blas.IsValid() && !m_blasOutdated )
		{
			return m_blas; // Nothing to do, we're up to date and valid
		}

		if( !m_transformedVertices )
		{
			return m_blas; // No transformed vertices, can't build BLAS
		}

		// At this point we know that the BLAS is either invalid or dirty. We at least need to update it.
		bool update = true;
		bool rebuild = false;

		if( !m_blas.IsValid() )
		{
			// the BLAS isn't valid, so we need to rebuild it.
			update = false;
			rebuild = true;
		}

		Tr2RtGeometryAL geometry;
		geometry.positions = Tr2RtPositionStreamAL(
			*m_transformedVertices,
			3 * sizeof( float ),
			lod->m_vertexCount,
			m_transformedVertexOffset,
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
			TRINITY_STATS_ZONE( "BLAS rebuild" );

			auto capacity = geometry;
			auto highestLod = mesh.GetHighestLodData();

			if( highestLod != lod )
			{
				// provide the capacity for the BLAS to grow into
				capacity.positions = Tr2RtPositionStreamAL(
					geometry.positions.m_vertexBuffer,
					3 * sizeof( float ),
					highestLod->m_vertexCount,
					m_transformedVertexOffset,
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
	TRINITY_STATS_ZONE( __FUNCTION__ );
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

void Tr2RaytracingGeometry::TransformMeshes( Tr2RenderContext& renderContext )
{
	std::vector<Tr2RaytracingVertexTransformInput> inputs;
	inputs.reserve( m_geometryData.size() );
	for( const auto& geom : m_geometryData )
	{
		if( geom.materialIndex != INVALID_MATERIAL )
		{
			inputs.push_back( { geom.mesh, geom.area, geom.material, geom.bakedMorphOffset } );
		}
	}
	m_vertexTransforms.Update( inputs, renderContext );
}

void Tr2RaytracingGeometry::BuildAccelerationStructures( Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "BuildAccelerationStructures" );
	TRINITY_STATS_ZONE( __FUNCTION__ );

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
		TRINITY_STATS_ZONE( "TLAS update" );
		if( instances.empty() )
		{
			m_tlas = Tr2RtTopLevelAccelerationStructureAL();
		}
		else if( FAILED( m_tlas.Update( instances.size(), instances.data(), renderContext ) ) )
		{
			TRINITY_STATS_ZONE( "TLAS create" );
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

void Tr2RaytracingGeometry::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_vertexTransforms.ReleaseResources();
	}
}
