// Copyright © 2023 CCP ehf.

#pragma once

#include <metal_stdlib>

// These values must be synchronized with defines in TrinityAL/metal/MetalWorkQueue.h

// buffers
#define CBUFFER( i ) buffer( 4 + i )
#define SRV( i ) buffer( 4 + i )
#define UAV( i ) buffer( 24 + i )

// textures
#define UAVT( i ) texture( 24 + i )



struct RayDesc
{
	float3 Origin;
	float3 Direction;
	float TMin;
	float TMax;
};

#define RAY_FLAG_NONE 0x00
#define RAY_FLAG_FORCE_OPAQUE 0x01
#define RAY_FLAG_FORCE_NON_OPAQUE 0x02
#define RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH 0x04
#define RAY_FLAG_SKIP_CLOSEST_HIT_SHADER 0x08
#define RAY_FLAG_CULL_BACK_FACING_TRIANGLES 0x10
#define RAY_FLAG_CULL_FRONT_FACING_TRIANGLES 0x20
#define RAY_FLAG_CULL_OPAQUE 0x40
#define RAY_FLAG_CULL_NON_OPAQUE 0x80
#define RAY_FLAG_SKIP_TRIANGLES 0x100
#define RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES 0x200


struct __MetalHitSV
{
	uint instance_id;
	float3 origin;
	float3 direction;
	float min_distance;
	float distance;
	float2 barycentric_coord;
};

#define __INTERSECION_TAGS metal::raytracing::instancing, metal::raytracing::triangle_data, metal::raytracing::world_space_data

struct __RtLocalMaterial
{
	device void* buffer;
};

struct RaytracingAccelerationStructureT
{
	metal::raytracing::instance_acceleration_structure tlas;
};

#define RaytracingAccelerationStructure const RaytracingAccelerationStructureT*

template <typename payload_t, typename global_input_t>
struct ShaderTableT
{
	metal::raytracing::intersection_function_table<__INTERSECION_TAGS> intersectionTable;
	metal::visible_function_table<void( thread payload_t&, __MetalHitSV, device __RtLocalMaterial*, constant global_input_t&, constant ShaderTableT<payload_t, global_input_t>& )> missShaderTable;
	metal::visible_function_table<void( thread payload_t&, __MetalHitSV, device __RtLocalMaterial*, constant global_input_t&, constant ShaderTableT<payload_t, global_input_t>& )> hitShaderTable;
	device __RtLocalMaterial* missMaterials;
	device __RtLocalMaterial* hitMaterials;
	constant global_input_t& globalInput;
};


template <typename T>
void __GetLocalRTBufferT( device __RtLocalMaterial* materials, uint index, thread T& dest )
{
	dest = ( (device T*)materials[index].buffer )[0];
}

#define __GetLocalRTBuffer( index, dest ) __GetLocalRTBufferT( __rtMaterials, index, dest )

template <typename payload_t, typename global_input_t>
void __TraceRay(
	device RaytracingAccelerationStructure accelerationStructure,
	uint rayFlags,
	uint instanceInclusionMask,
	uint rayContributionToHitGroupIndex,
	uint multiplierForGeometryContributionToHitGroupIndex,
	uint missShaderIndex,
	RayDesc ray_,
	thread payload_t& payload,

	constant ShaderTableT<payload_t, global_input_t>& shaderTable,
	constant global_input_t& globalInput )
{
	metal::raytracing::intersector<__INTERSECION_TAGS> i;
	i.assume_geometry_type( metal::raytracing::geometry_type::triangle );
	i.accept_any_intersection( rayFlags & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH );

	metal::raytracing::ray r;
	r.origin = ray_.Origin;
	r.direction = ray_.Direction;
	r.min_distance = ray_.TMin;
	r.max_distance = ray_.TMax;

	typename metal::raytracing::intersector<__INTERSECION_TAGS>::result_type intersection = i.intersect( r, accelerationStructure[0].tlas, instanceInclusionMask, shaderTable.intersectionTable, payload );

	__MetalHitSV hit;

	if( intersection.type == metal::raytracing::intersection_type::none )
	{
		hit.instance_id = 0;
		hit.origin = r.origin;
		hit.direction = r.direction;
		hit.min_distance = r.min_distance;
		hit.distance = r.max_distance;
		hit.barycentric_coord = { 0.0, 0.0 };

		shaderTable.missShaderTable[missShaderIndex]( payload, hit, shaderTable.missMaterials + 8 * missShaderIndex, globalInput, shaderTable );
	}
	else if( ( rayFlags & RAY_FLAG_SKIP_CLOSEST_HIT_SHADER ) == 0 )
	{
		hit.instance_id = intersection.instance_id;
		hit.origin = intersection.world_to_object_transform * float4( r.origin, 1.0 );
		hit.direction = intersection.world_to_object_transform * float4( r.direction, 0.0 );
		hit.min_distance = r.min_distance;
		hit.distance = intersection.distance;
		hit.barycentric_coord = intersection.triangle_barycentric_coord;

		uint offset = ( (device uint*)accelerationStructure )[2 + intersection.instance_id];

		shaderTable.hitShaderTable[intersection.instance_id]( payload, hit, shaderTable.hitMaterials + 8 * offset, globalInput, shaderTable );
	}
}

#define TraceRay( accelerationStructure, rayFlags, instanceInclusionMask, rayContributionToHitGroupIndex, multiplierForGeometryContributionToHitGroupIndex, missShaderIndex, ray, payload ) \
	__TraceRay( accelerationStructure, rayFlags, instanceInclusionMask, rayContributionToHitGroupIndex, multiplierForGeometryContributionToHitGroupIndex, missShaderIndex, ray, payload, __rtShaderTable, __rtGlobals )


#define DispatchRaysIndex() ( __dispatchRaysIndex )
#define DispatchRaysDimensions() ( __dispatchRaysDimensions )


#define InstanceID() ( __metalHitSV.instance_id )
#define ObjectRayOrigin() ( __metalHitSV.origin )
#define ObjectRayDirection() ( __metalHitSV.direction )
#define RayTMin() ( __metalHitSV.min_distance )
#define RayTCurrent() ( __metalHitSV.distance )

#define IgnoreHit() return false
