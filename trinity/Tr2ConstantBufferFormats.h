#pragma once

#ifndef Tr2ConstantBufferFormats_h
#define Tr2ConstantBufferFormats_h

class Tr2RenderContext;

struct Tr2PerFrameShadowPSData
{
	// Note: the SSAO depth pass hijacks lightRadius to store zNear
	float lightRadius;
	float zFar, unused2, unused3;
};

struct Tr2PerObjectPerPixelPointLightData
{
	// reg 1
	Vector3 position;
	float radius;
	// reg 2
	Vector3 direction;
	float spotLightConeAngleCos;
	// reg 3
	Vector3 color;
	float pointLightProportion;
	// reg 4
	float distanceFalloffKneeRadiusProportion;
	float distanceFalloffKneeValue;
	float specularRadiusMultiplier;
	float distanceFalloffPrecalc1;
}; // 4 registers

struct Tr2PerObjectVSData
{
	Matrix WorldMat;
	float boundingCylinderLocalHeight;
	Vector2 boundingCylinderLocalXZCenter;
	float boundingCylinderRotation;
};

struct Tr2PerObjectPSData
{
	float farFadeDistance;
	float nearFadeDistance;
	Vector2 padding;
	Vector4 highlightColor;	

	Tr2PerObjectPerPixelPointLightData pointLights[8];
};

struct Tr2PerFrameVSData
{
	Matrix ViewInverseTransposeMat;

	Vector4 sunDirWorld;
	Color sceneFogColor;

	Matrix ViewProjectionMat;
	Matrix ViewMat;
	Matrix ProjectionMat;
};


struct Tr2PerFrameVSDataDebug
{
	Matrix ViewInverseTransposeMat;
	Matrix ViewProjectionMat;
};


struct Tr2PerFramePSData
{
	Matrix ViewInverseTransposeMat;
	Color sceneAmbientColor;
	Color sceneFogColor;
	Vector4 sunDirWorld;
	Color sunDiffuseColor;
	Color sunSpecularColor;
	float maxFogAmount;
	float maxFogDistance;
	float minFogDistance;
	float cullDirection; // +1 for normal RH culling, -1 for LH culling
	Matrix ViewProjectionMat;
	float shScale;
	float shadowCount;
	float invShadowSize;
	float radius;
	Vector4 viewPort;	// xy - viewport width/height, zw - viewport offset
	Matrix ViewProjInverse;
};

void Tr2PopulatePerFrameVSDataTransformations( Tr2PerFrameVSData& data );

void Tr2BindPerFrameVSData( Tr2PerFrameVSData& data, Tr2RenderContext& renderContext );
void Tr2BindPerFramePSData( Tr2PerFramePSData& data, Tr2RenderContext& renderContext );

#endif