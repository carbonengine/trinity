////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "Tr2Fsr2Upscaling.h"
#include "Tr2RenderContextAL.h"
#include "Tr2PrimaryRenderContextAL.h"
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

extern bool g_upscalingDebug;

namespace Fsr2Utils
{
	void LogFsr2Message( FfxMsgType type, const wchar_t* message )
	{
		std::string m = std::string( "FSR2 Upscaling: " + std::string( CW2A( message ) ) );

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
}

Tr2Fsr2UpscalingTechnique::Tr2Fsr2UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter )
{
	SanitizeState();
}

Tr2Fsr2UpscalingTechnique::~Tr2Fsr2UpscalingTechnique()
{
}

std::vector<Tr2UpscalingAL::Setting> Tr2Fsr2UpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::Setting::QUALITY,
		Tr2UpscalingAL::Setting::BALANCED,
		Tr2UpscalingAL::Setting::PERFORMANCE,
		Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
	};
}

bool Tr2Fsr2UpscalingTechnique::IsTemporal() const
{
	return true;
}

Tr2UpscalingContextAL* Tr2Fsr2UpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2Fsr2UpscalingContext( m_setting, m_frameGeneration, params );
}

Tr2Fsr2UpscalingContext::Tr2Fsr2UpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_initializationParameters( {} ),
	m_setup( false )
{
	switch( setting )
	{
	case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
		m_upscaling = 1.0f;
		break;
	case Tr2UpscalingAL::Setting::QUALITY:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_QUALITY );
		break;
	case Tr2UpscalingAL::Setting::BALANCED:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_BALANCED );
		break;
	case Tr2UpscalingAL::Setting::PERFORMANCE:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_PERFORMANCE );
		break;
	case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
		m_upscaling = ffxFsr2GetUpscaleRatioFromQualityMode( FfxFsr2QualityMode::FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE );
		break;
	default:
		m_upscaling = 1.0f;
		break;
	}

	m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
	m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );

	auto device = params.renderContext.m_ownerDevice->m_device;

	// Setup DX12 interface.
	const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12( 1 );
	void* scratchBuffer = CCP_MALLOC( "FSR2 Scratch Buffer", scratchBufferSize );
	FfxErrorCode errorCode = ffxGetInterfaceDX12( &m_initializationParameters.backendInterface, device, scratchBuffer, scratchBufferSize, 1 );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR2 setup could not create interface %d", errorCode );
		return;
	}

	m_initializationParameters.backendInterface.device = ffxGetDeviceDX12( device );
	m_initializationParameters.maxRenderSize.width = m_renderWidth;
	m_initializationParameters.maxRenderSize.height = m_renderHeight;
	m_initializationParameters.displaySize.width = m_displayWidth;
	m_initializationParameters.displaySize.height = m_displayHeight;
	m_initializationParameters.flags = FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

	if( g_upscalingDebug )
	{
		m_initializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
		m_initializationParameters.fpMessage = &Fsr2Utils::LogFsr2Message;
	}

	errorCode = ffxFsr2ContextCreate( &m_context, &m_initializationParameters );
	m_setup = errorCode == FFX_OK;
	if( !m_setup )
	{
		CCP_LOGERR( "FSR2 setup could not create context %d", errorCode );
	}
}

Tr2Fsr2UpscalingContext::~Tr2Fsr2UpscalingContext()
{
	if( m_setup )
	{
		m_params.renderContext.FlushAndSyncDx12();
		auto errorCode = ffxFsr2ContextDestroy( &m_context );
		if( errorCode != FFX_OK )
		{
			CCP_LOGERR( "FSR2 could not clear the context %d", errorCode );
		}
		if( m_initializationParameters.backendInterface.scratchBuffer != nullptr )
		{
			CCP_FREE( m_initializationParameters.backendInterface.scratchBuffer );
			m_initializationParameters.backendInterface.scratchBuffer = nullptr;
		}
	}
}

bool Tr2Fsr2UpscalingContext::HasSharpening() const
{
	return true;
}

void Tr2Fsr2UpscalingContext::UpdateJitter()
{
	m_jitterX = 0.0f;
	m_jitterY = 0.0f;
	++m_jitterIndex;
	const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount( m_renderWidth, m_displayWidth );
	ffxFsr2GetJitterOffset( &m_jitterX, &m_jitterY, m_jitterIndex, jitterPhaseCount );
	m_jitterIndex = m_jitterIndex % jitterPhaseCount;
}

uint32_t Tr2Fsr2UpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | Tr2UpscalingAL::DispatchRequirements::VELOCITY | Tr2UpscalingAL::DispatchRequirements::REACTIVE;
}

Tr2UpscalingAL::Result Tr2Fsr2UpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}
	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	auto& renderContext = m_params.renderContext;
	// flush all barriers before handing control over to FSR2
	renderContext.FlushBarriersDx12();

	GPU_REGION_AL( renderContext, "Fsr2 Upscaling" );

	FfxFsr2DispatchDescription dispatchDescription = {};
	dispatchDescription.commandList = ffxGetCommandListDX12( renderContext.m_commandList );
	dispatchDescription.color = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.input, L"FSR2_InputColor" );
	dispatchDescription.depth = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.depth, L"FSR2_InputDepth" );
	dispatchDescription.motionVectors = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.velocity, L"FSR2_InputMotionVectors" );
	dispatchDescription.exposure = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.exposure, L"FSR2_InputExposure" );
	dispatchDescription.reactive = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.reactive, L"FSR2_InputReactiveMap" );
	dispatchDescription.transparencyAndComposition = Fsr2Utils::ConvertTextureToFfxResource( nullptr, L"FSR2_EmptyTransparencyAndCompositionMap" );

	dispatchDescription.output = Fsr2Utils::ConvertTextureToFfxResource( dispatchParameters.output, L"FSR2_OutputColor" , FFX_RESOURCE_STATE_UNORDERED_ACCESS );
	dispatchDescription.jitterOffset.x = m_jitterX;
	dispatchDescription.jitterOffset.y = m_jitterY;
	dispatchDescription.motionVectorScale.x = (float)m_renderWidth;
	dispatchDescription.motionVectorScale.y = (float)m_renderHeight;
	dispatchDescription.reset = m_reset;
	dispatchDescription.enableSharpening = true;
	dispatchDescription.sharpness = 0.8;
	dispatchDescription.frameTimeDelta = dispatchParameters.frameTimeDelta;

	dispatchDescription.preExposure = dispatchParameters.preExposure;
	dispatchDescription.renderSize.width = m_renderWidth;
	dispatchDescription.renderSize.height = m_renderHeight;
	dispatchDescription.cameraFar = dispatchParameters.frontClip;
	dispatchDescription.cameraNear = dispatchParameters.backClip;
	dispatchDescription.cameraFovAngleVertical = dispatchParameters.fieldOfView;
	dispatchDescription.viewSpaceToMetersFactor = 1.0f;

	FfxErrorCode errorCode = ffxFsr2ContextDispatch( &m_context, &dispatchDescription );
	if( errorCode != FFX_OK )
	{
		CCP_LOGERR( "FSR2 error while dispatching %d", errorCode );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	m_reset = false;

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
	return Tr2UpscalingAL::Result::OK;

}
#endif
