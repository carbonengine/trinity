__kernel 
void cs(__global float4* out) 
{
    out[0] = (float4)( 1.0, 2.0, 3.0, 4.0 );
}