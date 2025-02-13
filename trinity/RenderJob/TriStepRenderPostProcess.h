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

BLUE_DECLARE( Tr2Effect );


namespace PostProcessBlur
{
	// Some blur helpers
	enum BlurType
	{
		BT_Big,
		BT_Small
	};

	enum BlurChannel
	{
		BC_r,
		BC_g,
		BC_b,
		BC_a,
		BC_rgba
	};

	enum BlurProcess
	{
		BP_None,
		BP_Minimum,
		BP_Maximum
	};

	enum BlurFinalize
	{
		BF_None,
		BF_MaxOfAllChannels
	};

	struct BlurContext
	{
		BlurType type;
		BlurChannel channel;
		float outputSize;
		BlurProcess process;
		BlurFinalize finalize;

		uint32_t Hash()
		{
			return finalize * 1000 + process * 100 + type * 10 + channel;
		}
	};

	BlurContext CreateBlurContext( float outputSize );
	BlurContext CreateBlurContext( float outputSize, BlurType type );
	BlurContext CreateBlurContext( BlurType type, BlurChannel channel, float outputSize );
	BlurContext CreateBlurContext( BlurType type, BlurChannel channel, BlurProcess process, float outputSize );
	BlurContext CreateBlurContext( BlurType type, BlurChannel channel, BlurProcess process, BlurFinalize finalize, float outputSize );

	BlueSharedString GetBlurChannelOptionValue( BlurChannel channel );
	BlueSharedString GetBlurTypeOptionValue( BlurType type );
	BlueSharedString GetProcessTypeOptionValue( BlurProcess process);
	BlueSharedString GetFinalizeTypeOptionValue( BlurFinalize finalize );
}


namespace AMDSharpening
{
	typedef union
	{
		uint32_t u[4];
		float f[4];
	} uintfloat4;

	Vector4 AsVector( uintfloat4 v );

	struct CASConstants
	{
		uintfloat4 const0 = {};
		uintfloat4 const1 = {};
	};
}

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

	void py__init__( EveSpaceScene * scene, Tr2RenderTarget * source, Tr2RenderTarget* opaque_source );

