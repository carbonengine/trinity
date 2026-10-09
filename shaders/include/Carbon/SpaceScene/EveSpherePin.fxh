// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESPHEREPIN_FXH
#define CARBON_EVESPHEREPIN_FXH

// Data passed from the engine to all shader stages for EveSpherePinData draw calls in "per object" constant buffer.
struct EveSpherePinData
{
    // World transform of this pin
    float4x4 WorldMat;
    // XYZ - pin position, W - pin radius
    float4 PinPosition;
    // X - pin rotation, YZW - unused
    float4 PinRotation;
    // RGBA - pin color
    float4 PinColor;
    // X - alpha testing threshold, YZW - unused
    float4 PinThreshold;
    // X - sin(pin radius), Y - cos(pin radius), Z - sin(pin rotation), W - cos(pin rotation)
    float4 PinRadiusPrecalc;
    // XY - pin UV scale, ZW - pin UV offset
    float4 PinUV;
};

#endif