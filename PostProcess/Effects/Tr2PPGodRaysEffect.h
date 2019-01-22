////////////////////////////////////////////////////////////////////////////////
//
// Created:		1/17/2019 
// Copyright:	CCP 2019
//
#pragma once


/*
-   type: RenderEffect
name: "Downsample Depth"
condition: "GodRays and GodRaysIntensity>0"
effect:
type: Tr2Effect
effectFilePath: "res:/Graphics/Effect/Managed/Space/PostProcess/DownsampleDepth.fx"
renderTargets:
0: rt1
stepAttributes:
shaderBuffer: __framepsdata__

-   type: Clear
name: "Clear target for GodRays"
condition: "GodRays and GodRaysIntensity>0"
isColorCleared: True
color: [0.0, 0.0, 0.0, 0.0]
renderTargets:
0: rt2
-   type: RenderEffect
name: GodRays
condition: "GodRays and GodRaysIntensity>0"
effect:
type: Tr2Effect
effectFilePath: "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx"
parameters:
-   type: Tr2Vector4Parameter
name: "grFactors"
value: [1000.0, 0.2, 128.0, 2.0]
resources:
-   type: TriTextureParameter
name: NoiseTexMap
resourcePath: res:/texture/global/noise.dds
effectParameters:
Color: GodRaysColor
Intensity: "(GodRaysIntensity, 0.0, 1.0, 1.0)"
DepthMap: rt1

-   type: SetStdRndStates
name: Set Additive Render State
renderingMode: 5
-   type: RenderTexture
name: "Blit GodRays"
condition: "GodRays and GodRaysIntensity>0"
parameters:
renderTarget: rt2
renderTargets:
0: __sourcert__

*/

#ifndef Tr2PPGodRaysEffect_H
#define Tr2PPGodRaysEffect_H

#include "StdAfx.h"
#include "Shader/Tr2Effect.h"
#include "Tr2RenderTarget.h"
#include "Shader/Tr2ShaderBuffer.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Parameter/Tr2Vector4Parameter.h"
#include "Shader/Parameter/Tr2RuntimeTextureParameter.h"

BLUE_DECLARE( Tr2ShaderBuffer );
BLUE_DECLARE( Tr2RenderTarget );

BLUE_CLASS( Tr2PPGodRaysEffect ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPGodRaysEffect( IRoot* lockobj = NULL );
	~Tr2PPGodRaysEffect();

	void Render( Tr2RenderContext& renderContext, Tr2RenderTarget* downSampleRT, Tr2RenderTarget* godRayRT, Tr2RenderTarget* backBufferRT, Tr2ShaderBuffer* m_psData );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

private:
	Color m_godRayColor;
	float m_intensity;
	BlueSharedString m_noiseTexturePath;

	Vector4 m_intensityVector;

	Tr2EffectPtr m_downSampleEffect;
	Tr2EffectPtr m_effect;
};
TYPEDEF_BLUECLASS( Tr2PPGodRaysEffect );

#endif
