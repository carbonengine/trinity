////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "Tr2Fsr3Upscaling.h"
#include "Tr2RenderContextAL.h"
#include "Tr2PrimaryRenderContextAL.h"
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

namespace Fsr3Utils
{
	void LogFsr3Message( FfxMsgType type, const wchar_t* message )
	{
		std::string m = std::string( "FSR3 Upscaling: " + std::string( CW2A( message ) ) );

		if( type == FFX_MESSAGE_TYPE_ERROR )
		{
			CCP_LOGERR( m.c_str() );
		}
		else if( type == FFX_MESSAGE_TYPE_WARNING )
		{
			CCP_LOGWARN( m.c_str() );
		}
	}

	FfxResource ConvertTextureToFfxResource( Tr2TextureAL* texture, const wchar_t* textureName, FfxResourceStates state )
	{
		ID3D12Resource* res = nullptr;
		if( texture && texture->IsValid() )
		{
			res = texture->TrinityALImpl_GetObject()->GetResourceDx12();
			res->SetName( textureName );
		}

		auto desc = GetFfxResourceDescriptionDX12( res );
		return ffxGetResourceDX12( res, desc, const_cast<wchar_t*>( textureName ), state );
	}

	FfxSurfaceFormat GetFfxSurfaceFormat( Tr2RenderContextEnum::PixelFormat format )
	{
		switch( format )
		{
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_TYPELESS ):
			return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_FLOAT ):
			return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16B16A16_FLOAT ):
			return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32_FLOAT ):
			return FFX_SURFACE_FORMAT_R32G32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8_UINT ):
			return FFX_SURFACE_FORMAT_R8_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32_UINT ):
			return FFX_SURFACE_FORMAT_R32_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_TYPELESS ):
			return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_UNORM ):
			return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB ):
			return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R11G11B10_FLOAT ):
			return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16_FLOAT ):
			return FFX_SURFACE_FORMAT_R16G16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16_UINT ):
			return FFX_SURFACE_FORMAT_R16G16_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_FLOAT ):
			return FFX_SURFACE_FORMAT_R16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_UINT ):
			return FFX_SURFACE_FORMAT_R16_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_UNORM ):
			return FFX_SURFACE_FORMAT_R16_UNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_SNORM ):
			return FFX_SURFACE_FORMAT_R16_SNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8_UNORM ):
			return FFX_SURFACE_FORMAT_R8_UNORM;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8_UNORM:
			return FFX_SURFACE_FORMAT_R8G8_UNORM;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32_FLOAT:
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_D32_FLOAT:
			return FFX_SURFACE_FORMAT_R32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_UNKNOWN ):
			return FFX_SURFACE_FORMAT_UNKNOWN;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_UINT:
			return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
		default:
			FFX_ASSERT_MESSAGE( false, "ValidationRemap: Unsupported format requested. Please implement." );
			return FFX_SURFACE_FORMAT_UNKNOWN;
		}
	}
}

Tr2Fsr3UpscalingTechnique::Tr2Fsr3UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( technique, setting, frameGeneration )
{
	SanitizeState();
}


bool Tr2Fsr3UpscalingTechnique::ReplacesSwapchain() const
{
	return m_frameGeneration;
}

