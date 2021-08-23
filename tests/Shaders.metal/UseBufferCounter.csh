#include "MetalDefines.h"

// [numthreads(4, 1, 1)]
kernel void mainCS(device uint* output [[ UAV(0) ]], uint3 globalIdx [[ thread_position_in_grid ]])
{
    // TODO
    output[globalIdx.x] = 0;// output.IncrementCounter();
}
