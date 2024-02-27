////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2DlssUpscaling.h"
#include "TriDevice.h"

Tr2DlssUpscaling::Tr2DlssUpscaling( IRoot* lockobj ):
	m_sharpeningAmount( 0.35f ),
	m_upscaling( 0.f ),
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 ),
	m_jitterIndex( 0 ),
	m_jitterY( 0 ),
	m_jitterX( 0 ),
	m_optimalSettings( {} ),
	m_options( {} ),
	m_useSharpening( true ),
	m_usingExposure( false ),
	m_currentSetting( Tr2Upscaling::Setting::COUNT ),
	m_useFrameGeneration( false ),
	m_vramUsage( 0 ),
	m_minWidthHeight( 0 ),
	m_actualFrames( 0 )
{

}

Tr2DlssUpscaling::~Tr2DlssUpscaling()
{

}

bool Tr2DlssUpscaling::OnModified( Be::Var* value )
{
	m_dirty = true;
	if( IsMatch( value, m_useSharpening ) )
	{
		auto adapter = gTriDev->GetAdapter();
		Tr2Streamline::Toggle( StreamlineUtils::SP_NIS, adapter, m_useSharpening );
	}
	return true;
}

bool Tr2DlssUpscaling::IsDirty() const
{
	return m_dirty;
}

bool Tr2DlssUpscaling::IsApplicable() const
{
	auto adapter = gTriDev->GetAdapter();

	return Tr2Streamline::IsAvailable( StreamlineUtils::SP_DLSS, adapter );
}

bool Tr2DlssUpscaling::SupportsFrameGeneration() const
{
	auto adapter = gTriDev->GetAdapter();

	return Tr2Streamline::IsAvailable( StreamlineUtils::SP_DLSSG, adapter );
}

Tr2Upscaling::UpscalingType Tr2DlssUpscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_TEMPORAL;
}

void Tr2DlssUpscaling::GetJitter( float& x, float& y )
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

	x = m_jitterX / (float)m_renderWidth;
	y = -m_jitterY / (float)m_renderHeight;
}

void Tr2DlssUpscaling::GetJitterOffset( float& x, float& y )
{
	x = m_jitterX;
	y = m_jitterY;
}

float Tr2DlssUpscaling::GetMipLevelBias() const
{
	return log2f((float)m_renderWidth/(float)m_displayWidth) - 1.0f;
}

void Tr2DlssUpscaling::GetRenderSize( uint32_t& width, uint32_t& height ) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

const std::vector<Tr2Upscaling::Setting> Tr2DlssUpscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = {
		Tr2Upscaling::QUALITY,
		Tr2Upscaling::BALANCED,
		Tr2Upscaling::PERFORMANCE,
		Tr2Upscaling::ULTRA_PERFORMANCE
	};
	return availableSettings;
}

void Tr2DlssUpscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
	Tr2DlssPlugin dlss;
	Tr2NisPlugin nis;

	if( setting == m_currentSetting && displayWidth == m_displayWidth && displayHeight == m_displayHeight )
	{
		return;	
	}

	m_currentSetting = setting;
	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;

	if( Tr2Streamline::IsRunning() )
	{
		if( Tr2Streamline::GetDlssPlugin( dlss ) )
		{
			m_options.mode = DlssUtils::dmBalanced;
			switch( setting )
			{
			case Tr2Upscaling::ULTRA_QUALITY:
				m_options.mode = DlssUtils::dmUltraQuality;
				break;
			case Tr2Upscaling::QUALITY:
				m_options.mode = DlssUtils::dmQuality;
				break;
			case Tr2Upscaling::BALANCED:
				m_options.mode = DlssUtils::dmBalanced;
				break;
			case Tr2Upscaling::PERFORMANCE:
				m_options.mode = DlssUtils::dmPerformance;
				break;
			case Tr2Upscaling::ULTRA_PERFORMANCE:
				m_options.mode = DlssUtils::dmUltraPerformance;
				break;
			default:
				CCP_LOGERR( "Invalid Setting Applied to Tr2DlssUpscaling: %d", setting );
				break;
			}

			m_options.outputHeight = m_displayHeight;
			m_options.outputWidth = m_displayWidth;
			m_options.hdr = true;
			m_optimalSettings = dlss.GetOptimalSettings( m_options.mode, m_options.outputWidth, m_options.outputHeight );

			m_renderWidth = m_optimalSettings.renderWidth == 0 ? m_displayWidth : m_optimalSettings.renderWidth;
			m_renderHeight = m_optimalSettings.renderHeight == 0 ? m_displayHeight : m_optimalSettings.renderHeight;

			m_upscaling = (float)m_options.outputHeight / (float)m_renderHeight;
			
			m_jitterSequence = Jitter::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );
			m_options.renderWidth = m_renderWidth;
			m_options.renderHeight = m_renderHeight;
		}
	}
}

void Tr2DlssUpscaling::SetUseFrameGeneration( bool enable )
{
	m_useFrameGeneration = enable;
}

bool Tr2DlssUpscaling::GetUseFrameGeneration( ) const 
{
	return m_useFrameGeneration;
}

