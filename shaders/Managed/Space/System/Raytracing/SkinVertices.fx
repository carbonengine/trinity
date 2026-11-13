// Copyright © 2026 CCP ehf.

// Default vertex transform for ray tracing geometry: applies morph targets and skinning.
// A material replaces it for its mesh areas with a "TransformVertices" technique, see TransformVertices.fxh.

#include "../../../../include/Carbon/System.fxh"
#include "../../../../include/Carbon/SpaceScene/Raytracing/TransformVertices.fxh"

[numthreads( TRANSFORM_VERTICES_GROUP_SIZE, 1, 1 )]
void SkinVertices( uint3 groupId : SV_GroupID, uint3 threadId : SV_GroupThreadID )
{
    TransformJob job;
    uint vtx;
    if( GetTransformVertex( groupId.x, threadId.x, job, vtx ) )
    {
        float3 pos = LoadPosition( job, vtx );
        pos = ApplyMorphs( job, vtx, pos );
        pos = ApplySkinning( job, vtx, pos );
        StoreTransformedPosition( job, vtx, pos );
    }
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 SkinVertices();
    }
}
