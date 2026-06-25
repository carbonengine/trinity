// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2RtPipelineStateALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "../ALLog.h"

extern uint32_t g_forceAnisotropy;

namespace
{
class DataStoreCache
{
public:
	DataStoreCache() :
		m_dataOffset( PAGE_SIZE )
	{
	}
	void* AddData( size_t size )
	{
		if( m_dataOffset + size > PAGE_SIZE )
		{
			std::unique_ptr<uint8_t[]> page( new uint8_t[std::max( size, PAGE_SIZE )] );
			m_dataPages.emplace_back( std::move( page ) );
			m_dataOffset = size;
			return m_dataPages.back().get();
		}
		auto result = m_dataPages.back().get() + m_dataOffset;
		m_dataOffset += size;
		return result;
	}

	template <typename T>
	T* AddData( size_t count = 1 )
	{
		return static_cast<T*>( AddData( sizeof( T ) * count ) );
	}

private:
	std::vector<std::unique_ptr<uint8_t[]>> m_dataPages;
	static const size_t PAGE_SIZE = 1024;
	size_t m_dataOffset;
};
}


namespace TrinityALImpl
{
Tr2RtPipelineStateAL::Tr2RtPipelineStateAL() :
	m_owner( nullptr )
{
}

Tr2RtPipelineStateAL::~Tr2RtPipelineStateAL()
{
	Destroy();
}

ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	// pipeline state obj is just an array of subobjects,
	// where each subobject describes a single element of the state
	DataStoreCache dataStore;
	std::vector<D3D12_STATE_SUBOBJECT> subObjects;
	subObjects.reserve( desc.m_shaders.size() * 4 + desc.m_hitGroups.size() * 2 + desc.m_localSignatures.size() + 2 );

	std::vector<TrinityALImpl::Tr2RootSignatureAL*> localSignatures;
	ON_BLOCK_EXIT( [&localSignatures] {
		for( auto it = begin( localSignatures ); it != end( localSignatures ); ++it )
		{
			delete *it;
		}
	} );

	for( auto it = begin( desc.m_localSignatures ); it != end( desc.m_localSignatures ); ++it )
	{
		TrinityALImpl::Tr2RootSignatureAL* localSignature = new TrinityALImpl::Tr2RootSignatureAL();
		localSignatures.push_back( localSignature );

		CR_RETURN_HR( CreateRootSignature( *localSignature, *it, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE, renderContext ) );

		auto signatureDesc = dataStore.AddData<D3D12_LOCAL_ROOT_SIGNATURE>();
		signatureDesc->pLocalRootSignature = localSignature->m_rootSignature.p;

		D3D12_STATE_SUBOBJECT subobject = {};
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		subobject.pDesc = signatureDesc;

		subObjects.push_back( subobject );
	}

