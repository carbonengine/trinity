// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ShaderProgramALDx12.h"
#include "Tr2ShaderALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "ALLog.h"
#include "Utilities.h"

using namespace Tr2RenderContextEnum;
extern uint32_t g_forceAnisotropy;

namespace
{

D3D12_SHADER_VISIBILITY ShaderVisibility( Tr2RenderContextEnum::ShaderType type )
{
	switch( type )
	{
	case Tr2RenderContextEnum::VERTEX_SHADER:
		return D3D12_SHADER_VISIBILITY_VERTEX;
	case Tr2RenderContextEnum::PIXEL_SHADER:
		return D3D12_SHADER_VISIBILITY_PIXEL;
	case Tr2RenderContextEnum::GEOMETRY_SHADER:
		return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case Tr2RenderContextEnum::HULL_SHADER:
		return D3D12_SHADER_VISIBILITY_HULL;
	case Tr2RenderContextEnum::DOMAIN_SHADER:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	default:
		return D3D12_SHADER_VISIBILITY_ALL;
	}
}

D3D12_DESCRIPTOR_RANGE CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t registerIndex, uint32_t registerSpace, uint32_t arrayCount, uint32_t offset = 0 )
{
	D3D12_DESCRIPTOR_RANGE range;
	range.RangeType = rangeType;
	range.NumDescriptors = arrayCount ? arrayCount : UINT_MAX;
	range.BaseShaderRegister = registerIndex;
	range.RegisterSpace = registerSpace;
	range.OffsetInDescriptorsFromTableStart = offset;
	return range;
}

D3D12_STATIC_SAMPLER_DESC CreateStaticSampler( const Tr2StaticSamplerAL& sampl, Tr2RenderContextEnum::ShaderType stage )
{
	auto& description = sampl.sampler;
	D3D12_STATIC_SAMPLER_DESC sampler;
	if( g_forceAnisotropy != 1 && ( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC ) )
	{
		sampler.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER( description.m_isComparisonFilter ? 1 : 0 );
	}
	else
	{
		sampler.Filter = D3D12_ENCODE_BASIC_FILTER(
			description.m_minFilter == TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_magFilter == TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_mipFilter == TF_POINT || description.m_mipFilter == TF_NONE ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_isComparisonFilter ? 1 : 0 );
	}
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressU );
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressV );
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressW );
	sampler.MipLODBias = description.m_mipLODBias;
	sampler.MaxAnisotropy = g_forceAnisotropy ? g_forceAnisotropy : description.m_maxAnisotropy;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC( description.m_comparisonFunc );
	if( description.m_borderColor[0] > 0 )
	{
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	}
	else if( description.m_borderColor[3] > 0 )
	{
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	}
	else
	{
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	}
	sampler.MinLOD = description.m_minLOD;
	sampler.MaxLOD = description.m_mipFilter == TF_NONE ? description.m_minLOD : description.m_maxLOD;

	sampler.ShaderRegister = sampl.registerIndex;
	sampler.RegisterSpace = sampl.registerSpace;
	sampler.ShaderVisibility = ShaderVisibility( stage );
	return sampler;
}

ALResult ValidateShaders( Tr2ShaderAL* shaders, size_t count )
{
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
	return S_OK;
}

}

namespace TrinityALImpl
{

Tr2RootSignatureAL::Tr2RootSignatureAL() :
	m_srvUavParameterCount( 0 ),
	m_srvUavParameterOffset( 0 ),
	m_samplerParameterCount( 0 ),
	m_samplerParameterOffset( 0 ),
	m_isCompute( false )
{
}

void Tr2RootSignatureAL::Destroy( Tr2PrimaryRenderContextAL& owner )
{
	if( m_rootSignature )
	{
		RELEASE_LATER( &owner, m_rootSignature );
		m_rootSignature = nullptr;
	}

	m_cbRegisters.clear();
	m_srvRegisters.clear();
	m_uavRegisters.clear();
	m_samplerRegisters.clear();

	m_registerMap = Tr2RegisterMapAL();

	m_srvUavParameterCount = 0;
	m_srvUavParameterOffset = 0;
	m_samplerParameterCount = 0;
	m_samplerParameterOffset = 0;
}

Tr2ShaderProgramAL::Tr2ShaderProgramAL() :
	m_CS( { nullptr, 0 } ),
	m_VS( { nullptr, 0 } ),
	m_PS( { nullptr, 0 } ),
	m_DS( { nullptr, 0 } ),
	m_HS( { nullptr, 0 } ),
	m_GS( { nullptr, 0 } ),
	m_owner( nullptr )
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

