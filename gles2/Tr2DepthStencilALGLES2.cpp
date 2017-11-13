#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2DepthStencilALGLES2.h"
#include "ALLog.h"
#include "Tr2RenderContextGLES2.h"

using namespace Tr2RenderContextEnum;

namespace
{
#define DEFINE_CHECK_EXT(name)													\
	bool Has##name()															\
	{																			\
		static bool initialized = false;										\
		static bool hasExt = false;												\
		if( !initialized )														\
		{																		\
			const char* extensions = (const char*)glGetString( GL_EXTENSIONS );	\
			hasExt = strstr( extensions, #name ) != 0;							\
			initialized = true;													\
		}																		\
		return hasExt;															\
	}
#define CHECK_EXT(name) (Has##name())

DEFINE_CHECK_EXT(OES_packed_depth_stencil)
DEFINE_CHECK_EXT(OES_depth24)
DEFINE_CHECK_EXT(OES_depth32)

}

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_depthRenderBuffer( 0 )
	, m_stencilRenderBuffer( 0  )
	, m_separateStencilBuffer( false )
	, m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
{
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
	Destroy();
}

ALResult Tr2DepthStencilAL::Create( 
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( flags != EX_NONE )
	{
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
#ifdef TRINITY_AL_MOBILE
    if( msaa.samples > 1 )
    {
        CCP_AL_LOGERR( "Tr2DepthStencilAL::Create: MSAA is not supported" );
        return E_INVALIDARG;
    }
#endif
    
	if( dsFormat == DSFMT_READABLE )
	{
		GLuint stencilRenderBuffer = 0;
#ifdef TRINITY_AL_MOBILE
		if( !CHECK_EXT(OES_packed_depth_stencil) )
#else
		if( !GL_EXT_packed_depth_stencil )
#endif
		{
			// we'll also need a stencil buffer
			GL_FAIL( glGenRenderbuffers( 1, &stencilRenderBuffer ) );
			GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, stencilRenderBuffer ) );
			GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height ) );
		}

		HRESULT hr = m_readableDepth.CreateDepthTexture( width, height, renderContext );
		if( FAILED( hr ) )
		{
			if( stencilRenderBuffer )
			{
				glDeleteRenderbuffers( 1, &stencilRenderBuffer );
			}
			return hr;
		}

		m_stencilRenderBuffer = stencilRenderBuffer;
		m_width  = width;
		m_height = height;
		m_format = dsFormat;
		m_msaa = msaa;
		ChangeObjectId();

		return S_OK;
	}
	
	GLenum glDepthFormat = 0, glStencilFormat = 0;
	bool separateStencilBuffer = true;
	
	switch( dsFormat )
	{
	case DSFMT_D24S8:
	case DSFMT_D24FS8:
	case DSFMT_D24X8:
	case DSFMT_AUTO:
#ifdef TRINITY_AL_MOBILE
		if( CHECK_EXT( OES_packed_depth_stencil ) )
		{
			glDepthFormat = GL_DEPTH24_STENCIL8_OES;
			separateStencilBuffer = false;
		}
		else
        {
            if( CHECK_EXT( OES_depth24 ) )
            {
                glDepthFormat = GL_DEPTH_COMPONENT24_OES;
            }
            else if( CHECK_EXT( OES_depth32 ) )
            {
                glDepthFormat = GL_DEPTH_COMPONENT32_OES;
            }
            else
            {
                glDepthFormat = GL_DEPTH_COMPONENT16;
            }
        }
#else
        if( GLEW_EXT_packed_depth_stencil )
		{
			glDepthFormat = GL_DEPTH24_STENCIL8_EXT;
			separateStencilBuffer = false;
		}
		else
		{
			glDepthFormat = GL_DEPTH_COMPONENT24;
			glStencilFormat = GL_STENCIL_INDEX8;
		}
#endif
		break;
	
	case DSFMT_D32F: 
	case DSFMT_D32:
	case DSFMT_D32F_LOCKABLE:
#ifdef TRINITY_AL_MOBILE
        if( CHECK_EXT( OES_depth32 ) )
        {
            glDepthFormat = GL_DEPTH_COMPONENT32_OES;
        }
        else
        {
            return E_INVALIDARG;
        }
#else
        glDepthFormat = GL_DEPTH_COMPONENT32;
#endif
		break;

#ifndef TRINITY_AL_MOBILE
	case DSFMT_D16_LOCKABLE:
		glDepthFormat = GL_DEPTH_COMPONENT16;
		break;
		
	case DSFMT_D15S1:
		glDepthFormat = GL_DEPTH_COMPONENT16;
		glStencilFormat = GL_STENCIL_INDEX1;
		break;

	case DSFMT_D16:
		glDepthFormat = GL_DEPTH_COMPONENT16;
		break;
#endif
	default:
		CCP_AL_LOGERR( "Unsupported depth stencil format %d", dsFormat );
		return E_INVALIDARG;
	}

	if( glDepthFormat )
	{
		GL_FAIL( glGenRenderbuffers( 1, &m_depthRenderBuffer ) );
		GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, m_depthRenderBuffer ) );
