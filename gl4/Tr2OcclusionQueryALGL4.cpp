#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2OcclusionQueryALGL4.h"

Tr2OcclusionQueryAL::Tr2OcclusionQueryAL()
:   m_query( 0 )
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
	Destroy();
}

ALResult Tr2OcclusionQueryAL::Create( Tr2RenderContextAL& renderContext )
{
    if( !renderContext.IsValid() )
    {
        return E_INVALIDARG;
    }
	Destroy();
	CR_GL( glGenQueries( 1, &m_query ) );
	return m_query ? S_OK : E_FAIL;
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_query != 0;
}

void Tr2OcclusionQueryAL::Destroy()
{
	if( m_query )
	{
		glDeleteQueries( 1, &m_query );
		m_query = 0;
	}
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& /*renderContext*/ )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	CR_GL( glBeginQuery( GL_SAMPLES_PASSED, m_query ) );
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& /*renderContext*/ )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	CR_GL( glEndQuery( GL_SAMPLES_PASSED ) );
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& /*renderContext*/, uint32_t& count, WaitMode waitMode )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( waitMode != WAIT )
	{
		GLuint ready = 0;
		CR_GL( glGetQueryObjectuiv( m_query, GL_QUERY_RESULT_AVAILABLE, &ready ) );
		if( !ready )
		{
			return S_FALSE;
		}
	}
	GL_FAIL( glGetQueryObjectuiv( m_query, GL_QUERY_RESULT, &count ) );
	return S_OK;
}

#endif