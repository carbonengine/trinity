
#include "StdAfx.h"
#include "Tr2PostProcessAttributes.h"
#include "Tr2PostProcessEnums.h"

namespace PostProcessEnums
{
	Be::VarChooser Tr2PostProcessPriorityChooser[] = {

		{ "UI",
		  BeCast( Priority::HIGH_PRIORITY ),
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
		{ "Scene Default",
		  BeCast( Priority::SCENE_DEFAULT_PRIORITY ),
		  "Scene Default (lowest) Priority" },
		{ 0 }
	};
	BLUE_REGISTER_ENUM_EX( "Tr2PostProcessPriority", Priority, Tr2PostProcessPriorityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

	Be::VarChooser Tr2PostProcessAttributeTypeChooser[] = {
		{ "SignalLossIntensity",
		  BeCast( AttributeType::SIGNAL_LOSS_INTENSITY ),
		  "Signal Loss Intensity" },
		{ "BloomBrightness",
		  BeCast( AttributeType::BLOOM_BRIGHTNESS ),
		  "Bloom Brightness" },
		{ "BloomLuminanceThreshold",
		  BeCast( AttributeType::BLOOM_LUMINANCE_THRESHOLD ),
		  "Bloom Luminance Threshold" },
		{ "BloomLuminanceScale",
		  BeCast( AttributeType::BLOOM_LUMINANCE_SCALE ),
		  "Bloom Luminance Scale" },
		{ "GrimeIntensity",
		  BeCast( AttributeType::GRIME_INTENSITY ),
		  "Grime Intensity" },
		{ "GrimePath",
		  BeCast( AttributeType::GRIME_PATH ),
		  "Grime Path" },
		{ "FilmGrainIntensity",
		  BeCast( AttributeType::FILM_GRAIN_INTENSITY ),
		  "Film Grain Intensity" },
		{ "FilmGrainSize",
		  BeCast( AttributeType::FILM_GRAIN_SIZE ),
		  "Film Grain Size" },
		{ "FilmGrainDensity",
		  BeCast( AttributeType::FILM_GRAIN_DENSITY ),
		  "Film Grain Density" },
		{ "FilmGrainContrast",
		  BeCast( AttributeType::FILM_GRAIN_CONTRAST ),
		  "Film Grain Contrast" },
		{ "FilmGrainBrightnessModifier",
		  BeCast( AttributeType::FILM_GRAIN_BRIGHTNESS_MODIFIER ),
		  "Film Grain Brightness Modifier" },
		{ "FilmGrainColored",
		  BeCast( AttributeType::FILM_GRAIN_COLORED ),
		  "Film Grain Colored" },
		{ "FilmGrainColorAmount",
		  BeCast( AttributeType::FILM_GRAIN_COLOR_AMOUNT ),
		  "Film Grain Color Amount" },
		{ "Saturation",
		  BeCast( AttributeType::SATURATION ),
		  "Saturation" },
		{ "FadeIntensity",
		  BeCast( AttributeType::FADE_INTENSITY ),
		  "Fade Intensity" },
		{ "FadeColor",
		  BeCast( AttributeType::FADE_COLOR ),
		  "Fade Color" },
		{ "LutIntensity",
		  BeCast( AttributeType::LUT_INTENSITY ),
		  "Lut Intensity" },
		{ "LutPath",
		  BeCast( AttributeType::LUT_PATH ),
		  "Lut Path" },
		{ "VignetteIntensity",
		  BeCast( AttributeType::VIGNETTE_INTENSITY ),
		  "Vignette Intensity" },
		{ "VignetteOpacity",
		  BeCast( AttributeType::VIGNETTE_OPACITY ),
		  "Vignette Opacity" },
		{ "VignetteColor",
		  BeCast( AttributeType::VIGNETTE_COLOR ),
		  "Vignette Color" },
		{ "VignetteDetail1Size",
		  BeCast( AttributeType::VIGNETTE_DETAIL1_SIZE ),
		  "Vignette Detail1 Size" },
		{ "VignetteDetail1Scroll",
		  BeCast( AttributeType::VIGNETTE_DETAIL1_SCROLL ),
		  "Vignette Detail1 Scroll" },
		{ "VignetteDetail2Size",
		  BeCast( AttributeType::VIGNETTE_DETAIL2_SIZE ),
		  "Vignette Detail2 Size" },
		{ "VignetteDetail2Scroll",
		  BeCast( AttributeType::VIGNETTE_DETAIL2_SCROLL ),
		  "Vignette Detail2 Scroll" },
		{ "VignetteShapePath",
		  BeCast( AttributeType::VIGNETTE_SHAPE_PATH ),
		  "Vignette Shape Path" },
		{ "VignetteDetailPath",
		  BeCast( AttributeType::VIGNETTE_DETAIL_PATH ),
		  "Vignette Detail Path" },
		{ "VignetteSineFrequency",
		  BeCast( AttributeType::VIGNETTE_SINE_FREQUENCY ),
		  "Vignette Sine Frequency" },
		{ "VignetteMinSineFrequency",
		  BeCast( AttributeType::VIGNETTE_MIN_SINE_FREQUENCY ),
		  "Vignette Min Sine Frequency" },
		{ "VignetteMaxSineFrequency",
		  BeCast( AttributeType::VIGNETTE_MAX_SINE_FREQUENCY ),
		  "Vignette Max Sine Frequency" },
		{ "DepthOfFieldScale",
		  BeCast( AttributeType::DEPTH_OF_FIELD_SCALE ),
		  "Depth Of Field Scale" },
		{ "DepthOfFieldFocalDistance",
		  BeCast( AttributeType::DEPTH_OF_FIELD_FOCAL_DISTANCE ),
		  "Depth Of Field Focal Distance" },
		{ "DepthOfFieldFocalLength",
		  BeCast( AttributeType::DEPTH_OF_FIELD_FOCAL_LENGTH ),
		  "Depth Of Field Focal Length" },
		{ "DepthOfFieldShape",
		  BeCast( AttributeType::DEPTH_OF_FIELD_SHAPE ),
		  "Depth Of Field Shape" },
		{ 0 }
	};
	BLUE_REGISTER_ENUM_EX( "Tr2PostProcessAttributeType", AttributeType, Tr2PostProcessAttributeTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}
	

BLUE_DEFINE( Tr2PostProcessAttributes );

const Be::ClassInfo* Tr2PostProcessAttributes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PostProcessAttributes, "" )
		MAP_INTERFACE( Tr2PostProcessAttributes )
		MAP_ATTRIBUTE_WITH_CHOOSER( "priority", priority, "", Be::READWRITE | Be::PERSIST | Be::ENUM, PostProcessEnums::Tr2PostProcessPriorityChooser )
		MAP_ATTRIBUTE( "intensity", intensity, "", Be::READ )

#define POSTPROCESSATTRIBUTE_DEFINE( NAME, GROUP, DESC )                                                                                                                   \
	MAP_ATTRIBUTE( #NAME "Enabled", NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST ) \
	MAP_ATTRIBUTE( #NAME, NAME.value, "The maximum value of " #NAME " in the postprocess " DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST )

#define POSTPROCESSATTRIBUTE_DEFINE_ENUM( NAME, GROUP, DESC, CHOOSER )                                                                                                     \
	MAP_ATTRIBUTE( #NAME "Enabled", NAME.enabled, "Enables " #NAME " in the postprocess \n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST | Be::PERSIST ) \
	MAP_ATTRIBUTE_WITH_CHOOSER( #NAME, NAME.value, DESC "\n:jessica-group: " #GROUP, Be::READWRITE | Be::PERSIST | Be::ENUM, CHOOSER )

		POSTPROCESSATTRIBUTE_DEFINE( signalLossIntensity, Signal Loss, "Float between zero and one \n:jessica-numeric-range: (0.0, 1.0)" )

		POSTPROCESSATTRIBUTE_DEFINE( bloomBrightness, Bloom, "\n:jessica-numeric-range: (0.0, 1000.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceThreshold, Bloom, "\n:jessica-numeric-range: (0.0, 1000.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( bloomLuminanceScale, Bloom, "\n:jessica-numeric-range: (0.0, 100.0)" )

		POSTPROCESSATTRIBUTE_DEFINE( grimeIntensity, Grime, "\n:jessica-numeric-range: (0.0, 1.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( grimePath, Grime, "DOES NOT BLEND!" )

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

		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldScale, Depth Of Field, "\n:jessica-numeric-range: (0.0, 100.0)" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalDistance, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE( depthOfFieldFocalLength, Depth Of Field, "" )
		POSTPROCESSATTRIBUTE_DEFINE_ENUM( depthOfFieldShape, Depth Of Field, "DOES NOT BLEND!", Tr2Bokeh::BokehShapeChooser )

		MAP_ATTRIBUTE( "owner", m_owner, "Post process owner", Be::READ )
		MAP_ATTRIBUTE( "sources", m_sources, "Post process sources", Be::READ )
		MAP_ATTRIBUTE( "weights", m_weights, "Post process weights", Be::READ )

	EXPOSURE_END()
}