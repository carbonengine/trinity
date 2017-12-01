#pragma once

#if TRINITY_PLATFORM == TRINITY_OPENGL4

#include "../include/Tr2ResourceSetAL.h"
#include "Tr2SamplerStateALGL4.h"

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
			GLuint sampler;
		};

		struct CLResource
		{
			CLResource();

			cl_mem resource;
			cl_sampler sampler;
			uint32_t registerIndex;
			bool isSampler;
		};

		std::vector<Texture> m_textures;
		std::vector<CLResource> m_clResources;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif