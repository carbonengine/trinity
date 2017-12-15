#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RESOURCE_SET>
	{
	public:
		Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
	private:
		static const uint32_t MAX_RESOURCES = 32;

		struct StageInput
		{
			StageInput();
			void Destroy();

			CComPtr<ID3D11ShaderResourceView> resources[MAX_RESOURCES];
			CComPtr<ID3D11SamplerState> samplers[MAX_RESOURCES];

			uint32_t resourceCount;
			uint32_t resourceOffset;
			uint32_t samplerCount;
			uint32_t samplerOffset;
			uint32_t resourceHash;
			uint32_t samplerHash;
		};

		StageInput m_stages[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
		bool m_empty;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif