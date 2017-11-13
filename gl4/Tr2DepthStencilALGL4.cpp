#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2DepthStencilALGL4.h"
#include "Tr2RenderContextGL4.h"
#include "ALLog.h"

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
	Tr2RenderContextEnum::ExFlag flag,
	Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( flag != EX_NONE )
	{
		return E_INVALIDARG;
	}
    
	if( dsFormat == DSFMT_READABLE )
	{
		GLuint stencilRenderBuffer = 0;
		CR_RETURN_HR( m_readableDepth.CreateDepthTexture( width, height, renderContext ) );

		m_width  = width;
		m_height = height;
		m_format = dsFormat;
		m_msaa = msaa;
		ChangeObjectId();

		return S_OK;
	}
	
	GLenum glDepthFormat = 0, glStencilFormat = 0;
	
	switch( dsFormat )
	{
	case DSFMT_D24S8:
	case DSFMT_D24FS8:
	case DSFMT_D24X8:
	case DSFMT_AUTO:
		glDepthFormat = GL_DEPTH24_STENCIL8;
		break;
	
	case DSFMT_D32F: 
	case DSFMT_D32:
	case DSFMT_D32F_LOCKABLE:
        glDepthFormat = GL_DEPTH_COMPONENT32;
		break;

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
	default:
		CCP_AL_LOGERR( "Unsupported depth stencil format %d", dsFormat );
		return E_INVALIDARG;
	}

	if( glDepthFormat )
	{
		GL_FAIL( glGenRenderbuffers( 1, &m_depthRenderBuffer ) );
		GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, m_depthRenderBuffer ) );
		GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glDepthFormat, width, height ) );
	}

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
