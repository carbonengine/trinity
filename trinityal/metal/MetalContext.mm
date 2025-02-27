//
//  Copyright © 2020 CCP. All rights reserved.
//
#if TRINITY_PLATFORM == TRINITY_METAL
#import <Foundation/Foundation.h>
#include "MetalContext.h"
#include "ALLog.h"

bool g_useParallelEncoding = false;
extern bool g_brokenMacOSNvidiaDrivers;

CCP_STATS_DECLARE( constantDataSize, "Trinity/AL/constantDataSize", true, CST_MEMORY, "Total size of constant data uploaded per frame" );

namespace TrinityALImpl
{

	MetalContext::MetalContext()
		: m_recordingFrameNumber( 1 )
		, m_renderedFrameNumber( 0 )
        , m_parallelEncodersCount( 0 )
		, m_gpuTimerRate( 1 )
		, m_beginCpuTime( 0 )
		, m_beginGpuTime( 0 )
		, m_gpuTimerRateMeasured( false )
	{
		m_utils            = new MetalUtils;
		m_device           = MTLCreateSystemDefaultDevice();
		m_commandQueue     = [m_device newCommandQueueWithMaxCommandBufferCount:1024];
        
        bool isNvidia = [m_device.name rangeOfString:@"NVidia" options:NSCaseInsensitiveSearch].location != NSNotFound;
        bool isIntel = [m_device.name rangeOfString:@"Intel" options:NSCaseInsensitiveSearch].location != NSNotFound;
		
        if( isNvidia || isIntel )
        {
            CCP_LOGWARN("Disabling parallel rendering on nVidia/Intel GPU");
            g_useParallelEncoding = false;
        }
		if( g_useParallelEncoding )
		{
			m_secondaryWorkQueues.resize( std::thread::hardware_concurrency() );
			for( auto& queue : m_secondaryWorkQueues )
			{
				queue.SetMetalContext( this );
			}
		}
		m_primaryWorkQueue.SetCommandQueue( m_commandQueue );
		m_primaryWorkQueue.SetMetalContext( this );
        
        if( @available( macOS 13.0, * ) )
        {
            if( m_device.argumentBuffersSupport >= MTLArgumentBuffersTier2 )
            {
                m_resourceHeap.Initialize( 2048, m_device );
            }
        }
		
		GenerateDummyTexture();
		GenerateDummyBuffer();
		
		if( @available( macOS 10.15, * ) )
		{
			[m_device sampleTimestamps:&m_beginCpuTime gpuTimestamp:&m_beginGpuTime];
		}
        else if( isNvidia )
        {
            g_brokenMacOSNvidiaDrivers = true;
        }
        for( auto& alloc : m_cbAllocator )
        {
            alloc.Initialize( m_device );
        }
	}

	MetalContext::~MetalContext()
	{
        m_pendingRelease.clear();
        
		delete m_utils;

		DestroyMetalBuffer( m_dummyBuffer );

		for( int i = 0; i < METAL_NUM_DUMMY_TEXTURES; ++i )
		{
			DestroyMetalTexture(m_dummyTexture[i]);
		}
		for( auto buffer : m_destroyedConstantBuffers )
		{
			CCPAlignedFree( buffer );
		}
	}

	id<MTLBuffer> MetalContext::CreateMetalBuffer( MetalWorkQueue* workQueue, size_t sizeInBytes, MTLResourceOptions options, const void *data )
	{
		id<MTLBuffer> buffer;

		MTLStorageMode storageMode = MTLStorageMode( ( options & MTLResourceStorageModeMask ) >> MTLResourceStorageModeShift );
		MTLCPUCacheMode cpuCacheMode = MTLCPUCacheMode( ( options & MTLResourceCPUCacheModeMask ) >> MTLResourceCPUCacheModeShift );

		METAL_LOG( @"Log:Creating buffer of length %lu", sizeInBytes );
		if( data )
		{
			if( storageMode == MTLStorageModePrivate )
			{
				id<MTLBuffer> staging = [m_device newBufferWithBytes:data length:sizeInBytes options:MTLResourceStorageModeManaged];
				buffer = [m_device newBufferWithLength:sizeInBytes options:options];
				workQueue->CopyBufferToBuffer( buffer, 0, staging, 0, sizeInBytes );
				DestroyMetalBuffer( staging );
			}
			else
			{
				buffer  = [m_device newBufferWithBytes:data length:sizeInBytes options:options];
			}
		}
		else
		{
			buffer  = [m_device newBufferWithLength:sizeInBytes options:options];
		}

		return buffer;
	}

