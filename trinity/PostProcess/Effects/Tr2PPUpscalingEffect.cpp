////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//

#include "StdAfx.h"
#include "Tr2PPUpscalingEffect.h"

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
	m_sharpeningEffect.CreateInstance( BlueClassTypeTraits<Tr2FidelityFxCas>::Class() );

	m_currentSharpeningTechnique = Tr2Sharpening::SHARPENING_TECHNIQUE_CAS;
}

Tr2PPUpscalingEffect::~Tr2PPUpscalingEffect()
{
}

bool Tr2PPUpscalingEffect::IsActive()
{
	return m_display && HasSharpening();
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
		/*if( m_currentUpscalingTechnique == Tr2Upscaling::UPSCALING_TECHNIQUE_DLSS && techniqueChanged )
		{
			auto adapter = gTriDev->GetAdapter();
			Tr2Streamline::Toggle( StreamlineUtils::SP_DLSS, adapter, false );
			if( m_frameGeneration )
			{
				Tr2Streamline::Toggle( StreamlineUtils::SP_DLSSG, adapter, false );
			}
		}*/

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
	return Tr2Upscaling::UpscalingType::UT_SPATIAL;
}

bool Tr2PPUpscalingEffect::IsTemporal() const
{
	return false;
}

bool Tr2PPUpscalingEffect::HasUpscaling() const
{
	return false;
}

bool Tr2PPUpscalingEffect::HasSharpening() const
{
	return m_currentSharpeningTechnique != Tr2Sharpening::Technique::SHARPENING_TECHNIQUE_NONE;
}

bool Tr2PPUpscalingEffect::NeedsExposureTexture() const
{
	return false;
}

bool Tr2PPUpscalingEffect::UsesExposureTexture() const
{
	return false;
}

bool Tr2PPUpscalingEffect::NeedsReactiveTexture() const
{
	return false;
}

void Tr2PPUpscalingEffect::Render( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	m_sharpeningEffect->Dispatch( renderContext, renderInfo, textures );
}
