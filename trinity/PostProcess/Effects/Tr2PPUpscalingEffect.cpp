////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//

#include "StdAfx.h"
#include "Tr2PPUpscalingEffect.h"

#include "Upscaling/Tr2Fsr2Upscaling.h"
#include "Upscaling/Tr2Fsr1Upscaling.h"
#include "Upscaling/Tr2NoopUpscaling.h"
#include "Upscaling/Tr2MetalFxUpscaling.h"
#include "Upscaling/Tr2DlssUpscaling.h"
#include "Upscaling/Tr2XeSSUpscaling.h"

#include "Sharpening/Tr2FidelityFxCas.h"
#include "Sharpening/Tr2NoopSharpening.h"

#include "Tr2Renderer.h"
#include "TriDevice.h"

Tr2PPUpscalingEffect::Tr2PPUpscalingEffect( IRoot* lockobj ) :
	m_currentSetting( Tr2Upscaling::Setting::COUNT ),
	m_renderHeight( 0 ),
	m_renderWidth( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 ),
	m_debugRenderSize( false )
{
	m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2NoopUpscaling>::Class() );
	m_sharpeningEffect.CreateInstance( BlueClassTypeTraits<Tr2FidelityFxCas>::Class() );

	m_currentUpscalingTechnique = Tr2Upscaling::UPSCALING_TECHNIQUE_NONE;
	m_currentSharpeningTechnique = Tr2Sharpening::SHARPENING_TECHNIQUE_CAS;
}

Tr2PPUpscalingEffect::~Tr2PPUpscalingEffect()
{
}

bool Tr2PPUpscalingEffect::IsActive()
{
	return m_display && ( HasSharpening() || HasUpscaling() );
}

bool Tr2PPUpscalingEffect::IsDirty()
{
	return m_upscalingEffect->IsDirty() || m_isDirty;
}

void Tr2PPUpscalingEffect::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
	m_upscalingEffect->Setup( setupContext, renderContext );
	m_sharpeningEffect->Setup( m_renderWidth, m_renderHeight );
}

void Tr2PPUpscalingEffect::SetUpscalingTechnique( Tr2Upscaling::Technique technique, bool frameGeneration )
{
	bool techniqueChanged = technique != m_currentUpscalingTechnique;
	if( techniqueChanged || m_frameGeneration != frameGeneration )
	{
		if( m_currentUpscalingTechnique == Tr2Upscaling::UPSCALING_TECHNIQUE_DLSS && techniqueChanged )
		{
			auto adapter = gTriDev->GetAdapter();
			Tr2Streamline::Toggle( StreamlineUtils::SP_DLSS, adapter, false );
			if( m_frameGeneration )
			{
				Tr2Streamline::Toggle( StreamlineUtils::SP_DLSSG, adapter, false );
			}
		}
		else if( m_currentUpscalingTechnique == Tr2Upscaling::UPSCALING_TECHNIQUE_XESS && techniqueChanged )
		{
			Tr2XeSSUpscaling::Shutdown();
		}

		m_upscalingEffect = nullptr;
		// reset the setting since it guards the applysetting bit
		m_currentSetting = Tr2Upscaling::Setting::COUNT;

		if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_FSR1 )
		{
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2Fsr1Upscaling>::Class() );
		}
		else if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_FSR2 )
		{
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2Fsr2Upscaling>::Class() );
		}
		else if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_SPATIAL )
		{
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2MetalFxSpatialUpscaling>::Class() );
		}
		else if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_TEMPORAL )
		{
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2MetalFxTemporalUpscaling>::Class() );
		}
		else if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_XESS )
		{
			Tr2XeSSUpscaling::Initialize();
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2XeSSUpscaling>::Class() );
		}
		else if( technique == Tr2Upscaling::UPSCALING_TECHNIQUE_DLSS )
		{
			auto adapter = gTriDev->GetAdapter();
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2DlssUpscaling>::Class() );
			m_upscalingEffect->SetUseFrameGeneration( frameGeneration );
			Tr2Streamline::Toggle( StreamlineUtils::SP_DLSS, adapter, true );
			if( m_frameGeneration != frameGeneration )
			{
				Tr2Streamline::Toggle( StreamlineUtils::SP_DLSSG, adapter, m_frameGeneration );
			}
		}
		else
		{
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2NoopUpscaling>::Class() );
		}

		m_currentUpscalingTechnique = technique;
		m_frameGeneration = frameGeneration;

		if( !m_upscalingEffect->IsApplicable() )
		{
			std::string upscalingName = std::string( "" );

			switch( technique )
			{
			case Tr2Upscaling::UPSCALING_TECHNIQUE_DLSS:
				upscalingName = "DLSS";
				break;
			case Tr2Upscaling::UPSCALING_TECHNIQUE_FSR1:
				upscalingName = "FSR1";
				break;
			case Tr2Upscaling::UPSCALING_TECHNIQUE_FSR2:
				upscalingName = "FSR2";
				break;
			case Tr2Upscaling::UPSCALING_TECHNIQUE_XESS:
				upscalingName = "XeSS";
				break;
			case Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_SPATIAL:
				upscalingName = "MetalFX Spatial";
				break;
			case Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_TEMPORAL:
				upscalingName = "MetalFX Temporal";
				break;
			default:
				upscalingName = "UNKNOWN";
				break;
			}
			CCP_LOGWARN( "%s upscaling is not available, defaulting to no upscaling.", upscalingName.c_str() );
			m_upscalingEffect = nullptr;
			m_upscalingEffect.CreateInstance( BlueClassTypeTraits<Tr2NoopUpscaling>::Class() );
			m_currentUpscalingTechnique = Tr2Upscaling::UPSCALING_TECHNIQUE_NONE;
		}

		if( m_currentUpscalingTechnique != Tr2Upscaling::UPSCALING_TECHNIQUE_NONE )
		{
			SetSharpeningTechnique( Tr2Sharpening::SHARPENING_TECHNIQUE_NONE );
		}
		else
		{
			SetSharpeningTechnique( Tr2Sharpening::SHARPENING_TECHNIQUE_CAS );
		}

		m_isDirty = true;
	}
}