	void MetalContext::DestroyMetalBuffer( id<MTLBuffer> buffer )
	{
#if !__has_feature(objc_arc)
		[buffer release];
#endif
	}


	MTLVertexDescriptor* MetalContext::CreateVertexLayout()
	{
		MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
		return vertexDescriptor;
	}
	
	void MetalContext::DestroyVertexLayout( MTLVertexDescriptor *vertexDescriptor )
	{
#if !__has_feature(objc_arc)
		[vertexDescriptor release];
#endif
	}

	void MetalContext::IndicateBufferModified( id<MTLBuffer> buffer, size_t offset, size_t length )
	{
		[buffer didModifyRange:NSMakeRange( offset, length )];
	}
	
	void MetalContext::DestroyConstantBuffer( void* buffer )
	{
		m_destroyedConstantBuffers.push_back( buffer );
	}

	MTLSamplerDescriptor* MetalContext::CreateSamplerDescriptor()
	{
		MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
		return samplerDescriptor;
	}

	void MetalContext::DestroySamplerDescriptor( MTLSamplerDescriptor* samplerDescriptor )
	{
#if !__has_feature(objc_arc)
		[samplerDescriptor release];
#endif
	}

	id<MTLSamplerState> MetalContext::CreateSamplerState( MTLSamplerDescriptor* samplerDescriptor )
	{
		id<MTLSamplerState> samplerState = [m_device newSamplerStateWithDescriptor:samplerDescriptor];
		return samplerState;
	}

	void MetalContext::DestroySamplerState( id<MTLSamplerState> samplerState )
	{
#if !__has_feature(objc_arc)
		[samplerState release];
#endif
	}

	id<MTLTexture> MetalContext::CreateMetalTexture(MTLTextureType metalTextureType,
													MTLPixelFormat  pixelFormat,
													size_t          width,
													size_t          height,
													size_t          depth,
													uint32          mipMapCount,
													MTLStorageMode  storageMode,
													MTLTextureUsage textureUsage,
													uint32          sampleCount,
													uint32          arrayLength)
	{
		METAL_LOG(@"Log:Creating texture of size %lux%lu", width, height);

		MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];

		textureDescriptor.textureType      = metalTextureType;
		textureDescriptor.pixelFormat      = pixelFormat;
		textureDescriptor.width            = width;
		textureDescriptor.height           = height;
		textureDescriptor.depth            = depth;
		textureDescriptor.mipmapLevelCount = MAX(mipMapCount,1);
		textureDescriptor.arrayLength      = arrayLength;
		textureDescriptor.usage            = textureUsage;
		textureDescriptor.sampleCount      = sampleCount;
		textureDescriptor.storageMode      = storageMode;

		id<MTLTexture> texture = [m_device newTextureWithDescriptor: textureDescriptor];

		if( !texture )
		{
			CCP_AL_LOGERR( "Failed to create a texture." );
		}

