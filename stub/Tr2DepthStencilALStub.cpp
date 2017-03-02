#include "StdAfx.h"
#include "Tr2DepthStencilALStub.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
	, m_msaaType( 0 )
	, m_msaaQuality( 0 )
	, m_isValid( false )
{
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
}

ALResult Tr2DepthStencilAL::Create( 
	uint32_t width, 
	uint32_t height, 
	Tr2RenderContextEnum::DepthStencilFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	Tr2RenderContextAL& renderContext )
{
	return CreateEx( width, height, format, msaaType, msaaQuality, 0, renderContext );
}

ALResult Tr2DepthStencilAL::CreateEx(	
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
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
	m_width = width;
	m_height = height;
	m_format = dsFormat;
	m_msaaQuality = msaaQuality;
	m_msaaType = msaaType;
	m_isValid = true;
	
	if( dsFormat == DSFMT_READABLE )
	{
		m_backingStore.Create2D(width, height, 1, PIXEL_FORMAT_A8_UNORM, 0, 0, renderContext);
	}

	return S_OK;
}
	
bool Tr2DepthStencilAL::IsValid() const
{
	return m_isValid;
}	

void Tr2DepthStencilAL::Destroy()
{
	m_isValid = false;
}

bool Tr2DepthStencilAL::IsReadable() const
{
	return m_format == DSFMT_READABLE && m_isValid;
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	return m_backingStore;
}

const Tr2TextureAL& Tr2DepthStencilAL::GetTexture() const
{
	return m_backingStore;
}

void Tr2DepthStencilAL::ReleaseALResource()
{
	if( !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

void Tr2DepthStencilAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
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
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_format, d.m_msaaType, d.m_msaaQuality, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
			m_isValid = true;
		}
	}
}

uint32_t Tr2DepthStencilAL::GetSharedHandle() const
{
	return 0;
}

#endif
