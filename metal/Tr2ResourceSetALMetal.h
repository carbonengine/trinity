#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2ResourceSetAL.h"
#include "MetalContext.h"

namespace TrinityALImpl
{

	class Tr2ResourceSetAL : public Tr2DeviceResourceAL<Tr2ResourceSetAL>
	{
	public:
		Tr2ResourceSetAL();
		~Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		id<MTLBuffer>       m_buffers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		id<MTLTexture>      m_textures[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		id<MTLSamplerState> m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];

		uint32_t m_buffersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
		NSRange m_texturesRange[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
		NSRange m_samplersRange[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
		std::string m_name;

		bool m_isValid;

		friend class ::Tr2RenderContextAL;
	};
}

#endif
