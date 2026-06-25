// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"

extern int g_dynamicExposureQualityRequirement;

#define RETURN_IF_ACTIVE( effect, currentQuality, minQuality )         \
	if( effect && effect->IsActive() && currentQuality >= minQuality ) \
	{                                                                  \
		return effect;                                                 \
	}                                                                  \
	return nullptr;

Tr2PostProcess2::Tr2PostProcess2( IRoot* lockobj ) :
	PARENTLOCK( m_luts )
{
}


Tr2PostProcess2::~Tr2PostProcess2()
{
}


float Tr2PostProcess2::GetMipLodBias() const
{
	float taa_bias = 0.0f;
	if( m_taa )
	{
		taa_bias = m_taa->IsActive() ? -1.0f : 0.0f;
	}

	return taa_bias;
}

void Tr2PostProcess2::GetAvilableSortedLuts( std::vector<const Tr2PPLutEffect*>& container, PostProcess::Quality qualitySetting ) const
{
	if( qualitySetting < PostProcess::LOW )
	{
		// This will never happen, but it will open for a scenario where we need to have game specific quality settings for each post process effect
		return;
	}
	container.clear();
	if( m_lut && m_lut->IsActive() )
	{
		container.push_back( m_lut );
	}
	for( const auto& lut : m_luts )
	{
		if( lut->IsActive() )
		{
			container.push_back( lut );
		}
	}
	std::sort( container.begin(), container.end(), []( const Tr2PPLutEffect* a, const Tr2PPLutEffect* b ) { return a->m_influence < b->m_influence; } );
}

void Tr2PostProcess2::AddLut( Tr2PPLutEffectPtr effect )
{
	m_luts.Append( effect );
}

void Tr2PostProcess2::ClearLuts()
{
	m_luts.Clear();
}

Tr2PPSignalLossEffectPtr Tr2PostProcess2::GetSignalLossIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_signalLoss, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetSignalLoss( Tr2PPSignalLossEffectPtr effect )
{
	m_signalLoss = effect;
}

Tr2PPGodRaysEffectPtr Tr2PostProcess2::GetGodRaysIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_godRays, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetGodRays( Tr2PPGodRaysEffectPtr effect )
{
	m_godRays = effect;
}

Tr2PPBloomEffectPtr Tr2PostProcess2::GetBloomIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_bloom, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetBloom( Tr2PPBloomEffectPtr effect )
{
	m_bloom = effect;
}

Tr2PPDynamicExposureEffectPtr Tr2PostProcess2::GetDynamicExposureIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_dynamicExposure, qualitySetting, (PostProcess::Quality)g_dynamicExposureQualityRequirement );
}

void Tr2PostProcess2::SetDynamicExposure( Tr2PPDynamicExposureEffectPtr effect )
{
	m_dynamicExposure = effect;
}

Tr2PPFilmGrainEffectPtr Tr2PostProcess2::GetFilmGrainIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_filmGrain, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetFilmGrain( Tr2PPFilmGrainEffectPtr effect )
{
	m_filmGrain = effect;
}

Tr2PPDesaturateEffectPtr Tr2PostProcess2::GetDesaturateIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_desaturate, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetDesaturate( Tr2PPDesaturateEffectPtr effect )
{
	m_desaturate = effect;
}

Tr2PPFadeEffectPtr Tr2PostProcess2::GetFadeIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_fade, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetFade( Tr2PPFadeEffectPtr effect )
{
	m_fade = effect;
}

Tr2PPVignetteEffectPtr Tr2PostProcess2::GetVignetteIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_vignette, qualitySetting, PostProcess::MEDIUM );
}

void Tr2PostProcess2::SetVignette( Tr2PPVignetteEffectPtr effect )
{
	m_vignette = effect;
}

Tr2PPFogEffectPtr Tr2PostProcess2::GetFogIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_fog, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetFog( Tr2PPFogEffectPtr effect )
{
	m_fog = effect;
}

Tr2PPTaaEffectPtr Tr2PostProcess2::GetTaaIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_taa, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetTaa( Tr2PPTaaEffectPtr effect )
{
	m_taa = effect;
}

Tr2PPDepthOfFieldEffectPtr Tr2PostProcess2::GetDepthOfFieldIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_depthOfField, qualitySetting, PostProcess::HIGH );
}

void Tr2PostProcess2::SetDepthOfField( Tr2PPDepthOfFieldEffectPtr effect )
{
	m_depthOfField = effect;
}

Tr2PPTonemappingEffectPtr Tr2PostProcess2::GetTonemappingIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_tonemapping, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetTonemapping( Tr2PPTonemappingEffectPtr effect )
{
	m_tonemapping = effect;
}

Tr2PPColorCorrectionEffectPtr Tr2PostProcess2::GetColorCorrectionIfAvailable( PostProcess::Quality qualitySetting ) const
{
	RETURN_IF_ACTIVE( m_colorCorrection, qualitySetting, PostProcess::LOW );
}

void Tr2PostProcess2::SetColorCorrection( Tr2PPColorCorrectionEffectPtr effect )
{
	m_colorCorrection = effect;
}

Tr2PPGenericEffectPtr Tr2PostProcess2::GetGenericEffectIfAvailable( PostProcess::Quality qualitySetting ) const
{
	if( m_generic )
	{
		RETURN_IF_ACTIVE( m_generic, qualitySetting, m_generic->m_quality );
	}
	return nullptr;
}

void Tr2PostProcess2::SetGenericEffect( Tr2PPGenericEffectPtr effect )
{
	m_generic = effect;
}
