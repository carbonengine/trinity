////////////////////////////////////////////////////////////////////////////////
//
// Created:		October 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2XeSSUpscaling.h"
#include "Tr2Renderer.h"


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <xess/xess_d3d12.h>
#include "../trinityal/dx12/Tr2TextureALDx12.h"
#include "../trinityal/dx12/Utilities.h"

Tr2XeSSUpscaling::Availability Tr2XeSSUpscaling::s_availability = Tr2XeSSUpscaling::Availability::XESS_AVAILABILITY_UNKNOWN;
xess_context_handle_t Tr2XeSSUpscaling::s_context = nullptr;
uint32_t Tr2XeSSUpscaling::s_creationNodeMask = 0;

namespace XessUtils
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



void Tr2XeSSUpscaling::Initialize()
{

	CCP_STATS_ZONE( __FUNCTION__ );
	if( !s_context )
	{

		USE_MAIN_THREAD_RENDER_CONTEXT();
		auto status = xessD3D12CreateContext( renderContext.GetPrimaryRenderContext().m_device, &s_context );
		if( status != XESS_RESULT_SUCCESS )
		{
			s_context = nullptr;
			CCP_LOGNOTICE( "XeSS: XeSS is not supported on this device. Result - %s.", XessUtils::ResultToString( status ) );
			return;
		}

		if( XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver( s_context ) )
		{
			CCP_LOGNOTICE( "XeSS: Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality" );
			return;
		}
		CCP_LOGNOTICE( "XeSS context created" );
	}
}

void Tr2XeSSUpscaling::Shutdown()
{
	if( s_context )
	{

		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.FlushAndSyncDx12( renderContext );

		xessDestroyContext( s_context );
		s_context = nullptr;
	}
}

Tr2XeSSUpscaling::Tr2XeSSUpscaling( IRoot* lockobj ) :
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 ),
	m_jitterIndex( 0 ),
	m_jitterX( 0.0f ),
	m_jitterY( 0.0f ),
	m_jitterXScale( 2.0f ),
	m_jitterYScale( 2.0f ),
	m_jitterOffsetXScale( 1.0f ),
	m_jitterOffsetYScale( 1.0f ),
	m_reset( true ),
	m_dirty( true ),
	m_setup( false ),
	m_initialized( false ),
	m_useReactive( false )
{
}

Tr2XeSSUpscaling::~Tr2XeSSUpscaling()
{
}

bool Tr2XeSSUpscaling::OnModified( Be::Var* value )
{
	m_dirty = true;
	return true;
}

bool Tr2XeSSUpscaling::IsDirty() const
{
	return m_dirty;
}

bool Tr2XeSSUpscaling::IsApplicable() const
{
	if( s_availability == Availability::XESS_AVAILABILITY_UNKNOWN )
	{
		s_availability = Availability::XESS_AVAILABILITY_UNAVAILABLE;

		USE_MAIN_THREAD_RENDER_CONTEXT();
		xess_context_handle_t c;
		if( xessD3D12CreateContext( renderContext.m_device, &c ) == xess_result_t::XESS_RESULT_SUCCESS )
		{
			xessDestroyContext( c );
			s_availability = Availability::XESS_AVAILABILITY_AVAILABLE;
		}
	}

	return s_availability == Availability::XESS_AVAILABILITY_AVAILABLE;
}

void Tr2XeSSUpscaling::GetJitter( float& x, float& y )
{
	if( m_upscaling == 0.0 )
	{
		x = 0;
		y = 0;
		return;
	}

	m_jitterX = m_jitterSequence[m_jitterIndex].first;
	m_jitterY = m_jitterSequence[m_jitterIndex].second;
	m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();

	x = m_jitterXScale * m_jitterX / (float)m_renderWidth;
	y = -m_jitterYScale * m_jitterY / (float)m_renderHeight;
}


void Tr2XeSSUpscaling::GetJitterOffset( float& x, float& y )
{
	x = m_jitterOffsetXScale * m_jitterX;
	y = m_jitterOffsetYScale * m_jitterY;
}

