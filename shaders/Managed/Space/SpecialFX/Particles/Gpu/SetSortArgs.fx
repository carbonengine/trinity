// Copyright © 2026 CCP ehf.


RWBuffer<uint> SortParameters;
Buffer<int> ParticleCounters;

[numthreads( 1, 1, 1 )]
void SetParameters()
{
    uint count = ParticleCounters[1];
    SortParameters[0] = ( ( max( count, (uint)1 ) - 1 ) >> 9 ) + 1;
    SortParameters[1] = 1;
    SortParameters[2] = 1;
    SortParameters[3] = count;
}


technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 SetParameters();
    }
}
