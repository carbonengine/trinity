// Copyright © 2024 CCP ehf.

#pragma once

#include "Eve/EveComponentRegistry.h"
#include "Tr2PostProcess2.h"
#include "Tr2PostProcessEnums.h"
#include "PriorityBlend.h"

BLUE_DECLARE( Tr2PostProcessAttributes );
BLUE_DECLARE_VECTOR( Tr2PostProcessAttributes );

BLUE_CLASS( Tr2PostProcessAttributes ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcessAttributes( IRoot* lockobj = NULL );
	~Tr2PostProcessAttributes();

	void Reset();

	void FromPostProcess( Tr2PostProcess2 * postprocess, PostProcessEnums::Priority priority, float intensity );

	static void MergeInto( Tr2PostProcess2 & postprocess, std::vector<Tr2PostProcessAttributes*> & attributes, PriorityBlend::AttributesDebugObserver<Tr2PostProcessAttributes>* debugObserver = nullptr );

	// public attributes, so we can access them from the outside
	float intensity;
	PostProcessEnums::Priority priority;
	PriorityBlend::Attribute<float> signalLossIntensity;

	PriorityBlend::Attribute<float> bloomBrightness;
	PriorityBlend::Attribute<float> bloomLuminanceThreshold;
	PriorityBlend::Attribute<float> bloomLuminanceScale;

	PriorityBlend::Attribute<float> bloomSizeScale = Bloom::DEFAULT_BLOOM_SIZE_SCALE;
	PriorityBlend::Attribute<float> bloomDirectionalWeight = Bloom::DEFAULT_BLOOM_DIRECTIONAL_WEIGHT;

	PriorityBlend::Attribute<float> bloomStepSize1 = Bloom::DEFAULT_BLOOM_STEP_1_SIZE;
	PriorityBlend::Attribute<float> bloomStepSize2 = Bloom::DEFAULT_BLOOM_STEP_2_SIZE;
	PriorityBlend::Attribute<float> bloomStepSize3 = Bloom::DEFAULT_BLOOM_STEP_3_SIZE;
	PriorityBlend::Attribute<float> bloomStepSize4 = Bloom::DEFAULT_BLOOM_STEP_4_SIZE;
	PriorityBlend::Attribute<float> bloomStepSize5 = Bloom::DEFAULT_BLOOM_STEP_5_SIZE;
	PriorityBlend::Attribute<float> bloomStepSize6 = Bloom::DEFAULT_BLOOM_STEP_6_SIZE;

	PriorityBlend::Attribute<Color> bloomStepTint1 = Bloom::DEFAULT_BLOOM_STEP_1_TINT;
	PriorityBlend::Attribute<Color> bloomStepTint2 = Bloom::DEFAULT_BLOOM_STEP_2_TINT;
	PriorityBlend::Attribute<Color> bloomStepTint3 = Bloom::DEFAULT_BLOOM_STEP_3_TINT;
	PriorityBlend::Attribute<Color> bloomStepTint4 = Bloom::DEFAULT_BLOOM_STEP_4_TINT;
	PriorityBlend::Attribute<Color> bloomStepTint5 = Bloom::DEFAULT_BLOOM_STEP_5_TINT;
	PriorityBlend::Attribute<Color> bloomStepTint6 = Bloom::DEFAULT_BLOOM_STEP_6_TINT;

	PriorityBlend::Attribute<float> grimeIntensity;
	PriorityBlend::Attribute<BlueSharedString> grimePath;

	PriorityBlend::Attribute<float> exposureAdjustment;

	PriorityBlend::Attribute<bool> filmGrainColored;
	PriorityBlend::Attribute<float> filmGrainColorAmount;
	PriorityBlend::Attribute<float> filmGrainIntensity;
	PriorityBlend::Attribute<float> filmGrainSize;
	PriorityBlend::Attribute<float> filmGrainDensity;
	PriorityBlend::Attribute<float> filmGrainContrast;
	PriorityBlend::Attribute<float> filmGrainBrightnessModifier;

	// negative is desaturation, positive is saturation
	PriorityBlend::Attribute<float> saturation;

	PriorityBlend::Attribute<float> fadeIntensity;
	PriorityBlend::Attribute<Color> fadeColor;

	PriorityBlend::Attribute<float> lutIntensity;
	PriorityBlend::Attribute<BlueSharedString> lutPath;

	// a container for the accumulation of luts
	std::set<std::pair<float, BlueSharedString>> prioritizedLuts;

	PriorityBlend::Attribute<float> vignetteIntensity;
	PriorityBlend::Attribute<float> vignetteOpacity;
	PriorityBlend::Attribute<Color> vignetteColor;
	PriorityBlend::Attribute<Vector2> vignetteDetail1Size;
	PriorityBlend::Attribute<Vector2> vignetteDetail1Scroll;
	PriorityBlend::Attribute<Vector2> vignetteDetail2Size;
	PriorityBlend::Attribute<Vector2> vignetteDetail2Scroll;
	PriorityBlend::Attribute<BlueSharedString> vignetteShapePath;
	PriorityBlend::Attribute<BlueSharedString> vignetteDetailPath;
	PriorityBlend::Attribute<float> vignetteSineFrequency;
	PriorityBlend::Attribute<float> vignetteMinSineFrequency;
	PriorityBlend::Attribute<float> vignetteMaxSineFrequency;

	PriorityBlend::Attribute<float> depthOfFieldScale;
	PriorityBlend::Attribute<float> depthOfFieldFocalDistance;
	PriorityBlend::Attribute<float> depthOfFieldFocalLength;
	PriorityBlend::Attribute<Tr2Bokeh::Shape> depthOfFieldShape;
	PriorityBlend::Attribute<bool> depthOfFieldForegroundBlurNeeded;

	PriorityBlend::Attribute<float> whiteTemperature = 6500.f;
	PriorityBlend::Attribute<float> whiteTint = 0.0f;
	PriorityBlend::Attribute<float> colorSaturation = 1.f;
	PriorityBlend::Attribute<float> colorContrast = 1.f;
	PriorityBlend::Attribute<float> colorGamma = 1.f;
	PriorityBlend::Attribute<Vector3> colorGain = Vector3( 1, 1, 1 );
	PriorityBlend::Attribute<Vector3> colorOffset = Vector3( 0, 0, 0 );
};

TYPEDEF_BLUECLASS( Tr2PostProcessAttributes );
