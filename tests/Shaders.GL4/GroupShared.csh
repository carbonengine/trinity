__kernel __attribute__((reqd_work_group_size(10, 10, 2)))
void cs(__global uint* out) 
{
    __local uint sum;
    uint3 globalIdx = (uint3)( get_global_id( 0 ), get_global_id( 1 ), get_global_id( 2 ) );
    if( all( globalIdx == 0 ) )
    {
        sum = 0;
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    
    atomic_add( &sum, globalIdx.x + globalIdx.y + globalIdx.z );

    barrier( CLK_LOCAL_MEM_FENCE );
    
    if( all( globalIdx == 0 ) )
    {
        out[0] = sum;
    }
}