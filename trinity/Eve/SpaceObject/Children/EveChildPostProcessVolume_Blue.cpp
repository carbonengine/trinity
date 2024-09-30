#include "StdAfx.h"
#include "EveChildPostProcessVolume.h"
#include "PostProcess/ITr2PostProcessOwner.h"
#include "PostProcess/Tr2PostProcessUtils.h"

BLUE_DEFINE_INTERFACE( ITr2PostProcessOwner );
BLUE_DEFINE( EveChildPostProcessVolume );

Be::VarChooser Tr2PostProcessPriorityChooser[] = {
	{ "High",
	  BeCast( PostProcess::Priority::HIGH_PRIORITY ),
	  "High Priority" },
	{ "Medium",
	  BeCast( PostProcess::Priority::MEDIUM_PRIORITY ),
	  "" },
	{ "Low",
	  BeCast( PostProcess::Priority::LOW_PRIORITY ),
	  "" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Tr2PostProcessPriority", PostProcess::Priority, Tr2PostProcessPriorityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser BokehShapeChooser[] = {
	{ "Disk", BeCast( Tr2Bokeh::Disk ), "A perfectly circular aperture" },
	{ "Triangle", BeCast( Tr2Bokeh::Triangle ), "An aperture with 3 sides" },
	{ "Rectangle", BeCast( Tr2Bokeh::Rectangle ), "An aperture with 4 sides" },
	{ "Pentagon", BeCast( Tr2Bokeh::Pentagon ), "An aperture with 5 sides" },
	{ "Hexagon", BeCast( Tr2Bokeh::Hexagon ), "An aperture with 6 sides" },
	{ "Heart", BeCast( Tr2Bokeh::Heart ), "A heart-shaped aperture <3" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "BokehShapeType", Tr2Bokeh::Shape, BokehShapeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* EveChildPostProcessVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildPostProcessVolume, "" )
		MAP_INTERFACE( ITr2PostProcessOwner )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "priority", m_postProcessAttributeOverrides.priority, "", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2PostProcessPriorityChooser )		
		MAP_ATTRIBUTE( "intensity", m_postProcessAttributeOverrides.intensity, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphere.center, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.radius, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST | Be::NOTIFY )

#define POSTPROCESSATTRIBUTE_DEFINE( NAME, GROUP, DESC )                                                                                                             \
		MAP_ATTRIBUTE( #NAME "Enabled", m_postProcessAttributeOverrides.NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST ) \
		MAP_ATTRIBUTE( #NAME, m_postProcessAttributeOverrides.NAME.value, "The maximum value of " #NAME " in the postprocess " DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST )
//
//#define POSTPROCESSATTRIBUTE_DEFINE_WITH_CURRENT( NAME, GROUP, DESC ) \
//		POSTPROCESSATTRIBUTE_DEFINE( NAME, GROUP, DESC ) \
//		MAP_ATTRIBUTE( #NAME "Current", m_postProcessAttributeOverrides.NAME.currentValue, "The current value of " #NAME " in the volume \n:jessica-group: " #GROUP, Be::READ | Be::PERSIST ) 

#define POSTPROCESSATTRIBUTE_DEFINE_ENUM( NAME, GROUP, DESC, CHOOSER ) \
		MAP_ATTRIBUTE( #NAME "Enabled", m_postProcessAttributeOverrides.NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST ) \
		MAP_ATTRIBUTE_WITH_CHOOSER( #NAME, m_postProcessAttributeOverrides.NAME.value, DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST | Be::ENUM, CHOOSER )

		POSTPROCESSATTRIBUTE_DEFINE( signalLossIntensity, Signal Loss, "Float between zero and one" )

		POSTPROCESSATTRIBUTE_DEFINE( bloomBrightness, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceThreshold, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceScale, Bloom, "" )

		POSTPROCESSATTRIBUTE_DEFINE( grimeIntensity, Grime, "" )
		POSTPROCESSATTRIBUTE_DEFINE( grimePath, Grime, "DOES NOT BLEND!" )

		POSTPROCESSATTRIBUTE_DEFINE( filmGrainIntensity, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainSize, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainDensity, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainContrast, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainBrightnessModifier, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainColored, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainColorAmount, Film Grain, "" )

		POSTPROCESSATTRIBUTE_DEFINE( saturation, Saturation, "negative values are desaturation, positive are saturation" )
		
		POSTPROCESSATTRIBUTE_DEFINE( fadeIntensity, Fade, "" )
		POSTPROCESSATTRIBUTE_DEFINE( fadeColor, Fade, "" )

		POSTPROCESSATTRIBUTE_DEFINE( lutIntensity, Lut, "" )
		POSTPROCESSATTRIBUTE_DEFINE( lutPath, Lut, "Blends up to 4 luts" )

		POSTPROCESSATTRIBUTE_DEFINE( vignetteIntensity, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteOpacity, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteColor, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteDetail1Size, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteDetail1Scroll, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteDetail2Size, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteDetail2Scroll, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteShapePath, Vignette, "DOES NOT BLEND!" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteDetailPath, Vignette, "DOES NOT BLEND!" )
		// should these blend?
		POSTPROCESSATTRIBUTE_DEFINE( vignetteSineFrequency, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteMinSineFrequency, Vignette, "" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteMaxSineFrequency, Vignette, "" )

		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldScale, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalDistance, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalLength, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE_ENUM( depthOfFieldShape, Depth Of Field, "DOES NOT BLEND!", BokehShapeChooser )

	EXPOSURE_END()
}