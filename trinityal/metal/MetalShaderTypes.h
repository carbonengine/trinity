// Copyright © 2023 CCP ehf.

//
// Header containing types and enum constants shared between Metal shaders and C/ObjC source.
//
#pragma once
#if TRINITY_PLATFORM == TRINITY_METAL
#include <simd/simd.h>

// Any changes here need to mirrored in the Metal shader strings
enum BlitPresentVertexInputIndex
{
	BlitPresentInputIndexVertices = 0,
};

enum BlitPresentFragmentInputIndex
{
	BlitPresentFragmentInputIndexTexture = 0,
};


struct BlitPresentVertex
{
	// Positions in pixel space (i.e. a value of 100 indicates 100 pixels from the origin/center)
	vector_float2 position;

	// 2D texture coordinate
	vector_float2 texCoord;
};
#endif