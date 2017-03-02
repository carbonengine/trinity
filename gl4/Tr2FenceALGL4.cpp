#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2FenceALGL4.h"


Tr2FenceAL::Tr2FenceAL()
	:m_isValid( false ),
	m_fence( nullptr )
{
}

Tr2FenceAL::~Tr2FenceAL()
{
	Destroy();
}

ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
    if( !glDeleteSync )
    {
        return E_FAIL;
    }
	m_isValid = true;
	return S_OK;
}

void Tr2FenceAL::Destroy()
{
	if( m_fence )
	{
		glDeleteSync( m_fence );
        m_fence = nullptr;
	}
	m_isValid = false;
}

bool Tr2FenceAL::IsValid() const
{
	return m_isValid;
}

ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( m_fence )
	{
		glDeleteSync( m_fence );
	}
	GL_FAIL( m_fence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
	if( !m_fence )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	switch( glClientWaitSync( m_fence, 0, 0 ) )
	{
	case GL_TIMEOUT_EXPIRED:
		isReached = false;
		return S_OK;
	case GL_WAIT_FAILED:
		return E_FAIL;
	default:
		isReached = true;
		return S_OK;
	}
}

ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( glClientWaitSync( m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0xfffffffffffffffful ) == GL_WAIT_FAILED )
	{
		return E_FAIL;
	}
	return S_OK;
}


#endif