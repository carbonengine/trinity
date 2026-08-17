////////////////////////////////////////////////////////////
//
//    Copyright: CCP 2026
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2ShaderAL.h"
#include "../Tr2RenderContextEnum.h"

// The Vulkan binding ABI, in one place.
//
// Three artifacts have to agree on these numbers, in three languages:
//
//   1. Tr2ShaderProgramALVulkan.cpp -- builds VkDescriptorSetLayoutBindings from them.
//   2. trinityal/tests/CMakeLists.txt -- computes dxc's -fvk-{b,t,s,u}-shift arguments
//      from the same arithmetic, so the emitted SPIR-V decorations land on the same
//      numbers.
//   3. trinityal/tests/Shaders.vulkan/*.{vsh,psh,csh} -- declare the register spaces
//      (b in space0, t/s/u in space1) that select the descriptor set.
//
// Nothing used to tie them together, and all three of this branch's ABI defects were
// the same failure with no assertion behind it: Phase 1's Tasks 4e and 4f moved every
// SRV/UAV binding as a side effect of a descriptor-*type* fix, and
// SampleTextureMipFromTexCoord.psh's DX9-style `register( ps, c0 )` silently escaped
// -fvk-b-shift entirely. trinityal/tests/ShaderBindingABI.cpp now asserts the contract
// by walking the shipped SPIR-V modules -- and it derives its expectations by calling
// straight into this header, so there is one source of truth for the arithmetic rather
// than a table of expected numbers that can drift away from the code.
//
// This header deliberately mentions no Vulkan type, so the test can include it without
// pulling in vulkan/vulkan.h.
namespace Tr2VulkanBindingABI
{
	// One 32-slot window per shader stage, four consecutive stage windows... six,
	// actually: the class block is 6 * REGISTER_SIZE so that all six
	// Tr2RenderContextEnum::ShaderType values get a window inside one class block.
	// These are 2019's values, restored by Phase 2a Task 1.
	static const uint32_t REGISTER_SIZE = 32;
	static const uint32_t CLASS_BLOCK = 6 * REGISTER_SIZE;

	// The binding-number block a register lives in: the four HLSL register classes
	// b/s/t/u. This is deliberately NOT the six-way descriptor-*kind* mapping
	// (RegisterTypeIndex in Tr2ShaderProgramALVulkan.cpp) -- that one answers "which
	// VkDescriptorType", which Vulkan splits six ways; this one answers "which binding
	// block", which HLSL splits four ways. Conflating the two is what Tasks 4e and 4f
	// did.
	//
	// b and u sharing block 0 is safe: CONSTANT_BUFFER registers go into
	// m_constantLayout (descriptor set 0) and every other register into
	// m_resourceLayout (set 1), so the two never share a set.
	enum RegisterClass
	{
		REGISTER_CLASS_CONSTANT_BUFFER = 0,   // b
		REGISTER_CLASS_SRV             = 1,   // t
		REGISTER_CLASS_SAMPLER         = 2,   // s
		REGISTER_CLASS_UAV             = 0    // u
	};

	// Descriptor sets. Constants live alone in set 0 so that the b/u block collision
	// above cannot bite; everything else shares set 1. The shader side spells this as
	// the register space (`space1` on every t/s/u declaration).
	enum DescriptorSet
	{
		DESCRIPTOR_SET_CONSTANTS = 0,
		DESCRIPTOR_SET_RESOURCES = 1
	};

	inline uint32_t RegisterClassOffset( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		if( registerType & Tr2ShaderRegisterAL::UAV_REGISTER_FLAG )
		{
			return REGISTER_CLASS_UAV;
		}
		if( registerType & Tr2ShaderRegisterAL::SRV_REGISTER_FLAG )
		{
			return REGISTER_CLASS_SRV;
		}
		return registerType == Tr2ShaderRegisterAL::SAMPLER
			? REGISTER_CLASS_SAMPLER : REGISTER_CLASS_CONSTANT_BUFFER;
	}

	inline uint32_t DescriptorSetIndex( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		return registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER
			? DESCRIPTOR_SET_CONSTANTS : DESCRIPTOR_SET_RESOURCES;
	}

	// binding = registerIndex + classOffset * 6 * 32 + shaderType * 32
	inline uint32_t BindingNumber(
		Tr2ShaderRegisterAL::RegisterType registerType,
		uint32_t registerIndex,
		Tr2RenderContextEnum::ShaderType shaderType )
	{
		return registerIndex
			+ RegisterClassOffset( registerType ) * CLASS_BLOCK
			+ uint32_t( shaderType ) * REGISTER_SIZE;
	}
}

#endif
