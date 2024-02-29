////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr2Upscaling.h"
#include "Tr2Renderer.h"


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include "../trinityal/dx12/Tr2TextureALDx12.h" 
#include "../trinityal/dx12/Tr2BufferALDx12.h" 

Tr2Fsr2Upscaling::Tr2Fsr2Upscaling( IRoot* lockobj ) :
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
}

Tr2Fsr2Upscaling::~Tr2Fsr2Upscaling()
{
	ClearFsrResources();
}

void Tr2Fsr2Upscaling::ClearFsrResources()
{
	if( m_isSetup )
	{
		// need to flush the device since we may be in the middle of using the scratchbuffer...
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.FlushAndSyncDx12( renderContext );

		ffxFsr2ContextDestroy( &m_context );
		if( m_initializationParameters.backendInterface.scratchBuffer != nullptr )
		{
			CCP_FREE( m_initializationParameters.backendInterface.scratchBuffer );
			m_initializationParameters.backendInterface.scratchBuffer = nullptr;
		}
	}
}

bool Tr2Fsr2Upscaling::OnModified( Be::Var* value )
{
	m_dirty = true;
	return true;
}

bool Tr2Fsr2Upscaling::IsDirty() const
{
	return m_dirty;
}

bool Tr2Fsr2Upscaling::IsApplicable() const
{
	return true;
}

void Tr2Fsr2Upscaling::GetJitter( float& x, float& y )
{
	m_jitterX = 0.0f;
	m_jitterY = 0.0f;
	++m_jitterIndex;
	const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount( m_renderWidth, m_displayWidth );
	ffxFsr2GetJitterOffset( &m_jitterX, &m_jitterY, m_jitterIndex, jitterPhaseCount );
	x = m_jitterXScale * m_jitterX / ( float ) m_renderWidth; 
	y = m_jitterYScale * m_jitterY / ( float ) m_renderHeight;
}


void Tr2Fsr2Upscaling::GetJitterOffset( float& x, float& y )
{
	x = m_jitterXScale * m_jitterX;
	y = m_jitterYScale * m_jitterY;
}

float Tr2Fsr2Upscaling::GetMipLevelBias() const
{
	return log2( 1.0f / m_upscaling ) - 1.0f;
}

void Tr2Fsr2Upscaling::GetRenderSize(uint32_t& width, uint32_t& height) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

Tr2Upscaling::UpscalingType Tr2Fsr2Upscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_TEMPORAL;
}

const std::vector<Tr2Upscaling::Setting> Tr2Fsr2Upscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = { 
		Tr2Upscaling::QUALITY,
		Tr2Upscaling::BALANCED,
		Tr2Upscaling::PERFORMANCE,
		Tr2Upscaling::ULTRA_PERFORMANCE
	};
	return availableSettings;
}

void Tr2Fsr2Upscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
	switch( setting )
	{
	case Tr2Upscaling::ULTRA_QUALITY:
		m_upscaling = 1.0f;
		break;
	case Tr2Upscaling::QUALITY:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_QUALITY );
		break;
	case Tr2Upscaling::BALANCED:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_BALANCED );
		break;
	case Tr2Upscaling::PERFORMANCE:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_PERFORMANCE );
		break;
	case Tr2Upscaling::ULTRA_PERFORMANCE:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE );
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2FSR2Upscaling: %d", setting );
		break;
	}

	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;

	m_renderWidth = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayWidth, m_upscaling );
	m_renderHeight = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayHeight, m_upscaling );
}

static void onFSR2Msg( FfxMsgType type, const wchar_t* message )
{
	BlueSharedString m = BlueSharedString( "FSR2: " + std::string( CW2A( message ) ) );

	if( type == FFX_MESSAGE_TYPE_ERROR )
	{
		CCP_LOGERR( m.c_str() );
	}
	else if( type == FFX_MESSAGE_TYPE_WARNING )
	{
		CCP_LOGWARN( m.c_str() );
	}
}

void Tr2Fsr2Upscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext)
{
	m_dirty = false;
	m_reset = true;

	auto device = renderContext.GetPrimaryRenderContext().m_device;

	ClearFsrResources( );

	// Setup DX12 interface.
	const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(1);
	void* scratchBuffer = CCP_MALLOC( "FSR2 Scratch Buffer", scratchBufferSize );
	FfxErrorCode errorCode = ffxGetInterfaceDX12( &m_initializationParameters.backendInterface, device, scratchBuffer, scratchBufferSize, 1);
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR2 setup could not create interface %d", errorCode );
	}
	m_usingExposure = setupContext.hasExposureTexture;
	m_initializationParameters.backendInterface.device = ffxGetDeviceDX12( device );
	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR2_ENABLE_DEPTH_INVERTED;

	if( m_useHDR )
	{
		m_initializationParameters.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
	}
	
//#if TRINITYDEV
	m_initializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
	m_initializationParameters.fpMessage = &onFSR2Msg;
//#endif

	errorCode = ffxFsr2ContextCreate( &m_context, &m_initializationParameters );
	m_isSetup = errorCode == FFX_OK;
	if( !m_isSetup )
	{
		CCP_LOGERR( "FSR2 setup could not create context %d", errorCode );
		ClearFsrResources();
	}
}

FfxResource Tr2Fsr2Upscaling::ConvertTextureToFfxResource( ITr2TextureProvider* texture, const BlueSharedStringW& textureName, FfxResourceStates state ){

	ID3D12Resource* res = nullptr;
	if( texture && texture->GetTexture() && texture->GetTexture()->IsValid() )
	{
		res = texture->GetTexture()->TrinityALImpl_GetObject()->GetResourceDx12();
	}

	auto desc = GetFfxResourceDescriptionDX12( res );
	return ffxGetResourceDX12( res, desc, const_cast<wchar_t*>( textureName.c_str() ), state );
}

void Tr2Fsr2Upscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	// flush all barriers before handing control over to FSR2
	renderContext.FlushBarriersDx12();

	GPU_REGION( renderContext, "Fsr2 Upscaling" );

	FfxFsr2DispatchDescription dispatchParameters = {};
	dispatchParameters.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchParameters.color = ConvertTextureToFfxResource( textures.input, BlueSharedStringW( L"FSR2_InputColor") );
	dispatchParameters.depth = ConvertTextureToFfxResource( textures.depth, BlueSharedStringW( L"FSR2_InputDepth" ) );
	dispatchParameters.motionVectors = ConvertTextureToFfxResource( textures.motion, BlueSharedStringW( L"FSR2_InputMotionVectors" ) );
	dispatchParameters.exposure = ConvertTextureToFfxResource( textures.exposure, BlueSharedStringW( L"FSR2_InputExposure" ) );
	dispatchParameters.reactive = ConvertTextureToFfxResource( textures.reactive, BlueSharedStringW( L"FSR2_InputReactiveMap" ) );
	dispatchParameters.transparencyAndComposition = ConvertTextureToFfxResource( nullptr, BlueSharedStringW( L"FSR2_EmptyTransparencyAndCompositionMap" ) );

	dispatchParameters.output = ConvertTextureToFfxResource( textures.output, BlueSharedStringW( L"FSR2_OutputColor" ), FFX_RESOURCE_STATE_UNORDERED_ACCESS );
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

	FfxErrorCode errorCode = ffxFsr2ContextDispatch( &m_context, &dispatchParameters );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR2 error while dispatching %d", errorCode );
	}
	m_lastTime = BeOS->GetCurrentFrameTime();

	m_reset = false;

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
}

bool Tr2Fsr2Upscaling::NeedsExposureTexture() const
{
	return true;
}

bool Tr2Fsr2Upscaling::UsesExposureTexture() const
{
	return m_usingExposure;
}
#endif
