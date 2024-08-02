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

extern bool g_upscalingDebug;

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

	FfxResource ConvertTextureToFfxResource( Tr2TextureAL* texture, const wchar_t* textureName )
	{
		ID3D12Resource* res = nullptr;
		if( texture && texture->IsValid() )
		{
			res = texture->TrinityALImpl_GetObject()->GetResourceDx12();
			res->SetName( textureName );
		}

		auto desc = GetFfxResourceDescriptionDX12( res );
		return ffxGetResourceDX12( res, desc, const_cast<wchar_t*>( textureName ), FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
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

Tr2Fsr3UpscalingTechnique::Tr2Fsr3UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter ),
	m_framegenSwapchain( nullptr ), 
	m_attachedToSwapchain( false ),
	m_supportsFrameGeneration( true )
{
	OSVERSIONINFOEX osvi = {};
	osvi.dwMajorVersion = 10;
	osvi.dwBuildNumber = 19041; // Required for ID3D12Device8 used by FSR3

	DWORDLONG conditionMask = 0;
	VER_SET_CONDITION( conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL );
	m_supportsFrameGeneration = VerifyVersionInfo( &osvi, VER_MAJORVERSION | VER_BUILDNUMBER, conditionMask );
	SanitizeState();
}

bool Tr2Fsr3UpscalingTechnique::ReplacesSwapchain() const
{
	return m_frameGeneration && m_supportsFrameGeneration;
}

void Tr2Fsr3UpscalingTechnique::ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue )
{
	// Create frameinterpolation swapchain
	m_framegenSwapchain = ffxGetSwapchainDX12( swapchain.Detach() );

	auto result = ffxReplaceSwapchainForFrameinterpolationDX12( ffxGetCommandQueueDX12( commandQueue ), m_framegenSwapchain );
	
	if( result != FFX_OK )
	{
		swapchain.Attach( ffxGetDX12SwapchainPtr( m_framegenSwapchain ) );
		m_frameGeneration = false;
		m_framegenSwapchain = nullptr;
		CCP_LOGERR( "Failed to replace DX12 swapchain with AMD frame interpolation swapchain. 0x%x", result );
		return;
	}
	swapchain.Attach( ffxGetDX12SwapchainPtr( m_framegenSwapchain ) );

	m_attachedToSwapchain = true;

	CCP_LOGNOTICE( "Successfully replaced DX12 swapchain with AMD frame interpolation swapchain" );
}

Tr2Fsr3UpscalingTechnique::~Tr2Fsr3UpscalingTechnique()
{
}

void Tr2Fsr3UpscalingTechnique::ReleaseResources()
{
	if( m_frameGeneration && m_framegenSwapchain )
	{
		auto result = ffxWaitForPresents( m_framegenSwapchain );
		if( result != FFX_OK )
		{
			CCP_LOGERR( "Failed to wait for presents. 0x%x", result );
		}
	}

	m_framegenSwapchain = nullptr;
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

bool Tr2Fsr3UpscalingTechnique::IsTemporal() const
{
	return true;
}

bool Tr2Fsr3UpscalingTechnique::SupportsFrameGeneration() const
{
	return m_supportsFrameGeneration;
}

void Tr2Fsr3UpscalingTechnique::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueAL::MarkFrameEvent( frameEvent );
	if( m_frameGeneration && frameEvent == Tr2RenderContextEnum::FRAME_EVENT_UPDATE_STARTED )
	{
		for( auto& context : m_contexts )
		{
			( (Tr2Fsr3UpscalingContext*)context.second.get() )->GenerateFrame( m_renderContext );
		}
	}
}

Tr2UpscalingContextAL* Tr2Fsr3UpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2Fsr3UpscalingContext( m_setting, m_frameGeneration, m_framegenSwapchain, params );
}	

