// Copyright © 2020 CCP ehf.

#pragma once
#if TRINITY_PLATFORM == TRINITY_METAL

static const char* sBlitShaderSource = R"(
	#include <metal_stdlib>
	#include <simd/simd.h>

	using namespace metal;


	enum BlitPresentVertexInputIndex
	{
		BlitPresentInputIndexVertices = 0,
	};

	enum BlitPresentFragmentInputIndex
	{
		BlitPresentFragmentInputIndexTexture  = 0,
	};

	struct BlitPresentVertex
	{
		// Positions in pixel space (i.e. a value of 100 indicates 100 pixels from the origin/center)
		float2 position;

		// 2D texture coordinate
		float2 texCoord;
	};

	// Vertex shader outputs and per-fragment inputs.
	struct BlitPresentData
	{
		float4 clipSpacePosition [[position]];
		float2 texCoord;
	};

	vertex
	BlitPresentData blitPresentVertexShader(
		uint vertexID [[ vertex_id ]],
		constant BlitPresentVertex *vertexArray [[ buffer(BlitPresentInputIndexVertices) ]])
	{
		BlitPresentData out;
		out.clipSpacePosition.xy = vertexArray[vertexID].position.xy;
		out.clipSpacePosition.z = 0.0;
		out.clipSpacePosition.w = 1.0;
		out.texCoord = vertexArray[vertexID].texCoord;

		return out;
	}

	fragment float4 blitPresentFragmentShader(
		BlitPresentData  in [[stage_in]],
		texture2d<half> colorScaleTexture [[ texture(BlitPresentFragmentInputIndexTexture) ]])
	{
		constexpr sampler textureSamplerLinear(mag_filter::linear,
												min_filter::linear,
												mip_filter::linear);

		return float4(colorScaleTexture.sample(textureSamplerLinear, in.texCoord));
	}
)";

static const char* sClearShaderSource = R"(
	#include <metal_stdlib>
	using namespace metal;

	kernel void ClearFloatBuffer(
		constant float* values [[ buffer(0) ]],
		constant uint* length [[ buffer(1) ]],
		device float* buffer [[ buffer(2) ]],
		uint3 globalIdx [[ thread_position_in_grid ]])
	{
		if (globalIdx.x < *length)
		{
			buffer[globalIdx.x] = values[0];
		}
	}

	kernel void ClearUIntBuffer(
		constant uint* values [[ buffer(0) ]],
		constant uint* length [[ buffer(1) ]],
		device uint* buffer [[ buffer(2) ]],
		uint3 globalIdx [[ thread_position_in_grid ]])
	{
		if (globalIdx.x < *length)
		{
			buffer[globalIdx.x] = values[0];
		}
	}

	kernel void ClearFloatTexture(
		constant float* values [[ buffer(0) ]],
		texture2d<float, access::write> texture [[ texture(0) ]],
		uint3 globalIdx [[ thread_position_in_grid ]])
	{
		float4 color(values[0], values[1], values[2], values[3]);

		if (globalIdx.x < texture.get_width() && globalIdx.y < texture.get_height())
		{
			texture.write(color, globalIdx.xy, 0);
		}
	}

	kernel void ClearUIntTexture(
		constant uint* values [[ buffer(0) ]],
		texture2d<uint, access::write> texture [[ texture(0) ]],
		uint3 globalIdx [[ thread_position_in_grid ]])
	{
		uint4 color(values[0], values[1], values[2], values[3]);

		if (globalIdx.x < texture.get_width() && globalIdx.y < texture.get_height())
		{
			texture.write(color, globalIdx.xy, 0);
		}
	}
)";

#endif
