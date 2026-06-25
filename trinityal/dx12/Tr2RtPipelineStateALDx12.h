// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


//#include "StdAfx.h"
#include "../include/Tr2RtPipelineStateAL.h"
#include "Tr2ShaderProgramALDx12.h"

namespace TrinityALImpl
{
class Tr2RtPipelineStateAL : public Tr2DeviceResourceAL<Tr2RtPipelineStateAL>
{
public:
	struct ShaderInfo
	{
		const TrinityALImpl::Tr2RootSignatureAL* localSignature;
		const void* shaderIdentifier;
	};

	Tr2RtPipelineStateAL();
	~Tr2RtPipelineStateAL();

	ALResult CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	ID3D12StateObjectProperties* GetStateInfo() const;
	ID3D12StateObject* GetStateObject() const;
	const Tr2RootSignatureAL& GetGlobalRootSignature() const;
	const std::vector<TrinityALImpl::Tr2RootSignatureAL*>& GetLocalSignatures() const;
	const ShaderInfo* GetShaderInfo( const wchar_t* name ) const;

private:
	ALResult CreateRootSignature( TrinityALImpl::Tr2RootSignatureAL& rootSignature, const Tr2ShaderSignatureAL& signature, D3D12_ROOT_SIGNATURE_FLAGS flags, Tr2PrimaryRenderContextAL& renderContext );

	CComPtr<ID3D12StateObject> m_state;
	CComPtr<ID3D12StateObjectProperties> m_stateInfo;
	TrinityALImpl::Tr2RootSignatureAL m_globalSignature;
	std::vector<TrinityALImpl::Tr2RootSignatureAL*> m_localSignatures;
	std::map<std::wstring, ShaderInfo, std::less<>> m_shaderInfoForName;
	Tr2PrimaryRenderContextAL* m_owner;
};
}

#endif