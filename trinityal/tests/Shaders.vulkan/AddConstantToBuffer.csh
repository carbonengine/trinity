cbuffer Constants: register(b0)
{
    float4 arg1;
};

Buffer<float4> arg2: register(t0, space1);


RWBuffer<float4> output: register(u0, space1);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = arg1 + arg2[0];
}