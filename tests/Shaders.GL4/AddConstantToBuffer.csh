__kernel 
void cs(__global float4* arg1, __global float4* arg2, __global float4* out) 
{
    out[0] = arg1[0] + arg2[0];
}