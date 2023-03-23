#include "StdAfx.h"
#include "WithRenderContextFixture.h"
#include <ITr2RenderContextEvents.h>

struct RenderContextCreation: public WithRenderContext {};

namespace
{
void SetUpPresentParameters( Tr2PresentParametersAL& presentParameters )
{
	Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, presentParameters.mode );
	presentParameters.mode.width = 1920;
	presentParameters.mode.height = 1080;
	presentParameters.backBufferCount = 1;
	presentParameters.msaaType = 1;
	presentParameters.msaaQuality = 0;
	presentParameters.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	presentParameters.outputWindow = WithWindow::GetWindowHandle();
	presentParameters.windowed = true;
	presentParameters.software = false;
	presentParameters.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE;
}

struct RenderContextEvents: public ITr2RenderContextEvents
{
	RenderContextEvents()
	{
		Reset();
	}

	void Reset()
	{
		m_timesOnContextCreatedCalled = 0;
		m_timesOnTextureUnsetCalled = 0;
	}

	void OnContextCreated( Tr2PrimaryRenderContextAL& renderContext )
	{
		m_timesOnContextCreatedCalled++;
	}

	void OnTextureUnset( const Tr2TextureAL& texture, Tr2RenderContextAL& renderContext )
	{
		m_timesOnTextureUnsetCalled++;
	}

	uint32_t m_timesOnContextCreatedCalled;
	uint32_t m_timesOnTextureUnsetCalled;
};

}

TEST_F( RenderContextCreation, RenderContextStartsAsInvalid )
{
	EXPECT_FALSE( renderContext->IsValid() );
}

TEST_F( RenderContextCreation, CanCreateRenderContext )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) ); 
}

TEST_F( RenderContextCreation, RenderContextIsValidAfterCreation )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) ); 
	EXPECT_TRUE( renderContext->IsValid() ); 
}

TEST_F( RenderContextCreation, RenderContextIsInvalidAfterDestroy )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) ); 
	renderContext->Destroy();
	EXPECT_FALSE( renderContext->IsValid() ); 
}

TEST_F( RenderContextCreation, CanChangeRenderContextPresentParameters )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) ); 
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetPresentParameters( 0, presentParameters ) );
}

TEST_F( RenderContextCreation, CreateDeviceCallsEventsOnContextCreated )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	RenderContextEvents events;

	renderContext->m_events = &events;
	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) ); 
	EXPECT_EQ( 1, events.m_timesOnContextCreatedCalled ); 
	renderContext->m_events = nullptr;
}

TEST_F( RenderContextCreation, RenderContextBackBufferWithCorrectDimensionsAfterCreation )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );
	EXPECT_TRUE( renderContext->GetDefaultBackBuffer().IsValid() );
	EXPECT_EQ( presentParameters.mode.width, renderContext->GetDefaultBackBuffer().GetWidth() );
	EXPECT_EQ( presentParameters.mode.height, renderContext->GetDefaultBackBuffer().GetHeight() );
}

TEST_F( RenderContextCreation, RenderContextBackBufferWithCorrectDimensionsAfterReset )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );
	presentParameters.mode.width += 50;
	presentParameters.mode.height += 50;
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetPresentParameters( 0, presentParameters ) );
	EXPECT_TRUE( renderContext->GetDefaultBackBuffer().IsValid() );
	EXPECT_EQ( presentParameters.mode.width, renderContext->GetDefaultBackBuffer().GetWidth() );
	EXPECT_EQ( presentParameters.mode.height, renderContext->GetDefaultBackBuffer().GetHeight() );
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

TEST_F( RenderContextCreation, CanCreateBC7TexturesWithSoftwareContext )
{
	Tr2PresentParametersAL presentParameters;
	SetUpPresentParameters( presentParameters );
	presentParameters.software = true;
	presentParameters.outputWindow = 0;

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, 0, presentParameters ) );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 64, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

#endif