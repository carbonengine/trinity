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
