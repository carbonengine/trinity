// Copyright © 2024 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "Tr2XessUpscaling.h"
#include "Tr2RenderContextAL.h"
#include "Tr2PrimaryRenderContextAL.h"
#include "../Tr2TextureALDx12.h"
#include "../Utilities.h"
#include <xess/xess_d3d12.h>

extern bool g_upscalingDebug;

namespace Tr2XessUpscalingUtils
{
const char* ResultToString( xess_result_t result )
{
	switch( result )
	{
	case XESS_RESULT_WARNING_NONEXISTING_FOLDER:
		return "Warning Nonexistent Folder";
	case XESS_RESULT_WARNING_OLD_DRIVER:
		return "Warning Old Driver";
	case XESS_RESULT_SUCCESS:
		return "Success";
	case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE:
		return "Unsupported Device";
	case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER:
		return "Unsupported Driver";
	case XESS_RESULT_ERROR_UNINITIALIZED:
		return "Uninitialized";
	case XESS_RESULT_ERROR_INVALID_ARGUMENT:
		return "Invalid Argument";
	case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY:
		return "Device Out of Memory";
	case XESS_RESULT_ERROR_DEVICE:
		return "Device Error";
	case XESS_RESULT_ERROR_NOT_IMPLEMENTED:
		return "Not Implemented";
	case XESS_RESULT_ERROR_INVALID_CONTEXT:
		return "Invalid Context";
	case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS:
		return "Operation in Progress";
	case XESS_RESULT_ERROR_UNSUPPORTED:
		return "Unsupported";
	case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY:
		return "Cannot Load Library";
	case XESS_RESULT_ERROR_UNKNOWN:
	default:
		return "Unknown";
	}
}

void LogXeSS( const char* message, xess_logging_level_t loggingLevel )
{
	switch( loggingLevel )
	{
	case xess_logging_level_t::XESS_LOGGING_LEVEL_DEBUG:
		CCP_LOGNOTICE( "DEBUG: %s", message );
		break;

	case xess_logging_level_t::XESS_LOGGING_LEVEL_INFO:
		CCP_LOGNOTICE( "%s", message );
		break;

	case xess_logging_level_t::XESS_LOGGING_LEVEL_WARNING:
		CCP_LOGWARN( "%s", message );
		break;

	case xess_logging_level_t::XESS_LOGGING_LEVEL_ERROR:
		CCP_LOGERR( "%s", message );
		break;
	}
}
}


Tr2XessUpscalingTechnique::Tr2XessUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter )
{
	SanitizeState();
}

Tr2XessUpscalingTechnique::~Tr2XessUpscalingTechnique()
{
}

std::vector<Tr2UpscalingAL::Setting> Tr2XessUpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::Setting::ULTRA_QUALITY,
		Tr2UpscalingAL::Setting::QUALITY,
		Tr2UpscalingAL::Setting::BALANCED,
		Tr2UpscalingAL::Setting::PERFORMANCE,
		Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
	};
}

bool Tr2XessUpscalingTechnique::IsTemporal() const
{
	return true;
}

bool Tr2XessUpscalingTechnique::IsAvailable() const
{
	return true;
}

Tr2UpscalingContextAL* Tr2XessUpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2XessUpscalingContext( m_setting, m_frameGeneration, params );
}

Tr2XessUpscalingContext::Tr2XessUpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_context( nullptr ),
	m_setup( false ),
	m_setupWithExposure( true )
{
	m_xessSetting = XESS_QUALITY_SETTING_PERFORMANCE;
	switch( setting )
	{
	case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
		m_xessSetting = XESS_QUALITY_SETTING_ULTRA_QUALITY;
		break;
	case Tr2UpscalingAL::Setting::QUALITY:
		m_xessSetting = XESS_QUALITY_SETTING_QUALITY;
		break;
	case Tr2UpscalingAL::Setting::BALANCED:
		m_xessSetting = XESS_QUALITY_SETTING_BALANCED;
		break;
	case Tr2UpscalingAL::Setting::PERFORMANCE:
		m_xessSetting = XESS_QUALITY_SETTING_PERFORMANCE;
		break;
	case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
		m_xessSetting = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2XeSSUpscaling: %d", setting );
		break;
	}

	Setup();
}

