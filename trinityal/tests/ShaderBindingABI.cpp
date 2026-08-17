// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

// The whole file is Vulkan-only. dx11, dx12 and metal compile the same `_SOURCES`
// list, and neither the SPIR-V modules nor the binding formula this asserts exist on
// those backends.
#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "vulkan/Tr2ShaderBindingABIVulkan.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>
#include <vector>

// Asserts the Vulkan binding ABI, statically, with no GPU and no VkDevice.
//
// Three artifacts have to agree on every descriptor set and binding number, in three
// languages, and until this file existed nothing produced a diagnostic when they
// drifted:
//
//   1. Tr2ShaderProgramALVulkan.cpp's binding formula (now
//      Tr2ShaderBindingABIVulkan.h), which builds the VkDescriptorSetLayoutBindings.
//   2. trinityal/tests/CMakeLists.txt's -fvk-{b,t,s,u}-shift arithmetic, which tells
//      dxc what to bake into the SPIR-V.
//   3. the Shaders.vulkan/*.{vsh,psh,csh} sources' own register declarations,
//      including the `space` that becomes the descriptor set.
//
// All three of this branch's ABI defects were that same failure: Phase 1's Tasks 4e
// and 4f moved every SRV/UAV binding as a side effect of a descriptor-*type* fix, and
// SampleTextureMipFromTexCoord.psh's DX9-style `register( ps, c0 )` folded its
// constant buffer into an implicit `$Globals` that dxc assigns binding 0 *without*
// applying -fvk-b-shift -- so the module read set 0 / binding 0 while the layout
// declared 32. Phase 2a's spot-check of five decorations missed it structurally: of the
// two constant buffers it sampled, one was `b1` compute and the other `b0` *vertex*,
// and at the vertex stage the shift is 0 * 32 = 0, so that case cannot distinguish
// "shift applied" from "shift ignored". The single discriminating case -- a `b0` at a
// non-vertex stage -- was the one never sampled.
//
// Every Shaders.vulkan/*.h generated header is a complete SPIR-V module already
// compiled into this binary, so the check is pure bytecode inspection: walk each
// module's OpDecorate stream for DescriptorSet and Binding, and compare what dxc
// actually emitted against what the AL's own formula computes for that shader's
// declared registers.
//
// The expected values are *derived, not tabulated*: EXPECTED_* below lists only what
// the HLSL source declares -- register class, index, stage -- and every number under
// test comes out of Tr2VulkanBindingABI::BindingNumber / DescriptorSetIndex, the same
// functions Tr2ShaderProgramALVulkan.cpp calls. A hardcoded table of sixteen expected
// binding numbers would keep passing while the formula drifted, which is exactly the
// failure mode this file exists to close. (RegisterClassOffset used to sit in an
// anonymous namespace in Tr2ShaderProgramALVulkan.cpp and was unreachable from here;
// it was lifted into Tr2ShaderBindingABIVulkan.h rather than copied, so there is still
// one source of truth.)

using namespace Tr2RenderContextEnum;

