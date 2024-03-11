#include "MetalDefines.h"
#include <metal_stdlib>

using namespace metal;
using namespace raytracing;

struct Attributes 
{
	float2 uv;
};

struct PerObjectData
{
	float4 material;
	float4 Clipdata1;
};

struct PerObjectDataPtr
{
	device PerObjectData *perObjectData;
};

//constant bool useIntersectionFunctions [[function_constant(0)]];


[[intersection(triangle, instancing)]]
bool ClosestHit( ray_data float4 & color 				[[ payload ]],
                 unsigned int geometryIndex     		[[ instance_intersection_function_table_offset ]],
				 device PerObjectDataPtr *resources    	[[ buffer(0) ]] )
{ 

	// this is a buffer of pointers (materials -> constantBuffer)
	PerObjectData perObjectData = resources[geometryIndex].perObjectData[0];
	//PrimitiveData ppd = *primitiveData;
	//color = ppd.material;

	color = perObjectData.material;
    return 1;
}


[[intersection(triangle, instancing)]]
bool ClosestHitWithPerObjData(	ray_data float4 & color 			[[ payload ]],
	                            float3 origin                   	[[ origin ]],
                                float3 direction                	[[ direction ]],
                 				unsigned int geometryIndex     		[[ instance_intersection_function_table_offset ]],
                 				constant Attributes& attrib 		[[ CBUFFER(0) ]],
								device PerObjectDataPtr *resources  [[ buffer(0) ]] )
{

	PerObjectData perObjectData = resources[geometryIndex].perObjectData[0];

	color = perObjectData.material * perObjectData.Clipdata1;

	return 1;
}

                              

/*
[[intersection(triangle, instancing)]]
bool ClosestHitWithTexture(	ray_data float3 & color 		[[payload]], 
							constant Attributes& attrib 	[[ CBUFFER(0) ]],
							sampler sampl 					[[ sampler(0) ]],
							texture2d<float4> Albedo		[[ texture(0) ]],
							constant PerObject& material	[[ CBUFFER(1) ]]
							)
{
    color = Albedo[0].sample(sampl, attrib.uv, 0) * material;
    return 1;
}*/

