#pragma once
#ifndef Tr2TextureALStub_h_
#define Tr2TextureALStub_h_

struct Tr2SubresourceData;

#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2RenderTargetAL;

class Tr2TextureAL :
	public Tr2BitmapDimensions,
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TEXTURE>
{
public:
	Tr2TextureAL();
	~Tr2TextureAL();

	Tr2TextureAL& operator=( Tr2TextureAL&& );
	Tr2TextureAL& operator=( Tr2TextureAL& );

	ALResult Create2D( uint32_t width,
					   uint32_t height,
					   uint32_t mipLevelCount,
					   Tr2RenderContextEnum::PixelFormat format,
					   Tr2RenderContextEnum::BufferUsage usage,
					   Tr2SubresourceData* initialData,
					   Tr2RenderContextAL& renderContext );

	ALResult Create2DArray(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		uint32_t arrayCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult CreateCube( uint32_t width,
						 uint32_t height,
						 uint32_t mipLevelCount,
						 Tr2RenderContextEnum::PixelFormat format,
						 Tr2RenderContextEnum::BufferUsage usage,
						 Tr2SubresourceData* initialData,
						 Tr2RenderContextAL& renderContext );

	ALResult CreateVolume( uint32_t width,
						   uint32_t height,
						   uint32_t depth,
						   uint32_t mipLevelCount,
						   Tr2RenderContextEnum::PixelFormat format,
						   Tr2RenderContextEnum::BufferUsage usage,
						   Tr2SubresourceData* initialData,
						   Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	bool IsAlias() const
	{
		return false;
	}
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage() const
	{
		return m_usage;
	}

	ALResult UpdateSubresource( uint32_t left,
								uint32_t top,
								uint32_t right,
								uint32_t bottom,
								const void* source,
								uint32_t sourcePitch,
								Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
									Tr2TextureAL& source,
									const Tr2TextureSubresource& sourceSubresource,
									Tr2RenderContextAL& renderContext );

	ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
									Tr2RenderTargetAL& source,
									const Tr2TextureSubresource& sourceSubresource,
									Tr2RenderContextAL& renderContext );

	ALResult Lock( uint32_t mipLevel,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Lock( uint32_t mipLevel,
				   uint32_t* ltrb,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Lock( uint32_t face,
				   uint32_t mipLevel,
				   uint32_t* ltrb,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );

	bool operator==( const Tr2TextureAL& other ) const
	{
		return m_data == other.m_data;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	size_t GetTextureSize() const;
	ALResult Create( 
		Tr2RenderContextEnum::TextureType type,
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mipLevelCount,
		uint32_t arrayCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2RenderContextAL& renderContext );

	std::shared_ptr<uint8_t> m_data;
	Tr2RenderContextEnum::BufferUsage m_usage;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_STUB )

#endif //Tr2TextureALStub_h_
