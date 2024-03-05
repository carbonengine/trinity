////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2ResourceSetAL.h"
#include "util/DescriptorHeapViewDx12.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL: public Tr2DeviceResourceAL<Tr2ResourceSetAL>
	{
	public:
		Tr2ResourceSetAL();
		~Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		std::shared_ptr<ShaderResourceViewDx12> m_srv[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		std::shared_ptr<UnorderedAccessViewDx12> m_uav[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		std::shared_ptr<SamplerStateDx12> m_sampler[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		uint32_t m_samplerCount;
		uint32_t m_resourceCount;
		uint32_t m_srvMask;
		uint32_t m_uavMask;
		Tr2PrimaryRenderContextAL* m_owner;

		std::vector<D3D12_RESOURCE_BARRIER> m_inTransitions;
		std::vector<D3D12_RESOURCE_BARRIER> m_outTransitions;
		std::vector<ID3D12Resource*> m_usedResources;
		std::string m_name;

		friend class ::Tr2RenderContextAL;
	};
}

#endif