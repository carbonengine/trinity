#include "StdAfx.h"
#include "Tr2RenderTargetALStub.h"

using namespace Tr2RenderContextEnum;

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

#pragma warning( disable: 4189 )	// Scopeguard

Tr2RenderTargetAL::Tr2RenderTargetAL()
	:m_isValid(false)
{
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
	m_format = PIXEL_FORMAT_UNKNOWN;
	m_mipCount = 1;
}

void Tr2RenderTargetAL::Destroy()
{
	m_isValid = false;
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
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_mipCount, d.m_format, d.m_msaaType, d.m_msaaQuality, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
			m_isValid = true;
		}
	}
}

void Tr2RenderTargetAL::ReleaseALResource()
{
	if( !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_mipCount		= m_mipCount;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}
	Destroy();
}

Tr2RenderTargetAL::~Tr2RenderTargetAL()
{
}

bool Tr2RenderTargetAL::IsValid() const
{
	return m_isValid;
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextAL& renderContext )
{
	return Create( width, height, mipLevelCount, format, 1, 0, renderContext );
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality,
	Tr2RenderContextAL& renderContext )
{
	return CreateEx( width, height, mipLevelCount, format, msaaType, msaaQuality, 0, 0, renderContext );
}

ALResult Tr2RenderTargetAL::CreateEx(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality,
	Tr2RenderContextEnum::BufferUsage usage,
	uint32_t flags,
	Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( width == 0 || height == 0 )
	{
		return E_INVALIDARG;
	}
	m_isValid = true;
	m_width = width;
	m_height = height;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_msaaType = msaaType;
	m_msaaQuality = msaaQuality;
	m_usage = usage;
	m_backingStore.Create2D(width, height, mipLevelCount, format, usage, nullptr, renderContext);
	return S_OK;
}



ALResult Tr2RenderTargetAL::Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext )
{
	if( !destination.IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2RenderTargetAL::CopySubresourceRegion(	
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{	
	return S_OK;
}

ALResult Tr2RenderTargetAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
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

ALResult Tr2RenderTargetAL::GetLockedRenderTarget( uint32_t mipLevel, uint32_t* ltrb, Tr2LockedRenderTargetAL& lockedRT, Tr2RenderContextAL& renderContext )
{
	if ( ltrb )
	{
		for ( int i = 0; i < 4; ++i )
		{
			lockedRT.m_lockedRect[i] = ltrb[i];
		}
	}
	else
	{
		
		lockedRT.m_lockedRect[0] = 0;
		lockedRT.m_lockedRect[1] = 0;
		lockedRT.m_lockedRect[2] = m_width;
		lockedRT.m_lockedRect[3] = m_height;
	}
	lockedRT.m_hasLockedRect = true;
	uint32_t width = (lockedRT.m_lockedRect[2] - lockedRT.m_lockedRect[0]);
	uint32_t height = (lockedRT.m_lockedRect[3] - lockedRT.m_lockedRect[1]);
	uint32_t pitch;
	if ( IsCompressed() )
	{
		pitch = m_width / 16;
	}
	else
	{
		pitch = m_width * GetBytesPerPixel( m_format );
	}

	lockedRT.m_pitch = pitch;
	int bpp = Tr2RenderContextEnum::GetBytesPerPixel( GetFormat() );
	size_t newSize = width * height * bpp;
	lockedRT.m_data.resize("Tr2LockedRenderTargetAL::m_buffer", newSize);
	return S_OK;
}

ALResult Tr2RenderTargetAL::Lock(	
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL& renderContext )
{
	return m_backingStore.Lock(mipLevel, ltrb, data, pitch, LOCK_WRITEONLY, renderContext);
}

ALResult Tr2RenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return m_backingStore.Unlock( renderContext );
}


void Tr2RenderTargetAL::SetHintLockOften()
{
}

uint32_t Tr2RenderTargetAL::GetSharedHandle() const
{
	return 0;
}

#endif // ( TRINITY_PLATFORM==TRINITY_STUB )
