// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcessRenderer.h"

BLUE_DEFINE( Tr2PostProcessRenderer );


Be::VarChooser BloomDebugChooser[] = {
	{ "None", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_NONE ), "No Debug" },
	{ "All", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_ALL ), "Show all steps" },
	{ "Step1", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP1 ), "Show step 1" },
	{ "Step2", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP2 ), "Show step 2" },
	{ "Step3", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP3 ), "Show step 3" },
	{ "Step4", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP4 ), "Show step 4" },
	{ "Step5", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP5 ), "Show step 5" },
	{ "Step6", BeCast( Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP6 ), "Show step 6" },
	{ 0 }
};


const Be::ClassInfo* Tr2PostProcessRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PostProcessRenderer, "Renders space scene post process" )

		MAP_INTERFACE( Tr2PostProcessRenderer )

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
		MAP_ATTRIBUTE( "bloomDebugShader", m_bloomDebugShader, "The bloom high pass effect", Be::READWRITE )


		MAP_ATTRIBUTE( "depthOfFieldCoCShader", m_depthOfFieldCoCShader, "The DoF Circle of Confusion shader", Be::READWRITE );
		MAP_ATTRIBUTE( "depthOfFieldBokehBlurShader", m_depthOfFieldBokehBlurShader, "The bokeh blur shader", Be::READWRITE );
		MAP_ATTRIBUTE( "depthOfFieldBokehFillShader", m_depthOfFieldBokehFillShader, "The bokeh fill shader", Be::READWRITE );
		MAP_ATTRIBUTE( "dynamicExposureToTextureShader", m_dynamicExposureToTextureShader, "exposure texture", Be::READWRITE );

		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_quality, "The quality of the post process", Be::READWRITE | Be::ENUM | Be::NOTIFY, PostProcess::PostProcessQualityChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "bloomDebugMode", m_bloomDebugMode, "bloom debug mode \n:jessica-group: Debug", Be::READWRITE | Be::NOTIFY | Be::ENUM, BloomDebugChooser )
		MAP_ATTRIBUTE( "useNewBloom", m_useNewBloom, "Use the new bloom effect", Be::READWRITE | Be::NOTIFY )

	EXPOSURE_END()
}