// Copyright © 2026 CCP ehf.


RWBuffer<uint> DrawParameters;
Buffer<int> ParticleCounters;

[numthreads( 1, 1, 1 )]
void SetParameters()
{
    uint count = ParticleCounters[1];
    DrawParameters[0] = count * 6;
    DrawParameters[1] = 1;
    DrawParameters[2] = 0;
    DrawParameters[3] = 0;
}


technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 SetParameters();
    }
}
