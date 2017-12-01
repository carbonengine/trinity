#pragma once

#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "../include/Tr2ResourceSetAL.h"
#include "Tr2SamplerStateALGLES2.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RESOURCE_SET>
	{
	public:
		Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
	private:
		struct Texture
		{
			std::shared_ptr<GLuint> texture;
			uint32_t registerIndex;
			GLenum type;
			GLint srgbDecode;
			Tr2SamplerStateAL::StateData sampler;
			bool hasMips;
		};

		std::vector<Texture> m_textures;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif