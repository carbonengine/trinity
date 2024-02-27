////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr2Upscaling.h"
#include "Tr2Renderer.h"


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "ffx_fsr2.h"
#include "dx12/ffx_fsr2_dx12.h"
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
	m_generateReactiveMap( true ),
	m_generateTransparencyMap( false ),
	m_autoReactiveScale( 1.f ),
	m_autoReactiveThreshold( 0.2f ),
	m_autoReactiveBinaryValue (0.9f ),
	m_autoReactiveUseTonemap( false ),
	m_autoReactiveUseInverseTonemap( false ),
	m_autoReactiveUseThreshold( true ),
	m_autoReactiveUseMax( true ),
	m_sharpness( 0.8f ),
	m_preexposure( 0.55f ),
	m_useHDR( true ),
	m_cancelMotionVectorJittering( false ),
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
		if( m_initializationParameters.callbacks.scratchBuffer != nullptr )
		{
			CCP_FREE( m_initializationParameters.callbacks.scratchBuffer );
			m_initializationParameters.callbacks.scratchBuffer = nullptr;
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

static void onFSR2Msg( FfxFsr2MsgType type, const wchar_t* message )
{
	BlueSharedString m = BlueSharedString( "FSR2: " + std::string( CW2A( message ) ) );

	if( type == FFX_FSR2_MESSAGE_TYPE_ERROR )
	{
		CCP_LOGERR( m.c_str() );
	}
	else if( type == FFX_FSR2_MESSAGE_TYPE_WARNING )
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
	const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
	void* scratchBuffer = CCP_MALLOC( "FSR2 Scratch Buffer", scratchBufferSize );
	FfxErrorCode errorCode = ffxFsr2GetInterfaceDX12( &m_initializationParameters.callbacks, device, scratchBuffer, scratchBufferSize );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR2 setup could not create interface %d", errorCode );
	}
	m_usingExposure = setupContext.hasExposureTexture;
	m_initializationParameters.device = ffxGetDeviceDX12( device );
	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR2_ENABLE_DEPTH_INVERTED;

	if( m_cancelMotionVectorJittering )
	{
		m_initializationParameters.flags |= FFX_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
	}
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

FfxResource Tr2Fsr2Upscaling::ConvertTextureToFfxResource( ITr2TextureProvider* texture, const wchar_t* textureName, FfxResourceStates state )
{
	if( texture && texture->GetTexture() && texture->GetTexture()->IsValid() )
	{
		return ffxGetResourceDX12( &m_context, texture->GetTexture()->TrinityALImpl_GetObject()->GetResourceDx12(), textureName, state );
	}
	return ffxGetResourceDX12( &m_context, nullptr, textureName, state );
}

void Tr2Fsr2Upscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	// flush all barriers before handing control over to FSR2
	renderContext.FlushBarriersDx12();
	ITr2TextureProvider* reactiveMap = nullptr;
	//if( m_generateReactiveMap )
	//{
	//	reactiveMap = renderInfo.GetTempTexture( 1.0f, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
	//	GPU_REGION( renderContext, "Fsr2 Generate Reactive Map" );
	//	FfxFsr2GenerateReactiveDescription generateReactiveParameters;
	//	generateReactiveParameters.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	//	generateReactiveParameters.colorOpaqueOnly = ConvertTextureToFfxResource( textures.opaqueOnly );
	//	generateReactiveParameters.colorPreUpscale = ConvertTextureToFfxResource( textures.input ); // opaque and transparency
	//	generateReactiveParameters.outReactive = ConvertTextureToFfxResource( reactiveMap, L"FSR2_InputReactiveMap", FFX_RESOURCE_STATE_UNORDERED_ACCESS );

	//	generateReactiveParameters.renderSize.width = m_renderWidth;
	//	generateReactiveParameters.renderSize.height = m_renderHeight;

	//	generateReactiveParameters.scale = m_autoReactiveScale;
	//	generateReactiveParameters.cutoffThreshold = m_autoReactiveThreshold;
	//	generateReactiveParameters.binaryValue = m_autoReactiveBinaryValue;

	//	generateReactiveParameters.flags = ( m_autoReactiveUseTonemap ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_TONEMAP : 0 ) |
	//		( m_autoReactiveUseInverseTonemap ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP : 0 ) |
	//		( m_autoReactiveUseThreshold ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_THRESHOLD : 0 ) |
	//		( m_autoReactiveUseMax ? FFX_FSR2_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX : 0 );

	//	FfxErrorCode errorCode = ffxFsr2ContextGenerateReactiveMask( &m_context, &generateReactiveParameters );
	//	if( errorCode != FFX_OK )
	//	{
	//		CCP_LOGERR( "FSR2 error while generating reactive map %d", errorCode );
	//	}
	//}

	GPU_REGION( renderContext, "Fsr2 Upscaling" );

	FfxFsr2DispatchDescription dispatchParameters = {};
	dispatchParameters.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchParameters.color = ConvertTextureToFfxResource( textures.input, L"FSR2_InputColor" );
	dispatchParameters.depth = ConvertTextureToFfxResource( textures.depth, L"FSR2_InputDepth" );
	dispatchParameters.motionVectors = ConvertTextureToFfxResource( textures.motion, L"FSR2_InputMotionVectors"  );
	dispatchParameters.exposure = ConvertTextureToFfxResource( textures.exposure, L"FSR2_InputExposure" );
	dispatchParameters.reactive = ConvertTextureToFfxResource( reactiveMap, m_generateReactiveMap ? L"FSR2_InputReactiveMap" : L"FSR2_EmptyInputReactiveMap" );
	dispatchParameters.transparencyAndComposition = ConvertTextureToFfxResource( nullptr, L"FSR2_EmptyTransparencyAndCompositionMap" );

	if( m_generateTransparencyMap )
	{
		dispatchParameters.enableAutoReactive = true;
		dispatchParameters.autoReactiveScale = 5.0f;
		dispatchParameters.autoReactiveMax = 0.9f;
		dispatchParameters.colorOpaqueOnly = ConvertTextureToFfxResource( textures.opaqueOnly );
		dispatchParameters.autoTcThreshold = 0.05f;
		dispatchParameters.autoTcScale = 1.0f;
	}

	dispatchParameters.output = ConvertTextureToFfxResource( textures.output, L"FSR2_OutputColor", FFX_RESOURCE_STATE_UNORDERED_ACCESS );
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
