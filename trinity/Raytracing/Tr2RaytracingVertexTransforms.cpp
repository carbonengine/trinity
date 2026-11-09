// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RaytracingVertexTransforms.h"

#include "Tr2RaytracingGeometry.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Shader.h"
#include "../Tr2RingBufferImpl.h"

#include <tuple>

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <../trinityal/dx12/Tr2BufferALDx12.h>
#endif

namespace
{
// Mirrors cbuffer TransformVerticesDispatch in TransformVertices.fxh.
struct TransformVerticesDispatchCBuffer
{
	uint32_t jobBase;
	uint32_t groupBase;
	uint32_t dispatchFirstGroup;
	uint32_t outVBIndex;
};

// The most thread groups in one dimension of a dispatch (D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION).
constexpr uint32_t MAX_DISPATCH_GROUPS = 65535;

// The first pass of the technique if it has a compute shader, otherwise null. Null while the shader is loading.
const Tr2Pass* FindComputePass( const Tr2Material& material, uint32_t techniqueIndex )
{
	auto shader = material.GetShaderStateInterface();
	if( !shader )
	{
		return nullptr;
	}
	const auto& techniques = shader->GetEffectDescription().techniques;
	if( techniqueIndex >= techniques.size() || techniques[techniqueIndex].passes.empty() )
	{
		return nullptr;
	}
	const Tr2Pass& pass = techniques[techniqueIndex].passes[0];
	return pass.stageInputs[Tr2RenderContextEnum::COMPUTE_SHADER].m_exists ? &pass : nullptr;
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

// Each job reads its input vertex buffer from the buffer heap, so jobs with different input buffers can share a
// dispatch.
const Tr2BufferAL* GetDispatchInputBuffer( const TriGeometryResLodData& )
{
	return nullptr;
}

// The shaders read the input and output buffers from the buffer heap, so nothing is bound by register.
class TransformBufferBinder
{
public:
	explicit TransformBufferBinder( const Tr2BufferAL& )
	{
	}
	void SetPass( const Tr2Pass& )
	{
	}
	void Bind( const Tr2BufferAL*, Tr2RenderContext& ) const
	{
	}
};

#else

// All jobs of a dispatch read the input vertex buffer that is bound to InVB.
const Tr2BufferAL* GetDispatchInputBuffer( const TriGeometryResLodData& lod )
{
	return &lod.m_vertexAllocation.GetBuffer();
}

// Register of the named resource or UAV in a shader stage, or -1 if the stage doesn't use it.
int FindResourceRegister( const Tr2EffectResourceMap& resources, const char* name )
{
	for( const auto& resource : resources )
	{
		if( resource.second.name && strcmp( resource.second.name, name ) == 0 )
		{
			return resource.first;
		}
	}
	return -1;
}

// Binds a dispatch's input vertex buffer to InVB and the output buffer to OutVB by register, so that the shared material
// isn't changed.
class TransformBufferBinder
{
public:
	explicit TransformBufferBinder( const Tr2BufferAL& output ) :
		m_output( output )
	{
	}
	void SetPass( const Tr2Pass& pass )
	{
		const auto& stage = pass.stageInputs[Tr2RenderContextEnum::COMPUTE_SHADER];
		m_inputRegister = FindResourceRegister( stage.resources, "InVB" );
		m_outputRegister = FindResourceRegister( stage.uavs, "OutVB" );
	}
	void Bind( const Tr2BufferAL* input, Tr2RenderContext& renderContext ) const
	{
		if( m_inputRegister >= 0 )
		{
			renderContext.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, uint32_t( m_inputRegister ), *input );
		}
		if( m_outputRegister >= 0 )
		{
			renderContext.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, uint32_t( m_outputRegister ), m_output );
		}
	}

private:
	const Tr2BufferAL& m_output;
	int m_inputRegister = -1;
	int m_outputRegister = -1;
};

#endif
}

// The shader that writes an area's vertices.
struct Tr2RaytracingVertexTransforms::TransformShader
{
	// The area's material, m_fallbackEffect, or null for a static area.
	const Tr2Material* material = nullptr;
	uint32_t techniqueIndex = 0;
	// The technique's compute pass. Null while the fallback is loading.
	const Tr2Pass* pass = nullptr;
	// The technique is TransformVerticesAnimated: its output changes every frame.
	bool isAnimated = false;
};

