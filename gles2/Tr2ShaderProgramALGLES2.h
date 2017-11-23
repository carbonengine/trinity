#pragma once

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"

#if TRINITY_PLATFORM==TRINITY_OPENGLES2

class Tr2ShaderProgramAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( Tr2ShaderAL** shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	struct ProgramData
	{
		GLuint program;
		int constantBuffers[16];
		int intConstant;
		int shadowStateInt;
		int shadowStateFloat;
		int shadowStateOffsets;
		std::map<std::pair<Tr2VertexDefinition::UsageCode, uint32_t>, int> attributes;
	};

	ALResult CreateProgram( ProgramData& program, GLuint vertexShader, GLuint pixelShader, bool useShadowStates );

	const Tr2ShaderAL* m_vertexShader;
	const Tr2ShaderAL* m_pixelShader;

	ProgramData m_program;
	ProgramData m_patchedProgram;
	bool m_isValid;

	friend class Tr2RenderContextAL;
};

#endif