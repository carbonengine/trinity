
#pragma once

#include "Eve/EveComponentRegistry.h"
#include "Tr2PostProcess2.h"
#include "Tr2PostProcessEnums.h"

BLUE_DECLARE( Tr2PostProcessAttributes );
BLUE_DECLARE_VECTOR( Tr2PostProcessAttributes );

namespace PostProcess
{
	template <typename T>
	struct Attribute
	{
		T value;
		bool enabled;

		Attribute( T value ) :
			value( value ),
			enabled( false ){}
	
		Attribute( T value, bool enabled ) :
			value( value ),
			enabled( enabled ){}
	};
}



class Tr2PostProcessAttributesDebugObserver;

BLUE_CLASS( Tr2PostProcessAttributes ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcessAttributes( IRoot* lockobj = NULL );
	~Tr2PostProcessAttributes();
	
	void Reset();

	void FromPostProcess( Tr2PostProcess2* postprocess, PostProcessEnums::Priority priority, float intensity );
	
	static void MergeInto( Tr2PostProcess2 & postprocess, std::vector<Tr2PostProcessAttributes*> & attributes, Tr2PostProcessAttributesDebugObserver* debugObserver = nullptr );

	// public attributes, so we can access them from the outside
	float intensity;
	PostProcessEnums::Priority priority;
	PostProcess::Attribute<float> signalLossIntensity;

	PostProcess::Attribute<float> bloomBrightness;
	PostProcess::Attribute<float> bloomLuminanceThreshold;
	PostProcess::Attribute<float> bloomLuminanceScale;

	PostProcess::Attribute<float> grimeIntensity;
	PostProcess::Attribute<BlueSharedString> grimePath;

	PostProcess::Attribute<float> exposureAdjustment;

	PostProcess::Attribute<bool> filmGrainColored;
	PostProcess::Attribute<float> filmGrainColorAmount;
	PostProcess::Attribute<float> filmGrainIntensity;
	PostProcess::Attribute<float> filmGrainSize;
	PostProcess::Attribute<float> filmGrainDensity;
	PostProcess::Attribute<float> filmGrainContrast;
	PostProcess::Attribute<float> filmGrainBrightnessModifier;

	// negative is desaturation, positive is saturation
	PostProcess::Attribute<float> saturation;

	PostProcess::Attribute<float> fadeIntensity;
	PostProcess::Attribute<Color> fadeColor;

	PostProcess::Attribute<float> lutIntensity;
	PostProcess::Attribute<BlueSharedString> lutPath;
	
	// a container for the accumulation of luts
	std::set<std::pair<float, BlueSharedString>> prioritizedLuts;

	PostProcess::Attribute<float> vignetteIntensity;
	PostProcess::Attribute<float> vignetteOpacity;
	PostProcess::Attribute<Color> vignetteColor;
	PostProcess::Attribute<Vector2> vignetteDetail1Size;
	PostProcess::Attribute<Vector2> vignetteDetail1Scroll;
	PostProcess::Attribute<Vector2> vignetteDetail2Size;
	PostProcess::Attribute<Vector2> vignetteDetail2Scroll;
	PostProcess::Attribute<BlueSharedString> vignetteShapePath;
	PostProcess::Attribute<BlueSharedString> vignetteDetailPath;
	PostProcess::Attribute<float> vignetteSineFrequency;
	PostProcess::Attribute<float> vignetteMinSineFrequency;
	PostProcess::Attribute<float> vignetteMaxSineFrequency;

	PostProcess::Attribute<float> depthOfFieldScale;
	PostProcess::Attribute<float> depthOfFieldFocalDistance;
	PostProcess::Attribute<float> depthOfFieldFocalLength;
	PostProcess::Attribute<Tr2Bokeh::Shape> depthOfFieldShape;

	PostProcess::Attribute<float> whiteTemperature = 6500.f;
	PostProcess::Attribute<float> whiteTint = 0.0f;
	PostProcess::Attribute<float> colorSaturation = 1.f;
	PostProcess::Attribute<float> colorContrast = 1.f;
	PostProcess::Attribute<float> colorGamma = 1.f;
	PostProcess::Attribute<Vector3> colorGain = Vector3( 1, 1, 1 );
	PostProcess::Attribute<Vector3> colorOffset = Vector3( 0, 0, 0 );
};

TYPEDEF_BLUECLASS( Tr2PostProcessAttributes );


class Tr2PostProcessAttributesDebugObserver
{
public:
	Tr2PostProcessAttributesDebugObserver();
	~Tr2PostProcessAttributesDebugObserver();

	void BeginAttribute( const char* name );
	void Influence( Tr2PostProcessAttributes* attributes, float weight );

	void EndAttribute( PyObject* value );

	BluePy GetDict() const;

private:
	PyObject* m_debugObject = nullptr;
	PyObject* m_currentAttribute = nullptr;
	PyObject* m_currentInfluencers = nullptr;
};