Tr2XessUpscalingContext::~Tr2XessUpscalingContext()
{
	Destroy();
}

bool Tr2XessUpscalingContext::HasSharpening() const
{
	return false;
}

uint32_t Tr2XessUpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::VELOCITY | Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE;
}

void Tr2XessUpscalingContext::UpdateJitter()
{
	if( m_setup )
	{
		m_jitterX = m_jitterSequence[m_jitterIndex].first;
		m_jitterY = m_jitterSequence[m_jitterIndex].second;

		m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
	}
}

void Tr2XessUpscalingContext::Destroy()
{
	if( m_context )
	{
		m_params.renderContext.FlushAndSyncDx12();
		xessDestroyContext( m_context );
		m_context = nullptr;
	}
}

void Tr2XessUpscalingContext::Setup()
{
	m_setup = false;

	xess_d3d12_init_params_t params{};
	params.outputResolution.x = m_displayWidth;
	params.outputResolution.y = m_displayHeight;
	params.qualitySetting = m_xessSetting;
	params.initFlags = XESS_INIT_FLAG_INVERTED_DEPTH | XESS_INIT_FLAG_USE_NDC_VELOCITY;
	if( m_setupWithExposure )
	{
		params.initFlags |= XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE;
	}

	auto& renderContext = m_params.renderContext;
	renderContext.FlushAndSyncDx12();
	renderContext.DirtyDescriptorCache();

	auto status = xessD3D12CreateContext( renderContext.m_ownerDevice->m_device, &m_context );
	if( status != XESS_RESULT_SUCCESS )
	{
		m_context = nullptr;
		CCP_LOGNOTICE( "XeSS: XeSS is not supported on this device. Result - %s.", Tr2XessUpscalingUtils::ResultToString( status ) );
		return;
	}

	if( XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver( m_context ) )
	{
		CCP_LOGNOTICE( "XeSS: Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality" );
	}
	else
	{
		CCP_LOGNOTICE( "XeSS context created" );
	}

	//Our motion vectors are in normalized texture coordinates (0 to 1), with a flipped Y-axis.
	//We configure XeSS to expect motion vectors in NDC coordinates (-1 to +1).
	//As such, we need to multiply our motion vectors 2.0f and flip Y to match XeSS's expectations!
	xessSetVelocityScale( m_context, 2.0f, -2.0f );

	auto ret = xessD3D12Init( m_context, &params );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not initialize. Result - %s.", Tr2XessUpscalingUtils::ResultToString( ret ) );
		return;
	}

	// Get version of XeSS
	xess_version_t ver;
	ret = xessGetVersion( &ver );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get version information. Result - %s.", Tr2XessUpscalingUtils::ResultToString( ret ) );
		return;
	}

	CCP_LOGNOTICE( "XeSS: Version - %u.%u.%u", ver.major, ver.minor, ver.patch );

	if( g_upscalingDebug )
	{
		// Set logging callback here.
		ret = xessSetLoggingCallback( m_context, XESS_LOGGING_LEVEL_DEBUG, Tr2XessUpscalingUtils::LogXeSS );
		if( ret != XESS_RESULT_SUCCESS )
		{
			CCP_LOGERR( "XeSS: Could not set logging callback Result - %s.", Tr2XessUpscalingUtils::ResultToString( ret ) );
			return;
		}
	}
	xess_2d_t inputRes = { 1, 1 };
	xess_2d_t inputMinRes = { 1, 1 };
	xess_2d_t inputMaxRes = { 1, 1 };
	xess_2d_t outputRes = { m_displayWidth, m_displayHeight };

	ret = xessGetOptimalInputResolution( m_context, &outputRes, m_xessSetting, &inputRes, &inputMinRes, &inputMaxRes );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get input resolution. Result - %s.", Tr2XessUpscalingUtils::ResultToString( ret ) );
		return;
	}

	m_renderWidth = inputRes.x;
	m_renderHeight = inputRes.y;

	m_upscaling = (float)m_displayWidth / (float)m_renderWidth;
	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( ceilf( m_upscaling ), 2.0f ), 2, 3 );

	CCP_LOGNOTICE( "XeSS: Initialized." );
	m_setup = true;
}

