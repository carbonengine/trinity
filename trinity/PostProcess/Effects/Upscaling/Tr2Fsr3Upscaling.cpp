////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr3Upscaling.h"
#include "Tr2Renderer.h"


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include "../trinityal/dx12/Tr2TextureALDx12.h" 
#include "../trinityal/dx12/Tr2BufferALDx12.h" 
#include "../trinityal/dx12/Utilities.h"

Tr2Fsr3Upscaling::Tr2Fsr3Upscaling( IRoot* lockobj ) :
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 ),
	m_jitterIndex( 0 ),
	m_jitterX( 0.0f ),
	m_jitterY( 0.0f ),
	m_jitterXScale( 2.0f ),
	m_jitterYScale( -2.0f ),
	m_usingExposure( false ),
	m_reset( true ),
	m_sharpness( 0.8f ),
	m_preexposure( 0.55f ),
	m_useHDR( true ),
	m_dirty( true ),
	m_isSetup(false)
{
	m_lastTime = BeOS->GetCurrentFrameTime();
	m_initializationParameters.backendInterfaceUpscaling.scratchBuffer = nullptr;
	m_initializationParameters.backendInterfaceFrameInterpolation.scratchBuffer = nullptr;
	m_initializationParameters.backendInterfaceSharedResources.scratchBuffer = nullptr;
}

Tr2Fsr3Upscaling::~Tr2Fsr3Upscaling()
{
	ClearFsrResources();
}

void Tr2Fsr3Upscaling::ClearFsrResources()
{
	if( m_isSetup )
	{
		// need to flush the device since we may be in the middle of using the scratchbuffer...
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.FlushAndSyncDx12( renderContext );

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

bool Tr2Fsr3Upscaling::OnModified( Be::Var* value )
{
	m_dirty = true;
	return true;
}

bool Tr2Fsr3Upscaling::IsDirty() const
{
	return m_dirty;
}

bool Tr2Fsr3Upscaling::IsApplicable() const
{
	return true;
}

void Tr2Fsr3Upscaling::GetJitter( float& x, float& y )
{
	m_jitterX = 0.0f;
	m_jitterY = 0.0f;
	++m_jitterIndex;
	const int32_t jitterPhaseCount = ffxFsr3UpscalerGetJitterPhaseCount( m_renderWidth, m_displayWidth );
	ffxFsr3UpscalerGetJitterOffset( &m_jitterX, &m_jitterY, m_jitterIndex, jitterPhaseCount );
	x = m_jitterXScale * m_jitterX / ( float ) m_renderWidth; 
	y = m_jitterYScale * m_jitterY / ( float ) m_renderHeight;
}


void Tr2Fsr3Upscaling::GetJitterOffset( float& x, float& y )
{
	x = m_jitterXScale * m_jitterX;
	y = m_jitterYScale * m_jitterY;
}

float Tr2Fsr3Upscaling::GetMipLevelBias() const
{
	return log2( 1.0f / m_upscaling ) - 1.0f;
}

void Tr2Fsr3Upscaling::GetRenderSize(uint32_t& width, uint32_t& height) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

Tr2Upscaling::UpscalingType Tr2Fsr3Upscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_TEMPORAL;
}

const std::vector<Tr2Upscaling::Setting> Tr2Fsr3Upscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = { 
		Tr2Upscaling::QUALITY,
		Tr2Upscaling::BALANCED,
		Tr2Upscaling::PERFORMANCE,
		Tr2Upscaling::ULTRA_PERFORMANCE
	};
	return availableSettings;
}

void Tr2Fsr3Upscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
 	switch( setting )
	{
	case Tr2Upscaling::ULTRA_QUALITY:
		m_upscaling = 1.0f;
		break;
	case Tr2Upscaling::QUALITY:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_QUALITY );
		break;
	case Tr2Upscaling::BALANCED:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_BALANCED );
		break;
	case Tr2Upscaling::PERFORMANCE:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_PERFORMANCE );
		break;
	case Tr2Upscaling::ULTRA_PERFORMANCE:
		m_upscaling = ffxFsr3GetUpscaleRatioFromQualityMode( FfxFsr3QualityMode::FFX_FSR3_QUALITY_MODE_ULTRA_PERFORMANCE );
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2Fsr3Upscaling: %d", setting );
		break;
	}

	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;

	m_renderWidth = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayWidth, m_upscaling );
	m_renderHeight = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayHeight, m_upscaling );
}

static void onFSR3Msg( FfxMsgType type, const wchar_t* message )
{
	BlueSharedString m = BlueSharedString( "FSR3 Upscaling: " + std::string( CW2A( message ) ) );

	if( type == FFX_MESSAGE_TYPE_ERROR )
	{
		CCP_LOGERR( m.c_str() );
	}
	else if( type == FFX_MESSAGE_TYPE_WARNING )
	{
		CCP_LOGWARN( m.c_str() );
	}
}

