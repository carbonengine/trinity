////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RESOURCE_SET>
	{
	public:
		Tr2ResourceSetAL();
		~Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;

		void UploadInitialCounts( Tr2RenderContextAL& renderContext );
	private:
		std::shared_ptr<ShaderResourceViewDx12> m_srv[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		std::shared_ptr<UnorderedAccessViewDx12> m_uav[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		std::shared_ptr<DescriptorHeapViewDx12> m_srvUav[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		std::shared_ptr<SamplerStateDx12> m_sampler[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		Tr2PrimaryRenderContextAL* m_owner;

		struct InitialCount
		{
			::Tr2BufferAL buffer;
			uint32_t initialCount;
		};

		std::vector<InitialCount> m_initialCounts;
		CComPtr<ID3D12Resource> m_initialCountBuffer;

		std::vector<D3D12_RESOURCE_BARRIER> m_inTransitions;
		std::vector<D3D12_RESOURCE_BARRIER> m_outTransitions;

		friend class ::Tr2RenderContextAL;
	};
}

#endif