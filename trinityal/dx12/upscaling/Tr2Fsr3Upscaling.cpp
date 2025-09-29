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

extern bool g_upscalingDebug;
CCP_STATS_DECLARED_ELSEWHERE( generatedFrames );

namespace Fsr3Utils
{
	void LogFsr3Message( uint32_t type, const wchar_t* message )
	{
		std::string m = std::string( "FSR3 Upscaling: " + std::string( CW2A( message ) ) );

		switch( type )
		{
		case FFX_API_MESSAGE_TYPE_ERROR:
			CCP_LOGERR( m.c_str() );
			break;
		case FFX_API_MESSAGE_TYPE_WARNING:
			CCP_LOGWARN( m.c_str() );
			break;
		default:
			CCP_LOG( m.c_str() );
		}
	}

	FfxApiResource ConvertTextureToFfxResource( Tr2TextureAL* texture, const wchar_t* textureName )
	{
		ID3D12Resource* res = nullptr;
		FfxApiResource ffxRes;

		if( texture && texture->IsValid() )
		{
			auto texObj = texture->TrinityALImpl_GetObject();
			res = texObj->GetResourceDx12();
			res->SetName( textureName );
			ffxRes = ffxApiGetResourceDX12( res );
			// Since we set the dx12 resource format as typeless and then we create views into that resource
			// so we need to get the actual format here...
			ffxRes.description.format = GetFfxSurfaceFormat( texture->GetFormat() );
		}
		else
		{
			ffxRes = ffxApiGetResourceDX12(nullptr);
		}

		return ffxRes;
	}

	FfxApiSurfaceFormat GetFfxSurfaceFormat( Tr2RenderContextEnum::PixelFormat format )
	{
		switch( format )
		{
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_TYPELESS ):
			return FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16B16A16_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8_UINT ):
			return FFX_API_SURFACE_FORMAT_R8_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32_UINT ):
			return FFX_API_SURFACE_FORMAT_R32_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_TYPELESS ):
			return FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_UNORM ):
			return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB ):
			return FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R11G11B10_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16G16_UINT ):
			return FFX_API_SURFACE_FORMAT_R16G16_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_FLOAT ):
			return FFX_API_SURFACE_FORMAT_R16_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_UINT ):
			return FFX_API_SURFACE_FORMAT_R16_UINT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_UNORM ):
			return FFX_API_SURFACE_FORMAT_R16_UNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R16_SNORM ):
			return FFX_API_SURFACE_FORMAT_R16_SNORM;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8_UNORM ):
			return FFX_API_SURFACE_FORMAT_R8_UNORM;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R8G8_UNORM:
			return FFX_API_SURFACE_FORMAT_R8G8_UNORM;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32_FLOAT:
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_D32_FLOAT:
			return FFX_API_SURFACE_FORMAT_R32_FLOAT;
		case( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_UNKNOWN ):
			return FFX_API_SURFACE_FORMAT_UNKNOWN;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_R32G32B32A32_UINT:
			return FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT;
		case Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_B8G8R8A8_UNORM:
			return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
		default:
			CCP_LOGERR( "FSR3: GetFfxSurfaceFormat: Unsupported format requested. Please implement." );
			return FFX_API_SURFACE_FORMAT_UNKNOWN;
		}
	}

}

Tr2Fsr3UpscalingTechnique::Tr2Fsr3UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter ),
	m_supportsFrameGeneration( true ),
	m_swapChainContext( nullptr )
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

	ffx::CreateContextDescFrameGenerationSwapChainWrapDX12 wrapSwapchainDesc{};
	wrapSwapchainDesc.gameQueue = commandQueue;
	wrapSwapchainDesc.swapchain = &swapchain;
	ffx::ReturnCode result = ffx::CreateContext( m_swapChainContext, nullptr, wrapSwapchainDesc );

	if( result != ffx::ReturnCode::Ok )
	{
		m_frameGeneration = false;
		m_swapChainContext = nullptr;
		CCP_LOGERR( "FSR3: Failed to replace DX12 swapchain with AMD frame interpolation swapchain. 0x%x", result );
		return;
	}

	IDXGIFactory* factory = nullptr;
	swapchain->GetParent( IID_PPV_ARGS( &factory ) );
	HRESULT hr = factory->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER );

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "FSR3: Failed to make window association for swapchain. 0x%x", hr );
	}

	CCP_LOGNOTICE( "FSR3: Successfully replaced DX12 swapchain with AMD frame interpolation swapchain" );
}

