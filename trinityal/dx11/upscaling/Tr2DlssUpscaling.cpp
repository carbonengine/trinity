////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2DlssUpscaling.h"
#include "include/Tr2StreamlineAL.h"

#include "../Tr2AdapterStructures.h"
#include "../Tr2VideoAdapterInfoALDx11.h" 
#include "../Tr2RenderContextDx11.h"
#include "../Tr2TextureALDx11.h"

extern bool g_upscalingDebug;

namespace DlssUtils
{
	sl::Resource GenerateTextureResource( Tr2TextureAL* texture )
	{
		if( texture && texture->IsValid() )
		{
			return { sl::ResourceType::eTex2d, texture->TrinityALImpl_GetObject()->GetResourceDx11(), nullptr, nullptr, 0 };
		}

		return { sl::ResourceType::eTex2d, nullptr, nullptr, nullptr, 0 };
	}
}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	Tr2UpscalingTechniqueDx11( renderContext, technique, setting, frameGeneration, adapter ),
	m_adapter( 0 ),
	m_contextIndex( 0 ),
	m_frameToken( nullptr )
{

	m_frameGeneration = false;

	m_streamlineSetup = false;

	
	//We need to create a dummy device to figure out if DLSS and frame generation actually is supported.

	{
		if( SL_FAILED( res, Tr2StreamlineAL::InitializeStreamline() ) )
		{
			CCP_LOGERR( "Streamline initialization failed with error %d", res );
			return;
		}

		
		CComPtr<IDXGIAdapter1> dxgiAdapter;
		CComPtr<IDXGIOutput> output;
		Tr2VideoAdapterInfo::GetVideoAdapterDX11( adapter, &dxgiAdapter, &output );


		const D3D_FEATURE_LEVEL levelWanted = D3D_FEATURE_LEVEL_11_0;
		
		D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;

		CComPtr<ID3D11Device> device;
		D3D_FEATURE_LEVEL levelSupported;
		CComPtr<ID3D11DeviceContext> m_context;
		
		
		if( SUCCEEDED( D3D11CreateDevice( dxgiAdapter, driverType, 0, 0, &levelWanted, 1, D3D11_SDK_VERSION, &device, &levelSupported, &m_context ) ) )
		{
			if( Tr2StreamlineAL::SetDevice( device, adapter ) == sl::Result::eOk )
			{
				m_isAvailable = Tr2StreamlineAL::IsDLSSAvailable();
			}
		}

		Tr2StreamlineAL::ReleaseStreamline();
	}
	
	//We're done gathering info, initialize Streamline again and await the actual device!
	
	if( SL_FAILED( res, Tr2StreamlineAL::InitializeStreamline() ) )
	{
		CCP_LOGERR( "Streamline initialization failed with error %d", res );
		return;
	}

	// framegen is not available on dx11!

	m_streamlineSetup = true;
	SanitizeState();
}

Tr2DlssUpscalingTechnique::~Tr2DlssUpscalingTechnique()
{
	Tr2StreamlineAL::ReleaseStreamline( );
}

bool Tr2DlssUpscalingTechnique::IsAvailable( ) const
{
	return m_isAvailable;
}

std::vector<Tr2UpscalingAL::Setting> Tr2DlssUpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::QUALITY, 
		Tr2UpscalingAL::BALANCED, 
		Tr2UpscalingAL::PERFORMANCE,
		Tr2UpscalingAL::ULTRA_PERFORMANCE
	};
}

bool Tr2DlssUpscalingTechnique::IsTemporal() const 
{
	return true;
}

void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx11::MarkFrameEvent( frameEvent );
	if( frameEvent == Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED )
	{
		if( SL_FAILED( res, Tr2StreamlineAL::GetNewFrameToken( m_frameToken ) ) )
		{
			CCP_LOGERR( "Could not get new frame token for Nvidia Streamline (%d)", res );
			return;
		}
		for( auto& context : m_contexts )
		{
			( (Tr2DlssUpscalingContext*)( context.second.get() ) )->SetFrameToken( m_frameToken );
		}
	}
}

