////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once


#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2ShaderAL.h"


namespace TrinityALImpl
{
	class Tr2ShaderProgramAL;

	// -------------------------------------------------------------
// Description:
//   A low level wrapper around shaders / shader programs. DX11
//   specific version will also hold on to byte code for the 
//   shader as this is needed for construction of input layouts.
//   Avoid using this class directly; instead use effects and Tr2Effect.
//   32bit - no support for shader blobs > 4 gig
// -------------------------------------------------------------
	class Tr2ShaderAL :
		public Tr2DeviceResourceAL<Tr2ShaderAL>
	{
	public:
		Tr2ShaderAL();
		~Tr2ShaderAL();

		ALResult Create(
			Tr2RenderContextEnum::ShaderType type,
			const Tr2ShaderBytecodeAL& bytecode,
			const Tr2ShaderBytecodeAL& patchedBytecode,
			const Tr2ShaderSignatureAL& signature,
			Tr2PrimaryRenderContextAL &renderContext );
		void Destroy();
		bool IsValid() const;

		bool operator==( const Tr2ShaderAL& shader ) const;

		Tr2RenderContextEnum::ShaderType GetType() const;
		ALResult GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const;
		const Tr2ShaderSignatureAL& GetSignature() const;

		Tr2ALMemoryType GetMemoryClass() const;

		void SetNullShaderType( Tr2RenderContextEnum::ShaderType type );
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2ShaderAL( const Tr2ShaderAL& shader );
		Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

		VkShaderModule m_shader;
		Tr2PrimaryRenderContextAL* m_owner;
		Tr2ShaderSignatureAL m_signature;
		CcpMallocBuffer m_bytecode;
		Tr2RenderContextEnum::ShaderType m_type;

		friend class TrinityALImpl::Tr2ShaderProgramAL;
	};
}

#endif 
