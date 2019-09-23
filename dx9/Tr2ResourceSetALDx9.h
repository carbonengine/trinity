#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL : public Tr2DeviceResourceAL<Tr2ResourceSetAL>
	{
	public:
		Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
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