float Tr2XeSSUpscaling::GetMipLevelBias() const
{
	return log2( 1.0f / m_upscaling ) - 1.0f;
}

void Tr2XeSSUpscaling::GetRenderSize( uint32_t& width, uint32_t& height ) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

Tr2Upscaling::UpscalingType Tr2XeSSUpscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_TEMPORAL;
}

const std::vector<Tr2Upscaling::Setting> Tr2XeSSUpscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = {
		Tr2Upscaling::ULTRA_QUALITY,
		Tr2Upscaling::QUALITY,
		Tr2Upscaling::BALANCED,
		Tr2Upscaling::PERFORMANCE,
	};
	return availableSettings;
}

ID3D12Resource* Tr2XeSSUpscaling::GetTexture( ITr2TextureProvider* textureProvider ) const
{
	if( textureProvider && textureProvider->GetTexture() && textureProvider->GetTexture()->TrinityALImpl_GetObject() )
	{
		return textureProvider->GetTexture()->TrinityALImpl_GetObject()->GetResourceDx12();
	}
	return nullptr;
}

void Tr2XeSSUpscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_xessSetting = XESS_QUALITY_SETTING_PERFORMANCE;
	switch( setting )
	{
	case Tr2Upscaling::ULTRA_QUALITY:
		m_xessSetting = XESS_QUALITY_SETTING_ULTRA_QUALITY;
		break;
	case Tr2Upscaling::QUALITY:
		m_xessSetting = XESS_QUALITY_SETTING_QUALITY;
		break;
	case Tr2Upscaling::BALANCED:
		m_xessSetting = XESS_QUALITY_SETTING_BALANCED;
		break;
	case Tr2Upscaling::PERFORMANCE:
		m_xessSetting = XESS_QUALITY_SETTING_PERFORMANCE;
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2XeSSUpscaling: %d", setting );
		break;
	}

	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;

	xess_2d_t inputRes = { 1, 1 };
	xess_2d_t inputMinRes = { 1, 1 };
	xess_2d_t inputMaxRes = { 1, 1 };
	xess_2d_t outputRes = { displayWidth, displayHeight };

	auto ret = xessGetOptimalInputResolution( s_context, &outputRes, m_xessSetting, &inputRes, &inputMinRes, &inputMaxRes );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get input resolution. Result - %s.", XessUtils::ResultToString( ret ) );
		return;
	}

	m_renderWidth = inputRes.x;
	m_renderHeight = inputRes.y;

	m_upscaling = (float)m_displayWidth / (float)m_renderWidth;
	m_jitterSequence = Jitter::GenerateHaltonSequence( 8 * (uint32_t)powf( ceilf( m_upscaling ), 2.0f ), 2, 3 );

	CCP_LOGNOTICE( "XeSS: Initialized." );
	m_initialized = true;
}

void Tr2XeSSUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
	m_dirty = false;
	m_setup = false;

	xess_d3d12_init_params_t params{};
	params.outputResolution.x = m_displayWidth;
	params.outputResolution.y = m_displayHeight;
	params.qualitySetting = m_xessSetting;
	params.visibleNodeMask = s_creationNodeMask;
	params.creationNodeMask = s_creationNodeMask++;
	params.initFlags = XESS_INIT_FLAG_INVERTED_DEPTH | XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE | XESS_INIT_FLAG_USE_NDC_VELOCITY;

	if( m_useReactive )
	{
		params.initFlags |= XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK;
	}

	params.pPipelineLibrary = nullptr;
	renderContext.FlushAndSyncDx12();
	renderContext.DirtyDescriptorCache();

	xess_result_t ret = xessD3D12Init( s_context, &params );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not initialize. Result - %s.", XessUtils::ResultToString( ret ) );
		return;
	}

	// Get version of XeSS
	xess_version_t ver;
	ret = xessGetVersion( &ver );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get version information. Result - %s.", XessUtils::ResultToString( ret ) );
		return;
	}

	CCP_LOGNOTICE( "XeSS: Version - %u.%u.%u", ver.major, ver.minor, ver.patch );