void Tr2Fsr3UpscalingTechnique::ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue )
{
	// Create frameinterpolation swapchain
	auto dx12Swapchain = swapchain.Detach();
	FfxSwapchain ffxSwapChain = ffxGetSwapchainDX12( dx12Swapchain );

	// make sure swapchain is not holding a ref to real swapchain
	//GetFramework()->GetSwapChain()->GetImpl()->SetDXGISwapChain( nullptr );
	FfxCommandQueue ffxGameQueue = ffxGetCommandQueueDX12( commandQueue );
	dx12Swapchain->Release();

	auto result = ffxReplaceSwapchainForFrameinterpolationDX12(ffxGameQueue, ffxSwapChain);
	
	if( result != FFX_OK )
	{
		CCP_LOGERR( "Failed to replace DX12 swapchain with AMD frame interpolation swapchain. 0x%x", result );
		return;
	}

	// Set frameinterpolation swapchain to engine
	IDXGISwapChain4* frameinterpolationSwapchain = ffxGetDX12SwapchainPtr( ffxSwapChain );

	swapchain.Attach( frameinterpolationSwapchain );

	// In case the app is handling Alt-Enter manually we need to update the window association after creating a different swapchain
	IDXGIFactory7* factory = nullptr;
	if( SUCCEEDED( frameinterpolationSwapchain->GetParent( IID_PPV_ARGS( &factory ) ) ) )
	{
		factory->MakeWindowAssociation( hwnd, DXGI_MWA_NO_WINDOW_CHANGES );
		factory->Release();
	}
	// Framework swapchain adds to the refcount, so we need to release the swapchain here
	//frameinterpolationSwapchain->Release();

	CCP_LOGNOTICE( "Successfully replaced DX12 swapchain with AMD frame interpolation swapchain" );
}

Tr2Fsr3UpscalingTechnique::~Tr2Fsr3UpscalingTechnique()
{

}

void Tr2Fsr3UpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
	renderContext.FlushAndSyncDx12( );
	for( auto& item : m_contexts )
	{
		((Tr2Fsr3UpscalingContext*)item.second.get())->Destroy( renderContext );
	}
}

std::vector<Tr2UpscalingAL::Setting> Tr2Fsr3UpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::Setting::QUALITY,
		Tr2UpscalingAL::Setting::BALANCED,
		Tr2UpscalingAL::Setting::PERFORMANCE,
		Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
	};
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingTechnique::Setup()
{
	return Tr2UpscalingAL::Result::OK;
}

bool Tr2Fsr3UpscalingTechnique::SupportsFrameGeneration() const
{
	return true;
}

void Tr2Fsr3UpscalingTechnique::MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueAL::MarkFrameEvent( renderContext, frameEvent );
	if( m_frameGeneration && frameEvent == Tr2RenderContextEnum::FRAME_EVENT_PRESENT_STARTED )
	{
		for( auto& context : m_contexts )
		{
			( (Tr2Fsr3UpscalingContext*)context.second.get() )->GenerateFrame( renderContext );
		}
	}
}

Tr2UpscalingContextAL* Tr2Fsr3UpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
	return new Tr2Fsr3UpscalingContext( displayWidth, displayHeight, m_setting, m_frameGeneration, sourceFormat, depthFormat );
}	

Tr2Fsr3UpscalingContext::Tr2Fsr3UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) :
	Tr2UpscalingContextAL( displayWidth, displayHeight, setting, frameGeneration, sourceFormat, depthFormat ),
	m_setup( false )
{
	switch( setting )
	{
	case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
		m_upscaling = 1.0f;
		break;
	case Tr2UpscalingAL::Setting::QUALITY:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_QUALITY );
		break;
	case Tr2UpscalingAL::Setting::BALANCED:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_BALANCED );
		break;
	case Tr2UpscalingAL::Setting::PERFORMANCE:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_PERFORMANCE );
		break;
	case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_ULTRA_PERFORMANCE );
		break;
	default:
		CCP_LOGERR( "Invalid upscaling setting applied: %d. Upscaling amount will be forced to 1.0" );
	}

	m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
	m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );
}

Tr2Fsr3UpscalingContext::~Tr2Fsr3UpscalingContext()
{
}

void Tr2Fsr3UpscalingContext::Destroy( Tr2RenderContextAL& renderContext )
{
	if( m_setup )
	{
		renderContext.FlushAndSyncDx12( );
		if( m_frameGeneration )
		{
			FfxSwapchain ffxSwapChain = ffxGetSwapchainDX12( renderContext.GetPrimaryRenderContextPointer()->m_swapchain );
			ffxWaitForPresents( ffxSwapChain );
			// disable frame generation before destroying context
			// also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
			m_frameGenerationConfig.frameGenerationEnabled = false;
			m_frameGenerationConfig.swapChain = ffxSwapChain;
			m_frameGenerationConfig.presentCallback = nullptr;
			m_frameGenerationConfig.HUDLessColor = FfxResource( {} );
			ffxFsr3ConfigureFrameGeneration( &m_context, &m_frameGenerationConfig );
			ffxRegisterFrameinterpolationUiResourceDX12( ffxSwapChain, FfxResource( {} ) );
		}

		auto errorCode = ffxFsr3ContextDestroy( &m_context );
		if( errorCode != FFX_OK )
		{
			CCP_LOGERR( "FSR3 could not clear the context %d", errorCode );
		}

		for( auto& buffer : m_ffxFsr3Backends )
		{
			CCP_FREE( buffer.scratchBuffer );
			buffer.scratchBuffer = nullptr;
		}
	}
}

