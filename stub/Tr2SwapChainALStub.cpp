#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_STUB

#include "Tr2SwapChainALStub.h"
#include "Tr2RenderContextStub.h"
#include "ALLog.h"


Tr2SwapChainAL::Tr2SwapChainAL()
	:m_windowHandle( Tr2WindowHandle() )
{
}

void Tr2SwapChainAL::ReleaseALResource()
{
}

void Tr2SwapChainAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
}

ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext )
{
	if ( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	Destroy();
	return m_backBuffer.Create( 
		4, 
		4, 
		1, 
		Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM, 
		Tr2MsaaDesc(),
		0,
		Tr2RenderContextEnum::EX_NONE,
		renderContext );
}

void Tr2SwapChainAL::Destroy()
{
	m_backBuffer.Destroy();
}

bool Tr2SwapChainAL::IsValid() const
{
	return m_backBuffer.IsValid();
}

ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
	return S_OK;
}

int Tr2SwapChainAL::GetWidth() const
{
	return m_backBuffer.GetWidth();
}

int Tr2SwapChainAL::GetHeight() const
{
	return m_backBuffer.GetHeight();
}
#endif // TRINITY_PLATFORM==TRINITY_STUB
