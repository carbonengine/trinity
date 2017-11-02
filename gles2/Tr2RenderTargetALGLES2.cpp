#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )
#include "Tr2RenderTargetALGLES2.h"
#include "Tr2RenderContextGLES2.h"

using namespace Tr2RenderContextEnum;

#include "ALLog.h"

Tr2RenderTargetAL::Tr2RenderTargetAL()
	:m_lockedOften( false ),
	m_isLocked( false ),
	m_msaaTarget( 0 )
{
	Destroy();
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

void Tr2RenderTargetAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();

	if( m_msaaTarget )
	{
		glDeleteRenderbuffers( 1, &m_msaaTarget );
		m_msaaTarget = 0;
	}

	m_msaa.samples = 0;
	m_msaa.quality = 0;
	
	m_backingStore.Destroy();

	CcpMallocBuffer empty;
	m_lockedData.swap( empty );
	m_isLocked = false;
}

void Tr2RenderTargetAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
	if( m_deviceLost.m_valid )
	{
		if( IsValid() )
		{
			m_deviceLost.m_valid = false;
		}
		else
		{
			const auto &d = m_deviceLost;

			if( d.m_width > 0 && d.m_height > 0 &&
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_mipCount, d.m_format, d.m_msaa, 0, EX_NONE, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

void Tr2RenderTargetAL::ReleaseALResource()
{
	if( !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format = m_format;
		m_deviceLost.m_width = m_width;
		m_deviceLost.m_height = m_height;
		m_deviceLost.m_mipCount = m_mipCount;
		m_deviceLost.m_msaa = m_msaa;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

Tr2RenderTargetAL::~Tr2RenderTargetAL()
{
	Destroy();
}

bool Tr2RenderTargetAL::IsValid() const
{
	return m_msaaTarget != 0 || m_backingStore.IsValid();
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::BufferUsage,
	Tr2RenderContextEnum::ExFlag,
	Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( msaa.samples > 1 )
	{
#if defined(TRINITY_AL_MOBILE)
        CCP_AL_LOGERR( "Tr2RenderTargetAL::Create: MSAA is not supported" );
        return E_FAIL;
#else
		GLenum internalFormat, targetFormat, targetType;
		if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, internalFormat, targetFormat, targetType ) )
		{
			return E_INVALIDARG;
		}

		CR_GL( glGenRenderbuffers( 1, &m_msaaTarget ) );
		CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, m_msaaTarget ) );
		CR_GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, msaa.samples, internalFormat, width, height ) );
		CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

		m_width = width;
		m_height = height;
		m_mipCount = 1;
		m_format = format;
		m_type = TEX_TYPE_2D;
#endif
	}
	else
	{
		CR_RETURN_HR( m_backingStore.Create2D( width, height, mipLevelCount, format, 0, nullptr, renderContext ) );
		float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
		Tr2SamplerStateAL::CreateStateData(
			Tr2SamplerDescription( TF_LINEAR,
				TF_LINEAR,
				TF_LINEAR,
				false,
				TA_CLAMP,
				TA_CLAMP,
				TA_CLAMP,
				0,
				0,
				CMP_ALWAYS,
				borderColor,
				0,
				1 ), m_backingStore.m_currentSampler );
		Tr2SamplerStateAL::Apply( GL_TEXTURE_2D, mipLevelCount != 1, m_backingStore.m_currentSampler );
		static_cast<Tr2BitmapDimensions&>(*this) = static_cast<Tr2BitmapDimensions&>( m_backingStore );
	}

	m_msaa = msaa;
	ChangeObjectId();
	
	return S_OK;
}

ALResult Tr2RenderTargetAL::Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return S_OK;
	}

	return renderContext.InternalResolveRT( destination, *this );
}

bool Tr2RenderTargetAL::operator==( const Tr2RenderTargetAL& other ) const 
{ 
	return m_backingStore == other.m_backingStore; 
}

