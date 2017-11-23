#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGL4

#include "Tr2ShaderProgramALGL4.h"
#include "Tr2RenderContextGL4.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;


Tr2ShaderProgramAL::Tr2ShaderProgramAL()
	:m_vertexShader( nullptr ),
	m_pixelShader( nullptr ),
	m_computeShader( nullptr ),
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

	if( count < 1 || count > 2 )
	{
		return E_INVALIDARG;
	}

	if( count == 1 )
	{
		if( !shaders[0]->IsValid() || shaders[0]->GetType() != COMPUTE_SHADER )
		{
			return E_INVALIDARG;
		}
		m_computeShader = shaders[0];
	}
	else
	{
		if( !shaders[0]->IsValid() || ( shaders[0]->GetType() != VERTEX_SHADER && shaders[0]->GetType() != PIXEL_SHADER ) )
		{
			return E_INVALIDARG;
		}
		if( !shaders[1]->IsValid() || ( shaders[1]->GetType() != VERTEX_SHADER && shaders[1]->GetType() != PIXEL_SHADER ) )
		{
			return E_INVALIDARG;
		}
		if( shaders[0]->GetType() == shaders[1]->GetType() )
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
	}

	m_isValid = true;
	return S_OK;
}

void Tr2ShaderProgramAL::Destroy()
{
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;
	m_computeShader = nullptr;
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