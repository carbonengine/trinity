////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#include "../RenderJob/TriRenderStep.h"
#include "Eve/EveSpaceScene.h"
#include "Tr2RenderTarget.h"
#include "Shader/Tr2Effect.h"
#include "../Tr2GpuResourcePool.h"
#include <PostProcess/Effects/Tr2PPBloomEffect.h>

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
		BlurType type = BT_Big;
		BlurChannel channel = BC_rgba;
		BlurProcess process = BP_None;
		BlurFinalize finalize = BF_None;

		uint32_t Hash() const
		{
			return finalize * 1000 + process * 100 + type * 10 + channel;
		}
	};

	BlurContext CreateBlurContext( BlurType type = BlurType::BT_Big, BlurChannel channel = BlurChannel::BC_rgba, BlurProcess process = BlurProcess::BP_None, BlurFinalize finalize = BlurFinalize::BF_None );

	BlueSharedString GetBlurChannelOptionValue( BlurChannel channel );
	BlueSharedString GetBlurTypeOptionValue( BlurType type );
	BlueSharedString GetProcessTypeOptionValue( BlurProcess process);
	BlueSharedString GetFinalizeTypeOptionValue( BlurFinalize finalize );
}

namespace GaussianDistribution
{
	struct GaussianData
	{
		Vector3 overallWeight;
		uint32_t count;
		Vector4 weightOffset[Bloom::MAX_FILTER_STEPS / 2];
	};

	float NormalDistribution( float x, float sigma, float weight );
	GaussianData CalculateGaussianPassParameters( float radius, float centerWeight, float normalizingFactor, Vector3 overallWeight, Vector2 direction );
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
BLUE_CLASS( Tr2PostProcessRenderer ) : 
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcessRenderer( IRoot* lockobj = 0 );

	enum class BloomDebugMode
	{
		BLOOM_DEBUG_NONE,
		BLOOM_DEBUG_ALL,
		BLOOM_DEBUG_STEP1,
		BLOOM_DEBUG_STEP2,
		BLOOM_DEBUG_STEP3,
		BLOOM_DEBUG_STEP4,
		BLOOM_DEBUG_STEP5,
		BLOOM_DEBUG_STEP6,
	};

	void Execute( 
		const Tr2TextureAL& destination, 
		Tr2GpuResourcePool::Texture source, 
		Tr2GpuResourcePool::Texture depthMap, 
		Tr2GpuResourcePool::Texture velocity, 
		Tr2GpuResourcePool::Texture opaqueColor, 
		EveSpaceScene* scene,
		Tr2UpscalingContextAL* upscalingContext, 
		Tr2GpuResourcePool& gpuResourcePool, 
		Tr2RenderContext& renderContext );

	PostProcess::Quality GetPostProcessingQuality() const;
	void SetPostProcessingQuality( PostProcess::Quality quality );

private:

	struct DownsampleData
	{
		Vector2 invSourceSize;
		float _padding1;
		float _padding2;
	};

	// exposure texture 
	void SetupExposureConversion( bool enable, float middleValue );

	// optional sharpening
	void ProcessSharpening( bool enable, uint32_t displayWidth, uint32_t displayHeight, float upscalingAmount );
	Tr2GpuResourcePool::Texture RenderSharpening( Tr2GpuResourcePool::Texture & input, Tr2GpuResourcePool & gpuResourcePool, Tr2RenderContext & renderContext );

	// bloom
	bool ProcessBloom( Tr2PPBloomEffect* bloom, Tr2PPDynamicExposureEffect* dynamicExposure );
	Tr2GpuResourcePool::Texture RenderBloom( Tr2GpuResourcePool::Texture& dest, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom );
	Tr2GpuResourcePool::Texture RenderBloomDebug( 
		std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS>& downsample,
		std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS>& upsample,
		Tr2GpuResourcePool::Texture& blitCurrent,
		Tr2GpuResourcePool& gpuResourcePool, 
		Tr2RenderContext& renderContext );
	;
	Tr2EffectPtr m_downSamplerLuminancePreserve;
	Tr2EffectPtr m_downSampler;
	Tr2EffectPtr m_upsamplerHorizontal;
	Tr2EffectPtr m_upsamplerVertical;
	Tr2EffectPtr m_bloomHighPassFilter;
	Tr2ConstantBufferAL m_bloomConstantBuffer;

	bool m_useNewBloom;
	BloomDebugMode m_bloomDebugMode;
	Tr2EffectPtr m_bloomDebugShader;


	// godRays
	bool ProcessGodRays( Tr2PPGodRaysEffect* godrays );
	void RenderGodRays( const Tr2TextureAL& dest, const Tr2TextureAL& depth, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays );
	Tr2EffectPtr m_godrayEffect;

	// signal loss
	bool ProcessSignalLoss( Tr2PPSignalLossEffect* signalLoss );
	void RenderSignalLoss( const Tr2TextureAL& dest, Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss );
	Tr2EffectPtr m_signalLossEffect;

