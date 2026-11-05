// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVELENSFLARE_FXH
#define CARBON_EVELENSFLARE_FXH


struct EveLensflarePerObjectData
{
    // Direction towards the lens flare that owns the occluder; W - flare size scale
    float4 LensflareFxDirectionScale;
    // Offset into the occlusion buffer where the occlusion results for this lens flar are stored.
    // X - foreground occlusion results, Y - background (planets, etc.) occlusion results, Z, W - unused
    uint4 OcclusionIndices;
};

// The buffer containing occlusion results of occlusion queries for lens flares. Read values need to be cast to float using asfloat() to get the occlusion ratio in the range [0, 1].
Buffer<uint> FlareOcclusionBuffer <bool AutoRegister = true;>;

float GetLensFlareOcclusion( uint index )
{
    return asfloat( FlareOcclusionBuffer[index] );
}

// Direction towards the lens flare that owns the occluder; W - flare size scale
// Can be used for querying lens flare position for shaders outside the lens flare system.
float4 LensflareFxDirectionScale <bool AutoRegister = true; >;

// Stores offsets into the occlusion buffer where the occlusion results for a lens flare are stored.
// X - offset into the occlusion buffer for foreground occlusion results (cast to uint with asuint())
// Y - offset into the occlusion buffer for background occlusion results (cast to uint with asuint())
// Z, W - unused
float4 LensflareFxOccScale <bool AutoRegister = true; >;


#endif