	for( auto it = begin( desc.m_shaders ); it != end( desc.m_shaders ); ++it )
	{
		auto& shader = *it;

		//	CREATE DXIL LIBRARY
		// 1. subobject = dxil library

		// dxil export
		auto rgsExportDesc = dataStore.AddData<D3D12_EXPORT_DESC>( shader.names.size() );
		for( size_t i = 0; i < shader.names.size(); ++i )
		{
			rgsExportDesc[i].Name = shader.names[i].exportName.c_str();
			rgsExportDesc[i].ExportToRename = shader.names[i].name.c_str();
			rgsExportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
		}

		// dxil library
		auto rgsLibDesc = dataStore.AddData<D3D12_DXIL_LIBRARY_DESC>();
		rgsLibDesc->DXILLibrary.BytecodeLength = shader.bytecode.size;
		rgsLibDesc->DXILLibrary.pShaderBytecode = shader.bytecode.bytecode;
		rgsLibDesc->NumExports = UINT( shader.names.size() );
		rgsLibDesc->pExports = rgsExportDesc;

		D3D12_STATE_SUBOBJECT subobject = {};
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		subobject.pDesc = rgsLibDesc;

		subObjects.push_back( subobject );

		// **************
		// 2. SHADER CONFIG
		auto shaderDesc = dataStore.AddData<D3D12_RAYTRACING_SHADER_CONFIG>();
		shaderDesc->MaxPayloadSizeInBytes = shader.payloadSize;
		shaderDesc->MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subobject.pDesc = shaderDesc;

		subObjects.push_back( subobject );

		// **************
		// 3. ASSOCIATE ROOT SIG TO RGS
		auto shaderPayloadAssociation = dataStore.AddData<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
		shaderPayloadAssociation->NumExports = UINT( shader.names.size() );
		auto names = dataStore.AddData<const wchar_t*>( shader.names.size() );
		for( size_t i = 0; i < shader.names.size(); ++i )
		{
			names[i] = shader.names[i].exportName.c_str();
		}
		shaderPayloadAssociation->pExports = names;
		shaderPayloadAssociation->pSubobjectToAssociate = &subObjects[subObjects.size() - 1];

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = shaderPayloadAssociation;

		subObjects.push_back( subobject );

		// **************
		// LOCAL ROOT SIGNATURES
		if( shader.localSignature != Tr2RtPipelineStateDescriptionAL::NO_SIGNATURE )
		{
			auto shaderPayloadAssociation = dataStore.AddData<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
			shaderPayloadAssociation->NumExports = UINT( shader.names.size() );
			auto names = dataStore.AddData<const wchar_t*>( shader.names.size() );
			for( size_t i = 0; i < shader.names.size(); ++i )
			{
				names[i] = shader.names[i].exportName.c_str();
			}
			shaderPayloadAssociation->pExports = names;
			shaderPayloadAssociation->pSubobjectToAssociate = &subObjects[shader.localSignature];

			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			subobject.pDesc = shaderPayloadAssociation;

			subObjects.push_back( subobject );

			for( size_t i = 0; i < shader.names.size(); ++i )
			{
				m_shaderInfoForName[shader.names[i].exportName] = { localSignatures[shader.localSignature] };
			}
		}
		else
		{
			for( size_t i = 0; i < shader.names.size(); ++i )
			{
				m_shaderInfoForName[shader.names[i].exportName] = { nullptr };
			}
		}
	}

	for( auto it = begin( desc.m_hitGroups ); it != end( desc.m_hitGroups ); ++it )
	{
		// **************
		// 4. HIT GROUP
		auto& hitGroup = *it;

		auto hitGroupDesc = dataStore.AddData<D3D12_HIT_GROUP_DESC>();
		hitGroupDesc->HitGroupExport = hitGroup.exportName.c_str();
		hitGroupDesc->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitGroupDesc->AnyHitShaderImport = hitGroup.anyHit.empty() ? nullptr : hitGroup.anyHit.c_str();
		hitGroupDesc->ClosestHitShaderImport = hitGroup.closestHit.empty() ? nullptr : hitGroup.closestHit.c_str();
		hitGroupDesc->IntersectionShaderImport = hitGroup.intersection.empty() ? nullptr : hitGroup.intersection.c_str();

		D3D12_STATE_SUBOBJECT subobject = {};
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobject.pDesc = hitGroupDesc;

		subObjects.push_back( subobject );

		// LOCAL SIGNATURE
		if( hitGroup.localSignature != Tr2RtPipelineStateDescriptionAL::NO_SIGNATURE )
		{
			auto shaderPayloadAssociation = dataStore.AddData<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
			shaderPayloadAssociation->NumExports = 1;
			auto names = dataStore.AddData<const wchar_t*>();
			names[0] = hitGroup.exportName.c_str();
			shaderPayloadAssociation->pExports = names;
			shaderPayloadAssociation->pSubobjectToAssociate = &subObjects[hitGroup.localSignature];

			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			subobject.pDesc = shaderPayloadAssociation;

			subObjects.push_back( subobject );

			m_shaderInfoForName[hitGroup.exportName] = { localSignatures[hitGroup.localSignature] };
		}
		else
		{
			m_shaderInfoForName[hitGroup.exportName] = { nullptr };
		}
	}
	// **************
	// SHADERPROGRAM
	// Create the global root signature and store the empty signature
	TrinityALImpl::Tr2RootSignatureAL rootSignature;
	CR_RETURN_HR( CreateRootSignature( rootSignature, desc.m_globalSignature, D3D12_ROOT_SIGNATURE_FLAG_NONE, renderContext ) );

	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSig.pDesc = &rootSignature.m_rootSignature.p;
	subObjects.push_back( globalRootSig );

	// pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;

	subObjects.push_back( pipelineConfigObject );

	// **************
	// Create the state
	D3D12_STATE_OBJECT_DESC pipelineDesc = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
	//pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = static_cast<UINT>( subObjects.size() );
	pipelineDesc.pSubobjects = subObjects.data();

	// device5 = dxrDevice
	CR_RETURN_HR( renderContext.m_device5->CreateStateObject( &pipelineDesc, IID_PPV_ARGS( &m_state ) ) );
	m_state.QueryInterface( &m_stateInfo );

	for( auto& data : m_shaderInfoForName )
	{
		data.second.shaderIdentifier = m_stateInfo->GetShaderIdentifier( data.first.c_str() );
	}

	m_globalSignature = rootSignature;
	m_localSignatures = localSignatures;
	localSignatures.clear();
	m_owner = &renderContext;
	return S_OK;
}

void Tr2RtPipelineStateAL::Destroy()
{
	if( m_owner )
	{
		for( auto it = begin( m_localSignatures ); it != end( m_localSignatures ); ++it )
		{
			( *it )->Destroy( *m_owner );
			delete *it;
		}
		m_localSignatures.clear();

		RELEASE_LATER( m_owner, m_state );
		RELEASE_LATER( m_owner, m_stateInfo );
		m_globalSignature.Destroy( *m_owner );
		m_state = nullptr;
		m_stateInfo = nullptr;
		m_owner = nullptr;
		m_shaderInfoForName.clear();
	}
}

bool Tr2RtPipelineStateAL::IsValid() const
{
	return m_owner != nullptr;
}

Tr2ALMemoryType Tr2RtPipelineStateAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}


void Tr2RtPipelineStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtPipelineStateAL";
}

const TrinityALImpl::Tr2RootSignatureAL& Tr2RtPipelineStateAL::GetGlobalRootSignature() const
{
	return m_globalSignature;
}

ID3D12StateObjectProperties* Tr2RtPipelineStateAL::GetStateInfo() const
{
	return m_stateInfo;
}

ID3D12StateObject* Tr2RtPipelineStateAL::GetStateObject() const
{
	return m_state;
}

