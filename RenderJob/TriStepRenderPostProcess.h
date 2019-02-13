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


// -------------------------------------------------------------
// Description:
//   A render step to render post process. Takes a scene and the source buffer as a parameter
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepRenderPostProcess ) : public TriRenderStep
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

	void py__init__( EveSpaceScene* scene, Tr2RenderTarget* source );
private:
	void PushRenderTarget( Tr2RenderContext& renderContext, Tr2RenderTarget* rt = nullptr );

	// bloom
	bool ProcessBloom( Tr2PPBloomEffect* bloom );
	void RenderBloom( Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom );
	Tr2EffectPtr m_bloomHighPassFilter;
	Tr2EffectPtr m_bloomHorizontalBlur;
	Tr2EffectPtr m_bloomVerticalBlur;

	// godRays
	bool ProcessGodRays( Tr2PPGodRaysEffect* godrays );
	void RenderGodRays( Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays );
	Tr2EffectPtr m_godRayDownSampleEffect;
	Tr2EffectPtr m_godrayEffect;

	// signal loss
	bool ProcessSignalLoss( Tr2PPSignalLossEffect* signalLoss );
	void RenderSignalLoss( Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss );
	Tr2EffectPtr m_signalLossEffect;

	// dynamic exposure
	bool ProcessDynamicExposure( Tr2PPDynamicExposureEffect* dynamicExposure );
	void RenderDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure );
	Tr2GpuBufferPtr m_localHistograms;
	Tr2GpuBufferPtr m_histogram;
	Tr2GpuBufferPtr m_exposure;

	uint32_t m_tilesX, m_tilesY, m_localHistogramCount, m_mergeHistogramXDim;

	Tr2EffectPtr m_dynamicExposureCreateHistogramShader;
	Tr2EffectPtr m_dynamicExposureMergeHistogramShader;
	Tr2EffectPtr m_dynamicExposureMeasureExposureShader;

	// film grain
	void ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain );

	// desaturate
	void ProcessDesaturate( Tr2PPDesaturateEffect* desaturate );

	// fade
	void ProcessFade( Tr2PPFadeEffect* fade );

	// LUT
	void ProcessLut( Tr2PPLutEffect* lut );

	// tonemapping
	Tr2EffectPtr m_tonemappingEffect;
	EveSpaceScenePtr m_scene;
	Tr2PostProcessRenderInfoPtr m_renderInfo;

        // General
	PostProcessingQuality m_quality;
};

TYPEDEF_BLUECLASS( TriStepRenderPostProcess );

#endif // TriStepRenderPostProcess_H