// An area whose vertices are written this frame.
struct Tr2RaytracingVertexTransforms::TransformRequest
{
	TransformShader shader;
	// The vertex buffer that all jobs of a dispatch read: the LOD's, or null with the buffer heap.
	const Tr2BufferAL* inputBuffer;
	Tr2RaytracingMesh* mesh;
	Tr2RaytracingMeshArea* area;
	uint32_t bakedMorphOffset;

	// Requests with the same dispatch key can share a dispatch: the same program and the same material data, and without
	// the buffer heap the same input vertex buffer. The program comes first, so that sorting puts the dispatches that
	// use it next to each other.
	auto DispatchKey() const
	{
		return std::make_tuple( shader.pass->shaderProgram, uintptr_t( shader.material ), shader.techniqueIndex, uintptr_t( inputBuffer ) );
	}
	// Requests with the same job key share a job and its output: the same mesh, transformed by the same shader.
	auto JobKey() const
	{
		return std::tuple_cat( DispatchKey(), std::make_tuple( uintptr_t( mesh ) ) );
	}
};

// Consecutive thread groups whose jobs share a shader and its material data.
struct Tr2RaytracingVertexTransforms::TransformDispatch
{
	// The first request of the dispatch's jobs. All of them have its shader and input buffer.
	const TransformRequest* run;
	// Index of the first thread group, relative to the frame's first thread group.
	uint32_t firstGroup;
	uint32_t groupCount;
};

struct Tr2RaytracingVertexTransforms::TransformBatch
{
	std::vector<Tr2TransformVerticesJob> jobs;
	// For each thread group, the index of its job in jobs.
	std::vector<Tr2TransformVerticesJobGroup> groups;
	std::vector<TransformDispatch> dispatches;
	// Number of output vertices of all jobs.
	uint32_t vertexCount = 0;
};

Tr2RaytracingVertexTransforms::Tr2RaytracingVertexTransforms()
{
	// read by the TransformVertices.fxh shaders
	GlobalStore().RegisterVariable( "TransformJobs", &Tr2RingBuffer::GetInstance<Tr2TransformVerticesJob>() );
	Tr2RingBuffer::GetInstance<Tr2TransformVerticesJob>().SetName( "TransformJobsBuffer" );
	GlobalStore().RegisterVariable( "TransformJobGroups", &Tr2RingBuffer::GetInstance<Tr2TransformVerticesJobGroup>() );
	Tr2RingBuffer::GetInstance<Tr2TransformVerticesJobGroup>().SetName( "TransformJobGroupsBuffer" );

	// InVB and OutVB are bound by Dispatch, so the effect has no parameters for them
	m_fallbackEffect.CreateInstance();
	m_fallbackEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/skinvertices.fx" );
}

void Tr2RaytracingVertexTransforms::ReleaseResources()
{
	m_dispatchConstants = Tr2ConstantBufferAL();
}

void Tr2RaytracingVertexTransforms::Update( const std::vector<Tr2RaytracingVertexTransformInput>& inputs, Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "TransformMeshes" );
	TRINITY_STATS_ZONE( __FUNCTION__ );

	auto& jobBuffer = Tr2RingBuffer::GetInstance<Tr2TransformVerticesJob>();
	auto& groupBuffer = Tr2RingBuffer::GetInstance<Tr2TransformVerticesJobGroup>();
	{
		// release the job data of frames the GPU has finished
		auto& primaryContext = renderContext.GetPrimaryRenderContext();
		auto frame = primaryContext.GetRecordingFrameNumber();
		auto renderedFrame = primaryContext.GetRenderedFrameNumber();
		jobBuffer.SetFrameNumbers( frame, renderedFrame );
		groupBuffer.SetFrameNumbers( frame, renderedFrame );
	}

	auto requests = CollectRequests( inputs );
	// The areas that need the meshes' changes have requested a job, or are outdated and request one next frame.
	for( const auto& input : inputs )
	{
		input.mesh->m_isDirty = false;
	}

	const TransformBatch batch = PlanBatch( requests );
	if( batch.dispatches.empty() )
	{
		return;
	}

	if( !m_outputVertices.IsValid() || m_outputVertices.GetDesc().count < 3 * batch.vertexCount )
	{
		m_outputVertices.Create( Tr2BufferDescriptionAL( uint32_t( sizeof( float ) ), 3 * batch.vertexCount, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ ), nullptr, renderContext.GetPrimaryRenderContext() );
	}

	const uint32_t groupBase = groupBuffer.UploadTransforms( batch.groups.data(), uint32_t( batch.groups.size() ) );
	const uint32_t jobBase = jobBuffer.UploadTransforms( batch.jobs.data(), uint32_t( batch.jobs.size() ) );
	groupBuffer.PrepareBuffer( renderContext );
	jobBuffer.PrepareBuffer( renderContext );
	{
		TRINITY_STATS_ZONE( "Prepare BoneTransformBuffer" );
		Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( renderContext );
	}
	{
		TRINITY_STATS_ZONE( "Prepare MorphTargetAnimationDataBuffer" );
		Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( renderContext );
	}

	Dispatch( batch, jobBase, groupBase, renderContext );
}

