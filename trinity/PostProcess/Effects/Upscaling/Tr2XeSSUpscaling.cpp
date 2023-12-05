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
uint32_t Tr2XeSSUpscaling::s_creationNodeMask = 0;

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
	m_useReactive( false ),
	m_context( nullptr )
{
}

Tr2XeSSUpscaling::~Tr2XeSSUpscaling()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( m_context )
	{
		xessDestroyContext( m_context );
		CCP_LOGNOTICE( "XeSS context destroyed" );
	}

	m_context = nullptr;
}

xess_context_handle_t Tr2XeSSUpscaling::CreateContext( Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );
	xess_context_handle_t context;
	
	auto status = xessD3D12CreateContext( renderContext.GetPrimaryRenderContext().m_device, &context );
	if( status != XESS_RESULT_SUCCESS )
	{
		CCP_LOGNOTICE( "XeSS: XeSS is not supported on this device. Result - %s.", ResultToString( status ) );
		return nullptr;
	}

	if( XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver( context ) )
	{
		CCP_LOGNOTICE( "XeSS: Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality" );
		return nullptr;
	}
	CCP_LOGNOTICE( "XeSS context created" );
	return context;
}

const char* Tr2XeSSUpscaling::ResultToString( xess_result_t result ) const
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
		// try to create a context
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( CreateContext( renderContext ) == nullptr )
		{
			s_availability = Availability::XESS_AVAILABILITY_UNAVAILABLE;
		}
		else
		{
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

	uint32_t totalPhases = 8 * (uint32_t)powf( ceilf(m_upscaling), 2.0f );
	m_jitterX = Jitter::Halton( m_jitterIndex, 3 ) - 0.5f;
	m_jitterY = Jitter::Halton( m_jitterIndex, 2 ) - 0.5f;
	m_jitterIndex = m_jitterIndex % totalPhases + 1;

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

void Tr2XeSSUpscaling::GetRenderSize(uint32_t& width, uint32_t& height) const
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

	if( m_context == nullptr )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_context = CreateContext(renderContext);
	}

	if( m_context == nullptr )
	{
		return;
	}


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

	auto ret = xessGetOptimalInputResolution( m_context, &outputRes, m_xessSetting, &inputRes, &inputMinRes, &inputMaxRes );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get input resolution. Result - %s.", ResultToString( ret ) );
		return;
	}

	m_renderWidth = inputRes.x;
	m_renderHeight = inputRes.y;

	m_upscaling = (float)m_displayWidth / (float)m_renderWidth;

	CCP_LOGNOTICE( "XeSS: Initialized." );
	m_initialized = true;
}

void Tr2XeSSUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext)
{
	m_dirty = false;
	m_setup = false;

	xess_d3d12_init_params_t params{};
	params.outputResolution.x = m_displayWidth;
	params.outputResolution.y = m_displayHeight;
	params.qualitySetting = m_xessSetting;
	params.creationNodeMask = s_creationNodeMask++;
	params.initFlags = XESS_INIT_FLAG_INVERTED_DEPTH | XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE | XESS_INIT_FLAG_USE_NDC_VELOCITY;

	if( m_useReactive )
	{
		params.initFlags |= XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK;
	}

	params.pPipelineLibrary = nullptr;
	renderContext.FlushBarriersDx12();

	xess_result_t ret = xessD3D12Init( m_context, &params );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not initialize. Result - %s.", ResultToString( ret ) );
		return;
	}
	
	// Get version of XeSS
	xess_version_t ver;
	ret = xessGetVersion( &ver );
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not get version information. Result - %s.", ResultToString( ret ) );
		return;
	}

	CCP_LOGNOTICE( "XeSS: Version - %u.%u.%u", ver.major, ver.minor, ver.patch );

//#if DEBUG
	// Set logging callback here.
	ret = xessSetLoggingCallback( m_context, XESS_LOGGING_LEVEL_DEBUG, LogXeSS );
	;
	if( ret != XESS_RESULT_SUCCESS )
	{
		CCP_LOGERR( "XeSS: Could not set logging callback Result - %s.", ResultToString( ret ) );
		return;

	}
//#endif

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

	auto transitioner = []( Tr2RenderContext& renderContext, ITr2TextureProvider* texture, D3D12_RESOURCE_STATES newState, bool transitionTo ) {
		if( texture && texture->GetTexture() && texture->GetTexture()->TrinityALImpl_GetObject() )
		{
			auto tex = texture->GetTexture()->TrinityALImpl_GetObject();
			auto defaultState = tex->GetResourceState();
			if( newState == defaultState )
				return;

			if( transitionTo )
				renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( tex->GetResourceDx12(), defaultState, newState ) );
			else
				renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( tex->GetResourceDx12(), newState, defaultState ) );
		}
	};
	namer( textures.input, "xess input" );
	namer( textures.output, "xess output" );
	namer( textures.depth, "xess depth" );
	namer( textures.motion, "xess motion" );
	namer( textures.exposure, "xess exposure" );
	namer( textures.opaqueOnly, "xess opaqueOnly" );

	// flush all barriers before handing control over to XeSS
	renderContext.FlushBarriersDx12();

	// transition from common to unordered access view, since the output texture must be in that state
	transitioner( renderContext, textures.output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true );
	transitioner( renderContext, textures.input, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true );
	transitioner( renderContext, textures.exposure, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false );
	transitioner( renderContext, textures.exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true );

	// flush all barriers before handing control over to XeSS
	renderContext.FlushBarriersDx12();

	xess_d3d12_execute_params_t params{};
	float x, y;
	GetJitterOffset( x, y );
	params.jitterOffsetX = x;
	params.jitterOffsetY = y;
	params.inputWidth = m_renderWidth;
	params.inputHeight = m_renderHeight;
	params.resetHistory = m_reset ? 1 : 0;
	params.exposureScale = 1.0f;

	params.pColorTexture = GetTexture(textures.input);
	params.pVelocityTexture = GetTexture(textures.motion);
	params.pOutputTexture = GetTexture(textures.output);
	params.pDepthTexture = GetTexture(textures.depth);
	params.pExposureScaleTexture = GetTexture(textures.exposure);
	params.pResponsivePixelMaskTexture = m_useReactive ? GetTexture( textures.reactive ) : nullptr;

	xess_result_t ret = xessD3D12Execute( m_context, renderContext.m_commandList, &params );

	// Trigger error report once.
	if( ret != XESS_RESULT_SUCCESS )
	{
		static bool s_Reported = false;
		if( !s_Reported )
		{
			s_Reported = true;
			CCP_LOGERR( "XeSS: Failed to execute. Result - %s.", ResultToString( ret ) );
		}
	}
	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
	m_reset = false;

	// transition back to common
	transitioner( renderContext, textures.output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false );
	transitioner( renderContext, textures.input, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false );
	transitioner( renderContext, textures.exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false );
	transitioner( renderContext, textures.exposure, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true );
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
