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
#include "Effects/Tr2PPTonemappingEffect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2PPSignalLossEffect );
BLUE_DECLARE( Tr2PPGodRaysEffect );
BLUE_DECLARE( Tr2PPBloomEffect );
BLUE_DECLARE( Tr2PPDynamicExposureEffect );
BLUE_DECLARE( Tr2PPFilmGrainEffect );
BLUE_DECLARE( Tr2PPDesaturateEffect );
BLUE_DECLARE( Tr2PPFadeEffect );
BLUE_DECLARE( Tr2PPLutEffect );
BLUE_DECLARE_VECTOR( Tr2PPLutEffect );
BLUE_DECLARE( Tr2PPVignetteEffect );
BLUE_DECLARE( Tr2PPFogEffect );
BLUE_DECLARE( Tr2PPTaaEffect );
BLUE_DECLARE( Tr2PPDepthOfFieldEffect );
BLUE_DECLARE( Tr2PPUpscalingEffect );
BLUE_DECLARE( Tr2PPTonemappingEffect );

BLUE_CLASS( Tr2PostProcess2 ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcess2( IRoot* lockobj = NULL );
	~Tr2PostProcess2();

	Tr2PPSignalLossEffectPtr GetSignalLoss() const { return m_signalLoss; }
	Tr2PPGodRaysEffectPtr GetGodRays() const{ return m_godRays; }
	Tr2PPBloomEffectPtr GetBloom() const { return m_bloom; }
	Tr2PPDynamicExposureEffectPtr GetDynamicExposure() const { return m_dynamicExposure; }
	Tr2PPFilmGrainEffectPtr GetFilmGrain() const { return m_filmGrain; }
	Tr2PPDesaturateEffectPtr GetDesaturate() const { return m_desaturate; }
	Tr2PPFadeEffectPtr GetFade() const { return m_fade; }
	void GetLuts( std::vector<const Tr2PPLutEffect*> &container ) const;
	Tr2PPVignetteEffectPtr GetVignette() const { return m_vignette; }
	Tr2PPFogEffectPtr GetFog() const { return m_fog; }
	Tr2PPTaaEffectPtr GetTaa() const { return m_taa; }
	Tr2PPDepthOfFieldEffectPtr GetDepthOfField() const { return m_depthOfField; }
	Tr2PPTonemappingEffectPtr GetTonemapping() const { return m_tonemapping; }

	void SetSignalLoss(Tr2PPSignalLossEffectPtr effect) { m_signalLoss = effect; }
	void SetGodRays(Tr2PPGodRaysEffectPtr effect) { m_godRays = effect; }
	void SetBloom(Tr2PPBloomEffectPtr effect) { m_bloom = effect; }
	void SetDynamicExposure(Tr2PPDynamicExposureEffectPtr effect) { m_dynamicExposure = effect; }
	void SetFilmGrain(Tr2PPFilmGrainEffectPtr effect) { m_filmGrain = effect; }
	void SetDesaturate(Tr2PPDesaturateEffectPtr effect) { m_desaturate = effect; }
	void SetFade(Tr2PPFadeEffectPtr effect) { m_fade = effect; }
	void AddLut(Tr2PPLutEffectPtr effect) { m_luts.Append(effect); }
	void SetVignette(Tr2PPVignetteEffectPtr effect) { m_vignette = effect; }
	void SetFog(Tr2PPFogEffectPtr effect) { m_fog = effect; }
	void SetTaa(Tr2PPTaaEffectPtr effect) { m_taa = effect; }
	void SetDepthOfField(Tr2PPDepthOfFieldEffectPtr effect) { m_depthOfField = effect; }
	void SetTonemapping(Tr2PPTonemappingEffectPtr effect) { m_tonemapping = effect; }

	// Helper method for scenes to decide on miplodbias
	float GetMipLodBias() const;


	// Jittering
	void GetJitter( uint32_t renderWidth, uint32_t renderHeight, float& x, float& y );

private:
	Tr2PPSignalLossEffectPtr m_signalLoss;
	Tr2PPGodRaysEffectPtr m_godRays;
	Tr2PPBloomEffectPtr m_bloom;
	Tr2PPDynamicExposureEffectPtr m_dynamicExposure;
	Tr2PPFilmGrainEffectPtr m_filmGrain;
	Tr2PPDesaturateEffectPtr m_desaturate;
	Tr2PPFadeEffectPtr m_fade;
	PTr2PPLutEffectVector m_luts;
	Tr2PPVignetteEffectPtr m_vignette;
	Tr2PPFogEffectPtr m_fog;
	Tr2PPTaaEffectPtr m_taa;
	Tr2PPDepthOfFieldEffectPtr m_depthOfField;
	Tr2PPTonemappingEffectPtr m_tonemapping;
};
TYPEDEF_BLUECLASS( Tr2PostProcess2 );

#endif // Tr2PostProcess_H





