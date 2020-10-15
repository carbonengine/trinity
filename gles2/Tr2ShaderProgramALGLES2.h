#pragma once

#if TRINITY_PLATFORM==TRINITY_OPENGLES2

#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2ShaderAL.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../Tr2VertexDefinition.h"

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

		const Tr2RegisterMapAL& GetRegisterMap() const;

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

		::Tr2ShaderAL m_vertexShader;
		::Tr2ShaderAL m_pixelShader;

		ProgramData m_program;
		ProgramData m_patchedProgram;
		Tr2RegisterMapAL m_registerMap;
		bool m_isValid;

		friend class ::Tr2RenderContextAL;
	};

}

#endif