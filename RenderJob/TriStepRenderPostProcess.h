////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once
#ifndef TriStepRenderPostProcess_H
#define TriStepRenderPostProcess_H

#include "TriRenderStep.h"
#include "Eve/EveSpaceScene.h"
#include "Tr2RenderTarget.h"
#include "PostProcess/Tr2PostProcessRenderInfo.h"
#include "Shader/Tr2Effect.h"

BLUE_DECLARE( Tr2PPFidelityFXEffect );
BLUE_DECLARE( Tr2Effect );


// -------------------------------------------------------------
// Description:
//   A render step to render post process. Takes a scene and the source buffer as a parameter
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepRenderPostProcess ) : 
	public TriRenderStep,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderPostProcess( IRoot* lockobj = 0 );
	~TriStepRenderPostProcess( void );

	enum PostProcessingQuality {
		LOW,
		MEDIUM,
		HIGH,

		COUNT
	};

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	// INotify
	bool OnModified( Be::Var * val );

	void py__init__( EveSpaceScene* scene, Tr2RenderTarget* source );
private:

	// Some blur helpers
	enum BlurType
	{
		Big,
		Small
	};

	enum BlurChannel
	{
		r,
		g,
		b,
		rgba
	};

	// bloom
	bool ProcessBloom( Tr2PPBloomEffect* bloom, Tr2PPDynamicExposureEffect* dynamicExposure );
	Tr2PostProcessRenderInfo::Texture RenderBloom( Tr2RenderTarget* dest, Tr2RenderContext & renderContext, Tr2PPBloomEffect * bloom );
	Tr2EffectPtr m_bloomHighPassFilter;

	// godRays
	bool ProcessGodRays( Tr2PPGodRaysEffect* godrays );
	void RenderGodRays( Tr2RenderTarget* dest, Tr2RenderContext & renderContext, Tr2PPGodRaysEffect * godrays );
	Tr2EffectPtr m_godrayEffect;

	// signal loss
	bool ProcessSignalLoss( Tr2PPSignalLossEffect* signalLoss );
	void RenderSignalLoss( Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss );
	Tr2EffectPtr m_signalLossEffect;

	// dynamic exposure
	bool ProcessDynamicExposure( Tr2RenderContext & renderContext, Tr2PPDynamicExposureEffect * dynamicExposure, Tr2PPBloomEffect * bloom );
	void RenderDynamicExposure( Tr2RenderTarget* dest, Tr2RenderContext & renderContext, Tr2PPDynamicExposureEffect * dynamicExposure );
	Tr2GpuBufferPtr m_localHistograms;
	Tr2GpuBufferPtr m_histogram;
	Tr2GpuBufferPtr m_exposure;

	uint32_t m_tilesX, m_tilesY, m_localHistogramCount, m_mergeHistogramXDim;

	Tr2EffectPtr m_dynamicExposureCreateHistogramShader;
	Tr2EffectPtr m_dynamicExposureMergeHistogramShader;
	Tr2EffectPtr m_dynamicExposureMeasureExposureShader;

	// depth of field
	bool ProcessDepthOfField( Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* fx );
	void RenderDepthOfField( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* depthOfField );	
	Tr2EffectPtr m_depthOfFieldCoCShader;
	Tr2EffectPtr m_depthOfFieldBokehBlendShader;
	Tr2EffectPtr m_depthOfFieldBokehForegroundBlurShader;
	Tr2EffectPtr m_depthOfFieldBokehForegroundFillShader;
	Tr2EffectPtr m_depthOfFieldBokehBackgroundBlurShader;
	Tr2EffectPtr m_depthOfFieldBokehBackgroundFillShader;
	
	// fidelityFX
	bool ProcessFidelityFX( Tr2RenderContext& renderContext, Tr2PPFidelityFXEffect* fx );
	Tr2PostProcessRenderInfo::Texture RenderFidelityFX( Tr2RenderTarget * src, Tr2RenderContext & renderContext, Tr2PPFidelityFXEffect * fx );
	Tr2EffectPtr m_fidelityFXShader;

	// fog
	bool ProcessFog( Tr2PPFogEffect* fog );
	void RenderFog( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPFogEffect* fog );
	Tr2EffectPtr m_fogColorEffect;
	Tr2EffectPtr m_fogCompositeEffect;

	// TAA
	bool ProcessTaa( Tr2PPTaaEffect* taa );
	void RenderTaa( Tr2RenderTarget * dest, Tr2RenderContext & renderContext, Tr2PPTaaEffect * taa );
	Tr2EffectPtr m_taaEffect;
	Tr2RenderTargetPtr m_accumulationBuffer;
	Tr2RenderTargetPtr m_velocityBuffer;

	// film grain
	bool ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain );
	void RenderFilmGrain( Tr2RenderTarget* dest, Tr2RenderContext & renderContext, Tr2PPFilmGrainEffect * filmGrain );
	Tr2EffectPtr m_grainShader;

	// desaturate
	void ProcessDesaturate( Tr2PPDesaturateEffect* desaturate );

	// fade
	void ProcessFade( Tr2PPFadeEffect* fade );

	// LUT
	void ProcessLut( Tr2PPLutEffect* lut );

	// vignette
	void ProcessVignette( Tr2PPVignetteEffect* vignette);

	// tonemapping
	Tr2EffectPtr m_tonemappingEffect;
	bool m_desaturateEnabled;
	bool m_fadeEnabled;
	bool m_lutEnabled;
	bool m_vignetteEnabled;
	bool m_sceneDirty;

	EveSpaceScenePtr m_scene;
	Tr2PostProcessRenderInfoPtr m_renderInfo;

	void SetRenderTarget( Tr2RenderTarget* rt );
	Tr2RenderTargetPtr GetRenderTarget() const;


    // General
	PostProcessingQuality m_quality;

	// Common
	void Blur( Tr2RenderTarget * dest, Tr2RenderTarget * src, Tr2RenderContext & renderContext, BlurType blurType = BlurType::Big, BlurChannel blurChannel = BlurChannel::rgba, float size = 0.5f );
	void DownSampleDepth( Tr2RenderContext & renderContext, Tr2RenderTarget* destination );

	Tr2EffectPtr m_downsampleDepthEffect;
	std::map<uint32_t, std::pair<Tr2EffectPtr, Tr2EffectPtr>> m_blurEffects;

	Tr2EffectPtr m_blurBigVertical;
	Tr2EffectPtr m_blurBigHorizontal;
};

TYPEDEF_BLUECLASS( TriStepRenderPostProcess );

#endif // TriStepRenderPostProcess_H