Tr2UpscalingAL::Result Tr2XessUpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	// Special case, if we have setup the context with exposure in mind, but we don't have exposure available then we need to reset the context
	if( m_setupWithExposure && dispatchParameters.exposure == nullptr )
	{
		CCP_LOGNOTICE( "XeSS: Resetting the context due to exposure not being available." );

		m_setupWithExposure = false;
		Destroy();
		Setup();
	}
	// and the reverse
	else if( !m_setupWithExposure && dispatchParameters.exposure != nullptr )
	{
		CCP_LOGNOTICE( "XeSS: Resetting the context due to exposure now being available." );

		m_setupWithExposure = true;
		Destroy();
		Setup();
	}

	auto TransitionTo = []( Tr2RenderContextAL& renderContext, const Tr2TextureAL* texture, D3D12_RESOURCE_STATES newState ) {
		if( texture && texture->TrinityALImpl_GetObject() )
		{
			auto tex = texture->TrinityALImpl_GetObject();
			renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( tex->GetResourceDx12(), tex->GetResourceState(), newState ) );
		}
	};

	auto TransitionFrom = []( Tr2RenderContextAL& renderContext, const Tr2TextureAL* texture, D3D12_RESOURCE_STATES oldState ) {
		if( texture && texture->TrinityALImpl_GetObject() )
		{
			auto tex = texture->TrinityALImpl_GetObject();
			renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( tex->GetResourceDx12(), oldState, tex->GetResourceState() ) );
		}
	};

	auto& renderContext = m_params.renderContext;

	// flush all barriers before changing the state of the textures
	renderContext.SetResourceSet( Tr2ResourceSetAL() );
	renderContext.FlushBarriersDx12();

	// transition from common to unordered access view, since the output texture must be in that state
	TransitionTo( renderContext, dispatchParameters.output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	TransitionTo( renderContext, dispatchParameters.input, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	TransitionTo( renderContext, dispatchParameters.exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	// flush all barriers before handing control over to XeSS, so the states are applied
	renderContext.FlushBarriersDx12();


	xess_d3d12_execute_params_t params{};
	params.jitterOffsetX = m_jitterX;
	params.jitterOffsetY = m_jitterY;
	params.inputWidth = m_renderWidth;
	params.inputHeight = m_renderHeight;
	params.resetHistory = dispatchParameters.reset ? 1 : 0;
	params.exposureScale = 1.0f;

	params.pColorTexture = dispatchParameters.input->TrinityALImpl_GetObject()->GetResourceDx12();
	params.pVelocityTexture = dispatchParameters.velocity->TrinityALImpl_GetObject()->GetResourceDx12();
	params.pOutputTexture = dispatchParameters.output->TrinityALImpl_GetObject()->GetResourceDx12();
	params.pDepthTexture = dispatchParameters.depth->TrinityALImpl_GetObject()->GetResourceDx12();
	params.pExposureScaleTexture = dispatchParameters.exposure ? dispatchParameters.exposure->TrinityALImpl_GetObject()->GetResourceDx12() : nullptr;
	params.pResponsivePixelMaskTexture = nullptr;

	xess_result_t ret = xessD3D12Execute( m_context, renderContext.m_commandList, &params );

	// Trigger error report once.
	if( ret != XESS_RESULT_SUCCESS )
	{
		static bool s_Reported = false;
		if( !s_Reported )
		{
			s_Reported = true;
			CCP_LOGERR( "XeSS: Failed to execute. Result - %s.", Tr2XessUpscalingUtils::ResultToString( ret ) );
		}
	}
	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();

	// transition back to what it was
	TransitionFrom( renderContext, dispatchParameters.output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	TransitionFrom( renderContext, dispatchParameters.input, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	TransitionFrom( renderContext, dispatchParameters.exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	return Tr2UpscalingAL::Result::OK;
}

#endif
