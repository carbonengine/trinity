#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2VertexLayoutALGLES2.h"
#include "Tr2RenderContextGLES2.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2VertexLayoutAL::Tr2VertexLayoutAL()
	{
	}

	ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		m_definition = std::unique_ptr<Tr2VertexDefinition>( new Tr2VertexDefinition( definition ) );
		if( definition.m_items.empty() )
		{
			return E_FAIL;
		}
		return S_OK;
	}

	ALResult Tr2VertexLayoutAL::SetLayout( const TrinityALImpl::Tr2ShaderAL* /*vertexShader*/, Tr2RenderContextAL& /*renderContext*/ ) const
	{
		// We set the actual layout in Tr2RenderContextAL::ApplyShadowRenderStates
		return E_FAIL;
	}

	bool Tr2VertexLayoutAL::IsValid() const
	{
		return m_definition.get() != nullptr;
	}

	void Tr2VertexLayoutAL::Destroy()
	{
		m_definition.reset();
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2VertexLayoutAL";
	}
}

#endif // DX9?