Tr2Fsr3UpscalingContext::Tr2Fsr3UpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, FfxSwapchain frameGenerationSwapchain, Tr2UpscalingAL::UpscalingContextParams params ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_setup( false ),
	m_framegenSwapchain( frameGenerationSwapchain )
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

	m_reset = true;

	auto device = ffxGetDeviceDX12( params.renderContext.GetPrimaryRenderContext().m_device );

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
		return;
	}

	m_initializationParameters.backendInterfaceSharedResources = m_ffxFsr3Backends[FSR3_BACKEND_SHARED_RESOURCES];
	m_initializationParameters.backendInterfaceUpscaling = m_ffxFsr3Backends[FSR3_BACKEND_UPSCALING];
	m_initializationParameters.backendInterfaceFrameInterpolation = m_ffxFsr3Backends[FSR3_BACKEND_FRAME_INTERPOLATION];
	m_initializationParameters.backBufferFormat = Fsr3Utils::GetFfxSurfaceFormat( params.sourceFormat );

	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.upscaleOutputSize.width = m_displayWidth;
	m_initializationParameters.upscaleOutputSize.height = m_displayHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR3_ENABLE_DEPTH_INVERTED;
	m_initializationParameters.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;
	if( !m_frameGeneration )
	{
		m_initializationParameters.flags |= FFX_FSR3_ENABLE_UPSCALING_ONLY;
	}

	if( g_upscalingDebug )
	{
		m_initializationParameters.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
		m_initializationParameters.fpMessage = &Fsr3Utils::LogFsr3Message;
	}

	errorCode = ffxFsr3ContextCreate( &m_context, &m_initializationParameters );
	m_setup = errorCode == FFX_OK;
	if( !m_setup )
	{
		CCP_LOGERR( "FSR3 setup could not create context %d", errorCode );
		return;
	}

	auto desc = Tr2BitmapDimensions( m_renderWidth, m_renderHeight, 1, Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8_UNORM );
	m_reactiveMask = std::make_unique<Tr2TextureAL>();
	m_reactiveMask->Create( desc, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, params.renderContext.GetPrimaryRenderContext() );

	if( m_frameGeneration )
	{
		m_frameGenerationConfig.frameGenerationEnabled = true;
		m_frameGenerationConfig.flags = 0;

		if( g_upscalingDebug )
		{
			m_frameGenerationConfig.flags |= FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_TEAR_LINES | FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_VIEW;
		}

		m_frameGenerationConfig.onlyPresentInterpolated = true;
		m_frameGenerationConfig.allowAsyncWorkloads = false;
		m_frameGenerationConfig.frameGenerationCallback = ffxFsr3DispatchFrameGeneration;
		m_frameGenerationConfig.swapChain = m_framegenSwapchain;
		m_frameGenerationConfig.HUDLessColor = Fsr3Utils::ConvertTextureToFfxResource( nullptr, L"FSR3_Hudless" );
	}
}

