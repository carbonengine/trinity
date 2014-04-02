#pragma once
#ifndef Tr2ImageIOHelpers_H
#define Tr2ImageIOHelpers_H

class Tr2ImageHandler;

namespace Tr2ImageIOHelpers
{

bool CreateTexture(	
	Tr2ImageHandler& ih, 
	Tr2TextureAL &out, 
	uint32_t &memoryUse, 
	Tr2PrimaryRenderContext &renderContext, 
	Tr2RenderContextEnum::BufferUsage usage = Tr2RenderContextEnum::USAGE_CPU_READ );

bool Create2DTexture(	
	Tr2ImageHandler& ih, 
	Tr2TextureAL &out, 
	uint32_t &memoryUse, 
	Tr2PrimaryRenderContext &renderContext, 
	Tr2RenderContextEnum::BufferUsage usage = Tr2RenderContextEnum::USAGE_CPU_READ );

bool CopyToTexture( 
	Tr2ImageHandler& ih, 
	Tr2TextureAL& texture, 
	unsigned int x, 
	unsigned int y, 
	unsigned int margin, 
	Tr2RenderContext& renderContext );

}

#endif