ALResult Tr2RenderTargetAL::CopySubresourceRegion( 
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{
#pragma warning( disable: 4189 )	// scopeguard

	if( !IsValid() || !source.IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	int x, y, width, height;
	if( ltrb )
	{
		x = ltrb[0];
		y = ltrb[1];
		width = ltrb[2] - x;
		height = ltrb[3] - y;
	}
	else
	{
		x = 0;
		y = 0;
		width = source.m_width;
		height = source.m_height;
	}
	if( width <= 0 || height <= 0 )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );

	renderContext.PushRenderTarget();
	renderContext.PushDepthStencil();
	ON_BLOCK_EXIT( 
		[&]{ 
			renderContext.PopDepthStencil(); 
			renderContext.PopRenderTarget(); 
		} );
	renderContext.SetRenderTarget( source );
	renderContext.SetDepthStencil( nullDS );

	GL_FAIL( glBindTexture( GL_TEXTURE_2D, *m_backingStore.m_texture ) );
	GL_FAIL( glCopyTexSubImage2D( GL_TEXTURE_2D, 0, destX, destY, x, y, width, height ) );

	return S_OK;
}

ALResult Tr2RenderTargetAL::GenerateMipMaps( Tr2RenderContextAL& /*renderContext*/ )
{
	if( m_mipCount == 1 )
	{
		return S_OK;
	}

	if( !IsValid() )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	GL_FAIL( glBindTexture( GL_TEXTURE_2D, *m_backingStore.m_texture ) );
#if !defined(TRINITY_AL_MOBILE)
	GL_IGNORE_ERROR( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT  ) );
#endif
	GL_FAIL( glGenerateMipmap( GL_TEXTURE_2D ) );

	return S_OK;
}

Tr2TextureAL& Tr2RenderTargetAL::GetTexture()
{ 
	return m_backingStore; 
}

const Tr2TextureAL& Tr2RenderTargetAL::GetTexture() const
{ 
	return m_backingStore; 
}

ALResult Tr2RenderTargetAL::GetRenderTargetData( uint32_t* ltrb, CcpMallocBuffer& buffer, uint32_t& pitch, Tr2RenderContextAL& renderContext )
{
	int x, y, width, height;
	if( ltrb )
	{
		x = ltrb[0];
		y = ltrb[3];
		width = ltrb[2] - x;
		height = y - ltrb[1];
	}
	else
	{
		x = 0;
		y = 0;
		width = m_width;
		height = m_height;
	}
	if( width <= 0 || height <= 0 )
	{
		return E_INVALIDARG;
	}

	renderContext.PushRenderTarget();
	renderContext.PushDepthStencil();
	ON_BLOCK_EXIT( 
		[&]{ 
			renderContext.PopDepthStencil(); 
			renderContext.PopRenderTarget(); 
		} );
	renderContext.SetRenderTarget( *this );
	renderContext.SetDepthStencil( nullDS );

	int bpp = Tr2RenderContextEnum::GetBytesPerPixel( GetFormat() );
	size_t newSize = width * height * bpp;
	if( buffer.empty() || buffer.size() < newSize )
	{
		buffer.resize( "Tr2RenderTargetAL::m_lockedData", newSize );
	}
	GLenum internalFormat, format, type;
	Tr2RenderContextAL::ConvertToGLPixelFormat( GetFormat(), internalFormat, format, type );
	GL_FAIL( glReadPixels( x, y, width, height, format, type, buffer.get() ) );
	pitch = width * bpp;
	return S_OK;
}

ALResult Tr2RenderTargetAL::Lock(	
	uint32_t /*mipLevel*/, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL& renderContext )
{	
	if( !m_backingStore.IsValid() || m_isLocked )
	{
		return E_FAIL;
	}

	CR_RETURN_HR( GetRenderTargetData( ltrb, m_lockedData, pitch, renderContext ) );

	data = m_lockedData.get();
	m_isLocked = true;
	return S_OK;
}

ALResult Tr2RenderTargetAL::Unlock( Tr2RenderContextAL& /*renderContext*/ )
{
	if( !m_lockedOften )
	{
		CcpMallocBuffer empty;
		m_lockedData.swap( empty );
	}
	m_isLocked = false;
	return S_OK;
}

void Tr2RenderTargetAL::SetHintLockOften()
{
	m_lockedOften = true;
}

Tr2MsaaDesc Tr2RenderTargetAL::GetMsaaDesc() const
{
	return m_msaa;
}

uintptr_t Tr2RenderTargetAL::GetSharedHandle() const
{ 
	return 0; 
}

Tr2ALMemoryType Tr2RenderTargetAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

#endif // ( TRINITY_PLATFORM==TRINITY_OPENGLES2 )