void Tr2PPUpscalingEffect::SetSharpeningTechnique( Tr2Sharpening::Technique technique )
{
	if( technique != m_currentSharpeningTechnique )
	{
		m_currentSharpeningTechnique = technique;
		m_sharpeningEffect = nullptr;
		if( technique == Tr2Sharpening::SHARPENING_TECHNIQUE_CAS )
		{
			m_sharpeningEffect.CreateInstance( BlueClassTypeTraits<Tr2FidelityFxCas>::Class() );
		}
		else
		{
			m_sharpeningEffect.CreateInstance( BlueClassTypeTraits<Tr2NoopSharpening>::Class() );
		}

		if( technique != Tr2Sharpening::SHARPENING_TECHNIQUE_NONE )
		{
			SetUpscalingTechnique( Tr2Upscaling::UPSCALING_TECHNIQUE_NONE, false );
		}

		m_isDirty = true;
	}
}

void Tr2PPUpscalingEffect::ApplySetting( Tr2Upscaling::Setting setting, uint32_t displayWidth, uint32_t displayHeight )
{
	if( m_currentSetting != setting || displayWidth != m_displayWidth || displayHeight != m_displayHeight )
	{
		m_displayWidth = displayWidth;
		m_displayHeight = displayHeight;
		m_upscalingEffect->ApplySetting( setting, m_displayWidth, m_displayHeight );
		m_isDirty = true;
		m_currentSetting = setting;
		m_upscalingEffect->GetRenderSize( m_renderWidth, m_renderHeight );
	}
}

float Tr2PPUpscalingEffect::GetUpscalingAmount() const
{
	return float( m_displayWidth ) / float( m_renderWidth );
}

float Tr2PPUpscalingEffect::GetMipLodBias()
{
	return m_upscalingEffect->GetMipLevelBias();
}

void Tr2PPUpscalingEffect::GetRenderSize( uint32_t& width, uint32_t& height )
{
	width = m_renderWidth;
	height = m_renderHeight;
}

void Tr2PPUpscalingEffect::GetDisplaySize( uint32_t& width, uint32_t& height )
{
	width = m_displayWidth;
	height = m_displayHeight;
}

void Tr2PPUpscalingEffect::GetJitter( float& x, float& y )
{
	if( m_debugRenderSize )
	{
		x = 0.0f;
		y = 0.0f;
		return;
	}
	m_upscalingEffect->GetJitter( x, y );
}

void Tr2PPUpscalingEffect::GetJitterOffset( float& x, float& y )
{
	if( m_debugRenderSize )
	{
		x = 0.0f;
		y = 0.0f;
		return;
	}
	m_upscalingEffect->GetJitterOffset( x, y );
}

Tr2Upscaling::UpscalingType Tr2PPUpscalingEffect::GetUpscalingType()
{
	return m_upscalingEffect->GetUpscalingType();
}

bool Tr2PPUpscalingEffect::IsTemporal() const
{
	return m_upscalingEffect->GetUpscalingType() == Tr2Upscaling::UT_TEMPORAL;
}

bool Tr2PPUpscalingEffect::HasUpscaling() const
{
	return m_currentUpscalingTechnique != Tr2Upscaling::Technique::UPSCALING_TECHNIQUE_NONE;
}

bool Tr2PPUpscalingEffect::HasSharpening() const
{
	return m_currentSharpeningTechnique != Tr2Sharpening::Technique::SHARPENING_TECHNIQUE_NONE;
}

bool Tr2PPUpscalingEffect::NeedsExposureTexture() const
{
	return m_upscalingEffect->NeedsExposureTexture();
}

bool Tr2PPUpscalingEffect::NeedsReactiveTexture() const
{
	return m_upscalingEffect->NeedsReactiveTexture();
}

void Tr2PPUpscalingEffect::Render( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	if( m_debugRenderSize )
	{
		// need to undo the jittering
		// render only the actual size of the input
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		renderContext.m_esm.PushRenderTarget( *textures.output->GetTexture() );
		float upscaling = GetUpscalingAmount();
		Tr2Renderer::DrawTexture( renderContext, *textures.input->GetTexture(), Vector2( 0, 0 ), Vector2( upscaling, upscaling ), Tr2Blitter::FILTER_LINEAR );
		renderContext.m_esm.PopRenderTarget();
		return;
	}

	m_upscalingEffect->Dispatch( renderContext, renderInfo, textures, sceneInformation );
	m_sharpeningEffect->Dispatch( renderContext, renderInfo, textures );
}
