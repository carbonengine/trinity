// Copyright © 2019 CCP ehf.

#pragma once

#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"
#include "Tr2ShaderAL.h"
#include "Tr2ShaderProgramAL.h"

namespace TrinityALImpl
{
class Tr2RtPipelineStateAL;
}

class Tr2RtPipelineStateDescriptionAL
{
public:
	~Tr2RtPipelineStateDescriptionAL();

	void AddShader( Tr2ShaderAL& shader, const wchar_t* exportName, const wchar_t* name, Tr2ShaderProgramAL shaderProgram );
	void AddShader( const wchar_t* exportName, const Tr2ShaderBytecodeAL& bytecode, const wchar_t* name, uint32_t payloadSize );
	void AddShaders( size_t count, const wchar_t** exportNames, const Tr2ShaderBytecodeAL& bytecode, const wchar_t** names, uint32_t payloadSize );
	void AddShaders( size_t count, const wchar_t** exportNames, const Tr2ShaderBytecodeAL& bytecode, const wchar_t** names, uint32_t payloadSize, const Tr2ShaderSignatureAL& signature );
	void AddHitGroup( const wchar_t* exportName, const wchar_t* anyHit, const wchar_t* closestHit, const wchar_t* intersection );
	void AddHitGroup( const wchar_t* exportName, const wchar_t* anyHit, const wchar_t* closestHit, const wchar_t* intersection, const Tr2ShaderSignatureAL& signature );
	void AddGlobalSignature( const Tr2ShaderSignatureAL& signature );
	uint32_t AddLocalSignature( const Tr2ShaderSignatureAL& signature );

private:
	static const uint32_t NO_SIGNATURE = 0xffffffff;

	struct ShaderName
	{
		std::wstring exportName;
		std::wstring name;
	};
	struct Shader
	{
		~Shader();
		std::vector<ShaderName> names;
		Tr2ShaderBytecodeAL bytecode;
		uint32_t payloadSize;
		uint32_t localSignature;
	};
	struct HitGroup
	{
		std::wstring exportName;
		std::wstring anyHit;
		std::wstring closestHit;
		std::wstring intersection;
		uint32_t localSignature;
	};

	std::vector<Shader> m_shaders;
	std::vector<HitGroup> m_hitGroups;
	Tr2ShaderSignatureAL m_globalSignature;
	std::vector<Tr2ShaderSignatureAL> m_localSignatures;

	friend class TrinityALImpl::Tr2RtPipelineStateAL;
};

class Tr2RtPipelineStateAL
{
public:
	Tr2RtPipelineStateAL();

	ALResult CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	TrinityALImpl::Tr2RtPipelineStateAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2RtPipelineStateAL> m_pipeline;

	friend class TrinityALImpl::Tr2ResourceSetAL;
};
