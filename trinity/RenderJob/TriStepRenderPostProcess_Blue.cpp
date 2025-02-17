#include "StdAfx.h"
#include "TriStepRenderPostProcess.h"

BLUE_DEFINE( TriStepRenderPostProcess );

Be::VarChooser PostProcessQualityChooser[] =
{
	{ "Low", BeCast( TriStepRenderPostProcess::LOW ), "Low Quality" },
	{ "Medium", BeCast( TriStepRenderPostProcess::MEDIUM ), "Medium Quality" },
	{ "High", BeCast( TriStepRenderPostProcess::HIGH ), "High Quality" },
	{ 0 }
};

Be::VarChooser BloomDebugChooser[] = {
	{ "None", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_NONE ), "No Debug" },
	{ "All", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_ALL ), "Show all steps" },
	{ "Step1", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP1 ), "Show step 1" },
	{ "Step2", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP2 ), "Show step 2" },
	{ "Step3", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP3 ), "Show step 3" },
	{ "Step4", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP4 ), "Show step 4" },
	{ "Step5", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP5 ), "Show step 5" },
	{ "Step6", BeCast( TriStepRenderPostProcess::BloomDebugMode::BLOOM_DEBUG_STEP6 ), "Show step 6" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "PostProcessQuality", TriStepRenderPostProcess::PostProcessingQuality, PostProcessQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* TriStepRenderPostProcess::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRenderPostProcess, "Render step for rendering post process" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( TriStepRenderPostProcess )

		MAP_ATTRIBUTE( "scene", m_scene, "The scene to be rendered", Be::READWRITE )
		MAP_ATTRIBUTE( "renderInfo", m_renderInfo, "The rendering information for the post process", Be::READWRITE )
		MAP_PROPERTY( "renderTarget", GetRenderTarget, SetRenderTarget, "Gets/Sets the main rendertarget")
		MAP_ATTRIBUTE( "bloomHighPassFilter", m_bloomHighPassFilter, "The bloom high pass effect", Be::READWRITE )
		MAP_ATTRIBUTE( "godrayEffect", m_godrayEffect, "The godray effect", Be::READWRITE )
		MAP_ATTRIBUTE( "signalLossEffect", m_signalLossEffect, "The signal loss effect", Be::READWRITE )
		MAP_ATTRIBUTE( "tonemappingEffect", m_tonemappingEffect, "The tone mapping effect", Be::READWRITE )
		MAP_ATTRIBUTE( "dynamicExposureCreateHistogramShader", m_dynamicExposureCreateHistogramShader, "The create histogram effect", Be::READWRITE )
		MAP_ATTRIBUTE( "dynamicExposureMergeHistogramShader", m_dynamicExposureMergeHistogramShader, "The merge histogram effect", Be::READWRITE )
		MAP_ATTRIBUTE( "dynamicExposureMeasureExposureShader", m_dynamicExposureMeasureExposureShader, "The measure exposure effect", Be::READWRITE )
		MAP_ATTRIBUTE( "fogColorEffect", m_fogColorEffect, "The fog color effect", Be::READWRITE )
		MAP_ATTRIBUTE( "fogCompositeEffect", m_fogCompositeEffect, "The fog composite effect", Be::READWRITE )
		MAP_ATTRIBUTE( "taaEffect", m_taaEffect, "The taa effect", Be::READWRITE )
		MAP_ATTRIBUTE( "accumulationBuffer0", m_accumulationBuffer0, "TAA accumulation buffer", Be::READWRITE )
		MAP_ATTRIBUTE( "accumulationBuffer1", m_accumulationBuffer1, "TAA accumulation buffer", Be::READWRITE )
		MAP_ATTRIBUTE( "bloomDebugShader", m_bloomDebugShader, "The bloom high pass effect", Be::READWRITE )


		MAP_ATTRIBUTE( "depthOfFieldCoCShader", m_depthOfFieldCoCShader, "The DoF Circle of Confusion shader", Be::READWRITE);
		MAP_ATTRIBUTE( "depthOfFieldBokehBlurShader", m_depthOfFieldBokehBlurShader, "The bokeh blur shader", Be::READWRITE );
		MAP_ATTRIBUTE( "depthOfFieldBokehFillShader", m_depthOfFieldBokehFillShader, "The bokeh fill shader", Be::READWRITE );
		MAP_ATTRIBUTE( "exposure", m_exposureTexture, "exposure texture", Be::READ );
		MAP_ATTRIBUTE( "dynamicExposureToTextureShader", m_dynamicExposureToTextureShader, "exposure texture", Be::READWRITE );
		MAP_ATTRIBUTE( "reactive", m_reactiveMask, "reactive texture", Be::READ );

		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_quality, "The quality of the post process", Be::READWRITE | Be::ENUM | Be::NOTIFY, PostProcessQualityChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "bloomDebugMode", m_bloomDebugMode, "bloom debug mode \n:jessica-group: Debug", Be::READWRITE | Be::NOTIFY | Be::ENUM, BloomDebugChooser )
		MAP_ATTRIBUTE( "useNewBloom", m_useNewBloom, "Use the new bloom effect", Be::READWRITE | Be::NOTIFY )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			py__init__,
			0,
			"Creates a render step that renders post processes\n"
			":param scene: an ITr2Scene object\n"
			":param source: an Tr2RenderTarget object"
		)

		EXPOSURE_CHAINTO( TriRenderStep )
}