#pragma once


#if TRINITY_PLATFORM == TRINITY_DIRECTX11

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
		~Tr2ShaderAL();

		ALResult Create(
			Tr2RenderContextEnum::ShaderType type,
			const Tr2ShaderBytecodeAL& bytecode,
			const Tr2ShaderBytecodeAL& patchedBytecode,
			const Tr2ShaderSignatureAL& signature,
			Tr2PrimaryRenderContextAL &renderContext );

		void Destroy();

		bool IsValid() const;
		Tr2RenderContextEnum::ShaderType GetType() const { return m_type; }
		ALResult GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const;
		const Tr2ShaderSignatureAL& GetSignature() const;

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2ShaderAL( const Tr2ShaderAL& shader );
		Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

		void ReleaseShader();

		Tr2RenderContextEnum::ShaderType	m_type;
		CcpMallocBuffer						m_bytecode;
		Tr2ShaderSignatureAL m_signature;
		uint32_t m_pipelineInputHash;
		union Shader
		{
			ID3D11VertexShader* vertexShader;
			ID3D11PixelShader* pixelShader;
			ID3D11ComputeShader* computeShader;
			ID3D11GeometryShader* geometryShader;
			ID3D11HullShader* hullShader;
			ID3D11DomainShader* domainShader;
		};
		Shader m_shader;
		Shader m_patchedShader;

		friend class Tr2RenderContextAL;
		friend class Tr2ShaderProgramAL;
		friend class Tr2VertexLayoutAL;
	};
}

#endif