Tr2Fsr3UpscalingTechnique::~Tr2Fsr3UpscalingTechnique()
{
	if( m_swapChainContext )
	{
		ffx::DestroyContext( m_swapChainContext );
		m_swapChainContext = nullptr;
	}
}

void Tr2Fsr3UpscalingTechnique::ReleaseResources()
{
	if( m_frameGeneration && m_swapChainContext )
	{
		ffx::DestroyContext( m_swapChainContext );
	}

	m_swapChainContext = nullptr;
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
}

Tr2UpscalingContextAL* Tr2Fsr3UpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2Fsr3UpscalingContext( m_setting, m_frameGeneration, m_swapChainContext, params );
}	

Tr2Fsr3UpscalingContext::Tr2Fsr3UpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, ffx::Context swapchainContext, Tr2UpscalingAL::UpscalingContextParams params ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_upscalingFfxContext( nullptr ),
	m_framegenerationFfxContext( nullptr ),
	m_swapchainFfxContext( swapchainContext ),
	m_frameGenerationConfig( {} ),
	m_backendDesc( {} ),
	m_frameGenerationCallback( nullptr ),
	m_setup( false )
{

	switch( setting )
	{
	case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
		m_upscaling = 1.0f;
		break;
	case Tr2UpscalingAL::Setting::QUALITY:
		m_upscaling = 1.5f;
		break;
	case Tr2UpscalingAL::Setting::BALANCED:
		m_upscaling = 1.7f;
		break;
	case Tr2UpscalingAL::Setting::PERFORMANCE:
		m_upscaling = 2.0f;
		break;
	case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
		m_upscaling = 3.0f;
		break;
	default:
		CCP_LOGERR( "FSR3: Invalid upscaling setting applied: %d. Upscaling amount will be forced to 1.0", int( setting ) );
	}

	m_backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
	m_backendDesc.device = params.renderContext.GetPrimaryRenderContext().m_device;

	m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
	m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );

	m_setup = SetupUpscaling() == Tr2UpscalingAL::Result::OK;

	if( m_frameGeneration )
	{
		m_frameGeneration = SetupFrameGen() == Tr2UpscalingAL::Result::OK;
	}
}

Tr2Fsr3UpscalingContext::~Tr2Fsr3UpscalingContext()
{
	if( m_setup )
	{
		m_params.renderContext.FlushAndSyncDx12();
		if( m_frameGeneration )
		{

			// disable frame generation before destroying context
			// also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
			m_frameGenerationConfig.frameGenerationEnabled = false;
			m_frameGenerationConfig.swapChain = m_params.renderContext.GetPrimaryRenderContext().m_swapchain;
			m_frameGenerationConfig.presentCallback = nullptr;
			m_frameGenerationConfig.HUDLessColor = FfxApiResource( {} );
			ffx::Configure( m_framegenerationFfxContext, m_frameGenerationConfig );

			// Destroy the contexts
			if( m_upscalingFfxContext )
			{
				ffx::DestroyContext( m_upscalingFfxContext );
				m_upscalingFfxContext = nullptr;
			}
			ffx::DestroyContext( m_framegenerationFfxContext );
			m_framegenerationFfxContext = nullptr;
		}
		else if( m_upscalingFfxContext )
		{
			ffx::DestroyContext( m_upscalingFfxContext );
			m_upscalingFfxContext = nullptr;
		}
		m_frameGenerationCallback = nullptr;
		m_setup = false;
	}
}

