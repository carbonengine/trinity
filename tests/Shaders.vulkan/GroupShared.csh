RWBuffer<uint> output: register(u0, space1);


groupshared uint sum;

[numthreads(10, 10, 2)]
void main(uint3 globalIdx : SV_DispatchThreadID)
{
    if( all( globalIdx == 0 ) )
    {
        sum = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    InterlockedAdd( sum, globalIdx.x + globalIdx.y + globalIdx.z );

    GroupMemoryBarrierWithGroupSync();
    if( all( globalIdx == 0 ) )
    {
        output[0] = sum;
    }
}