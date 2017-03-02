#pragma once
#ifndef Tr2ShaderALDx11_H
#define Tr2ShaderALDx11_H

#include "Tr2VertexDefinition.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

class Tr2RenderContextAL;

// -------------------------------------------------------------
// Description:
//   A low level wrapper around shaders / shader programs. DX11
//   specific version will also hold on to byte code for the 
//   shader as this is needed for construction of input layouts.
//   Avoid using this class directly; instead use effects and Tr2Effect.
//   32bit - no support for shader blobs > 4 gig
// -------------------------------------------------------------
class Tr2ShaderAL: 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER>
{
public:
	Tr2ShaderAL();
	~Tr2ShaderAL();

	ALResult Create( 
		Tr2PrimaryRenderContextAL &renderContext, 
		Tr2RenderContextEnum::ShaderType type, 
		const void* bytecode, 
		uint32_t bytecodeSize, 
		const void* patchedBytecode, 
		uint32_t patchedBytecodeSize, 
		const Tr2ShaderInputDefinition& inputDefinition );

	void Destroy();

	bool IsValid() const;
	bool operator==( const Tr2ShaderAL& shader ) const;
	Tr2RenderContextEnum::ShaderType GetType() const	{ return m_type; }
	ALResult GetBytecode( const void*& bytecode, uint32_t& size ) const;
	const Tr2ShaderInputDefinition& GetInputDefinition() const	{ return m_inputDefinition; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	void SetNullShaderType( Tr2RenderContextEnum::ShaderType type );

private:
	Tr2ShaderAL( const Tr2ShaderAL& shader );
	Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

	void ReleaseShader();
	ALResult Apply( Tr2RenderContextAL& renderContext ) const;
	ALResult ApplyPatchedShader( Tr2RenderContextAL& renderContext ) const;

	Tr2RenderContextEnum::ShaderType	m_type;
	CcpMallocBuffer						m_bytecode;	
	Tr2ShaderInputDefinition			m_inputDefinition;
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
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11

#endif // Tr2ShaderALDx11_H