#if DEBUG
	// Set logging callback here.
	ret = xessSetLoggingCallback( m_context, XESS_LOGGING_LEVEL_DEBUG, LogXeSS );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not set logging callback Result - %s.", XessUtils::ResultToString( ret ) );
		return;
	}
#endif

	m_setup = true;
}

void Tr2XeSSUpscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	if( !m_setup || !m_initialized )
	{
		return;
	}


	auto namer = []( ITr2TextureProvider* texture, const char* name ) {
		if( texture && texture->GetTexture() )
		{
			texture->GetTexture()->SetName( name );
		}
	};

	auto transitioner = []( Tr2RenderContext& renderContext, ITr2TextureProvider* texture, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState ) {
		if( texture && texture->GetTexture() && texture->GetTexture()->TrinityALImpl_GetObject() )
		{
			auto tex = texture->GetTexture()->TrinityALImpl_GetObject();
			if( newState == oldState )
				return;

			renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( tex->GetResourceDx12(), oldState, newState ) );
		}
	};
	namer( textures.input, "xess input" );
	namer( textures.output, "xess output" );
	namer( textures.depth, "xess depth" );
	namer( textures.motion, "xess motion" );
	namer( textures.exposure, "xess exposure" );
	namer( textures.opaqueOnly, "xess opaqueOnly" );
	namer( textures.reactive, "xess reative" );

	// flush all barriers before changing the state of the textures
	renderContext.FlushBarriersDx12();

	const D3D12_RESOURCE_STATES output_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	const D3D12_RESOURCE_STATES input_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	const D3D12_RESOURCE_STATES exposure_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	const D3D12_RESOURCE_STATES reactive_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	// transition from common to unordered access view, since the output texture must be in that state
	transitioner( renderContext, textures.output, output_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	transitioner( renderContext, textures.input, input_state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	transitioner( renderContext, textures.exposure, exposure_state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	if( m_useReactive )
	{
		transitioner( renderContext, textures.reactive, reactive_state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	}
	// flush all barriers before handing control over to XeSS, so the states are applied
	renderContext.FlushBarriersDx12();

	xess_d3d12_execute_params_t params{};
	params.jitterOffsetX = m_jitterX;
	params.jitterOffsetY = m_jitterY;
	params.inputWidth = m_renderWidth;
	params.inputHeight = m_renderHeight;
	params.resetHistory = m_reset ? 1 : 0;
	params.exposureScale = 1.0f;

	params.pColorTexture = GetTexture( textures.input );
	params.pVelocityTexture = GetTexture( textures.motion );
	params.pOutputTexture = GetTexture( textures.output );
	params.pDepthTexture = GetTexture( textures.depth );
	params.pExposureScaleTexture = GetTexture( textures.exposure );
	params.pResponsivePixelMaskTexture = m_useReactive ? GetTexture( textures.reactive ) : nullptr;

	xess_result_t ret = xessD3D12Execute( s_context, renderContext.m_commandList, &params );

	// Trigger error report once.
	if( ret != XESS_RESULT_SUCCESS )
	{
		static bool s_Reported = false;
		if( !s_Reported )
		{
			s_Reported = true;
			CCP_LOGERR( "XeSS: Failed to execute. Result - %s.", XessUtils::ResultToString( ret ) );
		}
	}
	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
	m_reset = false;

	// transition back to what it was
	transitioner( renderContext, textures.output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, output_state );
	transitioner( renderContext, textures.input, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, input_state );
	transitioner( renderContext, textures.exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, exposure_state );
	if( m_useReactive )
	{
		transitioner( renderContext, textures.reactive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, reactive_state );
	}
	renderContext.FlushBarriersDx12();
}

bool Tr2XeSSUpscaling::NeedsExposureTexture() const
{
	return true;
}

bool Tr2XeSSUpscaling::NeedsReactiveTexture() const
{
	return m_useReactive;
}
#endif
