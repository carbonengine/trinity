#pragma once
#ifndef BcDecompress_H
#define BcDecompress_H

#include "Tr2RenderContextEnum.h"
#include "Tr2HalHelperStructures.h"

bool BcDecompress( 
	uint32_t width, 
	uint32_t height, 
	uint32_t depth, 
	Tr2RenderContextEnum::PixelFormat format, 
	const Tr2SubresourceData& src, 
	std::unique_ptr<uint8_t[]>& decompressed );

#endif