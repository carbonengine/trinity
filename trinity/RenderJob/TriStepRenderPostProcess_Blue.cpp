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

		MAP_ATTRIBUTE( "fsrEasuShader", m_fidelityFXFsrEASUShader, "FSR EASU", Be::READWRITE )
		MAP_ATTRIBUTE( "fsrRCASShader", m_fidelityFXFsrRCASShader, "FSR RCASS", Be::READWRITE )
		
		MAP_ATTRIBUTE( "depthOfFieldCoCShader", m_depthOfFieldCoCShader, "The DoF Circle of Confusion shader", Be::READWRITE);
		MAP_ATTRIBUTE( "depthOfFieldBokehBlurShader", m_depthOfFieldBokehBlurShader, "The bokeh blur shader", Be::READWRITE );
		MAP_ATTRIBUTE( "depthOfFieldBokehFillShader", m_depthOfFieldBokehFillShader, "The bokeh fill shader", Be::READWRITE );

		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_quality, "The quality of the post process", Be::READWRITE | Be::ENUM | Be::NOTIFY, PostProcessQualityChooser )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			py__init__,
			2,
			"Creates a render step that renders post processes\n"
			":param scene: an ITr2Scene object\n"
			":param source: an Tr2RenderTarget object"
		)

		EXPOSURE_CHAINTO( TriRenderStep )
}