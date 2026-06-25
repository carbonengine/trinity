// Copyright © 2024 CCP ehf.

//
//  Tr2RtPipelineStateALMetal.hpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL


//#include "StdAfx.h"
#include "Tr2ShaderProgramALMetal.h"
#include "../include/Tr2RtPipelineStateAL.h"
#include "../include/Tr2ResourceSetAL.h"
#include "Tr2ShaderAL.h"
#include <Metal/Metal.h>

namespace TrinityALImpl
{
class Tr2RtPipelineStateAL : public Tr2DeviceResourceAL<Tr2RtPipelineStateAL>
{
public:
	Tr2RtPipelineStateAL();
	~Tr2RtPipelineStateAL();

	ALResult CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc,
									Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	struct HitGroupFunctions
	{
		id<MTLFunction> closestHit;
		id<MTLFunction> intersection;
		id<MTLFunction> anyHit;
	};

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	std::optional<uint32_t> GetRayGenIndex( const wchar_t* rayGenName ) const;
	id<MTLComputePipelineState> GetRtPipeline( uint32_t rayGenIndex ) const;
	const ::Tr2ShaderProgramAL& GetShaderProgram( uint32_t rayGenIndex ) const;
	const std::unordered_map<std::wstring, id<MTLFunction>>& GetFunctionMap() const;
	const std::unordered_map<std::wstring, HitGroupFunctions>& GetHitGroupMap() const;
	NSString* NSStringFromWchar( std::wstring name );

	Tr2ShaderSignatureAL m_globalSignature;

private:
	id<MTLFunction> CreateFunction( std::wstring name, id<MTLDevice> device, dispatch_data_t shaderData );

	std::unordered_map<std::wstring, id<MTLFunction>> m_intersectionFunctions;
	std::unordered_map<std::wstring, HitGroupFunctions> m_hitGroupMap;

	struct RayGenShader
	{
		std::wstring name;
		::Tr2ShaderProgramAL shaderProgram;
		id<MTLComputePipelineState> pipeline;
	};
	std::vector<RayGenShader> m_rayGenShaders;

	friend class Tr2RtShaderTableAL;
};
}

#endif
