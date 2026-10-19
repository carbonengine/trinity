// Copyright © 2026 CCP ehf.

#ifndef CARBON_INTERIOR_PERSCENEDATA_FXH
#define CARBON_INTERIOR_PERSCENEDATA_FXH


#include "../System.fxh"


// Per-scene (per-frame) constant buffer data for the vertex shader.
struct InteriorSceneDataVS
{
    float4x4 ViewInverseTransposeMat;
    float4 sunDirWorld;
    float4 sceneFogColor;
    
    float4x4 ViewProjectionMat;
    float4x4 ViewMat;
    float4x4 ProjectionMat;
};

// Per-scene (per-frame) constant buffer data for the pixel shader.
struct InteriorSceneDataPS
{
    float4x4 ViewInverseTransposeMat;
    float4 sceneAmbientColor;
    float4 sceneFogColor;
    float4 sunDirWorld;
    float4 sunDiffuseColor;
    float4 sunSpecularColor;
    float4 fogValues; // x = maxFogAmount, y = maxFogDistance, z = minFogDistance, w unused
    float4x4 ViewProjectionMat;
    float4 misc; // x = Global spherical harmonic scale factor, y = shadow count, z = inv.shadow size, w = radius
    float4 viewPort; // xy - viewport width/height, zw - viewport offset
    float4x4 ViewProjInverse;
};


#endif