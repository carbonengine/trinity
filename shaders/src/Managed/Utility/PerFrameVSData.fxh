// Copyright © 2026 CCP ehf.

#ifndef PERFRAMEVSDATA_FXH
#define PERFRAMEVSDATA_FXH

#include "../../include/Carbon/System.fxh"

struct PerFrameVSData
{
    float4x4 ViewInverseTransposeMat;
    float4 SunWorldDir;
    float4 SceneFogColor;
    float4x4 ViewProjectionMat;  
    float4x4 ViewMat;
    float4x4 ProjectionMat;
} PerFrameVS : register( PERFRAME_VS_STARTREGISTER );

#endif