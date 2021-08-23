#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2SwapChainALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "ALLog.h"

namespace TrinityALImpl
{

	Tr2SwapChainAL::Tr2SwapChainAL()
		: m_windowHandle( Tr2WindowHandle() )
	{
		m_backBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	}

	ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		NSView* view = (NSView *)windowHandle;
		if( !view.layer || ![view.layer isKindOfClass:CAMetalLayer.class])
		{
			return E_INVALIDARG;
		}
		Destroy();

		uint32_t width = 0;
		uint32_t height = 0;
		Tr2RenderContextEnum::PixelFormat pixelFormat = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;

		Tr2PresentParametersAL* presentParameters = renderContext.GetPresentParamaters();
		if( windowHandle == presentParameters->outputWindow )
		{
			width = presentParameters->mode.width;
			height = presentParameters->mode.height;
			pixelFormat = presentParameters->mode.format;
		}
		else
		{
			auto layer = (CAMetalLayer*)view.layer;
			auto scale = layer.contentsScale;
			layer.drawableSize = CGSizeMake( layer.bounds.size.width * scale, layer.bounds.size.height * scale );
			width = layer.drawableSize.width;
			height = layer.drawableSize.height;
			pixelFormat = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
		}

		Tr2MsaaDesc msaaDesc = Tr2MsaaDesc( presentParameters->msaaType, presentParameters->msaaQuality );
		Tr2BitmapDimensions textureInfo = Tr2BitmapDimensions( width, height, 1, pixelFormat );

		Tr2GpuUsage::Type gpuUsage = Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE;
		// Have to make this READ to get the screeshot facility to work
		Tr2CpuUsage::Type cpuUsage = Tr2CpuUsage::READ;

		CR_RETURN_HR( m_backBuffer.m_texture->Create( textureInfo, msaaDesc, gpuUsage, cpuUsage, nil, renderContext ) );

		m_windowHandle           = windowHandle;

		return S_OK;
	}

	void Tr2SwapChainAL::Destroy()
	{
		m_backBuffer.m_texture->Destroy();
	}

	bool Tr2SwapChainAL::IsValid() const
	{
		return m_backBuffer.IsValid();
	}

	ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
	{
		MetalContext* metalContext = renderContext.GetMetalContext();
		NSView* view = (NSView*)m_windowHandle;
		id<MTLTexture> backBufferTexture = m_backBuffer.m_texture->GetMetalTexture();
		metalContext->BlitToDrawableAndPresent( backBufferTexture, view );

		GetNextBackbuffer();

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

	void Tr2SwapChainAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2SwapChainAL";
	}

	void Tr2SwapChainAL::GetNextBackbuffer()
	{
	}
}

#endif // TRINITY_PLATFORM == TRINITY_METAL
