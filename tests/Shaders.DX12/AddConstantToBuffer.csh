cbuffer Constants: register(b1, space2)
{
    float4 arg1;
};

Buffer<float4> arg2: register(t0, space2);


RWBuffer<float4> output: register(u0, space2);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = arg1 + arg2[0];
}