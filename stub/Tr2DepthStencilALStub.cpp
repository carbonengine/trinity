#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2DepthStencilALStub.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_format( static_cast<DepthStencilFormat>( 0 ) ),
	m_width( 0 ),
	m_height( 0 )
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
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::ExFlag,
	Tr2RenderContextAL& renderContext )
{
	auto result = m_backingStore.Create2D( width, height, 1, PIXEL_FORMAT_R32_FLOAT, USAGE_CPU_READ, nullptr, renderContext );
	if( FAILED( result ) )
	{
		return result;
	}
	m_format = format;
	m_msaa = msaa;
	m_width = width;
	m_height = height;
	return S_OK;
}
	
bool Tr2DepthStencilAL::IsValid() const
{
	return m_backingStore.IsValid();
}	

void Tr2DepthStencilAL::Destroy()
{
	m_backingStore.Destroy();
	m_width = 0;
	m_height = 0;
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
		m_deviceLost.m_width		= GetWidth();
		m_deviceLost.m_height		= GetHeight();
		m_deviceLost.m_msaa = m_msaa;

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
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_format, d.m_msaa, EX_NONE, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

uint32_t Tr2DepthStencilAL::GetSharedHandle() const
{
	return 0;
}

uint32_t Tr2DepthStencilAL::GetWidth() const 
{ 
	return m_backingStore.GetWidth(); 
}

uint32_t Tr2DepthStencilAL::GetHeight() const 
{ 
	return m_backingStore.GetHeight(); 
}

const Tr2MsaaDesc& Tr2DepthStencilAL::GetMsaaDesc() const
{
	return m_msaa;
}

Tr2RenderContextEnum::DepthStencilFormat Tr2DepthStencilAL::GetFormat() const 
{ 
	return m_format; 
}

Tr2ALMemoryType Tr2DepthStencilAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

#endif
