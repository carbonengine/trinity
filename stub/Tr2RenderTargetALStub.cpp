#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2RenderTargetALStub.h"
#include "Tr2RenderContextStub.h"

using namespace Tr2RenderContextEnum;

#include "ALLog.h"

Tr2RenderTargetAL::Tr2RenderTargetAL()
{
	Destroy();
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

void Tr2RenderTargetAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();

	m_msaa.samples = 1;
	m_msaa.quality = 0;

	m_backingStore.Destroy();
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
}

bool Tr2RenderTargetAL::IsValid() const
{
	return m_backingStore.IsValid();
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2RenderContextEnum::ExFlag,
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
	auto result = m_backingStore.Create2D( width, height, mipLevelCount, format, usage, nullptr, renderContext );
	if( SUCCEEDED( result ) )
	{ 
		static_cast<Tr2BitmapDimensions&>( *this ) = m_backingStore;
		m_msaa = msaa;
	}
	return result;
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

uintptr_t Tr2RenderTargetAL::GetSharedHandle() const
{
	return 0;
}

Tr2MsaaDesc Tr2RenderTargetAL::GetMsaaDesc() const
{
	return m_msaa;
}

Tr2ALMemoryType Tr2RenderTargetAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

#endif // ( TRINITY_PLATFORM==TRINITY_STUB )
