float OcclusionBufferOffset <bool AutoRegister = true;>;
RWBuffer<uint> FlareOcclusionBuffer <bool AutoRegister = true;>;

groupshared float occlusions[4];

[numthreads( 4, 1, 1 )]
void CopyCountersCS( uint3 groupIdx : SV_GroupID, uint3 threadIdx : SV_GroupThreadID )
{
	uint flareOffset = groupIdx.x * 13;
	uint occluderOffset = flareOffset + 5 + threadIdx.x * 2;
	uint persistedOffset = flareOffset + 1 + threadIdx.x;

	uint total = FlareOcclusionBuffer[occluderOffset];
	if( total > 0 )
	{
		uint unoccluded = FlareOcclusionBuffer[occluderOffset + 1];

		float occlusion = float( unoccluded ) / float( total ) / 100.0;
		FlareOcclusionBuffer[persistedOffset] = asuint( occlusion );

		FlareOcclusionBuffer[occluderOffset] = 0;
		FlareOcclusionBuffer[occluderOffset + 1] = 0;
		occlusions[threadIdx.x] = occlusion;
	}
	else
	{
		occlusions[threadIdx.x] = asfloat( FlareOcclusionBuffer[persistedOffset] );
	}

    GroupMemoryBarrierWithGroupSync();

	if( threadIdx.x == 0 )
	{
		FlareOcclusionBuffer[flareOffset] = asuint( occlusions[0] * occlusions[1] * occlusions[2] * occlusions[3] );
	}
}

[numthreads( 13, 1, 1 )]
void ClearCS( uint3 threadIdx : SV_GroupThreadID )
{
	FlareOcclusionBuffer[asuint( OcclusionBufferOffset ) + threadIdx.x] = threadIdx.x < 5 ? asuint( 1.0 ) : 0;
}

technique CopyCounters
{
	pass P0
	{
        ComputeShader = compile cs_5_0 CopyCountersCS();
	}
}

technique Clear
{
	pass P0
	{
        ComputeShader = compile cs_5_0 ClearCS();
	}
}