void Tr2Fsr3Upscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
	m_dirty = false;
	m_reset = true;

	auto device = ffxGetDeviceDX12( renderContext.GetPrimaryRenderContext().m_device );

	ClearFsrResources();

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

	m_usingExposure = setupContext.hasExposureTexture;
	m_initializationParameters.backendInterfaceSharedResources = m_ffxFsr3Backends[FSR3_BACKEND_SHARED_RESOURCES];
	m_initializationParameters.backendInterfaceUpscaling = m_ffxFsr3Backends[FSR3_BACKEND_UPSCALING];
	m_initializationParameters.backendInterfaceFrameInterpolation = m_ffxFsr3Backends[FSR3_BACKEND_FRAME_INTERPOLATION];
	m_initializationParameters.backBufferFormat = FfxSurfaceFormat::FFX_SURFACE_FORMAT_R11G11B10_FLOAT;

	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.upscaleOutputSize.width = m_displayWidth;
	m_initializationParameters.upscaleOutputSize.height = m_displayHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR3_ENABLE_DEPTH_INVERTED;

	if( m_useHDR )
	{
		m_initializationParameters.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;
	}
	
#if 1
	m_initializationParameters.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
	m_initializationParameters.fpMessage = &onFSR3Msg;
#endif

	errorCode = ffxFsr3ContextCreate( &m_context, &m_initializationParameters );
	m_isSetup = errorCode == FFX_OK;
	if( !m_isSetup )
	{
		CCP_LOGERR( "FSR3 setup could not create context %d", errorCode );
		ClearFsrResources();
	}
}

FfxResource Tr2Fsr3Upscaling::ConvertTextureToFfxResource( Tr2RenderContext& renderContext, ITr2TextureProvider* texture, const wchar_t* textureName, FfxResourceStates state )
{
	ID3D12Resource* res	= nullptr;
	if( texture && texture->GetTexture() && texture->GetTexture()->IsValid() )
	{
		auto alTex = texture->GetTexture()->TrinityALImpl_GetObject();
		res = alTex->GetResourceDx12();
		res->SetName( textureName );
		//renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( res, alTex->GetResourceState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) );
	}

	auto desc = GetFfxResourceDescriptionDX12( res );
	return ffxGetResourceDX12( res, desc, const_cast<wchar_t*>( textureName ), state );
}

void Tr2Fsr3Upscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	if( !m_isSetup )
	{
		return;
	}
	// flush all barriers before handing control over to FSR3

	GPU_REGION( renderContext, "FSR3 Upscaling" );

	FfxFsr3DispatchUpscaleDescription dispatchParameters = {};
	dispatchParameters.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchParameters.color = ConvertTextureToFfxResource( renderContext, textures.input, L"FSR2_InputColor", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.depth = ConvertTextureToFfxResource( renderContext, textures.depth, L"FSR2_InputDepth", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.motionVectors = ConvertTextureToFfxResource( renderContext, textures.motion, L"FSR2_InputMotionVectors", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.exposure = ConvertTextureToFfxResource( renderContext, textures.exposure, L"FSR2_InputExposure", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.reactive = ConvertTextureToFfxResource( renderContext, textures.reactive, textures.reactive != nullptr ? L"FSR2_InputReactiveMap" : L"FSR2_EmptyInputReactiveMap", FfxResourceStates::FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.transparencyAndComposition = ConvertTextureToFfxResource( renderContext, nullptr, L"FSR3_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );

	dispatchParameters.upscaleOutput = ConvertTextureToFfxResource( renderContext, textures.output, L"FSR3_OutputColor", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ );
	dispatchParameters.jitterOffset.x = m_jitterX;
	dispatchParameters.jitterOffset.y = m_jitterY;
	dispatchParameters.motionVectorScale.x = (float) m_renderWidth;
	dispatchParameters.motionVectorScale.y = (float) m_renderHeight;
	dispatchParameters.reset = m_reset;
	dispatchParameters.enableSharpening = true;
	dispatchParameters.sharpness = m_sharpness;
	dispatchParameters.frameTimeDelta = TimeAsFloat( BeOS->GetCurrentFrameTime() - m_lastTime ) * 1000.0f;

	dispatchParameters.preExposure = m_preexposure;
	dispatchParameters.renderSize.width = m_renderWidth;
	dispatchParameters.renderSize.height = m_renderHeight;
	dispatchParameters.cameraFar = Tr2Renderer::GetFrontClip();
	dispatchParameters.cameraNear = Tr2Renderer::GetBackClip();
	dispatchParameters.cameraFovAngleVertical = Tr2Renderer::GetFieldOfView();
	dispatchParameters.viewSpaceToMetersFactor = 1.0f;

	renderContext.FlushBarriersDx12();
	FfxErrorCode errorCode = ffxFsr3ContextDispatchUpscale( &m_context, &dispatchParameters );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR3 error while dispatching %d", errorCode );
	}
	m_lastTime = BeOS->GetCurrentFrameTime();

	m_reset = false;

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
}

bool Tr2Fsr3Upscaling::NeedsExposureTexture() const
{
	return true;
}

bool Tr2Fsr3Upscaling::UsesExposureTexture() const
{
	return m_usingExposure;
}
#endif
