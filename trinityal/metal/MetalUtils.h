//
//  Created by Apple on 04/05/2020.
//  Copyright © 2020 CCP. All rights reserved.
//

#ifndef MetalUtils_h
#define MetalUtils_h
#if TRINITY_PLATFORM == TRINITY_METAL

#include <Metal/Metal.h>
#include "../Tr2RenderContextEnum.h"
#include "../Tr2VertexDefinition.h"

#define METAL_SIZEOF_VERTEX_FORMAT_TABLE 128
#define METAL_VISIBILITY_RESULT_SIZE_IN_BYTES 8

// Remove for prodcution code
#define METAL_LOG_OUTPUT 0
#if METAL_LOG_OUTPUT
#define METAL_LOG(...) NSLog(__VA_ARGS__)
#else
#define METAL_LOG(...)
#endif

namespace TrinityALImpl
{

	struct MetalColor
	{
		float red;
		float green;
		float blue;
		float alpha;
	};

	class MetalUtils
	{
	public:
		MetalUtils();
		~MetalUtils();

		MTLPixelFormat      GetMTLPixelFormat(Tr2RenderContextEnum::PixelFormat pixelFormat);
		MTLVertexFormat     GetMTLVertexFormat(Tr2VertexDefinition::DataType dataType);
		MTLTextureType      GetMTLTextureType(Tr2RenderContextEnum::TextureType type, uint32 arrayLength, uint32 sampleCount);
		MTLCullMode         GetMTLCullMode(Tr2RenderContextEnum::CullMode cullMode);
		MTLBlendFactor      GetMTLBlendFactor(uint32_t value);
		MTLCompareFunction  GetMTLCompareFunction(uint32_t value);
		MTLStencilOperation GetMTLStencilOperation(uint32_t value);
		MTLBlendOperation   GetMTLBlendOperation(uint32_t value);
		MTLColorWriteMask   GetMTLColorWriteMask(Tr2RenderContextEnum::ColorWriteEnable writeMask);
		MetalColor          GetMetalColorFromRGBA8(uint32_t color);

	private:
		MTLPixelFormat PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_SENTINEL];
		// Uh, this array size is a little sketchy
		MTLVertexFormat VertexFormatConversionTable[METAL_SIZEOF_VERTEX_FORMAT_TABLE];

		void SetupPixelFormatConversionTable();
		void SetupVertexFormatConversionTable();
	};

} // TrinityALImpl


#endif
#endif /* MetalUtils_h */
