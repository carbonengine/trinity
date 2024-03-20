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


[[visible]]
void ClosestHit( thread float4 & color ,
				 device void *resources )
{ 

	// this is a buffer of pointers (materials -> constantBuffer)
	PerObjectData perObjectData = ((device PerObjectDataPtr*)resources)[0].perObjectData[0];
	//PrimitiveData ppd = *primitiveData;
	//color = ppd.material;

	color = perObjectData.material;
}


[[visible]]
void ClosestHitWithPerObjData(	thread float4 & color,
								device void *resources  )
{

	PerObjectData perObjectData = ((device PerObjectDataPtr*)resources)[0].perObjectData[0];

	color = perObjectData.material;// * perObjectData.Clipdata1;
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

