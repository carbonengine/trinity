// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2SwapChainALStub.h"
#include "Tr2RenderContextStub.h"
#include "ALLog.h"

namespace TrinityALImpl
{

Tr2SwapChainAL::Tr2SwapChainAL() :
	m_windowHandle( Tr2WindowHandle() )
{
}

ALResult Tr2SwapChainAL::Create( Tr2WindowHandle, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	Destroy();
	return m_backBuffer.Create(
		Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM ),
		Tr2GpuUsage::RENDER_TARGET,
		renderContext );
}

void Tr2SwapChainAL::Destroy()
{
	m_backBuffer = ::Tr2TextureAL();
}

bool Tr2SwapChainAL::IsValid() const
{
	return m_backBuffer.IsValid();
}

ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& )
{
	return S_OK;
}

uint32_t Tr2SwapChainAL::GetWidth() const
{
	return m_backBuffer.GetWidth();
}

uint32_t Tr2SwapChainAL::GetHeight() const
{
	return m_backBuffer.GetHeight();
}

void Tr2SwapChainAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
{
}

ALResult Tr2SwapChainAL::SetName( const char* )
{
	return S_OK;
}

}

#endif // TRINITY_PLATFORM==TRINITY_STUB
