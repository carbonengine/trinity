// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierNoise.h"
#include "TriMath.h"

EveSmartLightAttributeModifierNoise::EveSmartLightAttributeModifierNoise( IRoot* lockobj ) :
	m_noiseAmplitude( 0.f ),
	m_noiseFrequency( 1.f ),
	m_noiseOctaves( 1 )
{
}

void EveSmartLightAttributeModifierNoise::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier )
{
	this->UpdateActivationStrength( activationMultiplier, updateContext.GetDeltaT() );
}

void EveSmartLightAttributeModifierNoise::ProcessAttributeModifier( Vector3& attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength )
{
	float activationStrength = this->GetActivationStrength( placement ) * modifierStrength;
	float activationAdjustedAmplitude = activationStrength * m_noiseAmplitude;

	if( activationAdjustedAmplitude > 0.f )
	{
		float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() ) * m_noiseFrequency, 2.f, 2.f, m_noiseOctaves ) );
		float noisifiedBrightness = ( ( noise + 1.0f ) / 2.0f ) * activationAdjustedAmplitude;
		attribute *= 1.f + activationStrength * ( noisifiedBrightness - 1.f );
	}
}