Tr2RaytracingVertexTransforms::TransformShader Tr2RaytracingVertexTransforms::ResolveShader( const Tr2RaytracingVertexTransformInput& input, const TriGeometryResLodData& lod ) const
{
	// The material's own technique, once it has loaded. TransformVertices re-runs when its inputs change,
	// TransformVerticesAnimated every frame. TransformVertices wins if a material has both.
	if( auto shader = input.material ? input.material->GetShaderStateInterface() : nullptr )
	{
		for( bool isAnimated : { false, true } )
		{
			uint32_t techniqueIndex;
			if( shader->GetTechniqueIndex( isAnimated ? m_animatedTechniqueName : m_techniqueName, techniqueIndex ) )
			{
				if( auto pass = FindComputePass( *input.material, techniqueIndex ) )
				{
					return { input.material, techniqueIndex, pass, isAnimated };
				}
			}
		}
	}

	// Otherwise skinned and morphed areas use the fallback, which may still be loading.
	const auto& lodArea = lod.m_areas[input.area->GetAreaIndex()];
	if( lodArea.m_isSkinned || lodArea.m_isMorphed )
	{
		return { m_fallbackEffect, 0, FindComputePass( *m_fallbackEffect, 0 ), false };
	}

	// Static areas use the LOD's shared static BLAS.
	return {};
}

std::vector<Tr2RaytracingVertexTransforms::TransformRequest> Tr2RaytracingVertexTransforms::CollectRequests( const std::vector<Tr2RaytracingVertexTransformInput>& inputs )
{
	std::vector<TransformRequest> requests;
	for( const auto& input : inputs )
	{
		const TriGeometryResLodData* lod = input.mesh->GetCurrentLodData();
		if( !lod )
		{
			continue;
		}

		const TransformShader shader = ResolveShader( input, *lod );
		input.area->SetVertexTransform( shader.material, shader.techniqueIndex );
		if( !shader.material )
		{
			continue; // a static area
		}
		if( !input.mesh->m_isDirty && !input.area->IsBlasOutdated() && !shader.isAnimated )
		{
			continue; // the area's BLAS is up to date
		}

		// The BLAS is refit from this frame's output. Drop the range of an earlier frame, because the output is repacked
		// every frame. PlanBatch sets the new range.
		input.area->MarkBlasOutdated();
		input.area->SetTransformedVertices( nullptr, 0 );
		if( !shader.pass )
		{
			// The fallback is still loading. The area stays outdated, so it requests a job again next frame, and until
			// then BuildBlas keeps the BLAS it has, if any.
			continue;
		}

		requests.push_back( { shader, GetDispatchInputBuffer( *lod ), input.mesh, input.area, input.bakedMorphOffset } );
	}
	return requests;
}