	FORWARD_HR( ValidateShaders( shaders, count ) );

	m_shaders.reserve( count );

	D3D12_ROOT_SIGNATURE_DESC signatureDesc;
	memset( &signatureDesc, 0, sizeof( signatureDesc ) );

	for( size_t i = 0; i < count; ++i )
	{
		switch( shaders[i].GetType() )
		{
		case VERTEX_SHADER:
			m_VS = MakeShaderBytecode( shaders[i] );
			if( !shaders[i].m_shader->m_signature.pipelineInputs.empty() )
			{
				signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			}
			m_iaInputs = shaders[i].m_shader->m_signature.pipelineInputs;
			break;
		case PIXEL_SHADER:
			m_PS = MakeShaderBytecode( shaders[i] );
			break;
		case DOMAIN_SHADER:
			m_DS = MakeShaderBytecode( shaders[i] );
			break;
		case HULL_SHADER:
			m_HS = MakeShaderBytecode( shaders[i] );
			break;
		case GEOMETRY_SHADER:
			m_GS = MakeShaderBytecode( shaders[i] );
			break;
		case COMPUTE_SHADER:
			m_CS = MakeShaderBytecode( shaders[i] );
			break;
		default:
			return E_INVALIDARG;
		}
		m_shaders.push_back( shaders[i] );
	}

	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>> ranges;

	for( size_t i = 0; i < count; ++i )
	{
		AddCbvParameters( shaders[i].GetType(), shaders[i].m_shader->m_signature, parameters, true );
	}

	for( size_t i = 0; i < count; ++i )
	{
		AddCbvParameters( shaders[i].GetType(), shaders[i].m_shader->m_signature, parameters, false );
	}

	m_rootSignature.m_srvUavParameterOffset = uint32_t( parameters.size() );
	for( size_t i = 0; i < count; ++i )
	{
		AddSrvUavParameters( shaders[i].GetType(), shaders[i].m_shader->m_signature, parameters, ranges );
	}

