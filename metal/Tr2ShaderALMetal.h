#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2ShaderAL.h"
#include "Tr2ResourceSetALMetal.h"


namespace TrinityALImpl
{
	// -------------------------------------------------------------
	// Description:
	//   A low level wrapper around shaders / shader programs.
	//   32bit - no support for shader blobs > 4 gig
	// -------------------------------------------------------------
	class Tr2ShaderAL : public Tr2DeviceResourceAL<Tr2ShaderAL>
	{
	public:
		Tr2ShaderAL();
		~Tr2ShaderAL();

		ALResult Create(
			Tr2RenderContextEnum::ShaderType type,
			const Tr2ShaderBytecodeAL& bytecode,
			const Tr2ShaderSignatureAL& signature,
			const char* shaderPath,
			Tr2PrimaryRenderContextAL &renderContext );

		void Destroy();

		bool IsValid() const;
		Tr2RenderContextEnum::ShaderType GetType() const;
		ALResult GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const;
		const Tr2ShaderSignatureAL& GetSignature() const;

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

		void SetNullShaderType( Tr2RenderContextEnum::ShaderType type );
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2ShaderSignatureAL m_signature;
		CcpMallocBuffer m_bytecode;
		Tr2RenderContextEnum::ShaderType m_type;
		
		// Used to quickly compare the resources declared here and those required by the shader
		ShaderResourceMask m_resourceMask;
		std::string m_name;
		
		friend class Tr2ShaderProgramAL;
	};
}

#endif