void Tr2Fsr3UpscalingContext::SetupForReuse()
{
	if( m_setup )
	{
		if( m_frameGeneration )
		{
			// disable frame generation just for a moment (reenabled in dispatch) 
			m_frameGenerationConfig.frameGenerationEnabled = false;
			m_frameGenerationConfig.swapChain = m_params.renderContext.GetPrimaryRenderContext().m_swapchain;
			m_frameGenerationConfig.frameGenerationCallback = nullptr;
			m_frameGenerationConfig.HUDLessColor = FfxApiResource( {} );
			ffx::Configure( m_framegenerationFfxContext, m_frameGenerationConfig );
		}
	}
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::SetupUpscaling()
{
	ffx::CreateContextDescUpscale createFsr{};

	createFsr.maxUpscaleSize = { m_displayWidth, m_displayHeight };
	createFsr.maxRenderSize = { m_renderWidth, m_renderHeight };
	createFsr.flags = FFX_UPSCALE_ENABLE_DEPTH_INVERTED | FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;

	// Do error checking in debug
	if( g_upscalingDebug )
	{
		createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
		createFsr.fpMessage = &Fsr3Utils::LogFsr3Message;
	}

	// Create the FSR context
	ffx::ReturnCode retCode = ffx::CreateContext( m_upscalingFfxContext, nullptr, createFsr, m_backendDesc );
	if( retCode != ffx::ReturnCode::Ok )
	{
		CCP_LOGERR( "FSR3: could not create context %d", retCode );
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler;
	ffx::QueryDescUpscaleGetGPUMemoryUsage upscalerGetGPUMemoryUsage{};
	upscalerGetGPUMemoryUsage.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

	ffx::Query( m_upscalingFfxContext, upscalerGetGPUMemoryUsage );
	CCP_LOGNOTICE( "FSR3: Upscaler Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f );

	return Tr2UpscalingAL::Result::OK;
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::SetupFrameGen()
{
	if( m_frameGeneration )
	{
		m_frameGenerationCallback = []( ffxDispatchDescFrameGeneration* params, void* pUserCtx ) -> ffxReturnCode_t {
			CCP_STATS_INC( generatedFrames );
			return ffxDispatch( static_cast<ffxContext*>( pUserCtx ), &params->header );
		};

		ffx::CreateContextDescFrameGeneration createFg{};
		createFg.displaySize = { m_displayWidth, m_displayHeight };
		createFg.maxRenderSize = { m_renderWidth, m_renderHeight };
		createFg.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED | FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;

		createFg.backBufferFormat = Fsr3Utils::GetFfxSurfaceFormat( m_params.renderContext.GetPrimaryRenderContext().GetBackBufferFormat() );

		ffx::ReturnCode retCode;
		ffx::CreateContextDescFrameGenerationHudless createFgHudless{};
		createFgHudless.hudlessBackBufferFormat = FfxApiSurfaceFormat::FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;

		retCode = ffx::CreateContext( m_framegenerationFfxContext, nullptr, createFg, m_backendDesc );

		if( retCode != ffx::ReturnCode::Ok )
		{
			CCP_LOGERR( "FSR3: could not create framegen context %d frame generation will not be enabled", retCode );
			return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
		}

		m_frameGenerationConfig.frameGenerationEnabled = true;
		m_frameGenerationConfig.frameGenerationCallback = m_frameGenerationCallback;
		m_frameGenerationConfig.frameGenerationCallbackUserContext = &m_framegenerationFfxContext;
		
		m_frameGenerationConfig.presentCallback = nullptr;
		m_frameGenerationConfig.presentCallbackUserContext = nullptr;
		
		m_frameGenerationConfig.swapChain = m_params.renderContext.GetPrimaryRenderContext().m_swapchain;
		m_frameGenerationConfig.HUDLessColor = FfxApiResource( {} ); // we don't have a HUDless texture yet

		m_frameGenerationConfig.frameID = 0;
		m_frameGenerationConfig.allowAsyncWorkloads = false;
		
		retCode = ffx::Configure( m_framegenerationFfxContext, m_frameGenerationConfig );
		if( retCode != ffx::ReturnCode::Ok )
		{
			CCP_LOGERR( "FSR3: could not configure framegen context %d frame generation will not be enabled", retCode );
			return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
		}

		FfxApiEffectMemoryUsage gpuMemoryUsageFrameGeneration;
		ffx::QueryDescFrameGenerationGetGPUMemoryUsage frameGenGetGPUMemoryUsage{};
		frameGenGetGPUMemoryUsage.gpuMemoryUsageFrameGeneration = &gpuMemoryUsageFrameGeneration;
		ffx::Query( m_framegenerationFfxContext, frameGenGetGPUMemoryUsage );

		CCP_LOGNOTICE( "FSR3: FrameGeneration Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageFrameGeneration.totalUsageInBytes / 1048576.f, gpuMemoryUsageFrameGeneration.aliasableUsageInBytes / 1048576.f );

		FfxApiEffectMemoryUsage gpuMemoryUsageFrameGenerationSwapchain;
		ffx::QueryFrameGenerationSwapChainGetGPUMemoryUsageDX12 frameGenSwapchainGetGPUMemoryUsage{};
		frameGenSwapchainGetGPUMemoryUsage.gpuMemoryUsageFrameGenerationSwapchain = &gpuMemoryUsageFrameGenerationSwapchain;
		ffx::Query( m_swapchainFfxContext, frameGenSwapchainGetGPUMemoryUsage );
		CCP_LOGNOTICE( "FSR3: Swapchain Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageFrameGenerationSwapchain.totalUsageInBytes / 1048576.f, gpuMemoryUsageFrameGenerationSwapchain.aliasableUsageInBytes / 1048576.f );
	}
	return Tr2UpscalingAL::Result::OK;
}

void Tr2Fsr3UpscalingContext::SetHudLessTexture( Tr2TextureAL* texture )
{
	if( m_frameGeneration && ( texture == nullptr || ( texture->TrinityALImpl_GetObject() != nullptr && m_frameGenerationConfig.HUDLessColor.resource != texture->TrinityALImpl_GetObject()->GetResourceDx12() ) ) )
	{
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

		 // Increment jitter index for frame
		++m_jitterIndex;

		ffx::ReturnCode retCode;
		int32_t jitterPhaseCount;
		ffx::QueryDescUpscaleGetJitterPhaseCount getJitterPhaseDesc{};
		getJitterPhaseDesc.displayWidth = m_displayWidth;
		getJitterPhaseDesc.renderWidth = m_renderWidth;
		getJitterPhaseDesc.pOutPhaseCount = &jitterPhaseCount;

		retCode = ffx::Query( m_upscalingFfxContext, getJitterPhaseDesc );
		if( retCode != ffx::ReturnCode::Ok )
		{
			CCP_LOGERR( "ffxQuery(FSR_GETJITTERPHASECOUNT) returned %d", retCode );
			return;
		}

		ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
		getJitterOffsetDesc.index = m_jitterIndex;
		getJitterOffsetDesc.phaseCount = jitterPhaseCount;
		getJitterOffsetDesc.pOutX = &m_jitterX;
		getJitterOffsetDesc.pOutY = &m_jitterY;

		retCode = ffx::Query( m_upscalingFfxContext, getJitterOffsetDesc );
		if( retCode != ffx::ReturnCode::Ok )
		{
			CCP_LOGERR( "ffxQuery(FSR_GETJITTEROFFSET) returned %d", retCode );
			return;
		}
		m_jitterIndex = m_jitterIndex % jitterPhaseCount;
	}
}

uint32_t Tr2Fsr3UpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPAQUE_ONLY | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | 
			Tr2UpscalingAL::DispatchRequirements::VELOCITY | Tr2UpscalingAL::DispatchRequirements::TRANSPARENCY | Tr2UpscalingAL::DispatchRequirements::REACTIVE;
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

	DispatchUpscaling( dispatchParameters );
	if( m_frameGeneration )
	{
		DispatchFrameGen( dispatchParameters );
	}

	m_reset = false;
	// increase the generated frame, so we at least have one frame active...
	CCP_STATS_INC( generatedFrames );

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();

	return Tr2UpscalingAL::Result::OK;
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::DispatchUpscaling( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	ffx::DispatchDescUpscale dispatchUpscale{};
	dispatchUpscale.commandList = m_params.renderContext.GetPrimaryRenderContext().m_commandList;
	dispatchUpscale.color = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.input, L"FSR3_InputColor" );
	dispatchUpscale.depth = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.depth, L"FSR3_InputDepth" );
	dispatchUpscale.motionVectors = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.velocity, L"FSR3_InputMotionVectors" );
	dispatchUpscale.exposure = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.exposure, L"FSR3_InputExposure" );
	dispatchUpscale.reactive = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.reactive, L"FSR3_InputReactiveMap" );
	dispatchUpscale.transparencyAndComposition = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.transparency, L"FSR3_TransparencyAndCompositionMap" );
	dispatchUpscale.output = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.output, L"FSR3_OutputColor" );

	dispatchUpscale.jitterOffset.x = m_jitterX;
	dispatchUpscale.jitterOffset.y = m_jitterY;
	dispatchUpscale.motionVectorScale.x = (float)m_renderWidth;
	dispatchUpscale.motionVectorScale.y = (float)m_renderHeight;
	dispatchUpscale.reset = m_reset;
	dispatchUpscale.enableSharpening = true;
	dispatchUpscale.sharpness = 0.9f;

	dispatchUpscale.frameTimeDelta = dispatchParameters.frameTimeDelta;

	dispatchUpscale.preExposure = dispatchParameters.preExposure;
	dispatchUpscale.renderSize.width = m_renderWidth;
	dispatchUpscale.renderSize.height = m_renderHeight;
	dispatchUpscale.upscaleSize.width = m_displayWidth;
	dispatchUpscale.upscaleSize.height = m_displayHeight;

	// Setup camera params as required
	dispatchUpscale.cameraFovAngleVertical = dispatchParameters.fieldOfView;

	dispatchUpscale.cameraFar = dispatchParameters.frontClip;
	dispatchUpscale.cameraNear = dispatchParameters.backClip;
	dispatchUpscale.viewSpaceToMetersFactor = 1.0;

	dispatchUpscale.flags = dispatchParameters.upscalingDebugView ? FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW : 0;

	ffx::ReturnCode retCode = ffx::Dispatch( m_upscalingFfxContext, dispatchUpscale );
	if( retCode != ffx::ReturnCode::Ok )
	{
		CCP_LOGERR( "FSR3 error while dispatching upscaling %d", retCode );
	}
	return Tr2UpscalingAL::Result::OK;
}

