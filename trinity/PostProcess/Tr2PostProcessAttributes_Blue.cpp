// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcessAttributes.h"
#include "Tr2PostProcessEnums.h"

namespace PostProcessEnums
{
Be::VarChooser Tr2PostProcessPriorityChooser[] = {

	{ "UI",
	  BeCast( Priority::UI_PRIORITY ),
	  "UI (Top) Priority" },
	{ "High",
	  BeCast( Priority::HIGH_PRIORITY ),
	  "High Priority" },
	{ "Medium",
	  BeCast( Priority::MEDIUM_PRIORITY ),
	  "Medium Priority" },
	{ "Low",
	  BeCast( Priority::LOW_PRIORITY ),
	  "Low Priority" },
	{ "SceneDefault",
	  BeCast( Priority::SCENE_DEFAULT_PRIORITY ),
	  "Scene Default (lowest) Priority" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Tr2PostProcessPriority", Priority, Tr2PostProcessPriorityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}


BLUE_DEFINE( Tr2PostProcessAttributes );

const Be::ClassInfo* Tr2PostProcessAttributes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PostProcessAttributes, "" )
		MAP_INTERFACE( Tr2PostProcessAttributes )
		MAP_ATTRIBUTE_WITH_CHOOSER( "priority", priority, "", Be::READWRITE | Be::PERSIST | Be::ENUM, PostProcessEnums::Tr2PostProcessPriorityChooser )
		MAP_ATTRIBUTE( "intensity", intensity, "", Be::READ )

#define POSTPROCESSATTRIBUTE_DEFINE( NAME, GROUP, DESC )                                                                                          \
	MAP_ATTRIBUTE( #NAME "Enabled", NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST ) \
	MAP_ATTRIBUTE( #NAME, NAME.value, "The maximum value of " #NAME " in the postprocess " DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST )

#define POSTPROCESSATTRIBUTE_DEFINE_ENUM( NAME, GROUP, DESC, CHOOSER )                                                                                          \
	MAP_ATTRIBUTE( #NAME "Enabled", NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST | Be::PERSIST ) \
	MAP_ATTRIBUTE_WITH_CHOOSER( #NAME, NAME.value, DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST | Be::ENUM, CHOOSER )

		POSTPROCESSATTRIBUTE_DEFINE( signalLossIntensity, Signal Loss, "Float between zero and one \n:jessica-numeric-range: (0.0, 1.0)" )

		POSTPROCESSATTRIBUTE_DEFINE( bloomBrightness, Bloom, "\n:jessica-numeric-range: (0.0, 25.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceThreshold, Bloom, "\n:jessica-numeric-range: (0.0, 25.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceScale, Bloom, "\n:jessica-numeric-range: (0.0, 25.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomSizeScale, Bloom, "\n:jessica-numeric-range: (0.0, 64.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomDirectionalWeight, Bloom, "Negative values define a horizontal flare, positive values define a cross, 0 is no shape\n:jessica-numeric-range: (-1.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize1, Bloom, "\n:jessica-numeric-range: (0.0, 4.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize2, Bloom, "\n:jessica-numeric-range: (0.0, 8.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize3, Bloom, "\n:jessica-numeric-range: (0.0, 16.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize4, Bloom, "\n:jessica-numeric-range: (0.0, 32.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize5, Bloom, "\n:jessica-numeric-range: (0.0, 64.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepSize6, Bloom, "\n:jessica-numeric-range: (0.0, 128.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint1, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint2, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint3, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint4, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint5, Bloom, "" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomStepTint6, Bloom, "" )

		POSTPROCESSATTRIBUTE_DEFINE( grimeIntensity, Grime, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( grimePath, Grime, "DOES NOT BLEND!" )

		POSTPROCESSATTRIBUTE_DEFINE( exposureAdjustment, Exposure, "Manual exposure adjustment value in stops" )

		POSTPROCESSATTRIBUTE_DEFINE( filmGrainIntensity, Film Grain, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainSize, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainDensity, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainContrast, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainBrightnessModifier, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainColored, Film Grain, "" )
		POSTPROCESSATTRIBUTE_DEFINE( filmGrainColorAmount, Film Grain, "\n:jessica-numeric-range: (0.0, 1.0)" )

		POSTPROCESSATTRIBUTE_DEFINE( saturation, Saturation, "negative values are desaturation, positive are saturation\n:jessica-numeric-range: (-1.0, 1.0)" )

		POSTPROCESSATTRIBUTE_DEFINE( fadeIntensity, Fade, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( fadeColor, Fade, "" )

		POSTPROCESSATTRIBUTE_DEFINE( lutIntensity, Lut, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( lutPath, Lut, "Blends up to 4 luts" )

		POSTPROCESSATTRIBUTE_DEFINE( vignetteIntensity, Vignette, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( vignetteOpacity, Vignette, "\n:jessica-numeric-range: (0.0, 1.0)" )
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

		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldScale, Depth Of Field, "\n:jessica-numeric-range: (0.0, 25.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalDistance, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalLength, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE_ENUM( depthOfFieldShape, Depth Of Field, "DOES NOT BLEND!", Tr2Bokeh::BokehShapeChooser )

		POSTPROCESSATTRIBUTE_DEFINE( whiteTemperature, Color Correction, "\n:jessica-numeric-range: (3000, 15000)" )
		POSTPROCESSATTRIBUTE_DEFINE( whiteTint, Color Correction, "\n:jessica-numeric-range: (-1.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( colorSaturation, Color Correction, "\n:jessica-numeric-range: (0.0, 2.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( colorContrast, Color Correction, "\n:jessica-numeric-range: (0.0, 2.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( colorGamma, Color Correction, "\n:jessica-numeric-range: (0.0, 2.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( colorGain, Color Correction, "\n:jessica-numeric-range: (0.0, 2.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( colorOffset, Color Correction, "\n:jessica-numeric-range: (0.0, 2.0)" )

	EXPOSURE_END()
}