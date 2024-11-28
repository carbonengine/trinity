////////////////////////////////////////////////////////////
//
//    Created:   November 2019
//    Copyright: CCP 2019
//

#pragma once

#include "StdAfx.h"
#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"
#include "Tr2ConstantBufferAL.h"
#include "Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2RtShaderTableAL;
}

class Tr2PrimaryRenderContextAL;
class Tr2RtPipelineStateAL;

// local materials
class Tr2RtLocalMaterialDescriptionAL
{
public:
	Tr2RtLocalMaterialDescriptionAL& SetConstants( uint32_t registerIndex, const Tr2ConstantBufferAL& buffer );
	Tr2RtLocalMaterialDescriptionAL& SetResourceSet( const Tr2ResourceSetDescriptionAL& resourceSet );
private:
	Tr2ResourceSetDescriptionAL m_resourceSet;
	Tr2ConstantBufferAL m_constants[8];

	friend class TrinityALImpl::Tr2RtShaderTableAL;
};


class Tr2RtShaderTableDescriptionAL
{
public:
	void AddRayGenShader( const wchar_t* name );
	void AddMissShader( const wchar_t* name );
	void AddHitGroup( const wchar_t* name );
	void AddHitGroup( const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material );

	void Reserve( size_t hitGroupCount );
private:
	struct ShaderRecord
	{
		std::wstring name;
		Tr2RtLocalMaterialDescriptionAL material;
	};
	std::vector<ShaderRecord> m_rayGenNames;
	std::vector<ShaderRecord> m_missNames;
	std::vector<ShaderRecord> m_hitGroupNames;

	friend class TrinityALImpl::Tr2RtShaderTableAL;
};


class Tr2RtShaderTableAL
{
public:
	Tr2RtShaderTableAL();

	ALResult Create( const Tr2RtShaderTableDescriptionAL& desc, const Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	TrinityALImpl::Tr2RtShaderTableAL* TrinityALImpl_GetObject() const;
private:
	std::shared_ptr<TrinityALImpl::Tr2RtShaderTableAL> m_shaderTable;
};
