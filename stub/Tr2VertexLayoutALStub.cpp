#include "StdAfx.h"
#include "Tr2VertexLayoutALStub.h"

#include "Tr2VertexDefinition.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

using namespace Tr2RenderContextEnum;

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
	ChangeObjectId();
	return S_OK;
}

void Tr2VertexLayoutAL::Destroy()
{
	m_definition.reset();
}

ALResult Tr2VertexLayoutAL::SetLayout( const Tr2ShaderAL* /*vertexShader*/, Tr2RenderContextAL& renderContext ) const
{
	return E_FAIL;
}

#endif
