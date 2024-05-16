#include "StdAfx.h"
#include "Tr2RaytracingGeometry.h"

#include "Resources/TriGeometryRes.h"

#include "Tr2Renderer.h"
#include "Shader/Tr2Shader.h"
#include "../Tr2BoneTransformBuffer.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <../trinityal/dx12/Tr2BufferALDx12.h>
#endif


//***********************************************************
// Tr2RaytracingPipelineStateManager
//***********************************************************


Tr2RaytracingPipelineStateManager::Tr2RaytracingPipelineStateManager() :
	m_nextName( 0 ),
	m_isDirty( true )
{
}

// remove material
bool Tr2RaytracingPipelineStateManager::AddLibrary( std::wstring& rayGenName, std::wstring& missName, Tr2Material* material, BlueSharedString techniqueName )
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

void Tr2RaytracingPipelineStateManager::AddLibrary( std::wstring& rayGenName, std::wstring& missName, const Tr2EffectLibrary& library )
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
		rayGenName = GetUniqueName();
		exportNamesRaw[count] = rayGenName.c_str();
		names[count++] = library.rayGenName.c_str();
	}
	if( !library.missName.empty() )
	{
		missName = GetUniqueName();
		exportNamesRaw[count] = missName.c_str();
		names[count++] = library.missName.c_str();
	}
	m_pipelineDesc.AddShaders( count, exportNamesRaw, *Tr2EffectStateManager::GetShaderLibraryCode( library.libraryHandle ), names, library.payloadSize );
	m_pipelineDesc.AddGlobalSignature( library.globalInput.signature );

	m_isDirty = true;

	m_libraries[library.libraryHandle] = std::make_pair( rayGenName, missName );
}

std::wstring Tr2RaytracingPipelineStateManager::AddHitGroup( const Tr2EffectLibrary& library )
{
	auto found = m_hitGroups.find( library.libraryHandle );
	if( found != end( m_hitGroups ) )
	{
		return found->second;
	}

	const wchar_t* names[3];
	std::wstring closestHitName, anyHitName, intersectionName;
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
	auto hitGroup = GetUniqueName();
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
	return L"rtShader_" + std::to_wstring( long( long( m_nextName++ ) ) );
}

// ***************** Tr2RaytracingMesh *****************

Tr2RaytracingMesh::Tr2RaytracingMesh() :
	m_skinnedVertices( nullptr ),
	m_meshIndex( 0 ),
	m_boneOffset( 0 ),
	m_skinnedVertexOffset( 0 ),
	m_isDirty( true ),
	m_screenSize( 0.f )
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

		m_meshData = m_geometry->GetMeshData( m_meshIndex, m_screenSize );

		m_transforms.clear();

		m_isDirty = true;
	}
	else if( m_screenSize != screenSize )
	{
		//screen size has changed, so check if we've changed to a different LOD
		auto meshData = m_geometry->GetMeshData( m_meshIndex, screenSize );
		m_isDirty = m_meshData == meshData;
		m_meshData = meshData;
		m_screenSize = screenSize;
	}
}

bool Tr2RaytracingMesh::SetBoneTransforms( size_t count, const granny_matrix_3x4* transforms, uint32_t offset )
{
	auto newSize = count * 3 * 4;
	if( m_transforms.size() != newSize )
	{
		m_transforms.resize( newSize );
		m_isDirty = true;
	}

	if( newSize > 0 && memcmp( m_transforms.data(), transforms, newSize * sizeof( float ) ) )
	{
		memcpy( m_transforms.data(), transforms, count * sizeof( granny_matrix_3x4 ) );
		m_isDirty = true;
	}

	m_boneOffset = offset;

	return m_isDirty;
}

