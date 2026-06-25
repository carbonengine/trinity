// Copyright © 2024 CCP ehf.

#include "EveSpaceObjectAttachmentUtils.h"
#include "TriMath.h"
#include <Tr2Renderer.h>

namespace EveSpaceObjectAttachmentUtils
{
float Blink( float blinkRate, float blinkPhase, float minScale, float maxScale )
{
	if( blinkRate == 0.0f )
	{
		return minScale;
	}
	const float FLASH_PEAK_TIME = 0.05f;
	float intPart;
	float f = modf( Tr2Renderer::GetAnimationTime() * blinkRate + blinkPhase, &intPart );

	float peak = FLASH_PEAK_TIME * blinkRate;
	float result = 0.0f;
	float end = peak * 4.0f;

	auto lerp = []( float a, float b, float f ) {
		return a + f * ( b - a );
	};

	if( peak < 0.0001f )
	{
		peak = 1.0f;
	}
	if( f < peak )
	{
		result = lerp( 0.0f, 1.0f, f / peak );
	}
	else if( f < end )
	{
		result = lerp( 1.0f, 0.0f, ( f - peak ) / ( end - peak ) );
	}
	return ( maxScale - minScale ) * result + minScale;
}

float FadeIn( float blinkRate, float blinkPhase )
{
	float intPart;
	return modf( ( Tr2Renderer::GetAnimationTime() + blinkPhase ) * blinkRate, &intPart );
}

float FadeOut( float blinkRate, float blinkPhase )
{
	return 1.0f - FadeIn( blinkRate, blinkPhase );
}

float FadeInOut( float blinkRate, float blinkPhase )
{
	float timeModPi = fmodf( Tr2Renderer::GetAnimationTime() * blinkRate * TRI_2PI, TRI_2PI );
	return ( sin( timeModPi + blinkPhase * TRI_2PI ) + 1.0f ) / 2.0f;
}

float Fade( FadeType type, float blinkRate, float blinkPhase )
{
	// returns the intensity
	switch( type )
	{
	case FT_BLINK: // Regular blinking (with a fixed curve)
		return Blink( blinkRate, blinkPhase, 0, 1 );
	case FT_FADEIN: // Fade IN (Ramp up)
		return FadeIn( blinkRate, blinkPhase );
	case FT_FADEOUT: // Fade OUT (Ramp down)
		return FadeOut( blinkRate, blinkPhase );
	case FT_FADEINOUT: // Fade IN/OUT (Sinewave)
		return FadeInOut( blinkRate, blinkPhase );
	default:
		return 1.0f;
	}
}
}