	m_rootSignature.m_samplerParameterOffset = uint32_t( parameters.size() );
	for( size_t i = 0; i < count; ++i )
	{
		AddSamplerParameters( shaders[i].GetType(), shaders[i].m_shader->m_signature, parameters, ranges );
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
	for( size_t i = 0; i < count; ++i )
	{
		for( auto& sampl : shaders[i].m_shader->m_signature.samplers )
		{
			samplers.push_back( CreateStaticSampler( sampl, shaders[i].GetType() ) );
		}
	}

	signatureDesc.NumParameters = UINT( parameters.size() );
	if( signatureDesc.NumParameters )
	{
		signatureDesc.pParameters = parameters.data();
	}

	signatureDesc.NumStaticSamplers = UINT( samplers.size() );
	signatureDesc.pStaticSamplers = samplers.data();

	CComPtr<ID3DBlob> rootSignatureBlob;
	CComPtr<ID3DBlob> errorBlob;
	auto hr = D3D12SerializeRootSignature( &signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob );
	if( FAILED( hr ) )
	{
		if( errorBlob )
		{
			CCP_AL_LOGERR( "Failed to serialize a root signature: %s", static_cast<const char*>( errorBlob->GetBufferPointer() ) );
		}
		else
		{
			CCP_AL_LOGERR( "Failed to serialize a root signature - unknown error" );
		}
		Destroy();
		return hr;
	}

	hr = renderContext.m_device->CreateRootSignature(
		0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS( &m_rootSignature.m_rootSignature ) );
	if( FAILED( hr ) )
	{
		Destroy();
		return hr;
	}

	m_rootSignature.m_isCompute = IsComputeProgramDx12();

	m_rootSignature.m_registerMap = Tr2RegisterMapAL( shaders, count );

	m_owner = &renderContext;

	if( m_name.empty() )
	{
		for( size_t i = 0; i < count; ++i )
		{
			auto& shaderName = shaders[i].m_shader->m_name;
			if( !shaderName.empty() )
			{
				if( !m_name.empty() )
					m_name += " + ";
				m_name += shaderName;
			}
		}
	}

	return S_OK;
}

void Tr2ShaderProgramAL::Destroy()
{
	if( m_drawInstanced )
	{
		RELEASE_LATER( m_owner, m_drawInstanced );
		m_drawInstanced = nullptr;
	}
	if( m_drawIndexedInstanced )
	{
		RELEASE_LATER( m_owner, m_drawIndexedInstanced );
		m_drawIndexedInstanced = nullptr;
	}
	auto owner = m_owner;
	if( m_owner )
	{
		m_rootSignature.Destroy( *m_owner );
	}

	m_shaders.clear();
	D3D12_SHADER_BYTECODE none = { nullptr, 0 };
	m_VS = none;
	m_PS = none;
	m_DS = none;
	m_HS = none;
	m_GS = none;
	m_CS = none;

	m_iaInputs.clear();


	m_indirectBufferLayout.constantBufferCount = {};

	m_owner = nullptr;

	// We want to notify the owner last as it may trigger destruction of this program
	if( owner )
	{
		owner->OnShaderProgramDestroyedDx12( this );
	}
}

ALResult Tr2ShaderProgramAL::CreateCommandSignatures( Tr2IndirectBufferLayoutAL& bufferLayout, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( m_drawInstanced )
	{
		bufferLayout = m_indirectBufferLayout;
		return S_OK;
	}

	m_indirectBufferLayout = {};

	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgs;

	for( auto& reg : m_rootSignature.m_cbRegisters )
	{
		if( reg.dynamic )
		{
			D3D12_INDIRECT_ARGUMENT_DESC arg;
			arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			arg.ConstantBufferView.RootParameterIndex = reg.parameter;
			indirectArgs.push_back( arg );

			m_indirectBufferLayout.constantBuffers[m_indirectBufferLayout.constantBufferCount++] = { Tr2RenderContextEnum::ShaderType( reg.stage ), reg.index };
		}
	}

	if( indirectArgs.empty() )
	{
		m_drawInstanced = renderContext.m_drawInstancedIndirect;
		m_indirectBufferLayout.drawStride = sizeof( D3D12_DRAW_ARGUMENTS );

		m_drawIndexedInstanced = renderContext.m_drawIndexedInstancedIndirect;
		m_indirectBufferLayout.drawIndexedStride = sizeof( D3D12_DRAW_INDEXED_ARGUMENTS );

		m_indirectBufferLayout.drawArgOffset = 0;
	}
	else
	{
		indirectArgs.push_back( {} );
		indirectArgs.back().Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_COMMAND_SIGNATURE_DESC signature;
		signature.ByteStride = UINT( sizeof( D3D12_DRAW_ARGUMENTS ) + 8 * ( indirectArgs.size() - 1 ) );
		signature.NodeMask = 0;
		signature.NumArgumentDescs = UINT( indirectArgs.size() );
		signature.pArgumentDescs = indirectArgs.data();

		FORWARD_HR( renderContext.m_device->CreateCommandSignature( &signature, indirectArgs.size() > 1 ? m_rootSignature.m_rootSignature : nullptr, IID_PPV_ARGS( &m_drawInstanced ) ) );
		m_indirectBufferLayout.drawStride = signature.ByteStride;
		signature.ByteStride = UINT( sizeof( D3D12_DRAW_INDEXED_ARGUMENTS ) + 8 * ( indirectArgs.size() - 1 ) );
		indirectArgs.back().Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		FORWARD_HR( renderContext.m_device->CreateCommandSignature( &signature, indirectArgs.size() > 1 ? m_rootSignature.m_rootSignature : nullptr, IID_PPV_ARGS( &m_drawIndexedInstanced ) ) );
		m_indirectBufferLayout.drawIndexedStride = signature.ByteStride;
		m_indirectBufferLayout.drawArgOffset = uint32_t( 8 * ( indirectArgs.size() - 1 ) );
	}


	bufferLayout = m_indirectBufferLayout;
	return S_OK;
}

bool Tr2ShaderProgramAL::IsValid() const
{
	return m_rootSignature.m_rootSignature != nullptr;
}

Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

D3D12_SHADER_BYTECODE Tr2ShaderProgramAL::MakeShaderBytecode( const ::Tr2ShaderAL& shader )
{
	D3D12_SHADER_BYTECODE result;
	result.BytecodeLength = shader.m_shader->m_bytecode.size();
	result.pShaderBytecode = shader.m_shader->m_bytecode.get();
	return result;
}

void Tr2ShaderProgramAL::AddSrvUavParameters(
	ShaderType shaderType,
	const Tr2ShaderSignatureAL& signature,
	std::vector<D3D12_ROOT_PARAMETER>& parameters,
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>>& ranges )
{
	for( auto& reg : signature.registers )
	{
		auto isSrv = reg.IsSrv();
		if( isSrv || reg.IsUav() )
		{
			Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType };
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
			if( isSrv )
			{
				m_rootSignature.m_srvRegisters.push_back( cbr );
				rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			}
			else
			{
				m_rootSignature.m_uavRegisters.push_back( cbr );
				rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			}
			ranges.push_back( std::make_unique<D3D12_DESCRIPTOR_RANGE>( CreateRange( rangeType, reg.registerIndex, reg.registerSpace, reg.arrayCount ) ) );

			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = ranges.back().get();
			parameter.ShaderVisibility = ShaderVisibility( shaderType );
			parameters.push_back( parameter );
			m_rootSignature.m_srvUavParameterCount++;
		}
	}
}

