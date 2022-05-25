#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL: public Tr2DeviceResourceAL<Tr2ResourceSetAL>
	{
	public:
		Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

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

		CComPtr<ID3D11UnorderedAccessView> m_uavs[MAX_RESOURCES];
		uint32_t m_uavOffset;
		uint32_t m_uavCount;
		std::string m_name;
		bool m_csUavs;

		bool m_empty;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif