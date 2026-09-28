// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveBoosterUtilities_H
#define EveBoosterUtilities_H

#include "Tr2ProceduralResources.h"
#include "Tr2LightManager.h"
#include "Color.h"

BLUE_DECLARE( EveSpriteSet );

Tr2ProceduralBuffer MakeChildBoosterBoxBuffer();
Tr2ProceduralBuffer MakeBoosterBoxBuffer();
Tr2ProceduralBuffer MakeBoosterStarBuffer();

struct EveBoosterFlareParams
{
	Color warpGlowColor;
	float glowScale;
	Color glowColor;
	float haloScaleX;
	float haloScaleY;
	float symHaloScale;
	Color haloColor;
	Color warpHaloColor;
};

void CreateBoosterFlares( EveSpriteSet& glows,
						  const Matrix& transform,
						  const EveBoosterFlareParams& params );

float ComputeBoosterLightFlicker( float phase, float amplitude, float frequency );
float GenerateBoosterLightPhase();

struct EveBoosterLightParams
{
	float lightWarpRadius;
	Color lightWarpColor;
	float lightRadius;
	Color lightColor;
	float lightFlickerAmplitude;
	float lightFlickerFrequency;
};

template <typename BoosterItemVector>
void AddBoosterLights( Tr2LightManager& lightManager,
					   const BoosterItemVector& items,
					   const Matrix& transform,
					   float intensity,
					   float warpIntensity,
					   const EveBoosterLightParams& params )
{
	warpIntensity = std::min( std::max( warpIntensity, 0.f ), 1.f );
	float radiusFactor = params.lightRadius * ( 1.f - warpIntensity ) + params.lightWarpRadius * warpIntensity;
	radiusFactor *= intensity;
	Color color = params.lightColor * ( 1.f - warpIntensity ) + params.lightWarpColor * warpIntensity;
	XMMATRIX transformXM = transform;
	for( const auto& item : items )
	{
		float flicker = ComputeBoosterLightFlicker( item.lightPhase, params.lightFlickerAmplitude, params.lightFlickerFrequency );
		lightManager.AddPointLight(
			Vector3( XMVector3TransformCoord( item.lightPosition, transformXM ) ),
			item.lightRadius * radiusFactor,
			color * flicker );
	}
}

Vector4 PadBoosterBoundingSphere( Vector4 boosterBoundingSphere, const Matrix& transform );

#endif
