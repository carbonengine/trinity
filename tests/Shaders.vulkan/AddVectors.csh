Buffer<float4> arg1: register(t0, space1);
Buffer<float4> arg2: register(t1, space1);


RWBuffer<float4> output: register(u0, space1);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = arg1[0] + arg2[0];
}