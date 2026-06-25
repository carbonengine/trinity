// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2InteriorConstantBufferFormats_h
#define Tr2InteriorConstantBufferFormats_h

// Trinity headers
#include "Tr2ConstantBufferFormats.h"

// maximum number of pointlights per object
#define MAX_INTERIOR_LIGHTS_PER_OBJECT ( 10 )

// interior pointlight data
struct Tr2InteriorPerObjectLightData
{
	// reg 1
	Vector3 position;
	float radius;
	// reg 2
	Vector3 color;
	float pointLightFalloff;
	// reg 3
	float shadow0Influence;
	float shadow1Influence;
	float coneCosAlphaOuter;
	float coneCosAlphaInner;
	// reg 4
	Vector3 spotDirection;
	float unused2;
	// reg 5
	Vector4 boxTransformRow3;
	// reg 6
	Vector4 boxTransformRow4;
}; // 6 registers

struct Tr2InteriorPerObjectPSData
{
	int32_t lightCount;
	int32_t padding[3];

	Tr2InteriorPerObjectLightData pointLights[MAX_INTERIOR_LIGHTS_PER_OBJECT];
	// data on the shadow casting point lights: [x,y,z] = position, [w] = radius
	Vector4 shadowCaster0;
	Vector4 shadowCaster1;
	// spotlight viewProjection matrices
	Matrix spotLights[4];
};

struct Tr2InteriorPerObjectVSData
{
	Matrix WorldMat;
	Vector4 uvLinearTransform;
	Vector2 uvTranslation;
	Vector2 padding;
};

struct Tr2InteriorPerLightPSData
{
	Tr2InteriorPerObjectLightData lightData;
	// Mirror to world matrix (in case of spotlight it is multiplied by light's viewProjection)
	Matrix mirrorToWorldMatrix;
	Matrix shadowMatrix[6];
	Vector4 shadowRect[6];
	Vector4 shadowInfluence[6];
	Matrix boundingBox;
	Vector4 additionalParameters; // x - specular intensity, yzw - unused
};

struct Tr2PerObjectParticleVSData
{
	Matrix WorldMat;
	Matrix InvViewMat;
};

// typedef for SSAO depth pass
typedef Tr2PerFrameShadowPSData Tr2InteriorPerFrameSsaoPSData;

#endif