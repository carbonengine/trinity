#pragma once

#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "../include/Tr2ShaderAL.h"


namespace TrinityALImpl
{
	// -------------------------------------------------------------
	// Description:
	//   A low level wrapper around shaders / shader programs. 
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
		Tr2RenderContextEnum::ShaderType GetType() const;
		ALResult GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const;
		const Tr2ShaderSignatureAL& GetSignature() const;

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2ShaderAL( const Tr2ShaderAL& shader );
		Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

		Tr2RenderContextEnum::ShaderType m_type;
		TrackableStdVector<uint8_t>	m_bytecode;
		Tr2ShaderSignatureAL m_signature;

		int32_t m_shader;
		int32_t m_patchedShader;

		friend class Tr2RenderContextAL;
		friend class Tr2ShaderProgramAL;
	};

}
#endif 
