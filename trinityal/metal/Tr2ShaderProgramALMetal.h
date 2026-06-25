// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2ResourceSetAL.h"
#include "Tr2ShaderALMetal.h"
#include "Tr2ResourceSetALMetal.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
class Tr2ShaderProgramAL : public Tr2DeviceResourceAL<Tr2ShaderProgramAL>
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

	id<MTLFunction> GetVertexShader() const;
	id<MTLFunction> GetFragmentShader() const;
	id<MTLFunction> GetComputeKernel() const;
	MTLSize GetThreadGroupSize() const;
	const ShaderResourceMask* GetResourceMasks() const;

	const Tr2RegisterMapAL& GetRegisterMap() const;

	const std::vector<Tr2ShaderPipelineInputAL>& GetInputs() const;
	size_t GetInputsHash() const;

	void SetDummyResources( TrinityALImpl::MetalWorkQueue& workQueue );

private:
	id<MTLFunction>
		CompileShader( const ::Tr2ShaderAL& shader, NSString* entryFunction, Tr2PrimaryRenderContextAL& renderContext );

	id<MTLFunction> m_vertexFunction;
	id<MTLFunction> m_fragmentFunction;
	id<MTLFunction> m_computeFunction;
	std::vector<Tr2ShaderPipelineInputAL> m_iaInputs;
	size_t m_iaInputsHash;
	ShaderResourceMask m_resourceMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	MetalContext* m_metalContext;
	Tr2RegisterMapAL m_registerMap;

	MTLSize m_threadGroupSize;
	std::string m_name;

	bool m_isValid;

	friend class Tr2RenderContextAL;
	friend class Tr2ResourceSetAL;
};
}

#endif