#ifndef TRINITY_AL_MOBILE
		if( msaa.samples > 1 )
		{
			GL_FAIL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa.samples, glDepthFormat, width, height ) );
		}
		else
#endif
		{
			GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glDepthFormat, width, height ) );
		}
	}

	if( separateStencilBuffer )
	{
		if( glStencilFormat )
		{
			GL_FAIL( glGenRenderbuffers( 1, &m_stencilRenderBuffer ) );
			GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, m_stencilRenderBuffer ) );
#ifndef TRINITY_AL_MOBILE
			if( msaa.samples > 1 )
			{
				GL_FAIL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa.samples, glStencilFormat, width, height ) );
			}
			else
#endif
			{
				GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glStencilFormat, width, height ) );
			}
		}
	}
	else
	{
		m_stencilRenderBuffer = m_depthRenderBuffer;
	}

	m_separateStencilBuffer = separateStencilBuffer;
	m_width  = width;
	m_height = height;
	m_format = dsFormat;
	m_msaa = msaa;
	ChangeObjectId();

	return S_OK;
}
	
bool Tr2DepthStencilAL::IsValid() const
{
	return m_depthRenderBuffer != 0 || m_readableDepth.IsValid();
}	

void Tr2DepthStencilAL::Destroy()
{
	if( m_depthRenderBuffer )
	{
		glDeleteRenderbuffers( 1, &m_depthRenderBuffer );
		m_depthRenderBuffer = 0;
	}
	if( m_separateStencilBuffer || m_stencilRenderBuffer )
	{
		glDeleteRenderbuffers( 1, &m_stencilRenderBuffer );
		m_stencilRenderBuffer = 0;
	}
	
	m_width			= 0;
	m_height		= 0;
	m_format		= DSFMT_UNKNOWN;
	m_msaa.samples = 0;
	m_msaa.quality	= 0;
	
	m_readableDepth.Destroy();
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	return m_readableDepth;
}

const Tr2TextureAL& Tr2DepthStencilAL::GetTexture() const
{
	return m_readableDepth;
}

bool Tr2DepthStencilAL::operator==( const Tr2DepthStencilAL& other ) const 
{ 
	return	m_depthRenderBuffer   == other.m_depthRenderBuffer && 
			m_stencilRenderBuffer == other.m_stencilRenderBuffer &&
			m_readableDepth.m_texture == other.m_readableDepth.m_texture;
}

uint32_t Tr2DepthStencilAL::GetWidth() const 
{ 
	return m_width; 
}

uint32_t Tr2DepthStencilAL::GetHeight() const 
{ 
	return m_height; 
}

const Tr2MsaaDesc& Tr2DepthStencilAL::GetMsaaDesc() const
{
	return m_msaa;
}

Tr2RenderContextEnum::DepthStencilFormat Tr2DepthStencilAL::GetFormat() const 
{ 
	return m_format; 
}

uintptr_t Tr2DepthStencilAL::GetSharedHandle() const 
{ 
	return 0; 
}

Tr2ALMemoryType Tr2DepthStencilAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

#endif