Tr2UpscalingAL::Result Tr2Fsr3UpscalingContext::DispatchFrameGen( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	ffx::DispatchDescFrameGenerationPrepare dispatchFgPrep{};
	dispatchFgPrep.depth = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.depth, L"FSR3_InputDepth" );
	dispatchFgPrep.motionVectors = Fsr3Utils::ConvertTextureToFfxResource( dispatchParameters.velocity, L"FSR3_InputMotionVectors" );
	dispatchFgPrep.flags = dispatchParameters.frameGenDebugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
	
	dispatchFgPrep.jitterOffset.x = m_jitterX;
	dispatchFgPrep.jitterOffset.y = m_jitterY;
	dispatchFgPrep.motionVectorScale.x = (float)m_renderWidth;
	dispatchFgPrep.motionVectorScale.y = (float)m_renderHeight;
	dispatchFgPrep.frameTimeDelta = dispatchParameters.frameTimeDelta;

	dispatchFgPrep.renderSize.width = m_renderWidth;
	dispatchFgPrep.renderSize.height = m_renderHeight;
	dispatchFgPrep.cameraFovAngleVertical = dispatchParameters.fieldOfView;
	dispatchFgPrep.cameraFar = dispatchParameters.frontClip;
	dispatchFgPrep.cameraNear = dispatchParameters.backClip;
	dispatchFgPrep.commandList = m_params.renderContext.GetPrimaryRenderContext().m_commandList;
	dispatchFgPrep.viewSpaceToMetersFactor = 1.f;
	dispatchFgPrep.frameID = dispatchParameters.currentFrameIndex;

	ffxDispatchDescFrameGenerationPrepareCameraInfo cameraInfo {};
	std::copy( std::begin( dispatchParameters.cameraForward ), std::end( dispatchParameters.cameraForward ), cameraInfo.cameraForward );
	std::copy( std::begin( dispatchParameters.cameraRight ), std::end( dispatchParameters.cameraRight ), cameraInfo.cameraRight );
	std::copy( std::begin( dispatchParameters.cameraUp ), std::end( dispatchParameters.cameraUp ), cameraInfo.cameraUp );
	std::copy( std::begin( dispatchParameters.cameraPos ), std::end( dispatchParameters.cameraPos ), cameraInfo.cameraPosition );

	dispatchFgPrep.header.pNext = &cameraInfo.header;
	
	m_frameGenerationConfig.frameID = dispatchParameters.currentFrameIndex;
	m_frameGenerationConfig.frameGenerationEnabled = true;
	m_frameGenerationConfig.frameGenerationCallback = m_frameGenerationCallback;
	m_frameGenerationConfig.flags = dispatchFgPrep.flags;

	auto retCode = ffx::Configure( m_framegenerationFfxContext, m_frameGenerationConfig );

	if( retCode != ffx::ReturnCode::Ok )
	{
		CCP_LOGERR( "FSR3 error while configuring frame generation before dispatch %d", retCode );
		m_frameGeneration = false;
	}
	else
	{
		if( ffx::Dispatch( m_framegenerationFfxContext, dispatchFgPrep ) != ffx::ReturnCode::Ok )
		{
			CCP_LOGERR( "FSR3 error while dispatching frame generation preparation during dispatch %d", retCode );
		}
	}
	
	return Tr2UpscalingAL::Result::OK;
}
#endif