Tr2Fsr3UpscalingContext::~Tr2Fsr3UpscalingContext()
{
	if( m_setup )
	{
		m_params.renderContext.FlushAndSyncDx12();

		if( m_frameGeneration )
		{
			// the swapchain might be still interpolatin, so it is very important to wait for it...
			auto result = ffxWaitForPresents( m_frameGenerationConfig.swapChain );
			if( result != FFX_OK )
			{
				CCP_LOGERR( "Failed to wait for presents. 0x%x", result );
			}

			m_frameGenerationConfig.frameGenerationEnabled = false;
			m_frameGenerationConfig.frameGenerationCallback = nullptr;
			m_frameGenerationConfig.HUDLessColor = Fsr3Utils::ConvertTextureToFfxResource( nullptr, L"FSR3_Hudless" );
			FfxErrorCode errorCode = ffxFsr3ConfigureFrameGeneration( &m_context, &m_frameGenerationConfig );
			if( errorCode != FFX_OK )
			{
				CCP_LOGERR( "FSR3 could not disable framegeneration 0x%x", errorCode );
			}
			m_frameGeneration = false;
			m_frameGenerationConfig = {};
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
		m_setup = false;
	}
}

void Tr2Fsr3UpscalingContext::SetHudLessTexture( Tr2TextureAL* texture )
{
	if( m_frameGeneration && ( texture == nullptr || ( texture->TrinityALImpl_GetObject() != nullptr && m_frameGenerationConfig.HUDLessColor.resource != texture->TrinityALImpl_GetObject()->GetResourceDx12() ) ) )
	{
		ffxWaitForPresents( m_frameGenerationConfig.swapChain );

		m_frameGenerationConfig.HUDLessColor = Fsr3Utils::ConvertTextureToFfxResource( texture, L"FSR3_Hudless" );
	}
}


bool Tr2Fsr3UpscalingContext::HasSharpening() const
{
	return true;
}

void Tr2Fsr3UpscalingContext::Tr2Fsr3UpscalingContext::UpdateJitter()
{
	if( m_setup )
	{
		m_jitterX = 0.0f;
		m_jitterY = 0.0f;
		++m_jitterIndex;
		const int32_t jitterPhaseCount = ffxFsr3UpscalerGetJitterPhaseCount( m_renderWidth, m_displayWidth );
		ffxFsr3UpscalerGetJitterOffset( &m_jitterX, &m_jitterY, m_jitterIndex, jitterPhaseCount );
		m_jitterIndex = m_jitterIndex % jitterPhaseCount;
	}
}

uint32_t Tr2Fsr3UpscalingContext::GetDispatchRequirements() const
{
	// we don't need to check for reactive, as it is generated by fsr3
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPAQUE_ONLY | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | Tr2UpscalingAL::DispatchRequirements::VELOCITY;
}


void Tr2Fsr3UpscalingContext::GenerateFrame( Tr2RenderContextAL& renderContext )
{
	if( !m_setup || m_reset )
	{
		return;
	}

	FfxErrorCode errorCode = ffxFsr3ConfigureFrameGeneration( &m_context, &m_frameGenerationConfig );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 could not configure frame generation 0x%x", errorCode );
		m_frameGeneration = false;
		return;
	}
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}
	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	if( dispatchParameters.frontClip + dispatchParameters.backClip + dispatchParameters.aspectRatio == 0.0 )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	// flush all barriers before handing control over to FSR3
	auto& renderContext = m_params.renderContext;
	GPU_REGION_AL( renderContext, "FSR3 Upscaling" );
	renderContext.FlushBarriersDx12();

	bool reactiveSuccess = true;
	{
		FfxFsr3GenerateReactiveDescription generateReactiveParameters = {};
		generateReactiveParameters.commandList = ffxGetCommandListDX12( renderContext.m_commandList );

		generateReactiveParameters.renderSize.width = m_renderWidth;
		generateReactiveParameters.renderSize.height = m_renderHeight;

		generateReactiveParameters.scale = 1.f;
		generateReactiveParameters.cutoffThreshold = 0.2f;
		generateReactiveParameters.binaryValue = 0.7f;
		generateReactiveParameters.flags = FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_THRESHOLD |
			FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX;

		generateReactiveParameters.outReactive = Fsr3Utils::ConvertTextureToFfxResource( m_reactiveMask.get(), L"FSR3_ReactiveMask" );
		generateReactiveParameters.colorOpaqueOnly = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.opaqueOnly, L"FSR3_Input_Opaque_Color" );
		generateReactiveParameters.colorPreUpscale = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.input, L"FSR3_Input_PreUpscaleColor" );
		FfxErrorCode errorCode = ffxFsr3ContextGenerateReactiveMask( &m_context, &generateReactiveParameters );
		reactiveSuccess = errorCode == FFX_OK;
		if( !reactiveSuccess )
		{
			CCP_LOGERR( "FSR3 error while generating reactive mask %d", errorCode );
		}
		renderContext.DirtyDescriptorCache();
	}
	
	FfxFsr3DispatchUpscaleDescription dispatchDescription = {};
	dispatchDescription.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchDescription.color = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.input, L"FSR3_InputColor" );
	dispatchDescription.depth = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.depth, L"FSR3_InputDepth" );
	dispatchDescription.motionVectors = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.velocity, L"FSR3_InputMotionVectors" );
	dispatchDescription.exposure = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.exposure, L"FSR3_InputExposure" );
	dispatchDescription.reactive = Fsr3Utils::ConvertTextureToFfxResource( reactiveSuccess ? m_reactiveMask.get() : nullptr, reactiveSuccess ? L"FSR3_InputReactiveMap" : L"FSR3_EmptyInputReactiveMap" );
	dispatchDescription.transparencyAndComposition = Fsr3Utils::ConvertTextureToFfxResource( nullptr, L"FSR3_EmptyTransparencyAndCompositionMap" );
	dispatchDescription.upscaleOutput = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.output, L"FSR3_OutputColor" );

	dispatchDescription.jitterOffset.x = m_jitterX;
	dispatchDescription.jitterOffset.y = m_jitterY;
	dispatchDescription.motionVectorScale.x = (float)m_renderWidth;
	dispatchDescription.motionVectorScale.y = (float)m_renderHeight;
	dispatchDescription.reset = m_reset;
	dispatchDescription.enableSharpening = true;
	dispatchDescription.sharpness = 0.9f;
	dispatchDescription.frameTimeDelta = dispatchParameters.frameTimeDelta;

	dispatchDescription.preExposure = dispatchParameters.preExposure;
	dispatchDescription.renderSize.width = m_renderWidth;
	dispatchDescription.renderSize.height = m_renderHeight;
	dispatchDescription.cameraFar = dispatchParameters.frontClip; 
	dispatchDescription.cameraNear = dispatchParameters.backClip; 
	dispatchDescription.cameraFovAngleVertical = dispatchParameters.fieldOfView;
	dispatchDescription.viewSpaceToMetersFactor = 1.0f;

	renderContext.FlushBarriersDx12();

	auto errorCode = ffxFsr3ContextDispatchUpscale( &m_context, &dispatchDescription );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 error while dispatching %d", errorCode );
	}

	m_reset = false;

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();

	return Tr2UpscalingAL::Result::OK;
}

#endif