	// dynamic exposure
	bool ProcessDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, Tr2PPBloomEffect* bloom, Tr2PostProcess2* postProcess );
	Tr2GpuResourcePool::Buffer RenderDynamicExposure( const Tr2TextureAL& dest, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure );
	void RenderDynamicExposureDebug( Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, const Tr2BufferAL& histogramBuffer );

	Tr2EffectPtr m_dynamicExposureCreateHistogramShader;
	Tr2EffectPtr m_dynamicExposureMergeHistogramShader;
	Tr2EffectPtr m_dynamicExposureMeasureExposureShader;
	Tr2EffectPtr m_dynamicExposureToTextureShader;
	Tr2EffectPtr m_dynamicExposureDebugShader;

	// depth of field
	bool ProcessDepthOfField( Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* fx );
	void RenderDepthOfField( const Tr2TextureAL& dest, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* depthOfField, bool temporal, float upscalingAmount );	
	Tr2EffectPtr m_depthOfFieldCoCShader;
	Tr2EffectPtr m_depthOfFieldBokehBlurShader;
	Tr2EffectPtr m_depthOfFieldBokehFillShader;
	Tr2EffectPtr m_depthOfFieldBokehTAAShader;
	uint32_t m_bokehFrameCounter;
	
	// fog
	bool ProcessFog( Tr2PPFogEffect* fog );
	void RenderFog( const Tr2TextureAL& dest, const Tr2TextureAL& source, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPFogEffect* fog );
	Tr2EffectPtr m_fogColorEffect;
	Tr2EffectPtr m_fogCompositeEffect;

	// TAA
	bool ProcessTaa( Tr2PPTaaEffect* taa );
	void RenderTaa( const Tr2TextureAL& dest, const Tr2TextureAL& velocity, const Tr2TextureAL& opaqueColor, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa, Tr2PPDynamicExposureEffect* dynamic_exposure );
	Tr2EffectPtr m_taaEffect, m_taaCopyEffect;
	uint32_t m_taaFrameCounter;

	// film grain
	bool ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain );
	void RenderFilmGrain( const Tr2TextureAL& dest, Tr2RenderContext& renderContext, Tr2PPFilmGrainEffect* filmGrain );
	Tr2EffectPtr m_grainShader;

	// desaturate
	void ProcessDesaturate( Tr2PPDesaturateEffect* desaturate );

	// fade
	void ProcessFade( Tr2PPFadeEffect* fade );

	// LUT
	void ProcessLut( std::vector<const Tr2PPLutEffect*> luts );

	// vignette
	void ProcessVignette( Tr2PPVignetteEffect* vignette);

	Tr2GpuResourcePool::Texture RenderUpscaling( 
		const Tr2TextureAL& dest, 
		const Tr2TextureAL& depth, 
		const Tr2TextureAL& velocity, 
		const Tr2TextureAL& opaqueColor, 
		const Matrix& reprojection,
		Tr2GpuResourcePool& gpuResourcePool, 
		Tr2RenderContext& renderContext, 
		Tr2UpscalingContextAL* upscalingContext, 
		Tr2PPDynamicExposureEffect* dynamicExposure );

	// tonemapping
	Tr2EffectPtr m_tonemappingEffect;
	bool m_desaturateEnabled;
	bool m_fadeEnabled;
	uint8_t m_lutsEnabled;
	bool m_vignetteEnabled;
	void ProcessColorCorrection( Tr2PPColorCorrectionEffect* colorCorrection );
	void ProcessTonemapping( Tr2PPTonemappingEffect* tonemapping );
	
	bool ProcessGenericEffect( Tr2PPGenericEffectPtr genericEffect );
	void RenderGenericEffect( const Tr2TextureAL& dest, const Tr2TextureAL& src, Tr2RenderContext& renderContext, Tr2PPGenericEffectPtr genericEffect );

	// General
	PostProcess::Quality m_quality;

	// Reactive mask
	Tr2EffectPtr m_reactiveMaskEffect;

	// transparency mask
	Tr2EffectPtr m_transparencyMaskEffect;

	[[nodiscard]] Tr2GpuResourcePool::Buffer GetExposureBuffer( Tr2GpuResourcePool& gpuResourcePool ) const;
	[[nodiscard]] Tr2GpuResourcePool::Texture GetBlackTexture( Tr2GpuResourcePool & gpuResourcePool ) const;

	// Common
	Tr2GpuResourcePool::Texture Blur( Tr2GpuResourcePool::Texture src, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, const PostProcessBlur::BlurContext& blurContext );
	Tr2GpuResourcePool::Texture DownSampleDepth( const Tr2TextureAL& depth, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );

	Tr2EffectPtr m_downsampleDepthEffect;
	std::map<uint32_t, std::pair<Tr2EffectPtr, Tr2EffectPtr>> m_blurEffects;

	Tr2EffectPtr m_blurBigVertical;
	Tr2EffectPtr m_blurBigHorizontal;

	// contrast adaptive sharpening, used if no upscaling is applied or if an upscaling does not have sharpening
	Tr2EffectPtr m_fidelityFxCasShader;

	Be::Time m_lastFrameTime;
};

TYPEDEF_BLUECLASS( Tr2PostProcessRenderer );