void Tr2ShaderProgramAL::AddCbvParameters(
	ShaderType shaderType,
	const Tr2ShaderSignatureAL& signature,
	std::vector<D3D12_ROOT_PARAMETER>& parameters,
	bool dynamicBuffers )
{
	for( auto& reg : signature.registers )
	{
		if( reg.registerType != Tr2ShaderRegisterAL::CONSTANT_BUFFER || reg.dynamic != dynamicBuffers )
		{
			continue;
		}

		D3D12_ROOT_PARAMETER parameter;
		parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameter.Descriptor.RegisterSpace = reg.registerSpace;
		parameter.Descriptor.ShaderRegister = reg.registerIndex;
		parameter.ShaderVisibility = ShaderVisibility( shaderType );

		Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType, reg.dynamic };
		m_rootSignature.m_cbRegisters.push_back( cbr );

		parameters.push_back( parameter );
	}
}

void Tr2ShaderProgramAL::AddSamplerParameters(
	ShaderType shaderType,
	const Tr2ShaderSignatureAL& signature,
	std::vector<D3D12_ROOT_PARAMETER>& parameters,
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>>& ranges )
{
	for( auto& reg : signature.registers )
	{
		if( reg.registerType == Tr2ShaderRegisterAL::SAMPLER )
		{
			Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType };
			m_rootSignature.m_samplerRegisters.push_back( cbr );
			ranges.push_back( std::make_unique<D3D12_DESCRIPTOR_RANGE>( CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, reg.registerIndex, reg.registerSpace, reg.arrayCount ) ) );

			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = ranges.back().get();
			parameter.ShaderVisibility = ShaderVisibility( shaderType );
			parameters.push_back( parameter );
			m_rootSignature.m_samplerParameterCount++;
		}
	}
}

const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
{
	return m_rootSignature.m_registerMap;
}

bool Tr2ShaderProgramAL::IsComputeProgramDx12() const
{
	return m_CS.pShaderBytecode != nullptr;
}

void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ShaderProgramAL";
	description["name"] = m_name;
}

ALResult Tr2ShaderProgramAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_rootSignature.m_rootSignature, name );
	return S_OK;
}
}
#endif