namespace
{

// ---------------------------------------------------------------------------
// The sixteen modules, as bytes.
// ---------------------------------------------------------------------------

const uint8_t g_addConstantToBufferCs[] = {
#include INCLUDE_SHADER_CODE( AddConstantToBuffer.cs )
};
const uint8_t g_addVectorsCs[] = {
#include INCLUDE_SHADER_CODE( AddVectors.cs )
};
const uint8_t g_constantColorPs[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
};
const uint8_t g_groupSharedCs[] = {
#include INCLUDE_SHADER_CODE( GroupShared.cs )
};
const uint8_t g_instancedRenderingVs[] = {
#include INCLUDE_SHADER_CODE( InstancedRendering.vs )
};
const uint8_t g_loadMsaaTexturePs[] = {
#include INCLUDE_SHADER_CODE( LoadMsaaTexture.ps )
};
const uint8_t g_outputTexCoordPs[] = {
#include INCLUDE_SHADER_CODE( OutputTexCoord.ps )
};
const uint8_t g_outputVectorCs[] = {
#include INCLUDE_SHADER_CODE( OutputVector.cs )
};
const uint8_t g_positionOnlyVs[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
};
const uint8_t g_positionOnlyWithPerObjectDataVs[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
};
const uint8_t g_sampleTextureCs[] = {
#include INCLUDE_SHADER_CODE( SampleTexture.cs )
};
const uint8_t g_sampleTextureFromTexCoordPs[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
};
const uint8_t g_sampleTextureMipFromTexCoordPs[] = {
#include INCLUDE_SHADER_CODE( SampleTextureMipFromTexCoord.ps )
};
const uint8_t g_sampleVolumeTexturePs[] = {
#include INCLUDE_SHADER_CODE( SampleVolumeTexture.ps )
};
const uint8_t g_texCoordAndPositionVs[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
};
const uint8_t g_writeToUavPs[] = {
#include INCLUDE_SHADER_CODE( WriteToUav.ps )
};

// ---------------------------------------------------------------------------
// What each source declares. Inputs to the formula, never its outputs.
// ---------------------------------------------------------------------------

struct DeclaredRegister
{
	Tr2ShaderRegisterAL::RegisterType registerType;
	uint32_t registerIndex;
};

struct ShaderModule
{
	const char* name;
	const uint8_t* bytecode;
	size_t byteCount;
	ShaderType stage;
	const DeclaredRegister* registers;
	size_t registerCount;
};

// One entry per HLSL declaration in the corresponding Shaders.vulkan source. The
// RegisterType subtype (SRV_BUFFER vs SRV_TEXTURE2D, ...) is spelled as the source
// declares it and as the tests' own Tr2ShaderSignatureAL calls spell it, so that a
// future descriptor-kind change that wrongly leaks into the binding blocks is caught.
const DeclaredRegister EXPECTED_ADD_CONSTANT_TO_BUFFER[] = {
	{ Tr2ShaderRegisterAL::CONSTANT_BUFFER, 1 },   // cbuffer Constants : register(b1)
	{ Tr2ShaderRegisterAL::SRV_BUFFER, 0 },        // Buffer<float4>   arg2   : t0, space1
	{ Tr2ShaderRegisterAL::UAV_BUFFER, 0 },        // RWBuffer<float4> output : u0, space1
};
const DeclaredRegister EXPECTED_ADD_VECTORS[] = {
	{ Tr2ShaderRegisterAL::SRV_BUFFER, 0 },
	{ Tr2ShaderRegisterAL::SRV_BUFFER, 1 },
	{ Tr2ShaderRegisterAL::UAV_BUFFER, 0 },
};
const DeclaredRegister EXPECTED_GROUP_SHARED[] = {
	{ Tr2ShaderRegisterAL::UAV_BUFFER, 0 },
};
const DeclaredRegister EXPECTED_LOAD_MSAA_TEXTURE[] = {
	{ Tr2ShaderRegisterAL::SRV_TEXTURE2DMS, 0 },
};
const DeclaredRegister EXPECTED_OUTPUT_VECTOR[] = {
	{ Tr2ShaderRegisterAL::UAV_BUFFER, 0 },
};
const DeclaredRegister EXPECTED_POSITION_ONLY_WITH_PER_OBJECT_DATA[] = {
	{ Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 },
};
const DeclaredRegister EXPECTED_SAMPLE_TEXTURE[] = {
	{ Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 },
	{ Tr2ShaderRegisterAL::SAMPLER, 0 },
	{ Tr2ShaderRegisterAL::UAV_BUFFER, 0 },
};
const DeclaredRegister EXPECTED_SAMPLE_TEXTURE_FROM_TEXCOORD[] = {
	{ Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 },
	{ Tr2ShaderRegisterAL::SAMPLER, 0 },
};
// The Fix 1 shader. `b0` at the *pixel* stage is the one case in the whole set that
// distinguishes "-fvk-b-shift applied" from "-fvk-b-shift ignored", because every
// other set-0 register here is either `b1` (which is non-zero either way) or vertex
// stage (where the shift is 0 * 32 = 0). Before Fix 1 this module decorated
// set 0 / binding 0 and this entry expected 32.
const DeclaredRegister EXPECTED_SAMPLE_TEXTURE_MIP_FROM_TEXCOORD[] = {
	{ Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 },
	{ Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 },
	{ Tr2ShaderRegisterAL::SAMPLER, 0 },
};
const DeclaredRegister EXPECTED_SAMPLE_VOLUME_TEXTURE[] = {
	{ Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 },
	{ Tr2ShaderRegisterAL::SRV_TEXTURE3D, 0 },
	{ Tr2ShaderRegisterAL::SAMPLER, 0 },
};
const DeclaredRegister EXPECTED_WRITE_TO_UAV[] = {
	{ Tr2ShaderRegisterAL::UAV_TEXTURE2D, 1 },
};

#define SHADER_MODULE( bytes, stage, expected ) \
	{ #bytes, bytes, sizeof( bytes ), stage, expected, sizeof( expected ) / sizeof( expected[0] ) }
#define SHADER_MODULE_NO_REGISTERS( bytes, stage ) \
	{ #bytes, bytes, sizeof( bytes ), stage, nullptr, 0 }

const ShaderModule ALL_MODULES[] = {
	SHADER_MODULE( g_addConstantToBufferCs, COMPUTE_SHADER, EXPECTED_ADD_CONSTANT_TO_BUFFER ),
	SHADER_MODULE( g_addVectorsCs, COMPUTE_SHADER, EXPECTED_ADD_VECTORS ),
	SHADER_MODULE_NO_REGISTERS( g_constantColorPs, PIXEL_SHADER ),
	SHADER_MODULE( g_groupSharedCs, COMPUTE_SHADER, EXPECTED_GROUP_SHARED ),
	SHADER_MODULE_NO_REGISTERS( g_instancedRenderingVs, VERTEX_SHADER ),
	SHADER_MODULE( g_loadMsaaTexturePs, PIXEL_SHADER, EXPECTED_LOAD_MSAA_TEXTURE ),
	SHADER_MODULE_NO_REGISTERS( g_outputTexCoordPs, PIXEL_SHADER ),
	SHADER_MODULE( g_outputVectorCs, COMPUTE_SHADER, EXPECTED_OUTPUT_VECTOR ),
	SHADER_MODULE_NO_REGISTERS( g_positionOnlyVs, VERTEX_SHADER ),
	SHADER_MODULE( g_positionOnlyWithPerObjectDataVs, VERTEX_SHADER, EXPECTED_POSITION_ONLY_WITH_PER_OBJECT_DATA ),
	SHADER_MODULE( g_sampleTextureCs, COMPUTE_SHADER, EXPECTED_SAMPLE_TEXTURE ),
	SHADER_MODULE( g_sampleTextureFromTexCoordPs, PIXEL_SHADER, EXPECTED_SAMPLE_TEXTURE_FROM_TEXCOORD ),
	SHADER_MODULE( g_sampleTextureMipFromTexCoordPs, PIXEL_SHADER, EXPECTED_SAMPLE_TEXTURE_MIP_FROM_TEXCOORD ),
	SHADER_MODULE( g_sampleVolumeTexturePs, PIXEL_SHADER, EXPECTED_SAMPLE_VOLUME_TEXTURE ),
	SHADER_MODULE_NO_REGISTERS( g_texCoordAndPositionVs, VERTEX_SHADER ),
	SHADER_MODULE( g_writeToUavPs, PIXEL_SHADER, EXPECTED_WRITE_TO_UAV ),
};

const size_t MODULE_COUNT = sizeof( ALL_MODULES ) / sizeof( ALL_MODULES[0] );

// ---------------------------------------------------------------------------
// A minimal SPIR-V decoration reader.
// ---------------------------------------------------------------------------
//
// SPIR-V is a stream of 32-bit words: a five-word header, then instructions whose
// first word packs the opcode in the low 16 bits and the instruction's total word
// count in the high 16. Walking by that word count -- rather than assuming any
// particular instruction order or that decorations are contiguous -- is the whole
// parser.

const uint32_t SPIRV_MAGIC = 0x07230203u;
const size_t SPIRV_HEADER_WORDS = 5;
const uint32_t OP_DECORATE = 71;
const uint32_t DECORATION_BINDING = 33;
const uint32_t DECORATION_DESCRIPTOR_SET = 34;

struct SpirvBinding
{
	uint32_t set;
	uint32_t binding;
};

bool operator<( const SpirvBinding& a, const SpirvBinding& b )
{
	return a.set != b.set ? a.set < b.set : a.binding < b.binding;
}

bool operator==( const SpirvBinding& a, const SpirvBinding& b )
{
	return a.set == b.set && a.binding == b.binding;
}

// gtest finds this by ADL and uses it instead of dumping the raw object bytes, so a
// failure reads "set 0/binding 32" rather than "8-byte object <00-00 ...>".
std::ostream& operator<<( std::ostream& out, const SpirvBinding& b )
{
	return out << "set " << b.set << "/binding " << b.binding;
}

std::string Describe( const std::vector<SpirvBinding>& bindings )
{
	std::ostringstream out;
	out << "{ ";
	for( size_t i = 0; i < bindings.size(); ++i )
	{
		out << ( i ? ", " : "" ) << "set " << bindings[i].set << "/binding " << bindings[i].binding;
	}
	out << ( bindings.empty() ? "(none) }" : " }" );
	return out.str();
}

// Reads every id that carries both a DescriptorSet and a Binding decoration.
// Returns an assertion result so that a malformed module reports as a parser failure
// rather than as an empty -- and therefore possibly passing -- binding list.
::testing::AssertionResult ReadDecoratedBindings( const uint8_t* bytes, size_t byteCount, std::vector<SpirvBinding>& out )
{
	out.clear();

	if( byteCount < SPIRV_HEADER_WORDS * sizeof( uint32_t ) || byteCount % sizeof( uint32_t ) != 0 )
	{
		return ::testing::AssertionFailure() << "not a SPIR-V module: " << byteCount << " bytes";
	}

	// memcpy rather than a reinterpret_cast: a uint8_t[] has no alignment guarantee.
	std::vector<uint32_t> words( byteCount / sizeof( uint32_t ) );
	memcpy( words.data(), bytes, byteCount );

	if( words[0] != SPIRV_MAGIC )
	{
		std::ostringstream magic;
		magic << std::hex << words[0];
		return ::testing::AssertionFailure()
			<< "bad SPIR-V magic 0x" << magic.str()
			<< " (expected 0x07230203; a byte-swapped 0x03022307 would mean the module "
			<< "was produced for the opposite endianness, which this walker does not handle)";
	}

	// id -> (set, binding), each present only once decorated.
	std::map<uint32_t, std::pair<bool, uint32_t> > sets, bindings;

	for( size_t i = SPIRV_HEADER_WORDS; i < words.size(); )
	{
		const uint32_t opcode = words[i] & 0xFFFFu;
		const uint32_t wordCount = words[i] >> 16;

		if( wordCount == 0 || i + wordCount > words.size() )
		{
			return ::testing::AssertionFailure()
				<< "malformed SPIR-V at word " << i << ": opcode " << opcode
				<< ", wordCount " << wordCount << ", " << words.size() << " words total";
		}

		if( opcode == OP_DECORATE && wordCount >= 4 )
		{
			const uint32_t target = words[i + 1];
			const uint32_t decoration = words[i + 2];
			const uint32_t value = words[i + 3];

			if( decoration == DECORATION_DESCRIPTOR_SET )
			{
				sets[target] = std::make_pair( true, value );
			}
			else if( decoration == DECORATION_BINDING )
			{
				bindings[target] = std::make_pair( true, value );
			}
		}

		i += wordCount;
	}

	for( auto it = bindings.begin(); it != bindings.end(); ++it )
	{
		auto setIt = sets.find( it->first );
		if( setIt == sets.end() )
		{
			return ::testing::AssertionFailure()
				<< "id " << it->first << " has a Binding decoration but no DescriptorSet decoration";
		}
		SpirvBinding b = { setIt->second.second, it->second.second };
		out.push_back( b );
	}
	for( auto it = sets.begin(); it != sets.end(); ++it )
	{
		if( bindings.find( it->first ) == bindings.end() )
		{
			return ::testing::AssertionFailure()
				<< "id " << it->first << " has a DescriptorSet decoration but no Binding decoration";
		}
	}

	std::sort( out.begin(), out.end() );
	return ::testing::AssertionSuccess();
}

}

// The main assertion. For each of the sixteen shipped SPIR-V modules, the multiset of
// (DescriptorSet, Binding) pairs dxc emitted must equal the multiset the AL's own
// formula computes from that shader's declared registers.
TEST( ShaderBindingABI, EverySpirvModuleDecoratesWhatTheALBindingFormulaComputes )
{
	ASSERT_EQ( size_t( 16 ), MODULE_COUNT )
		<< "Shaders.vulkan has sixteen sources; a shader added or removed without "
		<< "updating ALL_MODULES leaves the new one unchecked";

	for( size_t m = 0; m < MODULE_COUNT; ++m )
	{
		// The failing module is named in every message below rather than via
		// SCOPED_TRACE: SCOPED_TRACE here access-violates (0xC0000005) on destruction in
		// this build of the suite -- no other test in TrinityALTest uses it, so it has
		// never been exercised on any backend. Not investigated: out of scope for this
		// fix wave, and reported rather than worked around silently.
		const ShaderModule& module = ALL_MODULES[m];

		std::vector<SpirvBinding> actual;
		ASSERT_TRUE( ReadDecoratedBindings( module.bytecode, module.byteCount, actual ) )
			<< "while reading " << module.name;

		std::vector<SpirvBinding> expected;
		for( size_t r = 0; r < module.registerCount; ++r )
		{
			const DeclaredRegister& declared = module.registers[r];
			SpirvBinding b = {
				Tr2VulkanBindingABI::DescriptorSetIndex( declared.registerType ),
				Tr2VulkanBindingABI::BindingNumber( declared.registerType, declared.registerIndex, module.stage )
			};
			expected.push_back( b );
		}
		std::sort( expected.begin(), expected.end() );

		EXPECT_EQ( expected, actual )
			<< module.name << ": the AL formula gives " << Describe( expected )
			<< ", the SPIR-V module decorates " << Describe( actual );
	}
}

// Guards the walker itself, so that the assertion above cannot pass vacuously because
// the reader silently found nothing. The stream below places an OpDecorate after a
// longer instruction and interleaves an unrelated decoration, so only a reader that
// advances by wordCount and filters on the decoration operand lands on it.
TEST( ShaderBindingABI, TheDecorationWalkerFindsDecorationsItIsNotHandedInOrder )
{
	const uint32_t words[] = {
		SPIRV_MAGIC, 0x00010000u, 0u, 1u, 0u,             // 5-word header
		( 5u << 16 ) | 11u,   9u, 1u, 2u, 3u,             // some 5-word instruction, not OpDecorate
		( 4u << 16 ) | OP_DECORATE, 7u, 0u, 0u,           // OpDecorate %7 RelaxedPrecision-ish: ignored
		( 4u << 16 ) | OP_DECORATE, 7u, DECORATION_BINDING, 224u,
		( 3u << 16 ) | 5u,    7u, 0u,                     // a 3-word instruction in between
		( 4u << 16 ) | OP_DECORATE, 7u, DECORATION_DESCRIPTOR_SET, 1u,
	};

	std::vector<SpirvBinding> actual;
	ASSERT_TRUE( ReadDecoratedBindings( reinterpret_cast<const uint8_t*>( words ), sizeof( words ), actual ) );
	ASSERT_EQ( size_t( 1 ), actual.size() );
	EXPECT_EQ( 1u, actual[0].set );
	EXPECT_EQ( 224u, actual[0].binding );
}

#endif
