////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"
#include "../include/Tr2ResourceSetAL.h"

class Tr2ShaderProgramAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
public:
	Tr2ShaderProgramAL();
	~Tr2ShaderProgramAL();

	ALResult Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

	bool IsComputeProgramDx12() const;
private:
	void ParseRegisterSignature( 
		Tr2RenderContextEnum::ShaderType shaderType, 
		const Tr2ShaderSignatureAL& signature,
		std::vector<D3D12_ROOT_PARAMETER>& parameters,
		std::vector<D3D12_DESCRIPTOR_RANGE>& ranges,
		std::vector<D3D12_DESCRIPTOR_RANGE>& samplerRanges );
	static D3D12_SHADER_BYTECODE MakeShaderBytecode( const Tr2ShaderAL& shader );

	D3D12_SHADER_BYTECODE m_VS;
	D3D12_SHADER_BYTECODE m_PS;
	D3D12_SHADER_BYTECODE m_DS;
	D3D12_SHADER_BYTECODE m_HS;
	D3D12_SHADER_BYTECODE m_GS;
	D3D12_SHADER_BYTECODE m_CS;
	std::vector<Tr2ShaderAL> m_shaders;

	CComPtr<ID3D12RootSignature> m_rootSignature;
	Tr2PrimaryRenderContextAL* m_owner;

	struct CbRegister
	{
		uint32_t stage;
		uint32_t index;
		uint32_t parameter;
	};
	std::vector<CbRegister> m_cbRegisters;

	uint32_t m_srvMap[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	uint32_t m_uavMap[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	uint32_t m_samplerMap[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];

	uint32_t m_srvUavTableSize;
	uint32_t m_srvUavParameter;
	uint32_t m_samplerTableSize;
	uint32_t m_samplerParameter;

	friend class Tr2RenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
};

#endif