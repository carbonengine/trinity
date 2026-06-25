// Copyright © 2019 CCP ehf.

#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

namespace Bloom
{
const uint32_t MAX_BLOOM_STEPS = 6;
const uint32_t MAX_FILTER_STEPS = 128;

const float DEFAULT_BLOOM_SIZE_SCALE = 4.0f;
const float DEFAULT_BLOOM_DIRECTIONAL_WEIGHT = 0.0f;

const float DEFAULT_BLOOM_STEP_1_SIZE = 0.3f;
const float DEFAULT_BLOOM_STEP_2_SIZE = 1.0f;
const float DEFAULT_BLOOM_STEP_3_SIZE = 2.0f;
const float DEFAULT_BLOOM_STEP_4_SIZE = 10.0f;
const float DEFAULT_BLOOM_STEP_5_SIZE = 30.0f;
const float DEFAULT_BLOOM_STEP_6_SIZE = 64.0f;

const Color DEFAULT_BLOOM_STEP_1_TINT = Color( 0.3465f, 0.3465f, 0.3465f, 0.3465f );
const Color DEFAULT_BLOOM_STEP_2_TINT = Color( 0.138f, 0.138f, 0.138f, 0.138f );
const Color DEFAULT_BLOOM_STEP_3_TINT = Color( 0.1176f, 0.1176f, 0.1176f, 0.1176f );
const Color DEFAULT_BLOOM_STEP_4_TINT = Color( 0.066f, 0.066f, 0.066f, 0.066f );
const Color DEFAULT_BLOOM_STEP_5_TINT = Color( 0.066f, 0.066f, 0.066f, 0.066f );
const Color DEFAULT_BLOOM_STEP_6_TINT = Color( 0.061f, 0.061f, 0.061f, 0.061f );
}

namespace Tr2Bloom
{
enum BloomDebugMode
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

extern const Be::VarChooser BloomDebugChooser[];
}

BLUE_CLASS( Tr2PPBloomEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();


	Tr2PPBloomEffect( IRoot* lockobj = NULL );
	~Tr2PPBloomEffect();

	float m_luminanceThreshold;
	float m_luminanceScale;
	float m_bloomBrightness;
	bool m_exposureDependency;

	// new stuff!
	float m_sizeScale;
	int32_t m_steps;
	std::array<float, Bloom::MAX_BLOOM_STEPS> m_stepSizes;
	std::array<Color, Bloom::MAX_BLOOM_STEPS> m_stepTints;

	float m_directionalWeight;

	float m_grimeWeight;
	BlueSharedString m_grimePath;
};

TYPEDEF_BLUECLASS( Tr2PPBloomEffect );
