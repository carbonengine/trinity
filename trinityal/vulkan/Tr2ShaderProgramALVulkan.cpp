#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ShaderProgramALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2ShaderALVulkan.h"
#include "Tr2ShaderBindingABIVulkan.h"
#include "UtilitiesVulkan.h"


using namespace Tr2RenderContextEnum;


namespace
{
	VkDescriptorType GetDescriptorType( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		switch( registerType )
		{
		case Tr2ShaderRegisterAL::CONSTANT_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Tr2ShaderRegisterAL::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		default:
			// UAV is no longer a single value (Tr2ShaderAL.h:56-86): it is a family
			// flagged by UAV_REGISTER_FLAG, spanning UAV_BUFFER (64) through
			// UAV_TEXTURECUBEARRAY (74). Vulkan's VkDescriptorType genuinely
			// distinguishes storage buffer from storage image, so the two-way split
			// below -- mirroring dx12's ordinal comparison against *_STRUCTURED_BUFFER,
			// e.g. Tr2ResourceSetALDx12.cpp:128,186 -- is required for correctness, not
			// just to compile: the old code collapsed every UAV subtype to
			// STORAGE_IMAGE, which is wrong for UAV_BUFFER/UAV_STRUCTURED_BUFFER and is
			// already exercised by tests/Compute.cpp.
			if( registerType & Tr2ShaderRegisterAL::UAV_REGISTER_FLAG )
			{
				return registerType <= Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER
					? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
					: VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}
			// SRV has the identical shape (Tr2ShaderAL.h:63-73): a family flagged by
			// SRV_REGISTER_FLAG, spanning SRV_BUFFER (32) through SRV_TEXTURECUBEARRAY
			// (42). The 2019 code collapsed every SRV subtype to SAMPLED_IMAGE right
			// here via this same `default:` branch -- wrong for
			// SRV_BUFFER/SRV_STRUCTURED_BUFFER for the identical reason as UAV above.
			// It escaped Task 4e only because `default:` draws no compiler diagnostic;
			// fixed now (Task 4f) with the same two-way split, the same dx12 ordinal
			// comparison against *_STRUCTURED_BUFFER (e.g. Tr2ResourceSetALDx12.cpp:128).
			//
			// STORAGE_BUFFER, not UNIFORM_TEXEL_BUFFER, for the buffer-like half:
			// metal binds SRV_BUFFER/SRV_STRUCTURED_BUFFER exactly like
			// UAV_BUFFER/UAV_STRUCTURED_BUFFER -- a raw GPU buffer address, no
			// format/texel-view step (Tr2ShaderALMetal.mm:69-75 vs :139-144;
			// MetalWorkQueue.mm:3113-3125 reads gpuAddress+offset for both families the
			// same way). dx12's typed-vs-structured buffer SRV/UAV split
			// (Tr2BufferALDx12.cpp:158-177) is driven by the bound buffer's pixel
			// format *at creation time*, not by which RegisterType the shader declared,
			// so it can't be mirrored here anyway -- GetDescriptorType only ever sees
			// the registerType, never the runtime resource. And Tr2BufferALVulkan has
			// no VkBufferView machinery today (there is none anywhere under
			// trinityal/vulkan), so STORAGE_BUFFER needs no new plumbing where
			// UNIFORM_TEXEL_BUFFER would. Hence: STORAGE_BUFFER, mirroring UAV exactly.
			//
			// KNOWN WRONG, left in place deliberately for Phase 2a. HLSL Buffer<T>
			// compiles to a uniform texel buffer and RWBuffer<T> to a storage texel
			// buffer (verified: OpTypeImage ... Buffer, Sampled=1 and Sampled=2), so
			// SRV_BUFFER/UAV_BUFFER want UNIFORM_TEXEL_BUFFER/STORAGE_TEXEL_BUFFER and
			// a VkBufferView, while *_STRUCTURED_BUFFER wants STORAGE_BUFFER and a
			// VkDescriptorBufferInfo. Neither path exists in this backend. Fixing the
			// type here without the write path would only move the mismatch, which is
			// how the SRV_BUFFER layout/write mismatch was created in the first place.
			// Phase 2b owns this; Phase 2a measures it.
			if( registerType & Tr2ShaderRegisterAL::SRV_REGISTER_FLAG )
			{
				return registerType <= Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER
					? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
					: VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		}
	}

	// RegisterType can no longer be used directly as an array index (see the two
	// call sites in Create() below): Tr2ShaderAL.h's rewrite turned it from a dense
	// 0-3 enum into a flag-tagged one (SRV_* = 32-42, UAV_* = 64-74), so a direct
	// `array[registerType]` silently reads/writes far out of bounds for any SRV or
	// UAV register. This mirrors dx12's RegisterTypeIndex
	// (Tr2PrimaryRenderContextDx12.cpp:100) -- a RegisterType -> dense-slot mapping --
	// sized here to the six descriptor kinds GetDescriptorType above can return:
	// constant buffer, sampler, SRV buffer-like, SRV image-like, UAV buffer-like,
	// UAV image-like.
	// This mapping is descriptor-kind only; binding-number blocks are RegisterClassOffset's job.
	enum RegisterSlot
	{
		REGISTER_SLOT_CONSTANT_BUFFER,
		REGISTER_SLOT_SAMPLER,
		REGISTER_SLOT_SRV_BUFFER,
		REGISTER_SLOT_SRV_IMAGE,
		REGISTER_SLOT_UAV_BUFFER,
		REGISTER_SLOT_UAV_IMAGE,
		REGISTER_SLOT_COUNT
	};

	uint32_t RegisterTypeIndex( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		if( registerType & Tr2ShaderRegisterAL::UAV_REGISTER_FLAG )
		{
			return registerType <= Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER
				? REGISTER_SLOT_UAV_BUFFER : REGISTER_SLOT_UAV_IMAGE;
		}
		if( registerType & Tr2ShaderRegisterAL::SRV_REGISTER_FLAG )
		{
			return registerType <= Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER
				? REGISTER_SLOT_SRV_BUFFER : REGISTER_SLOT_SRV_IMAGE;
		}
		return registerType == Tr2ShaderRegisterAL::SAMPLER
			? REGISTER_SLOT_SAMPLER : REGISTER_SLOT_CONSTANT_BUFFER;
	}

	// The binding-number block a register lives in -- RegisterClassOffset, and the
	// whole binding formula with it -- now lives in Tr2ShaderBindingABIVulkan.h, so
	// that trinityal/tests/ShaderBindingABI.cpp can derive its expected SPIR-V
	// decorations by calling the same code this file calls, instead of tabulating
	// sixteen expected numbers that would keep passing while the formula drifted.
	// That header carries the full rationale (four HLSL classes vs. six descriptor
	// kinds; why b and u can share block 0; what Tasks 4e and 4f got wrong).
}

namespace TrinityALImpl
{
	Tr2ShaderProgramAL::Tr2ShaderProgramAL()
		:m_owner( nullptr ),
		m_resourceLayout( VK_NULL_HANDLE ),
		m_constantLayout( VK_NULL_HANDLE ),
		m_pipelineLayout( VK_NULL_HANDLE )
	{
	}

