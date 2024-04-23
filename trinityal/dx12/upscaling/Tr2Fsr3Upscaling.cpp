////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

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
}

Tr2Fsr3UpscalingTechnique::Tr2Fsr3UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( technique, setting, frameGeneration )
{
	SanitizeState();
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
	return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
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
	m_initializationParameters.backBufferFormat = FfxSurfaceFormat::FFX_SURFACE_FORMAT_R11G11B10_FLOAT;

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
	return Tr2UpscalingAL::Result::OK;
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
	dispatchDescription.cameraFar = dispatchParameters.frontClip; // reversed because of reversed depth
	dispatchDescription.cameraNear = dispatchParameters.backClip; // reversed because of reversed depth
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

	return Tr2UpscalingAL::Result::OK;
}

#endif