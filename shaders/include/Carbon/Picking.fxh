// Copyright © 2026 CCP ehf.

#ifndef CARBON_PICKING_FXH
#define CARBON_PICKING_FXH

// Picking object ID (64 bits) is split into two 32-bit unsigned integers for shader compatibility.
// These variables are provided by Trinity when rendering objects for picking purposes. 
// Picking pixel shader needs to forward these values to the output target.
uint pickingObjectLowBits;
uint pickingObjectHighBits;
uint areaId;


// The expected output of a picking pixel shader.
// To be filled with CreatePickingOutput or CreateInstancedPickingOutput functions.
struct TrinityPickingOutput
{
    uint4 pickingTarget : SV_Target;
};

// Creates a TrinityPickingOutput for a non-instanced object.
TrinityPickingOutput CreatePickingOutput()
{
    TrinityPickingOutput result;
    result.pickingTarget = uint4( pickingObjectLowBits, pickingObjectHighBits, areaId, 0 );
    return result;
}

// Creates a TrinityPickingOutput for an instanced object.
// Instance data is piped from the instance vertex buffer.
TrinityPickingOutput CreateInstancedPickingOutput( uint2 instanceData )
{
    TrinityPickingOutput result;
    result.pickingTarget = uint4( pickingObjectLowBits, pickingObjectHighBits, instanceData );
    return result;
}


#endif