void Tr2DlssUpscalingTechnique::AttachToDevice(CComPtr<ID3D11Device>& device) 
{

	if( SL_FAILED( res, Tr2StreamlineAL::SetDevice( device, m_adapter ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device. Error code %d", res );
		return;
	}
	CCP_LOGNOTICE( "NVidia Streamline successfully attached to device and adapter" );
}

Tr2UpscalingContextAL* Tr2DlssUpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2DlssUpscalingContext( m_setting, false, params, m_contextIndex++, m_frameToken );
}

Tr2DlssUpscalingContext::Tr2DlssUpscalingContext( 
	Tr2UpscalingAL::Setting setting, 
	bool frameGeneration, 
	Tr2UpscalingAL::UpscalingContextParams params,
	uint32_t contextNumber, 
	sl::FrameToken* frameToken ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_viewHandle( sl::ViewportHandle( contextNumber ) ),
	m_dlssMode( sl::DLSSMode::eOff ),
	m_frameToken( frameToken ),
	m_setup( false )
{


	m_dlssMode = sl::DLSSMode::eOff;
	switch( m_setting )
	{
	case Tr2UpscalingAL::QUALITY:
		m_dlssMode = sl::DLSSMode::eMaxQuality;
		break;
	case Tr2UpscalingAL::BALANCED:
		m_dlssMode = sl::DLSSMode::eBalanced;
		break;
	case Tr2UpscalingAL::PERFORMANCE:
		m_dlssMode = sl::DLSSMode::eMaxPerformance;
		break;
	case Tr2UpscalingAL::ULTRA_PERFORMANCE:
		m_dlssMode = sl::DLSSMode::eUltraPerformance;
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2DlssUpscaling: %d", m_setting );
		break;
	}

	m_dlssOptions.outputWidth = m_displayWidth;
	m_dlssOptions.outputHeight = m_displayHeight;
	m_dlssOptions.mode = m_dlssMode;

	if( SL_FAILED( result, Tr2StreamlineAL::GetDLSSOptimalSettings( m_dlssOptions, m_optimalSettings ) ) )
	{
		CCP_LOGERR( "Getting Optimal Settings for DLSS failed (%d)", result );
		return;
	}
	m_renderWidth = m_optimalSettings.optimalRenderWidth == 0 ? m_displayWidth : m_optimalSettings.optimalRenderWidth;
	m_renderHeight = m_optimalSettings.optimalRenderHeight == 0 ? m_displayHeight : m_optimalSettings.optimalRenderHeight;

	m_upscaling = (float)m_dlssOptions.outputHeight / (float)m_renderHeight;

	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );

	m_dlssOptions.colorBuffersHDR = sl::eTrue;
	m_dlssOptions.mode = m_dlssMode;
	m_dlssOptions.useAutoExposure = sl::eFalse;

	if( SL_FAILED( res, Tr2StreamlineAL::SetDLSSOptions( m_viewHandle, m_dlssOptions ) ) )
	{
		CCP_LOGERR( "Setting DLSS Options failed with (%d)", res );
		return;
	}

	m_nisOptions.hdrMode = sl::NISHDR::eLinear;
	m_nisOptions.mode = sl::NISMode::eScaler;
	m_nisOptions.sharpness = 0.1f;

	if( SL_FAILED( res, Tr2StreamlineAL::SetNISOptions( m_viewHandle, m_nisOptions ) ) )
	{
		CCP_LOGERR( "Setting NIS Options failed with (%d)", res );
		return;
	}

	auto dims = Tr2BitmapDimensions( m_displayWidth, m_displayHeight, 1, params.sourceFormat );
	m_dlssOutput.Create( dims, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, params.renderContext.GetPrimaryRenderContext() );

	CCP_LOGNOTICE( "DLSS Options set successfully" );

	m_setup = true;
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{
	Tr2StreamlineAL::FreeResources( sl::kFeatureDLSS, m_viewHandle );
	Tr2StreamlineAL::FreeResources( sl::kFeatureNIS, m_viewHandle );
}

void Tr2DlssUpscalingContext::SetFrameToken( sl::FrameToken* frameToken )
{
	m_frameToken = frameToken;
}

bool Tr2DlssUpscalingContext::HasSharpening() const
{
	return true;
}

void Tr2DlssUpscalingContext::UpdateJitter()
{
	if( m_setup )
	{
		m_jitterX = m_jitterSequence[m_jitterIndex].first;
		m_jitterY = m_jitterSequence[m_jitterIndex].second;

		m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
	}
}

uint32_t Tr2DlssUpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPAQUE_ONLY | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | Tr2UpscalingAL::DispatchRequirements::VELOCITY;
}

sl::Result Tr2DlssUpscalingContext::ReadyDLSSResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto inputResource = DlssUtils::GenerateTextureResource( dispatchParameters.input );
	auto outputResource = DlssUtils::GenerateTextureResource( &m_dlssOutput );
	auto depthResource = DlssUtils::GenerateTextureResource( dispatchParameters.depth );
	auto velocityResource = DlssUtils::GenerateTextureResource( dispatchParameters.velocity );
	auto exposureResource = DlssUtils::GenerateTextureResource( dispatchParameters.exposure );
	auto opaqueOnlyResource = DlssUtils::GenerateTextureResource( dispatchParameters.opaqueOnly );

	sl::Extent renderExtent = {};
	renderExtent.height = m_renderHeight;
	renderExtent.width = m_renderWidth;

	sl::Extent displayExtent = {};
	displayExtent.height = m_displayHeight;
	displayExtent.width = m_displayWidth;

	sl::Extent exposureExtent = {};
	exposureExtent.height = 1;
	exposureExtent.width = 1;

	sl::ResourceTag opaqueColorInTag = sl::ResourceTag{ &opaqueOnlyResource, sl::kBufferTypeOpaqueColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };
	sl::ResourceTag depthTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag mvecTag = sl::ResourceTag{ &velocityResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag exposureTag = sl::ResourceTag{ &exposureResource, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, &exposureExtent };

	sl::ResourceTag resources[] = { colorInTag, opaqueColorInTag, colorOutTag, depthTag, mvecTag, exposureTag };
	
	if( SL_FAILED( res, Tr2StreamlineAL::SetTags( m_params.renderContext, m_viewHandle, resources, 6 ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
		return res;
	}
	return sl::Result::eOk;
}


sl::Result Tr2DlssUpscalingContext::ReadyNISResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto inputResource = DlssUtils::GenerateTextureResource( &m_dlssOutput );
	auto outputResource = DlssUtils::GenerateTextureResource( dispatchParameters.output );

	sl::Extent displayExtent = {};
	displayExtent.height = m_displayHeight;
	displayExtent.width = m_displayWidth;

	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };

	sl::ResourceTag resources[] = { colorInTag, colorOutTag };

	if( SL_FAILED( res, Tr2StreamlineAL::SetTags( m_params.renderContext, m_viewHandle, resources, 2 ) ) )
	{
		CCP_LOGERR( "NIS Failed to tag resources (%d)", res );
		return res;
	}
	return sl::Result::eOk;
}

void Tr2DlssUpscalingContext::SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	m_commonConstants.cameraAspectRatio = dispatchParameters.aspectRatio;
	m_commonConstants.cameraFar = dispatchParameters.backClip;
	m_commonConstants.cameraFOV = dispatchParameters.fieldOfView;
	m_commonConstants.cameraMotionIncluded = sl::eTrue;
	m_commonConstants.cameraNear = dispatchParameters.frontClip;
	m_commonConstants.cameraFwd = sl::float3( dispatchParameters.cameraForward[0], dispatchParameters.cameraForward[1], dispatchParameters.cameraForward[2] );
	m_commonConstants.cameraPos = sl::float3( dispatchParameters.cameraPos[0], dispatchParameters.cameraPos[1], dispatchParameters.cameraPos[2] );
	m_commonConstants.cameraRight = sl::float3( dispatchParameters.cameraRight[0], dispatchParameters.cameraRight[1], dispatchParameters.cameraRight[2] );
	m_commonConstants.cameraUp = sl::float3( dispatchParameters.cameraUp[0], dispatchParameters.cameraUp[1], dispatchParameters.cameraUp[2] );

	m_commonConstants.cameraViewToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.projection );
	m_commonConstants.clipToCameraView = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.invProjection );
	m_commonConstants.clipToPrevClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.clipToPrevClip );
	m_commonConstants.depthInverted = sl::eTrue;
	m_commonConstants.jitterOffset = sl::float2( m_jitterX, m_jitterY );
	m_commonConstants.motionVectors3D = sl::eFalse;
	m_commonConstants.motionVectorsDilated = sl::eFalse;
	m_commonConstants.motionVectorsJittered = sl::eFalse;
	m_commonConstants.mvecScale = sl::float2( 1, 1 );
	m_commonConstants.orthographicProjection = sl::eFalse;
	m_commonConstants.prevClipToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.prevClipToClip );
	// unused things
	m_commonConstants.cameraPinholeOffset = sl::float2( 0, 0 );
	m_commonConstants.reset = m_reset ? sl::eTrue : sl::eFalse;

	if( SL_FAILED( result, Tr2StreamlineAL::SetConstants( m_commonConstants, *m_frameToken, m_viewHandle ) ) )
	{
		CCP_LOGERR( "Setting Nvidia Streamline common constants failed (%d)", result );
	}
	m_reset = false;
}

Tr2UpscalingAL::Result Tr2DlssUpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	SetCommonConstants( dispatchParameters );

	const sl::BaseStructure* handle[] = { &m_viewHandle };

	if( SL_FAILED( res, ReadyDLSSResources( dispatchParameters ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	if( SL_FAILED( result, Tr2StreamlineAL::EvaluateFeature( m_params.renderContext, sl::kFeatureDLSS, *m_frameToken, handle, _countof( handle ) ) ) )
	{
		CCP_LOGERR( "Failed to Dispatch DLSS (%d)", result );
	}

	if( SL_FAILED( res, ReadyNISResources( dispatchParameters ) ) )
	{
		CCP_LOGERR( "NIS Failed to tag resources (%d)", res );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	if( SL_FAILED( result, Tr2StreamlineAL::EvaluateFeature( m_params.renderContext, sl::kFeatureNIS, *m_frameToken, handle, _countof( handle ) ) ) )
	{
		CCP_LOGERR( "Failed to Dispatch NIS (%d)", result );
	}
	return Tr2UpscalingAL::OK;
}


#endif
