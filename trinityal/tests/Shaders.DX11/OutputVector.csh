// Copyright © 2023 CCP ehf.

RWBuffer<float4> output: register(u0);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = float4( 1, 2, 3, 4 );
}