ALResult Tr2RtPipelineStateAL::CreateRootSignature( TrinityALImpl::Tr2RootSignatureAL& rootSignature, const Tr2ShaderSignatureAL& signature, D3D12_ROOT_SIGNATURE_FLAGS flags, Tr2PrimaryRenderContextAL& renderContext )
{
	D3D12_ROOT_SIGNATURE_DESC signatureDesc;
	memset( &signatureDesc, 0, sizeof( signatureDesc ) );
	signatureDesc.Flags = flags;

	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>> ranges;

	const auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;

	auto CreateRange = []( D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t registerIndex, uint32_t registerSpace, uint32_t arrayCount, uint32_t offset = 0 ) {
		D3D12_DESCRIPTOR_RANGE range;
		range.RangeType = rangeType;
		range.NumDescriptors = arrayCount ? arrayCount : UINT_MAX;
		range.BaseShaderRegister = registerIndex;
		range.RegisterSpace = registerSpace;
		range.OffsetInDescriptorsFromTableStart = offset;
		return range;
	};

	for( auto& reg : signature.registers )
	{
		auto isSrv = reg.IsSrv();
		if( isSrv || reg.IsUav() )
		{
			Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType };
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
			if( isSrv )
			{
				rootSignature.m_srvRegisters.push_back( cbr );
				rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			}
			else
			{
				rootSignature.m_uavRegisters.push_back( cbr );
				rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			}
			ranges.push_back( std::make_unique<D3D12_DESCRIPTOR_RANGE>( CreateRange( rangeType, reg.registerIndex, reg.registerSpace, reg.arrayCount ) ) );

			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = ranges.back().get();
			parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			parameters.push_back( parameter );
			rootSignature.m_srvUavParameterCount++;
		}
	}
	for( auto& reg : signature.registers )
	{
		D3D12_ROOT_PARAMETER parameter;
		if( reg.registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER )
		{
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			parameter.Descriptor.RegisterSpace = reg.registerSpace;
			parameter.Descriptor.ShaderRegister = reg.registerIndex;
			parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType };
			rootSignature.m_cbRegisters.push_back( cbr );

			parameters.push_back( parameter );
		}
	}
	for( auto& reg : signature.registers )
	{
		if( reg.registerType == Tr2ShaderRegisterAL::SAMPLER )
		{
			Tr2RootSignatureAL::CbRegister cbr = { uint32_t( shaderType ), reg.registerIndex, uint32_t( parameters.size() ), reg.registerType };
			rootSignature.m_samplerRegisters.push_back( cbr );

			ranges.push_back( std::make_unique<D3D12_DESCRIPTOR_RANGE>( CreateRange( D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, reg.registerIndex, reg.registerSpace, reg.arrayCount ) ) );

			D3D12_ROOT_PARAMETER parameter;
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable.NumDescriptorRanges = 1;
			parameter.DescriptorTable.pDescriptorRanges = ranges.back().get();
			parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			parameters.push_back( parameter );
			rootSignature.m_samplerParameterCount++;
		}
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
	for( auto& sampl : signature.samplers )
	{
		auto& description = sampl.sampler;
		D3D12_STATIC_SAMPLER_DESC sampler;
		if( g_forceAnisotropy != 1 && ( description.m_minFilter == Tr2RenderContextEnum::TF_ANISOTROPIC || description.m_magFilter == Tr2RenderContextEnum::TF_ANISOTROPIC || description.m_mipFilter == Tr2RenderContextEnum::TF_ANISOTROPIC ) )
		{
			sampler.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER( description.m_isComparisonFilter ? 1 : 0 );
		}
		else
		{
			sampler.Filter = D3D12_ENCODE_BASIC_FILTER(
				description.m_minFilter == Tr2RenderContextEnum::TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
				description.m_magFilter == Tr2RenderContextEnum::TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
				description.m_mipFilter == Tr2RenderContextEnum::TF_POINT || description.m_mipFilter == Tr2RenderContextEnum::TF_NONE ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
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
		sampler.MaxLOD = description.m_mipFilter == Tr2RenderContextEnum::TF_NONE ? description.m_minLOD : description.m_maxLOD;

		sampler.ShaderRegister = sampl.registerIndex;
		sampler.RegisterSpace = sampl.registerSpace;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		samplers.push_back( sampler );
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
			auto errorMsg = static_cast<const char*>( errorBlob->GetBufferPointer() );
			CCP_AL_LOGERR( "Failed to serialize a root signature: %s", errorMsg );
		}
		else
		{
			CCP_AL_LOGERR( "Failed to serialize a root signature - unknown error" );
		}
		Destroy();
		return hr;
	}
	rootSignature.m_isCompute = true;
	rootSignature.m_registerMap = Tr2RegisterMapAL( &shaderType, &signature, 1 );

	return renderContext.m_device->CreateRootSignature(
		0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS( &rootSignature.m_rootSignature ) );
}

const std::vector<TrinityALImpl::Tr2RootSignatureAL*>& Tr2RtPipelineStateAL::GetLocalSignatures() const
{
	return m_localSignatures;
}

const Tr2RtPipelineStateAL::ShaderInfo* Tr2RtPipelineStateAL::GetShaderInfo( const wchar_t* name ) const
{
	auto found = m_shaderInfoForName.find( name );
	if( found == m_shaderInfoForName.end() )
	{
		return nullptr;
	}
	return &found->second;
}
}

#endif
