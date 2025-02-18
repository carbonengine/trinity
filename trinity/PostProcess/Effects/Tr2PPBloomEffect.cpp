////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPBloomEffect.h"




Tr2PPBloomEffect::Tr2PPBloomEffect( IRoot* lockobj ) :
	m_luminanceThreshold( -1.0f ),
	m_luminanceScale( 0.5f ),
	m_bloomBrightness( 0.2f ),
	m_sizeScale( 4.0f ),
	m_exposureDependency( false ),
	m_steps( Bloom::MAX_BLOOM_STEPS ),
	m_directionalWeight( 0.0f ),
	m_grimeWeight( 0.0f ),
	m_grimePath( "res:/texture/global/black.dds" )
{
	m_stepSizes = { Bloom::DEFAULT_BLOOM_STEP_1_SIZE, Bloom::DEFAULT_BLOOM_STEP_2_SIZE, Bloom::DEFAULT_BLOOM_STEP_3_SIZE, 
					Bloom::DEFAULT_BLOOM_STEP_4_SIZE, Bloom::DEFAULT_BLOOM_STEP_5_SIZE, Bloom::DEFAULT_BLOOM_STEP_6_SIZE };
	m_stepTints = { Bloom::DEFAULT_BLOOM_STEP_1_TINT, Bloom::DEFAULT_BLOOM_STEP_2_TINT, Bloom::DEFAULT_BLOOM_STEP_3_TINT,
					Bloom::DEFAULT_BLOOM_STEP_4_TINT, Bloom::DEFAULT_BLOOM_STEP_5_TINT, Bloom::DEFAULT_BLOOM_STEP_6_TINT };
}

Tr2PPBloomEffect::~Tr2PPBloomEffect()
{

}