	Tr2ShaderProgramAL::~Tr2ShaderProgramAL()
	{
		Destroy();
	}

	ALResult Tr2ShaderProgramAL::Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( count == 0 )
		{
			return E_INVALIDARG;
		}

		uint32_t bitmask = 0;

		for( size_t i = 0; i < count; ++i )
		{
			if( !shaders[i].IsValid() )
			{
				return E_INVALIDARG;
			}
			auto mask = 1 << shaders[i].GetType();
			if( ( mask & bitmask ) != 0 )
			{
				return E_INVALIDARG;
			}
			bitmask |= mask;
		}
		auto csBit = 1 << COMPUTE_SHADER;
		if( ( bitmask & csBit ) != 0 && ( bitmask & ~csBit ) != 0 )
		{
			return E_INVALIDARG;
		}

		m_shaderInfo.reserve( count );
		m_shaders.reserve( count );

		uint32_t poolSizes[REGISTER_SLOT_COUNT] = { 0 };
		VkDescriptorType poolTypes[REGISTER_SLOT_COUNT] = { };

		std::vector<VkDescriptorSetLayoutBinding> resourceSetBindings, constantBindings;

		for( size_t i = 0; i < count; ++i )
		{
			VkPipelineShaderStageCreateInfo info = {
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
			};
			switch( shaders[i].GetType() )
			{
			case VERTEX_SHADER:
				info.stage = VK_SHADER_STAGE_VERTEX_BIT;
				m_shaderInputs = shaders[i].m_shader->m_signature.pipelineInputs;
				break;
			case PIXEL_SHADER:
				info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case COMPUTE_SHADER:
				info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			case GEOMETRY_SHADER:
				info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				break;
			case HULL_SHADER:
				info.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				break;
			case DOMAIN_SHADER:
				info.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				break;
			}
			info.module = shaders[i].m_shader->m_shader;
			info.pName = "main";
			m_shaderInfo.push_back( info );
			m_shaders.push_back( shaders[i] );

			auto& inputs = shaders[i].m_shader->m_signature.registers;
			for( auto it = begin( inputs ); it != end( inputs ); ++it )
			{
				uint32_t slot = RegisterTypeIndex( it->registerType );

				VkDescriptorSetLayoutBinding binding = {
					Tr2VulkanBindingABI::BindingNumber( it->registerType, it->registerIndex, shaders[i].GetType() ),
					GetDescriptorType( it->registerType ),
					1,
					info.stage,
					nullptr
				};

				if( it->registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER )
				{
					constantBindings.push_back( binding );
				}
				else
				{
					poolTypes[slot] = binding.descriptorType;
					++poolSizes[slot];
					resourceSetBindings.push_back( binding );
				}

				RegisterInput ri = { binding.binding, shaders[i].GetType(), it->registerIndex, it->registerType };
				m_registerInput.push_back( ri );
			}
		}