private:
	// exposure texture 
	void SetupExposureConversion( bool enable, float middleValue );

	// optional sharpening
	void ProcessSharpening( bool enable, uint32_t displayWidth, uint32_t displayHeight, float upscalingAmount );
	Tr2PostProcessRenderInfo::Texture RenderSharpening( Tr2PostProcessRenderInfo::Texture& input, Tr2RenderContext& renderContext );

	// bloom
	bool ProcessBloom( Tr2PPBloomEffect* bloom, Tr2PPDynamicExposureEffect* dynamicExposure );
	Tr2PostProcessRenderInfo::Texture RenderBloom( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom );
	Tr2EffectPtr m_bloomHighPassFilter;

	// godRays
	bool ProcessGodRays( Tr2PPGodRaysEffect* godrays );
	void RenderGodRays( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays );
	Tr2EffectPtr m_godrayEffect;

	// signal loss
	bool ProcessSignalLoss( Tr2PPSignalLossEffect* signalLoss );
	void RenderSignalLoss( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss );
	Tr2EffectPtr m_signalLossEffect;

	// dynamic exposure
	bool ProcessDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, Tr2PPBloomEffect* bloom, Tr2PostProcess2* postProcess );
	void RenderDynamicExposure( Tr2RenderTarget* dest, Tr2RenderContext & renderContext, Tr2PPDynamicExposureEffect * dynamicExposure );
	void RenderDynamicExposureDebug( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure );
	Tr2GpuBufferPtr m_localHistograms;
	Tr2GpuBufferPtr m_histogram;
	Tr2GpuBufferPtr m_exposure;

	uint32_t m_tilesX, m_tilesY, m_localHistogramCount, m_mergeHistogramXDim;

	Tr2EffectPtr m_dynamicExposureCreateHistogramShader;
	Tr2EffectPtr m_dynamicExposureMergeHistogramShader;
	Tr2EffectPtr m_dynamicExposureMeasureExposureShader;
	Tr2EffectPtr m_dynamicExposureToTextureShader;
	Tr2EffectPtr m_dynamicExposureDebugShader;
	Tr2RenderTargetPtr m_exposureTexture;

	// depth of field
	bool ProcessDepthOfField( Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* fx );
	void RenderDepthOfField( Tr2RenderTarget * dest, Tr2RenderContext & renderContext, Tr2PPDepthOfFieldEffect * depthOfField, bool temporal, float upscalingAmount );	
	Tr2EffectPtr m_depthOfFieldCoCShader;
	Tr2EffectPtr m_depthOfFieldBokehBlurShader;
	Tr2EffectPtr m_depthOfFieldBokehFillShader;
	Tr2EffectPtr m_depthOfFieldBokehTAAShader;
	uint32_t m_bokehFrameCounter;
	
	// fog
	bool ProcessFog( Tr2PPFogEffect* fog );
	void RenderFog( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPFogEffect* fog );
	Tr2EffectPtr m_fogColorEffect;
	Tr2EffectPtr m_fogCompositeEffect;

	// TAA
	bool ProcessTaa( Tr2PPTaaEffect* taa );
	void RenderTaa( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa, Tr2PPDynamicExposureEffect* dynamic_exposure );
	Tr2EffectPtr m_taaEffect, m_taaCopyEffect;
	Tr2RenderTargetPtr m_accumulationBuffer0, m_accumulationBuffer1;
	Tr2RenderTargetPtr m_cooldownBuffer;
	Tr2RenderTargetPtr m_opaqueColorBuffer;
	uint32_t m_taaFrameCounter;

	// film grain
	bool ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain );
	void RenderFilmGrain( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPFilmGrainEffect* filmGrain );
	Tr2EffectPtr m_grainShader;

	// desaturate
	void ProcessDesaturate( Tr2PPDesaturateEffect* desaturate );

	// fade
	void ProcessFade( Tr2PPFadeEffect* fade );

	// LUT
	void ProcessLut( std::vector<const Tr2PPLutEffect*> luts );

	// vignette
	void ProcessVignette( Tr2PPVignetteEffect* vignette);

	//Tr2Upscaling::UpscalingType ProcessUpscaling( Tr2PPUpscalingEffect * upscaling, Tr2RenderContext & renderContext );
	Tr2PostProcessRenderInfo::Texture RenderUpscaling( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2UpscalingContextAL* upscalingContext, Tr2PPDynamicExposureEffect* dynamicExposure );

	// tonemapping
	Tr2EffectPtr m_tonemappingEffect;
	bool m_desaturateEnabled;
	bool m_fadeEnabled;
	uint8_t m_lutsEnabled;
	bool m_vignetteEnabled;
	void ProcessTonemapping( Tr2PPTonemappingEffect* tonemapping );

	bool m_sceneDirty;


	EveSpaceScenePtr m_scene;
	Tr2PostProcessRenderInfoPtr m_renderInfo;

	void SetRenderTarget( Tr2RenderTarget* rt );
	Tr2RenderTargetPtr GetRenderTarget() const;


	// General
	PostProcessingQuality m_quality;

	// Reactive mask
	Tr2EffectPtr m_reactiveMaskEffect;
	Tr2RenderTargetPtr m_reactiveMask;

	// Velocity Map
	void SetupVelocityMap();
	Tr2RenderTargetPtr m_velocityBuffer;

	// Common
	void Blur( Tr2RenderTarget& dest, Tr2RenderTarget& src, Tr2RenderContext& renderContext, PostProcessBlur::BlurContext& blurContext );
	void DownSampleDepth( Tr2RenderContext& renderContext, Tr2RenderTarget* destination );

	Tr2EffectPtr m_downsampleDepthEffect;
	std::map<uint32_t, std::pair<Tr2EffectPtr, Tr2EffectPtr>> m_blurEffects;

	Tr2EffectPtr m_blurBigVertical;
	Tr2EffectPtr m_blurBigHorizontal;

	// contrast adaptive sharpening, used if no upscaling is applied or if an upscaling does not have sharpening
	Tr2EffectPtr m_fidelityFxCasShader;

	Be::Time m_lastFrameTime;
	uint32_t m_upscalingContextID;
};

TYPEDEF_BLUECLASS( TriStepRenderPostProcess );

#endif // TriStepRenderPostProcess_H