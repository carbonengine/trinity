// Copyright © 2020 CCP ehf.

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
#define METAL_LOG( ... ) NSLog( __VA_ARGS__ )
#else
#define METAL_LOG( ... )
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

	MTLPixelFormat GetMTLPixelFormat( Tr2RenderContextEnum::PixelFormat pixelFormat );
	MTLVertexFormat GetMTLVertexFormat( Tr2VertexDefinition::DataType dataType );
	MTLTextureType GetMTLTextureType( Tr2RenderContextEnum::TextureType type, uint32 arrayLength, uint32 sampleCount );
	MTLCullMode GetMTLCullMode( Tr2RenderContextEnum::CullMode cullMode );
	MTLBlendFactor GetMTLBlendFactor( uint32_t value );
	MTLCompareFunction GetMTLCompareFunction( uint32_t value );
	MTLStencilOperation GetMTLStencilOperation( uint32_t value );
	MTLBlendOperation GetMTLBlendOperation( uint32_t value );
	MTLColorWriteMask GetMTLColorWriteMask( Tr2RenderContextEnum::ColorWriteEnable writeMask );
	MetalColor GetMetalColorFromRGBA8( uint32_t color );

private:
	MTLPixelFormat PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_SENTINEL];
	// Uh, this array size is a little sketchy
	MTLVertexFormat VertexFormatConversionTable[METAL_SIZEOF_VERTEX_FORMAT_TABLE];

	void SetupPixelFormatConversionTable();
	void SetupVertexFormatConversionTable();
};

struct ConstantBufferToken
{
	void Invalidate()
	{
		frame--;
	}
	void Reset()
	{
		frame = 0;
		offset = 0;
	}

	// page index (32 bits) and offset (32 bits)
	std::atomic<uint64_t> offset = 0;
	std::atomic<uint64_t> frame = 0;
};


class ConstantBufferAllocator
{
public:
	struct Entry
	{
		uint32_t offset;
		uint32_t page;
	};

	ConstantBufferAllocator();
	ConstantBufferAllocator( const ConstantBufferAllocator& ) = delete;

	void Initialize( id<MTLDevice> device );
	void Reset();

	Entry Allocate( const void* data, uint32_t size );
	id<MTLBuffer> GetPage( uint32_t page );

	uint32_t GetTotalUploadedSize() const;

	static const uint32_t CONST_PAGE_SIZE = 2 * 1024 * 1024;
	static const uint32_t CONST_ALIGNMENT = 256;
	static const uint32_t MAX_PAGE_COUNT = 32;

private:
	void CreatePage( uint32_t index );

	// page index (32 bits) and offset (32 bits)
	std::atomic<uint64_t> m_offset;
	uint8_t* m_pageContents[MAX_PAGE_COUNT];
	id<MTLBuffer> m_pages[MAX_PAGE_COUNT];
	id<MTLDevice> m_device;
	std::mutex m_pagesMutex;
	uint32_t m_totalUploadedSize;
};


} // TrinityALImpl


#endif
#endif /* MetalUtils_h */
