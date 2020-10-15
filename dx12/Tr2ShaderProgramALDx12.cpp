////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ShaderProgramALDx12.h"
#include "Tr2ShaderALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

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
}

namespace TrinityALImpl
{

	Tr2ShaderProgramAL::Tr2ShaderProgramAL()
		:m_srvUavTableSize( 0 ),
		m_samplerTableSize( 0 ),
		m_srvUavParameter( 0xffffffff ),
		m_samplerParameter( 0xffffffff ),
		m_owner( nullptr ),
		m_VS( D3D12_SHADER_BYTECODE{ nullptr, 0 } ),
		m_PS( D3D12_SHADER_BYTECODE{ nullptr, 0 } ),
		m_DS( D3D12_SHADER_BYTECODE{ nullptr, 0 } ),
		m_HS( D3D12_SHADER_BYTECODE{ nullptr, 0 } ),
		m_GS( D3D12_SHADER_BYTECODE{ nullptr, 0 } ),
		m_CS( D3D12_SHADER_BYTECODE{ nullptr, 0 } )
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

		m_shaders.reserve( count );

		D3D12_ROOT_SIGNATURE_DESC signatureDesc;
		memset( &signatureDesc, 0, sizeof( signatureDesc ) );

		std::vector<D3D12_ROOT_PARAMETER> parameters;
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		std::vector<D3D12_DESCRIPTOR_RANGE> samplerRanges;

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
			}
			ParseRegisterSignature( shaders[i].GetType(), shaders[i].m_shader->m_signature, parameters, ranges, samplerRanges );
			m_shaders.push_back( shaders[i] );
		}

		if( !ranges.empty() )
		{
			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = UINT( ranges.size() );
			parameter.DescriptorTable.pDescriptorRanges = &ranges[0];
			parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			m_srvUavParameter = uint32_t( parameters.size() );

			parameters.push_back( parameter );
		}
		if( !samplerRanges.empty() )
		{
			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = UINT( samplerRanges.size() );
			parameter.DescriptorTable.pDescriptorRanges = &samplerRanges[0];
			parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			m_samplerParameter = uint32_t( parameters.size() );

			parameters.push_back( parameter );
		}

		signatureDesc.NumParameters = UINT( parameters.size() );
		if( signatureDesc.NumParameters )
		{
			signatureDesc.pParameters = &parameters[0];
		}

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
			IID_PPV_ARGS( &m_rootSignature ) );
		if( FAILED( hr ) )
		{
			Destroy();
			return hr;
		}

		m_srvUavTableSize = uint32_t( ranges.size() );
		m_samplerTableSize = uint32_t( samplerRanges.size() );

		m_registerMap = Tr2RegisterMapAL( shaders, count );

		m_owner = &renderContext;

		return S_OK;
	}

	void Tr2ShaderProgramAL::Destroy()
	{
		if( m_rootSignature && m_owner )
		{
			RELEASE_LATER( m_owner, m_rootSignature );
			m_rootSignature = nullptr;
		}
		if( m_owner )
		{
			m_owner->OnShaderProgramDestroyedDx12( this );
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

		m_cbRegisters.clear();
		m_srvRegisters.clear();
		m_uavRegisters.clear();
		m_samplerRegisters.clear();

		m_registerMap = Tr2RegisterMapAL();

		m_srvUavTableSize = 0;
		m_srvUavParameter = 0xffffffff;
		m_samplerTableSize = 0;
		m_samplerParameter = 0xffffffff;
		m_owner = nullptr;
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return m_rootSignature != nullptr;
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

	void Tr2ShaderProgramAL::ParseRegisterSignature(
		ShaderType shaderType,
		const Tr2ShaderSignatureAL& signature,
		std::vector<D3D12_ROOT_PARAMETER>& parameters,
		std::vector<D3D12_DESCRIPTOR_RANGE>& ranges,
		std::vector<D3D12_DESCRIPTOR_RANGE>& samplerRanges )
	{
		for( auto it = begin( signature.registers ); it != end( signature.registers ); ++it )
		{
			auto CreateRange = [=]( D3D12_DESCRIPTOR_RANGE_TYPE rangeType ) -> D3D12_DESCRIPTOR_RANGE {
				D3D12_DESCRIPTOR_RANGE range;
				range.RangeType = rangeType;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = it->registerIndex;
				range.RegisterSpace = UINT( shaderType );
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				return range;
			};

			D3D12_ROOT_PARAMETER parameter;
			switch( it->registerType )
			{
			case Tr2ShaderRegisterAL::CONSTANTS:
			{
				parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				parameter.Descriptor.RegisterSpace = UINT( shaderType );
				parameter.Descriptor.ShaderRegister = it->registerIndex;
				parameter.ShaderVisibility = ShaderVisibility( shaderType );

				CbRegister cbr = { uint32_t( shaderType ), it->registerIndex, uint32_t( parameters.size() ) };
				m_cbRegisters.push_back( cbr );

				parameters.push_back( parameter );
				break;
			}
			case Tr2ShaderRegisterAL::SAMPLER:
			{
				CbRegister cbr = { uint32_t( shaderType ), it->registerIndex, uint32_t( samplerRanges.size() ) };
				m_samplerRegisters.push_back( cbr );
				samplerRanges.push_back( CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ) );
				break;
			}
			case Tr2ShaderRegisterAL::RESOURCE:
			{
				CbRegister cbr = { uint32_t( shaderType ), it->registerIndex, uint32_t( ranges.size() ) };
				m_srvRegisters.push_back( cbr );
				ranges.push_back( CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE_SRV ) );
				break;
			}
			case Tr2ShaderRegisterAL::UAV:
			{
				CbRegister cbr = { uint32_t( shaderType ), it->registerIndex, uint32_t( ranges.size() ) };
				m_uavRegisters.push_back( cbr );
				ranges.push_back( CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE_UAV ) );
				break;
			}
			default:
				continue;
			}
		}
	}

	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}

	bool Tr2ShaderProgramAL::IsComputeProgramDx12() const
	{
		return m_CS.pShaderBytecode != nullptr;
	}

	void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderProgramAL";
	}
}
#endif