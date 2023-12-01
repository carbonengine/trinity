////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2ShaderAL.h"


namespace TrinityALImpl
{
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
		Tr2ALMemoryType GetMemoryClass() const;

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2ShaderAL( const Tr2ShaderAL& shader );
		Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

		Tr2ShaderSignatureAL m_signature;
		CcpMallocBuffer m_bytecode;
		Tr2RenderContextEnum::ShaderType m_type;
		std::string m_name;

		friend class Tr2ShaderProgramAL;
	};
}

#endif 
