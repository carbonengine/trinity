Texture2D<float> tex: register(t0, space2);
SamplerState sampl: register(s0, space2);
RWBuffer<float4> output: register(u0, space2);

[numthreads(1, 1, 1)]
void main()
{
    float avg = tex[uint2(0, 0)] + tex[uint2(1, 0)] + tex[uint2(0, 1)] + tex[uint2(1, 1)];
    float sampled = tex.SampleLevel( sampl, float2( 0.5, 0.5 ), 0.0 ).x;
    output[0] = float4( avg, sampled, 0, 0 );
}