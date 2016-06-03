__kernel 
void cs( __read_only image2d_t tex, __const sampler_t sampl, __global float4* output ) 
{
    float avg = read_imagef( tex, (int2)( 0, 0 ) ).x + 
        read_imagef( tex, (int2)( 1, 0 ) ).x + 
        read_imagef( tex, (int2)( 0, 1 ) ).x + 
        read_imagef( tex, (int2)( 1, 1 ) ).x;
    float sampled = read_imagef( tex, sampl, (float2)( 0.5, 0.5 ) ).x;
    output[0] = (float4)( avg, sampled, 0, 0 );
}