// Copyright © 2019 CCP ehf.

#pragma once
#ifndef Tr2PostProcess2_H
#define Tr2PostProcess2_H

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
#include "Effects/Tr2PPColorCorrectionEffect.h"
#include "Effects/Tr2PPGenericEffect.h"

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
BLUE_DECLARE( Tr2PPColorCorrectionEffect );

BLUE_CLASS( Tr2PostProcess2 ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcess2( IRoot* lockobj = NULL );
	~Tr2PostProcess2();

	Tr2PPSignalLossEffectPtr GetSignalLossIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPGodRaysEffectPtr GetGodRaysIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPBloomEffectPtr GetBloomIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPDynamicExposureEffectPtr GetDynamicExposureIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPFilmGrainEffectPtr GetFilmGrainIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPDesaturateEffectPtr GetDesaturateIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPFadeEffectPtr GetFadeIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPVignetteEffectPtr GetVignetteIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPFogEffectPtr GetFogIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPTaaEffectPtr GetTaaIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPDepthOfFieldEffectPtr GetDepthOfFieldIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPTonemappingEffectPtr GetTonemappingIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPColorCorrectionEffectPtr GetColorCorrectionIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	Tr2PPGenericEffectPtr GetGenericEffectIfAvailable( PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;

	void GetAvilableSortedLuts( std::vector<const Tr2PPLutEffect*> & container, PostProcess::Quality qualitySetting = PostProcess::Quality::HIGH ) const;
	void ClearLuts();

	void SetSignalLoss( Tr2PPSignalLossEffectPtr effect );
	void SetGodRays( Tr2PPGodRaysEffectPtr effect );
	void SetBloom( Tr2PPBloomEffectPtr effect );
	void SetDynamicExposure( Tr2PPDynamicExposureEffectPtr effect );
	void SetFilmGrain( Tr2PPFilmGrainEffectPtr effect );
	void SetDesaturate( Tr2PPDesaturateEffectPtr effect );
	void SetFade( Tr2PPFadeEffectPtr effect );
	void AddLut( Tr2PPLutEffectPtr effect );
	void SetVignette( Tr2PPVignetteEffectPtr effect );
	void SetFog( Tr2PPFogEffectPtr effect );
	void SetTaa( Tr2PPTaaEffectPtr effect );
	void SetDepthOfField( Tr2PPDepthOfFieldEffectPtr effect );
	void SetTonemapping( Tr2PPTonemappingEffectPtr effect );
	void SetColorCorrection( Tr2PPColorCorrectionEffectPtr effect );
	void SetGenericEffect( Tr2PPGenericEffectPtr effect );

	// Helper method for scenes to decide on miplodbias
	float GetMipLodBias() const;

	float m_exposureAdjustment = 0;

private:
	Tr2PPSignalLossEffectPtr m_signalLoss;
	Tr2PPGodRaysEffectPtr m_godRays;
	Tr2PPBloomEffectPtr m_bloom;
	Tr2PPDynamicExposureEffectPtr m_dynamicExposure;
	Tr2PPFilmGrainEffectPtr m_filmGrain;
	Tr2PPDesaturateEffectPtr m_desaturate;
	Tr2PPFadeEffectPtr m_fade;
	Tr2PPLutEffectPtr m_lut;

	PTr2PPLutEffectVector m_luts;
	Tr2PPVignetteEffectPtr m_vignette;
	Tr2PPFogEffectPtr m_fog;
	Tr2PPTaaEffectPtr m_taa;
	Tr2PPDepthOfFieldEffectPtr m_depthOfField;
	Tr2PPTonemappingEffectPtr m_tonemapping;
	Tr2PPColorCorrectionEffectPtr m_colorCorrection;
	Tr2PPGenericEffectPtr m_generic;
};
TYPEDEF_BLUECLASS( Tr2PostProcess2 );

#endif // Tr2PostProcess_H
