Buffer<float4> arg1: register(t0);
Buffer<float4> arg2: register(t1);


RWBuffer<float4> output: register(u0);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = arg1[0] + arg2[0];
}