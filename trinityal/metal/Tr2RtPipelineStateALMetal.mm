// Copyright © 2024 CCP ehf.

//
//  Tr2RtPipelineStateALMetal.cpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "../ALLog.h"
#include "MetalShaderStrings.h"


namespace TrinityALImpl
{
Tr2RtPipelineStateAL::Tr2RtPipelineStateAL()
{
}

Tr2RtPipelineStateAL::~Tr2RtPipelineStateAL()
{
}

NSString* Tr2RtPipelineStateAL::NSStringFromWchar( std::wstring name )
{
	NSString* sName = [[NSString alloc] initWithBytes:name.data()
											   length:name.size() * sizeof( wchar_t )
											 encoding:NSUTF32LittleEndianStringEncoding];

	return sName;
}

id<MTLFunction>
	Tr2RtPipelineStateAL::CreateFunction( std::wstring name, id<MTLDevice> device, dispatch_data_t shaderData )
{
	NSError* error = nullptr;
	id<MTLLibrary> mtlLib = [device newLibraryWithData:shaderData error:&error];
#if !__has_feature( objc_arc )
	dispatch_release( shaderData );
#endif

	if( !mtlLib )
	{
		CCP_AL_LOGERR( "Tr2ShaderProgramAL: Failed to create Metal shader library. Error: %s",
					   error.localizedDescription.UTF8String );
#if !__has_feature( objc_arc )
		if( error )
		{
			[error release];
		}
#endif
		return nil;
	}

	NSString* NSname = NSStringFromWchar( name );

	// Fill out a dictionary of function constant values.
	// MTLFunctionConstantValues *constants = [[MTLFunctionConstantValues alloc] init];
	// The second constant turns the use of intersection functions on and off.
	//bool _useIntersectionFunctions = true;
	//[constants setConstantValue:&_useIntersectionFunctions type:MTLDataTypeBool atIndex:0];

	id<MTLFunction> fn = [mtlLib newFunctionWithName:NSname]; //constantValues:constants error:&error];

	return fn;
}

ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc,
													  Tr2PrimaryRenderContextAL& renderContext )
{
	if( @available( macOS 11.0, * ) )
	{
		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( desc.m_shaders.empty() )
		{
			return E_INVALIDCALL;
		}

		// create shader programs for ray gen shaders
		std::vector<RayGenShader> rayGenShaders;

		for( auto& shader : desc.m_shaders )
		{
			for( auto& name : shader.names )
			{
				if( name.name == L"RayGen" )
				{
					::Tr2ShaderAL rayGenShader;
					FORWARD_HR( rayGenShader.Create( Tr2RenderContextEnum::COMPUTE_SHADER,
													 shader.bytecode,
													 desc.m_globalSignature,
													 "",
													 renderContext ) );
					rayGenShader.TrinityALImpl_GetObject()->m_entryPointNameOverride = NSStringFromWchar( name.name );

					::Tr2ShaderProgramAL shaderProgram;
					FORWARD_HR( shaderProgram.Create( &rayGenShader, 1, renderContext ) );
					rayGenShaders.push_back( { name.exportName, shaderProgram } );
				}
			}
		}
		if( rayGenShaders.empty() )
		{
			return E_INVALIDARG;
		}

		id<MTLDevice> mtlDevice = renderContext.GetMetalContext()->GetDevice();

		NSMutableArray* linkedFunctions = [[NSMutableArray alloc] init];

		for( auto& shader : desc.m_shaders )
		{
			dispatch_data_t shaderData = dispatch_data_create( shader.bytecode.bytecode,
															   shader.bytecode.size,
															   dispatch_get_global_queue( 0, 0 ),
															   DISPATCH_DATA_DESTRUCTOR_DEFAULT );
			NSError* error = nullptr;
			id<MTLLibrary> mtlLib = [mtlDevice newLibraryWithData:shaderData error:&error];
			if( !mtlLib )
			{
				CCP_AL_LOGERR( "Tr2RtPipelineStateAL: Failed to create Metal shader library. Error: %s",
							   error.localizedDescription.UTF8String );
				return E_FAIL;
			}

			for( auto& name : shader.names )
			{
				if( name.name == L"RayGen" )
				{
					continue;
				}
				auto descriptor = [MTLFunctionDescriptor functionDescriptor];
				descriptor.name = NSStringFromWchar( name.name );
				descriptor.specializedName = NSStringFromWchar( name.exportName );

				error = nullptr;
				id<MTLFunction> fn = [mtlLib newFunctionWithDescriptor:descriptor error:&error];
				if( fn == nil )
				{
					CCP_AL_LOGERR(
						"Tr2RtPipelineStateAL: Failed to get function %S from Metal shader library. Error: %s",
						name.name.c_str(),
						error.localizedDescription.UTF8String );
					return E_FAIL;
				}

				m_intersectionFunctions[name.exportName] = fn;
				[linkedFunctions addObject:fn];
			}
		}

		if( m_intersectionFunctions.size() == 0 )
		{
			return E_FAIL;
		}

		for( auto& hGroup : desc.m_hitGroups )
		{
			auto& dest = m_hitGroupMap[hGroup.exportName];

			if( !hGroup.anyHit.empty() )
			{
				auto found = m_intersectionFunctions.find( hGroup.anyHit );
				if( found == end( m_intersectionFunctions ) )
				{
					return E_INVALIDARG;
				}
				dest.anyHit = found->second;
			}
			if( !hGroup.intersection.empty() )
			{
				auto found = m_intersectionFunctions.find( hGroup.intersection );
				if( found == end( m_intersectionFunctions ) )
				{
					return E_INVALIDARG;
				}
				dest.intersection = found->second;
			}
			if( !hGroup.closestHit.empty() )
			{
				auto found = m_intersectionFunctions.find( hGroup.closestHit );
				if( found == end( m_intersectionFunctions ) )
				{
					return E_INVALIDARG;
				}
				dest.closestHit = found->second;
			}
		}

		MTLLinkedFunctions* mtlLinkedFunctions = nil;
		// Attach the additional functions to an MTLLinkedFunctions object
		mtlLinkedFunctions = [[MTLLinkedFunctions alloc] init];
		mtlLinkedFunctions.functions = linkedFunctions;

		id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();

		for( auto& rayGen : rayGenShaders )
		{
			MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
			descriptor.computeFunction = rayGen.shaderProgram.TrinityALImpl_GetObject()->GetComputeKernel();
			// add the functions to the pipeline
			descriptor.linkedFunctions = mtlLinkedFunctions;
			// Set to YES to allow the compiler to make certain optimizations.
			descriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;
			// Create compute pipelines will execute code on the GPU
			// Create the compute pipeline state which does all the raytracing
			NSError* error = nullptr;
			rayGen.pipeline = [device newComputePipelineStateWithDescriptor:descriptor
																	options:0
																 reflection:nil
																	  error:&error];

			if( !rayGen.pipeline )
			{
				CCP_LOGERR( "Failed to create a raytracing pipeline state: %s", error.localizedDescription.UTF8String );
			}
		}
		m_globalSignature = desc.m_globalSignature;
		m_rayGenShaders = std::move( rayGenShaders );

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

std::optional<uint32_t> Tr2RtPipelineStateAL::GetRayGenIndex( const wchar_t* rayGenName ) const
{
	auto found = find_if( begin( m_rayGenShaders ), end( m_rayGenShaders ), [rayGenName]( const auto& x ) {
		return x.name == rayGenName;
	} );
	if( found == end( m_rayGenShaders ) )
	{
		return {};
	}
	return found - begin( m_rayGenShaders );
}

const ::Tr2ShaderProgramAL& Tr2RtPipelineStateAL::GetShaderProgram( uint32_t rayGenIndex ) const
{
	return m_rayGenShaders[rayGenIndex].shaderProgram;
}

bool Tr2RtPipelineStateAL::IsValid() const
{
	return !m_rayGenShaders.empty();
}

Tr2ALMemoryType Tr2RtPipelineStateAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

id<MTLComputePipelineState> Tr2RtPipelineStateAL::GetRtPipeline( uint32_t rayGenIndex ) const
{
	return m_rayGenShaders[rayGenIndex].pipeline;
}

void Tr2RtPipelineStateAL::Destroy()
{
	m_intersectionFunctions.clear();
	m_hitGroupMap.clear();
	m_rayGenShaders.clear();
}

void Tr2RtPipelineStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtPipelineStateAL";
}

const std::unordered_map<std::wstring, id<MTLFunction>>& Tr2RtPipelineStateAL::GetFunctionMap() const
{
	return m_intersectionFunctions;
}

const std::unordered_map<std::wstring, Tr2RtPipelineStateAL::HitGroupFunctions>&
	Tr2RtPipelineStateAL::GetHitGroupMap() const
{
	return m_hitGroupMap;
}
}

#endif
