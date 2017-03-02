#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

// Note: GLES has similar functionality with boolean occlusion queries:
// http://www.khronos.org/registry/gles/extensions/EXT/EXT_occlusion_query_boolean.txt

#include "Tr2OcclusionQueryALGLES2.h"
#include "ALLog.h"

#ifdef TRINITY_AL_MOBILE
#include <EGL/egl.h>

namespace
{
    bool extensionInitialized = false;
    typedef void (GL_APIENTRYP PFNGLGENQUERIESPROC) (GLsizei n, GLuint* ids);
    PFNGLGENQUERIESPROC glGenQueries = nullptr;
    typedef void (GL_APIENTRYP PFNGLDELETEQUERIESPROC) (GLsizei n, const GLuint* ids);
    PFNGLDELETEQUERIESPROC glDeleteQueries = nullptr;
    typedef void (GL_APIENTRYP PFNGLBEGINQUERYPROC) (GLenum target, GLuint id);
    PFNGLBEGINQUERYPROC glBeginQuery = nullptr;
    typedef void (GL_APIENTRYP PFNGLENDQUERYPROC) (GLenum target);
    PFNGLENDQUERYPROC glEndQuery = nullptr;
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVPROC) (GLuint id, GLenum pname, GLuint* params);
    PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv = nullptr;

#define GL_SAMPLES_PASSED 0x8C2F
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_QUERY_RESULT 0x8866
    
    void InitializeExtension()
    {
        if( extensionInitialized )
        {
            return;
        }
        extensionInitialized = true;
        glGenQueries = (PFNGLGENQUERIESPROC)eglGetProcAddress( "glGenQueriesEXT" );
        glDeleteQueries = (PFNGLDELETEQUERIESPROC)eglGetProcAddress( "glDeleteQueriesEXT" );
        glBeginQuery = (PFNGLBEGINQUERYPROC)eglGetProcAddress( "glBeginQueryEXT" );
        glEndQuery = (PFNGLENDQUERYPROC)eglGetProcAddress( "glEndQueryEXT" );
        glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)eglGetProcAddress( "glGetQueryObjectuivEXT" );
    }
}
#endif

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
#ifdef TRINITY_AL_MOBILE
    InitializeExtension();
    if( !glGenQueries )
    {
        CCP_AL_LOGERR( "Tr2OcclusionQueryAL::Create: occlusion queries are not supported on this device" );
        return E_FAIL;
    }
#endif
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