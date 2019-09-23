#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2FenceALGLES2.h"
#include "Tr2RenderContextGLES2.h"

#ifdef __ANDROID__
namespace
{

    typedef void (GL_APIENTRYP DeleteSyncProc) ( GLsync );
    DeleteSyncProc glDeleteSyncAPPLE = nullptr;
    
    typedef GLsync (GL_APIENTRYP FenceSyncProc) ( GLenum, GLbitfield );
    FenceSyncProc glFenceSyncAPPLE = nullptr;
    
    typedef GLenum (GL_APIENTRYP ClientWaitSyncProc) ( GLsync, GLbitfield, uint64_t );
    ClientWaitSyncProc glClientWaitSyncAPPLE = nullptr;
    
    typedef void (GL_APIENTRYP GenFencesProc) ( GLsizei, GLuint* );
    GenFencesProc glGenFences = nullptr;
    
    typedef void (GL_APIENTRYP DeleteFencesProc) ( GLsizei, const GLuint* );
    DeleteFencesProc glDeleteFences = nullptr;
    
    typedef void (GL_APIENTRYP SetFenceProc) ( GLuint, GLenum );
    SetFenceProc glSetFence = nullptr;
    
    typedef GLboolean (GL_APIENTRYP TestFenceProc) ( GLuint );
    TestFenceProc glTestFence = nullptr;
    
    typedef void (GL_APIENTRYP FinishFenceProc) ( GLuint );
    FinishFenceProc glFinishFence = nullptr;
    
    void InitializeExtensions()
    {
		static bool initialized = false;
		if( !initialized )
		{
			const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
            if( strstr( extensions, "APPLE_sync" ) )
            {
                glDeleteSyncAPPLE = (DeleteSyncProc)eglGetProcAddress( "glDeleteSyncAPPLE" );
                glFenceSyncAPPLE = (FenceSyncProc)eglGetProcAddress( "glFenceSyncAPPLE" );
                glClientWaitSyncAPPLE = (ClientWaitSyncProc)eglGetProcAddress( "glClientWaitSyncAPPLE" );
            }
            else if( strstr( extensions, "NV_fence" ) )
            {
                glGenFences = (GenFencesProc)eglGetProcAddress( "glGenFencesNV" );
                glDeleteFences = (DeleteFencesProc)eglGetProcAddress( "glDeleteFencesNV" );
                glSetFence = (SetFenceProc)eglGetProcAddress( "glSetFenceNV" );
                glTestFence = (TestFenceProc)eglGetProcAddress( "glTestFenceNV" );
                glFinishFence = (FinishFenceProc)eglGetProcAddress( "glFinishFenceNV" );
            }
			initialized = true;
		}
    }
#define glDeleteSync glDeleteSyncAPPLE
#define glFenceSync glFenceSyncAPPLE
#define glClientWaitSync glClientWaitSyncAPPLE
    
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_TIMEOUT_EXPIRED 0x911b
#define GL_WAIT_FAILED 0x911d
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x1

}
#endif

namespace TrinityALImpl
{
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
#ifdef __ANDROID__
    InitializeExtensions();
#endif
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
#ifdef __ANDROID__
    if( glGenFences )
    {
        GLuint fence;
        glGenFences( 1, &fence );
        m_fence = (GLsync)fence;
    }
    else
#endif
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
#ifdef __ANDROID__
        if( glDeleteFences )
        {
            GLuint fence = (GLuint)m_fence;
            glDeleteFences( 1, &fence );
        }
        else
#endif
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
#ifdef __ANDROID__
    if( glSetFence )
    {
        GL_FAIL( glSetFence( (GLuint)m_fence, GL_ALL_COMPLETED_NV ) );
        return S_OK;
    }
#endif
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
#ifdef __ANDROID__
    if( glTestFence )
    {
        isReached = glTestFence( (GLuint)m_fence );
        return S_OK;
    }
#endif
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
#ifdef __ANDROID__
    if( glFinishFence )
    {
        GL_FAIL( glFinishFence( (GLuint)m_fence ) );
        return S_OK;
    }
#endif
	if( glClientWaitSync( m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0xfffffffffffffffful ) == GL_WAIT_FAILED )
	{
		return E_FAIL;
	}
	return S_OK;
}

void Tr2FenceAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2FenceAL";
}
}

#endif