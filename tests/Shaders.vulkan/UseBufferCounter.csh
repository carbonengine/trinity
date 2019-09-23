RWStructuredBuffer<uint> output: register(u0, space1);


[numthreads(4, 1, 1)]
void main(uint3 globalIdx : SV_DispatchThreadID)
{
    output[globalIdx.x] = output.IncrementCounter();
}