void Tr2DlssUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
	m_usingExposure = setupContext.hasExposureTexture;
	if( Tr2Streamline::IsRunning() )
	{
		Tr2DlssPlugin dlss;
		if( Tr2Streamline::GetDlssPlugin( dlss ) )
		{
			dlss.SetSettings( m_options );
		}
		if( m_useSharpening )
		{
			Tr2NisPlugin nis;
			if( Tr2Streamline::GetNisPlugin( nis ) )
			{
				NisUtils::NisOptions nisOptions = {
					m_sharpeningAmount,
					false
				};
				nis.SetSettings( nisOptions );
			}
			else
			{
				m_useSharpening = false;
			}
		}
	}
	m_dirty = false;
}

void Tr2DlssUpscaling::SetConstants( const Tr2Upscaling::SceneInformation& sceneInformation )
{
	auto proj = sceneInformation.projection;
	auto invproj = Inverse( sceneInformation.projection );
	// not sure this is correct, but lets test it...
	auto reprojection = sceneInformation.reprojection;
	auto invreprojection = Inverse( sceneInformation.reprojection );
	auto view = sceneInformation.view;

	auto camPos = sceneInformation.cameraPos;
	auto forward = Vector3( view._13, view._23, view._33 );
	auto up = Vector3( view._12, view._22, view._32 );
	auto right = Vector3( view._11, view._21, view._31 );

	StreamlineUtils::CommonConstants commonConst = {
		// projection
		{
			proj._11,
			proj._12,
			proj._13,
			proj._14,
			proj._21,
			proj._22,
			proj._23,
			proj._24,
			proj._31,
			proj._32,
			proj._33,
			proj._34,
			proj._41,
			proj._42,
			proj._43,
			proj._44
		},
		// inverse projection
		{
			invproj._11,
			invproj._12,
			invproj._13,
			invproj._14,
			invproj._21,
			invproj._22,
			invproj._23,
			invproj._24,
			invproj._31,
			invproj._32,
			invproj._33,
			invproj._34,
			invproj._41,
			invproj._42,
			invproj._43,
			invproj._44 
		},
		// reprojection
		{
			reprojection._11,
			reprojection._12,
			reprojection._13,
			reprojection._14,
			reprojection._21,
			reprojection._22,
			reprojection._23,
			reprojection._24,
			reprojection._31,
			reprojection._32,
			reprojection._33,
			reprojection._34,
			reprojection._41,
			reprojection._42,
			reprojection._43,
			reprojection._44 
		},
		// inverse reprojection
		{
			invreprojection._11,
			invreprojection._12,
			invreprojection._13,
			invreprojection._14,
			invreprojection._21,
			invreprojection._22,
			invreprojection._23,
			invreprojection._24,
			invreprojection._31,
			invreprojection._32,
			invreprojection._33,
			invreprojection._34,
			invreprojection._41,
			invreprojection._42,
			invreprojection._43,
			invreprojection._44 
		},
		// jitter offset
		{
			m_jitterX, m_jitterY
		},
		// motion scale
		{1,1},
		{ camPos.x, camPos.y, camPos.z },
		{ up.x, up.y, up.z },
		{ right.x, right.y, right.z },
		{ forward.x, forward.y, forward.z },
		sceneInformation.frontClip,
		sceneInformation.backClip,
		sceneInformation.fieldOfView,
		sceneInformation.aspectRatio,
		sceneInformation.reset
	};

	Tr2Streamline::SetCommonConstants( commonConst );
}

void Tr2DlssUpscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	if( Tr2Streamline::IsRunning() )
	{ 
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		// flush all barriers before handing control over to DLSS
		renderContext.FlushBarriersDx12();
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11

#endif

		Tr2DlssPlugin dlss;
		Tr2NisPlugin nis;

		if( Tr2Streamline::GetDlssPlugin( dlss ) )
		{
			DlssUtils::DlssResources resources = {};
			resources.input = textures.input ? textures.input->GetTexture() : nullptr;
			resources.opaqueOnly = textures.opaqueOnly ? textures.opaqueOnly->GetTexture() : nullptr;
			resources.output = textures.output ? textures.output->GetTexture() : nullptr;
			resources.depth = textures.depth ? textures.depth->GetTexture() : nullptr;
			resources.velocity = textures.motion ? textures.motion->GetTexture() : nullptr;
			resources.exposure = textures.exposure ? textures.exposure->GetTexture() : nullptr;

			SetConstants( sceneInformation );
			dlss.SetResources( m_options, resources, renderContext );
			dlss.Dispatch( renderContext );

			m_actualFrames = dlss.GetNumFramesActuallyPresented();
			m_vramUsage = dlss.GetEstimatedVRAMUsageInBytes();
			m_minWidthHeight = dlss.GetMinWidthOrHeight();
		}
		if( m_useSharpening && Tr2Streamline::GetNisPlugin( nis ) )
		{
			NisUtils::NisResources resources = {};
			resources.input = textures.input ? textures.output->GetTexture() : nullptr;
			resources.output = textures.output ? textures.output->GetTexture() : nullptr;

			nis.SetResources( resources, renderContext );
			nis.Dispatch( renderContext );
		}


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		// the descriptor cache is dirty, mark it so
		renderContext.DirtyDescriptorCache();
		renderContext.FlushBarriersDx12();
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
#endif
	}
}

bool Tr2DlssUpscaling::NeedsExposureTexture() const
{
	return true;
}

bool Tr2DlssUpscaling::UsesExposureTexture() const
{
	return m_usingExposure;
}


bool Tr2DlssUpscaling::NeedsReactiveTexture() const
{
	return false;
}