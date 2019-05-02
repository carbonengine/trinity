RWBuffer<float4> output: register(u0, space2);

[numthreads(1, 1, 1)]
void main()
{
    output[0] = float4( 1, 2, 3, 4 );
}