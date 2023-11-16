////////////////////////////////////////////////////////////////////////////////
//
// Created:	January 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PostProcess2_H
#define Tr2PostProcess2_H

#include "Tr2PostProcessRenderInfo.h"
#include "Shader/Tr2Effect.h"
#include "Effects/Tr2PPSignalLossEffect.h"
#include "Effects/Tr2PPGodRaysEffect.h"
#include "Effects/Tr2PPBloomEffect.h"
#include "Effects/Tr2PPDynamicExposureEffect.h"
#include "Effects/Tr2PPFilmGrainEffect.h"
#include "Effects/Tr2PPDesaturateEffect.h"
#include "Effects/Tr2PPFadeEffect.h"
#include "Effects/Tr2PPLutEffect.h"
#include "Effects/Tr2PPVignetteEffect.h"
#include "Effects/Tr2PPFogEffect.h"
#include "Effects/Tr2PPTaaEffect.h"
#include "Effects/Tr2PPDepthOfFieldEffect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2PPSignalLossEffect );
BLUE_DECLARE( Tr2PPGodRaysEffect );
BLUE_DECLARE( Tr2PPBloomEffect );
BLUE_DECLARE( Tr2PPDynamicExposureEffect );
BLUE_DECLARE( Tr2PPFidelityFXEffect );
BLUE_DECLARE( Tr2PPFilmGrainEffect );
BLUE_DECLARE( Tr2PPDesaturateEffect );
BLUE_DECLARE( Tr2PPFadeEffect );
BLUE_DECLARE( Tr2PPLutEffect );
BLUE_DECLARE( Tr2PPVignetteEffect );
BLUE_DECLARE( Tr2PPFogEffect );
BLUE_DECLARE( Tr2PPTaaEffect );
BLUE_DECLARE( Tr2PPDepthOfFieldEffect );

BLUE_CLASS( Tr2PostProcess2 ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcess2( IRoot* lockobj = NULL );
	~Tr2PostProcess2();

	Tr2PPSignalLossEffectPtr GetSignalLoss() { return m_signalLoss; }
	Tr2PPGodRaysEffectPtr GetGodRays() { return m_godRays; }
	Tr2PPBloomEffectPtr GetBloom() { return m_bloom; }
	Tr2PPDynamicExposureEffectPtr GetDynamicExposure() { return m_dynamicExposure; }
	Tr2PPFidelityFXEffectPtr GetFidelityFX() { return m_fidelityFX; }
	Tr2PPFilmGrainEffectPtr GetFilmGrain() { return m_filmGrain; }
	Tr2PPDesaturateEffectPtr GetDesaturate() { return m_desaturate; }
	Tr2PPFadeEffectPtr GetFade() { return m_fade; }
	Tr2PPLutEffectPtr GetLut() { return m_lut; }
	Tr2PPVignetteEffectPtr GetVignette() { return m_vignette; }
	Tr2PPFogEffectPtr GetFog() { return m_fog; }
	Tr2PPTaaEffectPtr GetTaa() { return m_taa; }
	Tr2PPDepthOfFieldEffectPtr GetDepthOfField() { return m_depthOfField; }

	// Helper method for scenes to decide on miplodbias
	float GetMipLodBias() const;

private:
	Tr2PPSignalLossEffectPtr m_signalLoss;
	Tr2PPGodRaysEffectPtr m_godRays;
	Tr2PPBloomEffectPtr m_bloom;
	Tr2PPDynamicExposureEffectPtr m_dynamicExposure;
	Tr2PPFidelityFXEffectPtr m_fidelityFX;
	Tr2PPFilmGrainEffectPtr m_filmGrain;
	Tr2PPDesaturateEffectPtr m_desaturate;
	Tr2PPFadeEffectPtr m_fade;
	Tr2PPLutEffectPtr m_lut;
	Tr2PPVignetteEffectPtr m_vignette;
	Tr2PPFogEffectPtr m_fog;
	Tr2PPTaaEffectPtr m_taa;
	Tr2PPDepthOfFieldEffectPtr m_depthOfField;
};
TYPEDEF_BLUECLASS( Tr2PostProcess2 );

#endif // Tr2PostProcess_H