bool Tr2RaytracingMesh::IsGood() const
{
	return m_geometry && m_geometry->IsGood() && m_geometry->GetMeshData( m_meshIndex );
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

TriGeometryResMeshData* Tr2RaytracingMesh::GetMeshData() const
{
	return m_meshData;
}

uint32_t Tr2RaytracingMesh::GetTransformOffset() const
{
	return m_boneOffset;
}

const Tr2BufferAL& Tr2RaytracingMesh::GetSkinnedVertexBuffer( Tr2RenderContext& renderContext )
{
	return *m_skinnedVertices;
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
	return m_meshData->m_vertexAllocation.GetBuffer();
}

const Tr2BufferAL& Tr2RaytracingMesh::GetIndexBuffer() const
{
	return m_meshData->m_indexAllocation.GetBuffer();
}

// ***************** Tr2RaytracingMeshArea *****************

Tr2RaytracingMeshArea::Tr2RaytracingMeshArea( uint32_t index ) :
	m_areaIndex( index )
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
	auto meshData = mesh.GetMeshData();
	if( meshData->m_areas[m_areaIndex].m_isSkinned )
	{
		if( m_blas.IsValid() && !m_blasOutdated )
		{
			return m_blas; //Nothing to do, we're up to date and valid
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

		auto positions = Tr2RtPositionStreamAL( 
			mesh.GetSkinnedVertexBuffer( renderContext ), 
			3 * sizeof( float ),
			meshData->m_vertexCount,
			mesh.GetSkinnedVertexOffset(),
			0,
			Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
		auto indices = Tr2RtIndicesStreamAL( meshData->m_indexAllocation.GetBuffer(), meshData->m_indexAllocation.GetStride(), meshData->m_indexAllocation.GetStartIndex() + meshData->m_areas[m_areaIndex].m_firstIndex, meshData->m_areas[m_areaIndex].m_primitiveCount * 3 );

		if( update )
		{
			if( FAILED( m_blas.Update( positions, indices, renderContext.GetPrimaryRenderContext() ) ) )
			{
				//This will fail gracefully whenever the LOD changes, so fall back to rebuild in this case.
				//CCP_LOGERR( "Failed to update BLAS, recreating it instead" );
				rebuild = true; //update failed, so rebuild it instead
			}
		}

		if( rebuild )
		{
			if( FAILED( m_blas.Create( positions, indices, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_BUILD | Tr2RtBuildFlags::ALLOW_UPDATE, renderContext.GetPrimaryRenderContext() ) ) )
			{
				CCP_LOGERR( "Failed to create BLAS!" );
			}
		}

		m_blasOutdated = false;

		return m_blas;
	}
	else
	{
		if( !meshData->m_areas[m_areaIndex].m_staticBlas.IsValid() )
		{
			auto vertexStream = Tr2RtPositionStreamAL(
				meshData->m_vertexAllocation.GetBuffer(),
				meshData->m_vertexAllocation.GetStride(),
				meshData->m_vertexCount,
				meshData->m_vertexAllocation.GetOffset() / meshData->m_vertexAllocation.GetStride(),
				0,
				Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
			auto indexStream = Tr2RtIndicesStreamAL(
				meshData->m_indexAllocation.GetBuffer(),
				meshData->m_indexAllocation.GetStride(),
				meshData->m_indexAllocation.GetStartIndex() + meshData->m_areas[m_areaIndex].m_firstIndex,
				meshData->m_areas[m_areaIndex].m_primitiveCount * 3 );

			meshData->m_areas[m_areaIndex].m_staticBlas.Create(
				vertexStream,
				indexStream,
				Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY,
				Tr2RtBuildFlags::PREFER_FAST_TRACE,
				renderContext.GetPrimaryRenderContext() );
		}
		return meshData->m_areas[m_areaIndex].m_staticBlas;
	}
}

const Tr2ConstantBufferAL& Tr2RaytracingMeshArea::GetGeometryConstants( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext )
{
	auto meshData = mesh.GetMeshData();
	if( !meshData->m_areas[m_areaIndex].m_rtGeometryConstants.IsValid() )
	{
		if( SUCCEEDED( meshData->m_areas[m_areaIndex].m_rtGeometryConstants.Create( sizeof( TriRtGeometryConstants ), renderContext.GetPrimaryRenderContext() ) ) )
		{
			TriRtGeometryConstants* data;
			if( SUCCEEDED( meshData->m_areas[m_areaIndex].m_rtGeometryConstants.Lock( (void**)&data, renderContext ) ) )
			{
				data->indexBufferOffset = meshData->m_areas[m_areaIndex].m_firstIndex;
				data->indexBufferStride = meshData->m_indexAllocation.GetStride();
				data->vertexBufferStride = meshData->m_vertexAllocation.GetStride();
				data->texCoord0Offset = 0;
				Tr2VertexDefinition def;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, def );
				for( auto it = begin( def.m_items ); it != end( def.m_items ); ++it )
				{
					if( it->m_usage == Tr2VertexDefinition::TEXCOORD && it->m_usageIndex == 0 && it->m_stream == 0 )
					{
						data->texCoord0Offset = it->m_offset;
					}
				}
				meshData->m_areas[m_areaIndex].m_rtGeometryConstants.Unlock( renderContext );
			}
		}
	}
	return meshData->m_areas[m_areaIndex].m_rtGeometryConstants;
}

// ***************** Tr2RaytracingGeometry *****************
Tr2RaytracingGeometry::Tr2RaytracingGeometry()
{
	m_skinVerticesEffect.CreateInstance();
	m_skinVerticesEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/skinvertices.fx" );
}

Tr2BufferAL* Tr2RaytracingGeometry::GetGpuBuffer( unsigned )
{
	return const_cast<Tr2BufferAL*>( &m_tlas.GetBuffer() );
}

void Tr2RaytracingGeometry::BeginSceneUpdate()
{
	m_geometryData.clear();
}


void Tr2RaytracingGeometry::EndSceneUpdate( Tr2RenderContext& renderContext )
{
	PrepareShaderTableDescription( renderContext );
	TransformMeshes( renderContext );
	BuildAccelerationStructures( renderContext );
}

void Tr2RaytracingGeometry::PrepareShaderTableDescription( Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "PrepareShaderTableDescription" );
	CCP_STATS_ZONE( __FUNCTION__ );
	m_shaderTableDesc = Tr2RtShaderTableDescriptionAL();
	std::map<uint32_t, std::pair<std::wstring, uint32_t>> seenLibraries;

	uint32_t materialIndex = 0;
	Tr2RtLocalMaterialDescriptionAL material;
	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
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
		auto foundLib = seenLibraries.find( lib.libraryHandle );
		if( foundLib == end( seenLibraries ) )
		{
			auto hitGroupName = m_pipelineManager->AddHitGroup( lib );
			seenLibraries[lib.libraryHandle] = std::make_pair( hitGroupName, materialIndex );

			it->materialIndex = materialIndex++;
			if( it->perObjectData )
			{
				material.SetConstants( Tr2Renderer::GetPerObjectPSStartRegister(), *it->perObjectData );
			}
			it->material->ApplyMaterialDataForRtMaterial( techniqueIndex, &it->mesh->GetVertexBuffer(), &it->mesh->GetIndexBuffer(), material, renderContext );
			m_shaderTableDesc.AddHitGroup( hitGroupName.c_str(), material );
		}
		else if( !lib.localInput.signature.registers.empty() )
		{
			it->materialIndex = materialIndex++;
			if( it->perObjectData )
			{
				material.SetConstants( Tr2Renderer::GetPerObjectPSStartRegister(), *it->perObjectData );
			}
			it->material->ApplyMaterialDataForRtMaterial( techniqueIndex, &it->mesh->GetVertexBuffer(), &it->mesh->GetIndexBuffer(), material, renderContext );
			m_shaderTableDesc.AddHitGroup( foundLib->second.first.c_str(), material );
		}
		else
		{
			it->materialIndex = foundLib->second.second;
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
	uint32_t transformOffset;
	uint32_t inVB;
	uint32_t outVB;
	uint32_t outVBOffset;
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

	std::vector<Tr2RaytracingMesh*> outdatedMeshes;
	outdatedMeshes.reserve( m_geometryData.size() );

	{
		CCP_STATS_ZONE( "Tr2BoneTransformBuffer" );
		Tr2BoneTransformBuffer::GetInstance().PrepareBuffer( renderContext );
	}

	uint32_t skinnedVertexCount = 0;

	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
		if( it->materialIndex == INVALID_MATERIAL )
		{
			continue;
		}

		Tr2RaytracingMesh* mesh = it->mesh;
		if( mesh->GetMeshData()->m_areas[it->area->GetAreaIndex()].m_isSkinned )
		{
			if( !mesh->GetAndResetDirtyFlag() )
			{
				continue;
			}

			outdatedMeshes.push_back( mesh );

			skinnedVertexCount += mesh->GetMeshData()->m_vertexCount;
		}
	}

	if( !m_skinnedVertices.IsValid() || m_skinnedVertices.GetDesc().count < skinnedVertexCount )
	{
		const uint32_t vertexSize = 3 * sizeof( float );
		m_skinnedVertices.Create( Tr2BufferDescriptionAL( vertexSize, skinnedVertexCount, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ ), nullptr, renderContext.GetPrimaryRenderContext() );
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

	ON_BLOCK_EXIT( [&]{ renderContext.ResourceBarrierDx12( barrier ); } );
#endif

	if( !outdatedMeshes.empty() )
	{
		CCP_STATS_ZONE( "Dispatch" );

		CTr2RuntimeGpuBuffer inVB;
		CTr2RuntimeGpuBuffer outVB;

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

		uint32_t outOffset = 0;

		for( auto it = begin( outdatedMeshes ); it != end( outdatedMeshes ); ++it )
		{
			Tr2RaytracingMesh* mesh = *it;
			TriGeometryResMeshData* meshData = mesh->GetMeshData();

			auto vertexCount = meshData->m_vertexCount;

			SkinningShaderCBuffer* constData;
			m_skinVerticesData.Lock( (void**)&constData, renderContext );

			constData->vertexCount = vertexCount;
			constData->stride = meshData->m_vertexAllocation.GetStride() / 4;
			auto offsets = FindOffsets( meshData->m_vertexDeclaration );
			constData->positionOffset = offsets.positionOffset / 4 + meshData->m_vertexAllocation.GetOffset() / 4;
			constData->boneOffset = offsets.boneOffset / 4 + meshData->m_vertexAllocation.GetOffset() / 4;
			constData->transformOffset = mesh->GetTransformOffset();
			constData->inVB = meshData->m_vertexAllocation.GetBuffer().GetSrvIndexInHeap();
			constData->outVB = m_skinnedVertices.GetUavIndexInHeap();
			constData->outVBOffset = outOffset * 3;

			m_skinVerticesData.Unlock( renderContext );

			renderContext.SetConstants( m_skinVerticesData, Tr2RenderContextEnum::COMPUTE_SHADER, perObjVSRegister );

#if TRINITY_PLATFORM != TRINITY_DIRECTX12
			inVB.m_buffer = meshData->m_vertexAllocation.GetBuffer();
			outVB.m_buffer = m_skinnedVertices;

			m_skinVerticesEffect->SetParameter( m_inVertexBufferTechniqueName, &inVB );
			m_skinVerticesEffect->SetParameter( m_outVertexBufferTechniqueName, &outVB );
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

			outOffset += meshData->m_vertexCount;
		}
	}
#if TRINITY_PLATFORM != TRINITY_DIRECTX12
	renderContext.SetResourceSet( Tr2ResourceSetAL() );
	m_skinVerticesEffect->SetParameter( m_inVertexBufferTechniqueName, static_cast<ITr2GpuBuffer*>( nullptr ) );
	m_skinVerticesEffect->SetParameter( m_outVertexBufferTechniqueName, static_cast<ITr2GpuBuffer*>( nullptr ) );
#endif
}

void Tr2RaytracingGeometry::BuildAccelerationStructures( Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "BuildAccelerationStructures" );
	CCP_STATS_ZONE( __FUNCTION__ );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	renderContext.FlushBarriersDx12();
#endif
	Tr2RtInstanceAL instance;

	std::vector<Tr2RtInstanceAL> instances;
	instances.reserve( m_geometryData.size() );
	for( auto it = begin( m_geometryData ); it != end( m_geometryData ); ++it )
	{
		if( it->materialIndex == INVALID_MATERIAL )
		{
			continue;
		}
		
		instance.flags = it->isTransparent ? Tr2RtInstanceAL::FORCE_NON_OPAQUE : Tr2RtInstanceAL::NONE;
		instance.materialIndex = it->materialIndex;
		instance.blas = it->area->BuildBlas( *it->mesh, renderContext );
		if( !instance.blas.IsValid() )
		{
			continue;
		}

		auto m = Transpose( it->worldTransform );
		memcpy( instance.transform[0], &m.GetX(), 4 * sizeof( float ) );
		memcpy( instance.transform[1], &m.GetY(), 4 * sizeof( float ) );
		memcpy( instance.transform[2], &m.GetZ(), 4 * sizeof( float ) );
		instances.push_back( instance );
	}

	if( instances.empty() )
	{
		m_tlas = Tr2RtTopLevelAccelerationStructureAL();
	}
	else if( FAILED( m_tlas.Update( instances.size(), instances.data(), renderContext ) ) )
	{
		m_tlas.Create( instances.size(), instances.data(), Tr2RtBuildFlags::PREFER_FAST_TRACE, renderContext.GetPrimaryRenderContext() );
	}
}

void Tr2RaytracingGeometry::AddGeometry( Tr2RaytracingMesh& mesh, Tr2RaytracingMeshArea& area, Tr2Material* material, const Tr2ConstantBufferAL* perObjectData, const Matrix& worldTransform )
{
	if( !mesh.IsGood() )
	{
		return;
	}

	GeometryData obj;
	obj.mesh = &mesh;
	obj.area = &area;
	obj.material = material;
	obj.perObjectData = perObjectData;
	obj.worldTransform = worldTransform;
	obj.materialIndex = INVALID_MATERIAL;
	obj.isTransparent = false;
	m_geometryData.push_back( obj );
}

bool Tr2RaytracingGeometry::HasGeometry() const
{
	return m_tlas.IsValid();
}

Tr2RtTopLevelAccelerationStructureAL Tr2RaytracingGeometry::GetTLAS() const
{
    return m_tlas;
}

Tr2RaytracingGeometry::VtxOffsets Tr2RaytracingGeometry::FindOffsets( unsigned declHandle )
{
	auto found = m_offsets.find( declHandle );
	if( found != m_offsets.end() )
	{
		return found->second;
	}
	VtxOffsets offsets = { 0, 0 };
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
	}
	m_offsets[declHandle] = offsets;
	return offsets;
}

void Tr2RaytracingGeometry::ReleaseResources( TriStorage s )
{
	if( (s & TRISTORAGE_ALL) == TRISTORAGE_ALL )
	{
		m_skinVerticesData = Tr2ConstantBufferAL();
	}
}
	