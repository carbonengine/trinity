// Copyright © 2023 CCP ehf.

cbuffer Constants: register(b1)
{
    float4 arg1;
};

Buffer<float4> arg2: register(t0);


RWBuffer<float4> output: register(u0);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = arg1 + arg2[0];
}