Tr2RaytracingVertexTransforms::TransformBatch Tr2RaytracingVertexTransforms::PlanBatch( std::vector<TransformRequest>& requests )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	// Sorted, the requests that share a job are next to each other, and so are the jobs that can share a dispatch.
	std::sort( begin( requests ), end( requests ), []( const TransformRequest& a, const TransformRequest& b ) {
		return a.JobKey() < b.JobKey();
	} );

	TransformBatch batch;
	for( size_t runStart = 0; runStart < requests.size(); )
	{
		// a run of requests that can share a dispatch
		const TransformRequest& run = requests[runStart];
		const uint32_t runFirstGroup = uint32_t( batch.groups.size() );

		size_t i = runStart;
		for( ; i < requests.size() && requests[i].DispatchKey() == run.DispatchKey(); ++i )
		{
			const TransformRequest& request = requests[i];
			if( i == runStart || request.mesh != requests[i - 1].mesh )
			{
				// A new job. It transforms the whole LOD, because the area's indices refer to the LOD's vertex range.
				const auto& lod = *request.mesh->GetCurrentLodData();
				const auto job = CreateJob( *request.mesh, lod, request.bakedMorphOffset, batch.vertexCount, uint32_t( batch.groups.size() ) );
				const uint32_t groupCount = ( job.vertexCount + Tr2TransformVerticesJob::GROUP_SIZE - 1 ) / Tr2TransformVerticesJob::GROUP_SIZE;
				batch.groups.insert( end( batch.groups ), groupCount, Tr2TransformVerticesJobGroup{ uint32_t( batch.jobs.size() ) } );
				batch.jobs.push_back( job );
				batch.vertexCount += job.vertexCount;
			}
			request.area->SetTransformedVertices( &m_outputVertices, batch.jobs.back().outVBVertexOffset );
		}

		// Cut the run into dispatches. A job that doesn't fit continues in the next dispatch, see GetTransformVertex in
		// TransformVertices.fxh.
		const uint32_t runEndGroup = uint32_t( batch.groups.size() );
		for( uint32_t firstGroup = runFirstGroup; firstGroup < runEndGroup; firstGroup += MAX_DISPATCH_GROUPS )
		{
			batch.dispatches.push_back( { &run, firstGroup, std::min( MAX_DISPATCH_GROUPS, runEndGroup - firstGroup ) } );
		}

		runStart = i;
	}
	return batch;
}

void Tr2RaytracingVertexTransforms::Dispatch( const TransformBatch& batch, uint32_t jobBase, uint32_t groupBase, Tr2RenderContext& renderContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	// the jobs write to separate ranges of the output
	renderContext.PushDisableUAVBarriersDx12();
	ON_BLOCK_EXIT( [&] { renderContext.PopDisableUAVBarriersDx12(); } );

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_outputVertices.TrinityALImpl_GetObject()->GetGpuResource();
	barrier.Transition.StateBefore = m_outputVertices.TrinityALImpl_GetObject()->GetDefaultState();
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	renderContext.ResourceBarrierDx12( barrier );
	renderContext.FlushBarriersDx12();

	std::swap( barrier.Transition.StateBefore, barrier.Transition.StateAfter );

	ON_BLOCK_EXIT( [&] { renderContext.ResourceBarrierDx12( barrier ); } );
#endif

	if( !m_dispatchConstants.IsValid() )
	{
		m_dispatchConstants.Create( uint32_t( sizeof( TransformVerticesDispatchCBuffer ) ), renderContext.GetPrimaryRenderContext() );
	}
	const auto perObjVSRegister = Tr2Renderer::GetPerObjectVSStartRegister();
	TransformBufferBinder binder( m_outputVertices );

	const TransformShader* previous = nullptr;
	for( const auto& dispatch : batch.dispatches )
	{
		const TransformShader& shader = dispatch.run->shader;

		// Apply the pass state only when it changes, rather than calling the material's RunComputeShader(), which
		// applies everything for every dispatch. The dispatches are sorted by program, then material and technique.
		const bool programChanged = !previous || previous->pass->shaderProgram != shader.pass->shaderProgram;
		const bool materialChanged = programChanged || previous->material != shader.material || previous->techniqueIndex != shader.techniqueIndex;
		previous = &shader;

		if( programChanged )
		{
			renderContext.m_esm.ApplyShaderProgram( shader.pass->shaderProgram );
			renderContext.m_esm.ApplyRenderStates( shader.pass->renderStates );
		}
		if( materialChanged )
		{
			shader.material->ApplyMaterialDataForPass( shader.techniqueIndex, 0, renderContext );
			binder.SetPass( *shader.pass );
		}

		// set after the material data, so that the material can't override them
		TransformVerticesDispatchCBuffer* constants;
		m_dispatchConstants.Lock( (void**)&constants, renderContext );
		constants->jobBase = jobBase;
		constants->groupBase = groupBase;
		constants->dispatchFirstGroup = dispatch.firstGroup;
		constants->outVBIndex = m_outputVertices.GetUavIndexInHeap();
		m_dispatchConstants.Unlock( renderContext );
		renderContext.SetConstants( m_dispatchConstants, Tr2RenderContextEnum::COMPUTE_SHADER, perObjVSRegister );

		binder.Bind( dispatch.run->inputBuffer, renderContext );
		renderContext.RunComputeShader( dispatch.groupCount, 1, 1 );
	}
	renderContext.ResetResourceBindings();
}

