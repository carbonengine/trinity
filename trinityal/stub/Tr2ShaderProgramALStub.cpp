// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2ShaderProgramALStub.h"
#include "Tr2ShaderALStub.h"
#include "Tr2PrimaryRenderContextStub.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

Tr2ShaderProgramAL::Tr2ShaderProgramAL() :
	m_isValid( false )
{
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

	uint32_t mask = 0;
	for( size_t i = 0; i < count; ++i )
	{
		if( !shaders[i].IsValid() )
		{
			return E_INVALIDARG;
		}
		uint32_t bit = 1 << shaders[i].GetType();
		if( ( mask & bit ) != 0 )
		{
			return E_INVALIDARG;
		}
		mask |= bit;
	}
	m_isValid = true;
	return S_OK;
}

void Tr2ShaderProgramAL::Destroy()
{
	m_isValid = false;
}

bool Tr2ShaderProgramAL::IsValid() const
{
	return m_isValid;
}

Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
{
}

const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
{
	return m_registerMap;
}

ALResult Tr2ShaderProgramAL::SetName( const char* )
{
	return S_OK;
}
}
#endif