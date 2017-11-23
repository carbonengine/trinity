#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2ShaderProgramALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"

using namespace Tr2RenderContextEnum;

Tr2ShaderProgramAL::Tr2ShaderProgramAL()
	:m_isValid( false )
{
	for( int32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		m_shaders[i] = &nullShader[i];
	}
}

ALResult Tr2ShaderProgramAL::Create( Tr2ShaderAL** shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
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
		if( !shaders[i]->IsValid() )
		{
			return E_INVALIDARG;
		}
		uint32_t bit = 1 << shaders[i]->GetType();
		if( ( mask & bit ) != 0 )
		{
			return E_INVALIDARG;
		}
		mask |= bit;
	}
	for( size_t i = 0; i < count; ++i )
	{
		m_shaders[shaders[i]->GetType()] = shaders[i];
	}
	m_isValid = true;
	return S_OK;
}

void Tr2ShaderProgramAL::Destroy()
{
	for( int32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		m_shaders[i] = &nullShader[i];
	}
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

#endif