Tr2TransformVerticesJob Tr2RaytracingVertexTransforms::CreateJob( const Tr2RaytracingMesh& mesh, const TriGeometryResLodData& lod, uint32_t bakedMorphOffset, uint32_t outVBVertexOffset, uint32_t firstGroup )
{
	const uint32_t INVALID_OFFSET = Tr2TransformVerticesJob::INVALID_OFFSET;

	// element offsets are relative to the vertex, the job's offsets to the start of the vertex buffer
	const uint32_t allocationOffset = lod.m_vertexAllocation.GetOffset();
	auto toBufferOffset = [&]( uint32_t elementOffset ) {
		return elementOffset == INVALID_OFFSET ? INVALID_OFFSET : allocationOffset + elementOffset;
	};

	auto offsets = FindOffsets( lod.m_mesh->m_vertexDeclarationHandle );

	Tr2TransformVerticesJob job = {};
	job.vertexCount = lod.m_vertexCount;
	job.vertexStride = lod.m_vertexAllocation.GetStride();
	job.inVBIndex = lod.m_vertexAllocation.GetBuffer().GetSrvIndexInHeap();
	job.outVBVertexOffset = outVBVertexOffset;

	job.positionOffset = toBufferOffset( offsets.positionOffset );
	job.normalOffset = toBufferOffset( offsets.normalOffset );
	job.normalType = offsets.normalType;
	job.texCoord0Offset = toBufferOffset( offsets.texCoord0Offset );
	job.texCoord0Type = offsets.texCoord0Type;

	job.boneOffset = toBufferOffset( offsets.boneOffset );
	job.boneWeightsOffset = toBufferOffset( offsets.boneWeightsOffset );
	job.transformOffset = mesh.GetTransformOffset();

	job.morphAnimationDataOffset = 0;
	job.morphAnimationDataCount = 0;
	job.morphTargetPositionOffset = 0;
	job.morphTargetStride = 0;
	job.morphTargetSize = 0;
	job.bakedMorphTargetPositionOffset = INVALID_OFFSET;
	if( lod.m_morphTargetAllocation.IsValid() )
	{
		auto morphOffsets = FindOffsets( lod.m_morphVertexDeclaration );
		job.morphAnimationDataOffset = mesh.m_morphAnimationDataOffset;
		job.morphAnimationDataCount = mesh.m_morphAnimationDataCount;
		job.morphTargetPositionOffset = uint32_t( lod.m_morphTargetAllocation.GetOffset() + sizeof( TriMorphTargetGeometryConstants ) + morphOffsets.positionOffset );
		job.morphTargetStride = lod.m_bytesPerMorphTargetVertex;
		job.morphTargetSize = lod.m_bytesPerMorphTargetVertex * lod.m_vertexCount;
		job.bakedMorphTargetPositionOffset = bakedMorphOffset;
	}

	job.firstGroup = firstGroup;
	job.padding = 0;
	return job;
}

Tr2RaytracingVertexTransforms::VtxOffsets Tr2RaytracingVertexTransforms::FindOffsets( unsigned declHandle )
{
	auto found = m_offsets.find( declHandle );
	if( found != m_offsets.end() )
	{
		return found->second;
	}
	VtxOffsets offsets;
	Tr2VertexDefinition def;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( declHandle, def ) )
	{
		return offsets;
	}
	for( auto it = begin( def.m_items ); it != end( def.m_items ); ++it )
	{
		if( it->m_usageIndex == 0 )
		{
			switch( it->m_usage )
			{
			case Tr2VertexDefinition::POSITION:
				offsets.positionOffset = it->m_offset;
				break;
			case Tr2VertexDefinition::NORMAL:
				offsets.normalOffset = it->m_offset;
				offsets.normalType = it->m_dataType;
				break;
			case Tr2VertexDefinition::TEXCOORD:
				offsets.texCoord0Offset = it->m_offset;
				offsets.texCoord0Type = it->m_dataType;
				break;
			case Tr2VertexDefinition::BLENDINDICES:
				offsets.boneOffset = it->m_offset;
				break;
			case Tr2VertexDefinition::BLENDWEIGHTS:
				offsets.boneWeightsOffset = it->m_offset;
				break;
			default:
				break;
			}
		}
	}
	m_offsets[declHandle] = offsets;
	return offsets;
}
