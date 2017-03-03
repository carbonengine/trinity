#pragma once
#ifndef Tr2ShaderALDx9_H
#define Tr2ShaderALDx9_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "../Tr2VertexDefinition.h"

class Tr2RenderContextAL;


// -------------------------------------------------------------
// Description:
//   A low level wrapper around shaders / shader programs. 
//   32bit - no support for shader blobs > 4 gig
// -------------------------------------------------------------
class Tr2ShaderAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER>
{
public:
	Tr2ShaderAL();
	~Tr2ShaderAL();

	ALResult Create( 
		Tr2RenderContextAL& renderContext, 
		Tr2RenderContextEnum::ShaderType type, 
		const void* bytecode, 
		uint32_t bytecodeSize, 
		const void* patchedBytecode, 
		uint32_t patchedBytecodeSize, 
		const Tr2ShaderInputDefinition& inputDefinition );

	void Destroy();

	bool IsValid() const;
	bool operator==( const Tr2ShaderAL& shader ) const;
	Tr2RenderContextEnum::ShaderType GetType() const;
	ALResult GetBytecode( const void*& bytecode, uint32_t& size ) const;
	const Tr2ShaderInputDefinition& GetInputDefinition() const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	void SetNullShaderType( Tr2RenderContextEnum::ShaderType type );
private:
	Tr2ShaderAL( const Tr2ShaderAL& shader );
	Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

	ALResult CreateCL( const void* bytecode, uint32_t bytecodeSize, const Tr2ShaderInputDefinition& inputDefinition, Tr2RenderContextAL& renderContext );

	Tr2RenderContextEnum::ShaderType m_type;
	TrackableStdVector<uint8_t>	m_bytecode;
	Tr2ShaderInputDefinition m_inputDefinition;

	std::map<std::pair<Tr2VertexDefinition::UsageCode, unsigned>, unsigned> m_inputs;

	struct GLShader
	{
		GLuint shader;
		GLint constantBuffers[16];
	};

	GLShader m_shader;
	GLShader m_patchedShader;
	cl_kernel m_clKernel;

	friend class Tr2RenderContextAL;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11

#endif // Tr2ShaderALDx9_H