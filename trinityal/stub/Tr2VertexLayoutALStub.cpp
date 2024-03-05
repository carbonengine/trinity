#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2VertexLayoutALStub.h"
#include "Tr2VertexDefinition.h"
#include "Tr2RenderContextStub.h"


using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

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

	void Tr2VertexLayoutAL::Destroy()
	{
		m_definition.reset();
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
	{
	}

	ALResult Tr2VertexLayoutAL::SetName( const char* )
	{
		return S_OK;
	}
}
#endif
