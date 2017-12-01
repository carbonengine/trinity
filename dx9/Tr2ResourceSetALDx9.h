#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "../include/Tr2ResourceSetAL.h"
#include "Tr2SamplerStateALDx9.h"

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
		static const uint32_t MAX_RESOURCES = 16;

		struct Texture
		{
			uint32_t registerIndex;
			CComPtr<IDirect3DBaseTexture9> texture;
		};

		struct SamplerState
		{
			uint32_t registerIndex;
			D3DSAMPLERSTATETYPE state;
			uint32_t value;
		};

		std::vector<Texture> m_textures;
		std::vector<SamplerState> m_samplerStates;
		uint32_t m_samplerHash;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif