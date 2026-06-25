// Copyright © 2024 CCP ehf.

//
//  Tr2RtShaderTableALMetal.cpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtShaderTableALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2SamplerStateALMetal.h"


namespace TrinityALImpl
{
Tr2RtShaderTableAL::Tr2RtShaderTableAL()
{
}

Tr2RtShaderTableAL::~Tr2RtShaderTableAL()
{
	Destroy();
}

// Shader tables contain shader records. Shader records contain a shader identifier and root arguments used to look up resources
ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc,
									 const ::Tr2RtPipelineStateAL& pipeline,
									 Tr2PrimaryRenderContextAL& renderContext )
{
	if( @available( macOS 13.0, * ) )
	{
		auto missFunctionTableDescriptor = [[MTLVisibleFunctionTableDescriptor alloc] init];
		missFunctionTableDescriptor.functionCount = desc.m_missNames.size();

		auto closestHitFunctionTableDescriptor = [[MTLVisibleFunctionTableDescriptor alloc] init];
		closestHitFunctionTableDescriptor.functionCount = desc.m_hitGroupNames.size();

		auto anyHitFunctionTableDescriptor = [[MTLIntersectionFunctionTableDescriptor alloc] init];
		anyHitFunctionTableDescriptor.functionCount = desc.m_hitGroupNames.size();

		const int BUFFERS_PER_SHADER = sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants ) /
			sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants[0] );

		std::vector<uint64_t> hitBuffers;
		hitBuffers.reserve( desc.m_hitGroupNames.size() * BUFFERS_PER_SHADER );

		auto FillMaterial = [&]( auto& ptrs, auto& material ) {
			for( auto& cb : material.m_constants )
			{
				uint64_t address = 0;
				if( cb && cb->IsValid() )
				{
					uint64_t offset = renderContext.UploadConstants( *cb );
					uint32_t page = uint32_t( offset >> 32 );
					// have a vector of offsets (gpu address + addr) so for each instance it has it's own pointer into a large buffer.
					// then bind that vector to the shadertable
					id<MTLBuffer> cBuffer =
						renderContext.GetMetalContext()->GetConstantBufferAllocator().GetPage( page );

					renderContext.UseConstantBuffer( cBuffer );
					address = cBuffer.gpuAddress + offset;
				}
				ptrs.push_back( address );
			}
		};

		auto& intersectionFunctions = pipeline.TrinityALImpl_GetObject()->GetFunctionMap();
		auto& hitGroupMap = pipeline.TrinityALImpl_GetObject()->GetHitGroupMap();

		std::vector<FunctionTables> functionTables;

		// Create a table large enough to hold all of the intersection functions. Metal
		// links intersection functions into the compute pipeline state, potentially with
		// a different address for each compute pipeline. Therefore, the intersection
		// function table is specific to the compute pipeline state that created it, and you
		// can use it with only that pipeline.
		for( auto& rayGen : pipeline.TrinityALImpl_GetObject()->m_rayGenShaders )
		{
			FunctionTables functions;
			functions.miss = [rayGen.pipeline newVisibleFunctionTableWithDescriptor:missFunctionTableDescriptor];
			functions.closestHit =
				[rayGen.pipeline newVisibleFunctionTableWithDescriptor:closestHitFunctionTableDescriptor];
			functions.anyHit =
				[rayGen.pipeline newIntersectionFunctionTableWithDescriptor:anyHitFunctionTableDescriptor];

			auto SetFunction = [&]( auto& table, const id<MTLFunction>& function, int index ) {
				auto handle = [rayGen.pipeline functionHandleWithFunction:function];
				if( !handle )
				{
					return E_INVALIDARG;
				}
				[table setFunction:handle atIndex:index];
				return S_OK;
			};

			// Map each piece of scene hit group to its intersection function.
			int groupIndex = 0;
			for( auto& hitGroup : desc.m_hitGroupNames )
			{
				auto found = hitGroupMap.find( hitGroup.name );
				if( found == end( hitGroupMap ) )
				{
					return E_FAIL;
				}
				if( found->second.intersection )
				{
					return E_FAIL;
				}
				if( found->second.anyHit )
				{
					CR_RETURN_HR( SetFunction( functions.anyHit, found->second.anyHit, groupIndex ) );
				}
				else
				{
					auto signature = MTLIntersectionFunctionSignatureInstancing |
						MTLIntersectionFunctionSignatureTriangleData | MTLIntersectionFunctionSignatureWorldSpaceData;
					[functions.anyHit setOpaqueTriangleIntersectionFunctionWithSignature:signature atIndex:groupIndex];
				}
				if( found->second.closestHit )
				{
					CR_RETURN_HR( SetFunction( functions.closestHit, found->second.closestHit, groupIndex ) );
				}
				if( functionTables.empty() )
				{
					FillMaterial( hitBuffers, hitGroup.material );
				}
				++groupIndex;
			}
			if( functionTables.empty() )
			{
				m_missMaterialOffset = hitBuffers.size() * sizeof( uint64_t );
			}
			int missIndex = 0;
			for( auto& names : desc.m_missNames )
			{
				auto found = intersectionFunctions.find( names.name );
				if( found == end( intersectionFunctions ) )
				{
					return E_FAIL;
				}
				CR_RETURN_HR( !SetFunction( functions.miss, found->second, missIndex ) );
				if( functionTables.empty() )
				{
					FillMaterial( hitBuffers, names.material );
				}
				missIndex++;
			}
			functionTables.push_back( functions );
		}

		m_functionTables = std::move( functionTables );

		m_rayGenMaterialOffset = hitBuffers.size() * sizeof( uint64_t );
		for( auto& name : desc.m_rayGenNames )
		{
			FillMaterial( hitBuffers, name.material );
		}

		id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
		if( !hitBuffers.empty() )
		{
			m_materialBuffer = [device newBufferWithBytes:hitBuffers.data()
												   length:hitBuffers.size() * sizeof( uint64_t )
												  options:MTLResourceStorageModeManaged];
			for( auto& tables : m_functionTables )
			{
				[tables.anyHit setBuffer:m_materialBuffer offset:0 atIndex:0];
			}
		}

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

void Tr2RtShaderTableAL::Destroy()
{
	if( @available( macOS 11.0, * ) )
	{
		m_functionTables.clear();
	}
	m_materialBuffer = nullptr;
}

bool Tr2RtShaderTableAL::IsValid() const
{
	if( @available( macOS 11.0, * ) )
	{
		return !m_functionTables.empty();
	}
	return false;
}

API_AVAILABLE( macos( 13.0 ) )
Tr2RtShaderTableAL::ShaderTableData Tr2RtShaderTableAL::GetShaderTableData( uint32_t rayGenIndex,
																			uint64_t globalInputAddress ) const
{
	auto& tables = m_functionTables[rayGenIndex];
	return ShaderTableData{ tables.anyHit.gpuResourceID,
							tables.miss.gpuResourceID,
							tables.closestHit.gpuResourceID,

							m_materialBuffer.gpuAddress + m_missMaterialOffset,
							m_materialBuffer.gpuAddress,
							globalInputAddress };
}

API_AVAILABLE( macos( 13.0 ) )
void Tr2RtShaderTableAL::AddUsedResources( uint32_t rayGenIndex, std::vector<id<MTLResource>>& readResources ) const
{
	auto& tables = m_functionTables[rayGenIndex];
	readResources.push_back( tables.anyHit );
	readResources.push_back( tables.miss );
	readResources.push_back( tables.closestHit );
	readResources.push_back( m_materialBuffer );
}

void Tr2RtShaderTableAL::SetGlobalInputBuffer( uint32_t rayGenIndex,
											   const id<MTLBuffer>& buffer,
											   uint32_t offset ) const
{
	if( @available( macOS 11.0, * ) )
	{
		[m_functionTables[rayGenIndex].anyHit setBuffer:buffer offset:offset atIndex:1];
	}
}


id<MTLBuffer> Tr2RtShaderTableAL::GetMaterialBuffer() const
{
	return m_materialBuffer;
}

NSUInteger Tr2RtShaderTableAL::GetRayGenMaterialOffset( uint32_t rayGenIndex ) const
{
	const int BUFFERS_PER_SHADER = sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants ) /
		sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants[0] );
	return m_rayGenMaterialOffset + sizeof( uint64_t ) * BUFFERS_PER_SHADER * rayGenIndex;
}

Tr2ALMemoryType Tr2RtShaderTableAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtShaderTableAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtShaderTableAL";
}
}


#endif