		if( !resourceSetBindings.empty() )
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
			layoutInfo.bindingCount = uint32_t( resourceSetBindings.size() );
			layoutInfo.pBindings = resourceSetBindings.data();

			VkDescriptorSetLayout layout;
			CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &layoutInfo, nullptr, &layout ) ) );

			m_resourceLayout = layout;

			for( uint32_t i = 0; i < _countof( poolSizes ); ++i )
			{
				if( !poolSizes[i] )
				{
					continue;
				}
				// poolTypes[i] was recorded from the real registerType at tally time
				// (above), rather than reconstructed here via GetDescriptorType(
				// RegisterType(i) ) -- i is a synthetic slot index (see RegisterSlot),
				// not a real RegisterType value, so casting it back would silently
				// mis-dispatch (e.g. i == REGISTER_SLOT_UAV_BUFFER has no
				// UAV_REGISTER_FLAG bit set and would fall through to
				// VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE instead of STORAGE_BUFFER).
				VkDescriptorPoolSize poolSize = { poolTypes[i], poolSizes[i] };
				m_poolSizes.push_back( poolSize );
			}
		}

		if( !constantBindings.empty() )
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
			layoutInfo.bindingCount = uint32_t( constantBindings.size() );
			layoutInfo.pBindings = constantBindings.data();

			VkDescriptorSetLayout layout;
			CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &layoutInfo, nullptr, &layout ) ) );

			m_constantLayout = layout;

			for( uint32_t i = 0; i < _countof( poolSizes ); ++i )
			{
				if( !poolSizes[i] )
				{
					continue;
				}
				// poolTypes[i] was recorded from the real registerType at tally time
				// (above), rather than reconstructed here via GetDescriptorType(
				// RegisterType(i) ) -- i is a synthetic slot index (see RegisterSlot),
				// not a real RegisterType value, so casting it back would silently
				// mis-dispatch (e.g. i == REGISTER_SLOT_UAV_BUFFER has no
				// UAV_REGISTER_FLAG bit set and would fall through to
				// VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE instead of STORAGE_BUFFER).
				VkDescriptorPoolSize poolSize = { poolTypes[i], poolSizes[i] };
				m_poolSizes.push_back( poolSize );
			}
		}

		uint32_t size = 0;
		VkDescriptorSetLayout layouts[2];
		if( m_resourceLayout || m_constantLayout )
		{
			if( m_constantLayout )
			{
				layouts[size++] = m_constantLayout;
			}
			else
			{
				VkDescriptorSetLayoutCreateInfo emptyLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 0, nullptr };
				VkDescriptorSetLayout emptyLayout = VK_NULL_HANDLE;
				CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &emptyLayoutInfo, nullptr, &emptyLayout ) ) );

				layouts[size++] = emptyLayout;
			}
			if( m_resourceLayout )
			{
				layouts[size++] = m_resourceLayout;
			}
		}
		VkPipelineLayoutCreateInfo layoutInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			size,
			layouts,
			0,
			nullptr
		};

		CR_RETURN_HR( Vk2Al( vkCreatePipelineLayout( renderContext.m_device, &layoutInfo, nullptr, &m_pipelineLayout ) ) );
		m_owner = &renderContext;

		m_registerMap = Tr2RegisterMapAL( shaders, count );

		return S_OK;
	}


	void Tr2ShaderProgramAL::Destroy()
	{
		if( m_owner )
		{
			m_owner->DestroyLaterVulkan( m_resourceLayout, vkDestroyDescriptorSetLayout );
			m_owner->DestroyLaterVulkan( m_constantLayout, vkDestroyDescriptorSetLayout );
			m_owner->DestroyLaterVulkan( m_pipelineLayout, vkDestroyPipelineLayout );
			m_resourceLayout = VK_NULL_HANDLE;
			m_constantLayout = VK_NULL_HANDLE;
			m_pipelineLayout = VK_NULL_HANDLE;
			m_owner = nullptr;
		}
		m_shaders.clear();
		m_shaderInfo.clear();
		m_shaderInputs.clear();

		m_poolSizes.clear();
		m_registerInput.clear();
		m_registerMap = Tr2RegisterMapAL();
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return !m_shaderInfo.empty();
	}

	Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderProgramAL";
	}

	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}

	ALResult Tr2ShaderProgramAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}

#endif