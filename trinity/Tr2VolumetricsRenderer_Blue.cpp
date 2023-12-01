////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2VolumetricsRenderer.h"
#include "TriRenderBatch.h"


const Be::VarChooser Tr2VolumetricQuality_Chooser[] = {
	{ "Low", BeCast( Tr2VolumerticQuality::Low ), "" },
	{ "Medium", BeCast( Tr2VolumerticQuality::Medium ), "" },
	{ "High", BeCast( Tr2VolumerticQuality::High ), "" },
	{ "Ultra", BeCast( Tr2VolumerticQuality::Ultra ), "" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"Tr2VolumerticQuality",
	Tr2VolumerticQuality,
	Tr2VolumetricQuality_Chooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( Tr2VolumetricsRenderer );

const Be::ClassInfo* Tr2VolumetricsRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VolumetricsRenderer, "" )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"quality",
			m_quality,
			"Volumetrics quality setting. Different volumetric renderables may take it into account",
			Be::READWRITE | Be::ENUM,
			Tr2VolumetricQuality_Chooser )
		MAP_ATTRIBUTE(
			"scaleFactor",
			m_scaleFactor,
			"Volumetrics resulution factor (1 - full size, 0.5 - 1/2 resolution, etc.)",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"blur",
			m_blur,
			"Blur volumetrics to remove noise",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"castShadows",
			m_castShadows,
			"Volumetrics cast shadows onto opaque geometry",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"receiveShadows",
			m_receiveShadows,
			"Volumetrics receive shadows from opaque geometry",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"volumeSlices",
			m_volumeSlices,
			"Frustum-align volume texture (represented as texture array) containing volumetic colors/density",
			Be::READ )
		MAP_ATTRIBUTE(
			"downsampledDepth",
			m_downsampledDepth,
			"",
			Be::READ )
		MAP_ATTRIBUTE(
			"blurScratch",
			m_blurScratch,
			"",
			Be::READ )
	EXPOSURE_END()
}
