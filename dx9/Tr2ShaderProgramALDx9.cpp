#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX9 

#include "Tr2ShaderProgramALDx9.h"
#include "Tr2RenderContextDx9.h"


using namespace Tr2RenderContextEnum;

Tr2ShaderProgramAL::Tr2ShaderProgramAL()
	:m_vertexShader( nullptr ),
	m_pixelShader( nullptr ),
	m_isValid( false )
{
}

ALResult Tr2ShaderProgramAL::Create( Tr2ShaderAL** shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	if( count != 2 )
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
	if( mask != ( ( 1 << VERTEX_SHADER ) | ( 1 << PIXEL_SHADER ) ) )
	{
		return E_INVALIDARG;
	}

	if( shaders[0]->GetType() == VERTEX_SHADER )
	{
		m_vertexShader = shaders[0];
		m_pixelShader = shaders[1];
	}
	else
	{
		m_vertexShader = shaders[1];
		m_pixelShader = shaders[0];
	}
	m_isValid = true;
	return S_OK;
}

void Tr2ShaderProgramAL::Destroy()
{
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;
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