bool Tr2Fsr3UpscalingContext::IsTemporal() const
{
	return true;
}

void Tr2Fsr3UpscalingContext::Tr2Fsr3UpscalingContext::UpdateJitter()
{
	m_jitterX = 0.0f;
	m_jitterY = 0.0f;
	++m_jitterIndex;
	const int32_t jitterPhaseCount = ffxFsr3UpscalerGetJitterPhaseCount( m_renderWidth, m_displayWidth );
	ffxFsr3UpscalerGetJitterOffset( &m_jitterX, &m_jitterY, m_jitterIndex, jitterPhaseCount );
	m_jitterIndex = m_jitterIndex % jitterPhaseCount;
}

uint32_t Tr2Fsr3UpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | Tr2UpscalingAL::DispatchRequirements::VELOCITY | Tr2UpscalingAL::DispatchRequirements::REACTIVE;
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::Setup( Tr2RenderContextAL& renderContext )
{
	m_reset = true;

	auto device = ffxGetDeviceDX12( renderContext.GetPrimaryRenderContext().m_device );

	// Setup DX12 interface.
	FfxErrorCode errorCode = 0;

	int effectCounts[] = { 1, 1, 2 };
	std::vector names{ "FSR3_BACKEND_SHARED_RESOURCES", "FSR3_BACKEND_UPSCALING", "FSR3_BACKEND_FRAME_INTERPOLATION" };
	for( auto i = 0; i < FSR3_BACKEND_COUNT; i++ )
	{
		const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12( effectCounts[i] );
		void* scratchBuffer = CCP_CALLOC( names[i], scratchBufferSize, 1 );
		memset( scratchBuffer, 0, scratchBufferSize );
		errorCode |= ffxGetInterfaceDX12( &m_ffxFsr3Backends[i], device, scratchBuffer, scratchBufferSize, effectCounts[i] );
	}

	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 setup could not create interface %d", errorCode );
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	m_initializationParameters.backendInterfaceSharedResources = m_ffxFsr3Backends[FSR3_BACKEND_SHARED_RESOURCES];
	m_initializationParameters.backendInterfaceUpscaling = m_ffxFsr3Backends[FSR3_BACKEND_UPSCALING];
	m_initializationParameters.backendInterfaceFrameInterpolation = m_ffxFsr3Backends[FSR3_BACKEND_FRAME_INTERPOLATION];
	m_initializationParameters.backBufferFormat = Fsr3Utils::GetFfxSurfaceFormat( m_sourceFormat );

	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.upscaleOutputSize.width = m_displayWidth;
	m_initializationParameters.upscaleOutputSize.height = m_displayHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR3_ENABLE_DEPTH_INVERTED;
	m_initializationParameters.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;

	m_initializationParameters.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
	m_initializationParameters.fpMessage = &Fsr3Utils::LogFsr3Message;

	errorCode = ffxFsr3ContextCreate( &m_context, &m_initializationParameters );
	m_setup = errorCode == FFX_OK;
	if( !m_setup )
	{
		CCP_LOGERR( "FSR3 setup could not create context %d", errorCode );
		Destroy( renderContext );
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	if( m_frameGeneration )
	{
		auto ffxSwapChain = ffxGetSwapchainDX12( renderContext.GetPrimaryRenderContextPointer()->m_swapchain );

		// configure frame generation
		// FfxResourceDescription hudLessDesc = GetFfxResourceDescription( m_pHudLessTexture[m_curUiTextureIndex]->GetResource(), (FfxResourceUsage)0 );
		// FfxResource hudLessResource = ffxGetResourceDX12( m_pHudLessTexture[m_curUiTextureIndex]->GetResource()->GetImpl()->DX12Resource(), hudLessDesc, L"FSR3_HudLessBackbuffer", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );

		m_frameGenerationConfig.frameGenerationEnabled = true;
		m_frameGenerationConfig.flags = 0;
		m_frameGenerationConfig.flags |= FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_TEAR_LINES;// | FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_VIEW;
		m_frameGenerationConfig.onlyPresentInterpolated = true;
		m_frameGenerationConfig.HUDLessColor = FfxResource( {} );
		m_frameGenerationConfig.allowAsyncWorkloads = false;
		m_frameGenerationConfig.frameGenerationCallback = ffxFsr3DispatchFrameGeneration;
		m_frameGenerationConfig.swapChain = ffxSwapChain;
	}

	return Tr2UpscalingAL::Result::OK;
}

void Tr2Fsr3UpscalingContext::GenerateFrame( Tr2RenderContextAL& renderContext )
{
	if( !m_setup || m_reset )
	{
		return;
	}
	renderContext.FlushBarriersDx12();
	renderContext.DirtyDescriptorCache();
	FfxErrorCode errorCode = ffxFsr3ConfigureFrameGeneration( &m_context, &m_frameGenerationConfig );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 setup could not configure frame generation 0x%x", errorCode );
		m_frameGeneration = false;
		return;
	}
	CCP_LOGERR( "FSR3 configured frame gen" );

	renderContext.DirtyDescriptorCache();
}


Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}
	if( !AreDisplayParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	if( dispatchParameters.frontClip + dispatchParameters.backClip + dispatchParameters.aspectRatio == 0.0 )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	// flush all barriers before handing control over to FSR3

	GPU_REGION_AL( renderContext, "FSR3 Upscaling" );

	FfxFsr3DispatchUpscaleDescription dispatchDescription = {};
	dispatchDescription.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchDescription.color = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.input, L"FSR2_InputColor", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.depth = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.depth, L"FSR2_InputDepth", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.motionVectors = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.velocity, L"FSR2_InputMotionVectors", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.exposure = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.exposure, L"FSR2_InputExposure", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.reactive = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.reactive, dispatchParameters.reactive != nullptr ? L"FSR2_InputReactiveMap" : L"FSR2_EmptyInputReactiveMap", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.transparencyAndComposition = Fsr3Utils::ConvertTextureToFfxResource( nullptr, L"FSR3_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );

	dispatchDescription.upscaleOutput = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.output, L"FSR3_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchDescription.jitterOffset.x = m_jitterX;
	dispatchDescription.jitterOffset.y = m_jitterY;
	dispatchDescription.motionVectorScale.x = (float)m_renderWidth;
	dispatchDescription.motionVectorScale.y = (float)m_renderHeight;
	dispatchDescription.reset = m_reset;
	dispatchDescription.enableSharpening = true;
	dispatchDescription.sharpness = 0.8f;
	dispatchDescription.frameTimeDelta = dispatchParameters.frameTimeDelta;

	dispatchDescription.preExposure = dispatchParameters.preExposure;
	dispatchDescription.renderSize.width = m_renderWidth;
	dispatchDescription.renderSize.height = m_renderHeight;
	dispatchDescription.cameraFar = dispatchParameters.frontClip; 
	dispatchDescription.cameraNear = dispatchParameters.backClip; 
	dispatchDescription.cameraFovAngleVertical = dispatchParameters.fieldOfView;
	dispatchDescription.viewSpaceToMetersFactor = 1.0f;

	renderContext.FlushBarriersDx12();
	FfxErrorCode errorCode = ffxFsr3ContextDispatchUpscale( &m_context, &dispatchDescription );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 error while dispatching %d", errorCode );
	}

	m_reset = false;

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
	CCP_LOGERR( "FSR3 dispatched" );

	return Tr2UpscalingAL::Result::OK;
}

#endif