		return texture;
	}

	void MetalContext::GenerateDummyTexture()
	{
		uint32_t textureDim = 8;
        // Enough size for TextureCube
		uint32_t texData[textureDim][textureDim * textureDim];
		
		struct TextureProperties
		{
			MTLTextureType type;
			size_t width;
			size_t height;
			size_t depth;
			uint32_t samples;
		};

		const TextureProperties dummyTextureProperties[METAL_NUM_DUMMY_TEXTURES] = {
			TextureProperties{MTLTextureType1D, textureDim, 1, 1, 1},
			TextureProperties{MTLTextureType2D, textureDim, textureDim, 1, 1},
			// 4 samples is supported by all devices according to docs
			TextureProperties{MTLTextureType2DMultisample, textureDim, textureDim, 1, 4},
			TextureProperties{MTLTextureTypeCube, textureDim, textureDim, 1, 1},
            TextureProperties{MTLTextureType3D, textureDim, textureDim, textureDim, 1},
            TextureProperties{MTLTextureType2DArray, textureDim, textureDim, 1, 1},
		};

		// Generate texture data
		for( int z = 0; z < textureDim; z++ )
		{
			for( int y = 0; y < textureDim; y++ )
			{
				for( int x = 0; x < textureDim; x++ )
				{
					uint32_t colorSquare = ( (y + x) % 2 );
					texData[z][(y * textureDim) + x] = colorSquare ? 0xFFE542FA : 0xFFFFFFFF;
				}
			}
		}

		for( uint32_t i = 0; i < METAL_NUM_DUMMY_TEXTURES; ++i )
		{
			uint32_t samples = dummyTextureProperties[i].samples;
			
			m_dummyTexture[i] = CreateMetalTexture(
					dummyTextureProperties[i].type,
					MTLPixelFormatRGBA8Unorm,
					dummyTextureProperties[i].width,
					dummyTextureProperties[i].height,
					dummyTextureProperties[i].depth,
					1,
					MTLStorageModePrivate,
					MTLTextureUsageShaderRead,
					samples
			);
			
			// Don't upload data to multisample texture
			if( samples > 1 )
			{
				continue;
			}

			// Calculate the number of bytes per row in the image
			uint32_t bytesPerRow   = sizeof(uint32_t) * textureDim;
			uint32_t bytesPerImage = bytesPerRow * textureDim;

			m_primaryWorkQueue.UploadTexture( m_dummyTexture[i], texData, 0, 0, bytesPerRow, bytesPerImage, 0, false );
		}

		MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
		m_dummySampler = [m_device newSamplerStateWithDescriptor:samplerDescriptor];

#if !__has_feature(objc_arc)
		[samplerDescriptor release];
#endif
	}

	void MetalContext::GenerateDummyBuffer()
	{
		float bufferData[1024] = {};

		m_dummyBuffer = CreateMetalBuffer( &m_primaryWorkQueue, sizeof( bufferData ), MTLResourceStorageModePrivate, bufferData );
	}

	id<MTLTexture> MetalContext::GetDummyTexture( MTLTextureType textureType )
	{
		for( uint32_t i = 0; i < METAL_NUM_DUMMY_TEXTURES; ++i )
		{
			if ( m_dummyTexture[i].textureType == textureType )
			{
				return m_dummyTexture[i];
			}
		}
        
        CCP_AL_LOGERR( "Dummy texture type not available. Accessing this texture can cause crashes on certain GPUs." );
        CCP_ASSERT( false );
        
		return nil;
	}

	id<MTLSamplerState> MetalContext::GetDummySampler()
	{
		return m_dummySampler;
	}

	id<MTLBuffer> MetalContext::GetDummyBuffer( NSUInteger *outSize, MTLVertexFormat *outFormat )
	{
		if( outSize )
		{
			*outSize = m_dummyBuffer.allocatedSize;
		}

		if( outFormat )
		{
			*outFormat = MTLVertexFormatFloat4;
		}

		return m_dummyBuffer;
	}

	id<MTLTexture> MetalContext::CreateSRGBViewOfMetalTexture( id<MTLTexture> texture )
	{
		switch( texture.pixelFormat )
		{
		case MTLPixelFormatRGBA8Unorm:
		case MTLPixelFormatBGRA8Unorm:
		case MTLPixelFormatBC1_RGBA:
		case MTLPixelFormatBC2_RGBA:
		case MTLPixelFormatBC3_RGBA:
		case MTLPixelFormatBC7_RGBAUnorm:
			// Metal is nice enough to make the SRGB version always one greater than the base format
			return [texture newTextureViewWithPixelFormat:MTLPixelFormat((uint32_t)texture.pixelFormat + 1)];
		default:
			return texture;
		}
	}
	
	id<MTLTexture> MetalContext::CreateUAVOfMetalTexture( id<MTLTexture> texture, uint32_t mipLevel  )
	{
		switch( texture.textureType )
		{
		case MTLTextureTypeCube:
			return [texture newTextureViewWithPixelFormat:texture.pixelFormat textureType:MTLTextureType2DArray levels:NSMakeRange( mipLevel, 1 ) slices:NSMakeRange(0, 6)];
		case MTLTextureTypeCubeArray:
			return [texture newTextureViewWithPixelFormat:texture.pixelFormat textureType:MTLTextureType2DArray levels:NSMakeRange( mipLevel, 1 ) slices:NSMakeRange(0, 6 * texture.arrayLength)];
		default:
			if( mipLevel > 0 )
			{
				return [texture newTextureViewWithPixelFormat:texture.pixelFormat textureType:texture.textureType levels:NSMakeRange( mipLevel, 1 ) slices:NSMakeRange(0, texture.arrayLength)];
			}
			return texture;
		}

	}

	void MetalContext::DestroyMetalTexture(id<MTLTexture> texture)
	{
	#if !__has_feature(objc_arc)
		[texture release];
	#endif
	}

    uint32_t MetalContext::AllocateHeapIndex( id<MTLTexture> texture )
    {
        if( !texture || !m_resourceHeap )
        {
            return 0xffffffff;
        }
        if( @available( macOS 13.0, * ) )
        {
            return m_resourceHeap.Allocate( texture.gpuResourceID._impl );
        }
        else
        {
            return 0xffffffff;
        }
    }

    uint32_t MetalContext::AllocateHeapIndex( id<MTLBuffer> buffer )
    {
        if( !buffer || !m_resourceHeap )
        {
            return 0xffffffff;
        }
        if( @available( macOS 13.0, * ) )
        {
            return m_resourceHeap.Allocate( buffer.gpuAddress );
        }
        else
        {
            return 0xffffffff;
        }
    }

    uint32_t MetalContext::AllocateHeapIndex( id<MTLSamplerState> sampler )
    {
        if( !sampler || !m_resourceHeap )
        {
            return 0xffffffff;
        }
        if( @available( macOS 13.0, * ) )
        {
            return m_resourceHeap.Allocate( sampler.gpuResourceID._impl );
        }
        else
        {
            return 0xffffffff;
        }
    }

    void MetalContext::DeallocateHeapIndex( uint32_t index )
    {
        m_resourceHeap.Deallocate( index );
    }

    id<MTLBuffer> MetalContext::GetHeapViewBuffer() const
    {
        return m_resourceHeap.GetBuffer();
    }

	bool MetalContext::IsResourceInUse( uint64_t resourceLastAccessedFrame ) const
	{
		return GetRenderedFrameNumber() < resourceLastAccessedFrame;
	}

	uint64_t MetalContext::GetRecordingFrameNumber() const
	{
		return m_recordingFrameNumber;
	}

	uint64_t MetalContext::GetRenderedFrameNumber() const
	{
		return m_renderedFrameNumber;
	}

	void MetalContext::BlitToDrawableAndPresent( id<MTLTexture> srcTexture, NSView* view )
	{
		if( m_primaryWorkQueue.BlitToDrawableAndPresent( srcTexture, view, &m_renderedFrameNumber ) )
		{
            CCP_STATS_SET( constantDataSize, GetConstantBufferAllocator().GetTotalUploadedSize() );
			++m_recordingFrameNumber;
            for( auto& alloc : m_cbAllocator )
            {
                alloc.Reset();
            }
			for( auto buffer : m_destroyedConstantBuffers )
			{
				CCPAlignedFree( buffer );
			}
			m_destroyedConstantBuffers.clear();
            if( m_resourceHeap )
            {
                m_resourceHeap.SetFrameIndices( m_recordingFrameNumber, m_renderedFrameNumber );
            }
            
            FlushPendingRelease( m_renderedFrameNumber );
		}
		if( !m_gpuTimerRateMeasured )
		{
			if( @available( macOS 10.15, * ) )
			{
				DeviceTimestamp endCpuTime, endGpuTime;
				[m_device sampleTimestamps:&endCpuTime gpuTimestamp:&endGpuTime];
				
				m_gpuTimerRate = double( endCpuTime - m_beginCpuTime ) / double( endGpuTime - m_beginGpuTime );
			}
			m_gpuTimerRateMeasured = true;
		}
	}
	
	double MetalContext::GetGpuTimerRate() const
	{
		return m_gpuTimerRate;
	}

	id<MTLDevice> MetalContext::GetDevice()
	{
		return m_device;
	}

    id<MTLCommandQueue> MetalContext::GetCommandQueue()
    {
        return m_commandQueue;
    }

	id<MTLRenderPipelineState> MetalContext::GetCachedRenderPipelineState( size_t renderPipelineDescriptorHash )
	 {
		 id<MTLRenderPipelineState> pipelineState = nil;

		 auto pipelineStateIt = m_renderPipelineStateMap.find(renderPipelineDescriptorHash);
		 if (pipelineStateIt != m_renderPipelineStateMap.end())
		 {
			 pipelineState =  pipelineStateIt->second;
		 }
		 else if( IsInParallelEncoding() )
		 {
			 m_pipelineCacheMutex.lock();
			 pipelineStateIt = m_newRenderPipelineStateMap.find(renderPipelineDescriptorHash);
			 if (pipelineStateIt != m_newRenderPipelineStateMap.end())
			 {
				 pipelineState =  pipelineStateIt->second;
			 }
			 m_pipelineCacheMutex.unlock();
		 }
		 return pipelineState;
	 }

	 void MetalContext::AddRenderPipelineStateToCache( size_t renderPipelineDescriptorHash, id<MTLRenderPipelineState> pipelineState )
	 {
		 if( IsInParallelEncoding() )
		 {
			 m_pipelineCacheMutex.lock();
			 m_newRenderPipelineStateMap.emplace( renderPipelineDescriptorHash, pipelineState );
			 m_pipelineCacheMutex.unlock();
		 }
		 else
		 {
			 m_renderPipelineStateMap.emplace( renderPipelineDescriptorHash, pipelineState );
		 }
	 }

	 id<MTLComputePipelineState> MetalContext::GetCachedComputePipelineState( size_t computePipelineDescriptorHash )
	 {
		 id<MTLComputePipelineState> pipelineState = nil;

		 auto pipelineStateIt = m_computePipelineStateMap.find( computePipelineDescriptorHash );
		 if( pipelineStateIt != m_computePipelineStateMap.end() )
		 {
			 pipelineState =  pipelineStateIt->second;
		 }
		 return pipelineState;
	 }

	 void MetalContext::AddComputePipelineStateToCache( size_t computePipelineDescriptorHash, id<MTLComputePipelineState> pipelineState )
	 {
		 m_computePipelineStateMap.emplace( computePipelineDescriptorHash, pipelineState );
	 }

	uint32_t MetalContext::BeginParallelEncoding( uint32_t requestedEncodersCount )
	{
		if( m_secondaryWorkQueues.empty() )
		{
			return 0;
		}
		m_primaryWorkQueue.ReleaseEncoder( true );
		
		m_parallelEncodersCount = std::min( uint32_t( m_secondaryWorkQueues.size() ), requestedEncodersCount );
		for( uint32_t i = 0; i < m_parallelEncodersCount; ++i )
		{
			m_secondaryWorkQueues[i].BeginParallelEncoding( &m_primaryWorkQueue );
		}
		
		return m_parallelEncodersCount;
	}

	void MetalContext::EndParallelEncoding()
	{ 
		for( uint32_t i=0; i < m_parallelEncodersCount; ++i )
		{
			m_secondaryWorkQueues[i].EndParallelEncoding();
		}
		
		m_primaryWorkQueue.EndParallelEncoding();
		
		m_renderPipelineStateMap.merge( m_newRenderPipelineStateMap );
		m_newRenderPipelineStateMap.clear();
		
		m_parallelEncodersCount = 0;
	}
	
	bool MetalContext::IsInParallelEncoding() const
	{
		return m_parallelEncodersCount > 0;
	}

	MetalWorkQueue* MetalContext::GetSecondaryWorkQueue( uint32_t index )
	{
		if( index >= m_secondaryWorkQueues.size() )
		{
			return nullptr;
		}
		return &m_secondaryWorkQueues[index];
	}
	
	MetalWorkQueue* MetalContext::GetPrimaryWorkQueue()
	{
		return &m_primaryWorkQueue;
	}

ConstantBufferAllocator& MetalContext::GetConstantBufferAllocator()
{
    return m_cbAllocator[GetRecordingFrameNumber() % 3];
}

void MetalContext::ReleaseLater( id<NSObject> obj )
{
    m_pendingRelease.push_back( { obj, GetRecordingFrameNumber() } );
}

void MetalContext::FlushPendingRelease( uint64_t renderedFrame )
{
    m_pendingRelease.erase( std::remove_if( begin( m_pendingRelease ), end( m_pendingRelease ), [&]( auto& item ) { return item.frame <= renderedFrame; } ), end( m_pendingRelease ) );
}

} // namespace TrinityALImpl
#endif
