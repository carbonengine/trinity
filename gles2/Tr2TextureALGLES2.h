#pragma once
#ifndef Tr2TextureALGLES2_h_
#define Tr2TextureALGLES2_h_

struct Tr2SubresourceData;

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a GPU
// texture.
// SeeAlso:
//   Tr2SubresourceData
// -------------------------------------------------------------
class Tr2TextureAL :
public Tr2BitmapDimensions,
public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TEXTURE>
{
public:
	Tr2TextureAL();

	~Tr2TextureAL();

	Tr2TextureAL& operator=( Tr2TextureAL&& );
	Tr2TextureAL& operator=( const Tr2TextureAL& other );

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
		Tr2PrimaryRenderContextAL &renderContext )
	{
		return E_FAIL;
	}

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
		return m_isAlias;
	}
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage()		const
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
		return m_texture == other.m_texture;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	ALResult CreateUAV( Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_FAIL;
	}

private:
	Tr2RenderContextEnum::BufferUsage m_usage;
	// Texture is owned by other AL object (render target, depth stencil)
	bool m_isAlias;

private:
	Tr2TextureAL( const Tr2TextureAL& other ) /* = delete */;

	ALResult CreateDepthTexture( uint32_t width,
								 uint32_t height,
								 Tr2RenderContextAL& renderContext );
	ALResult LockWriting( uint32_t face,
						  uint32_t mipLevel,
						  uint32_t* ltrb,
						  void*& data,
						  uint32_t& pitch,
						  Tr2RenderContextAL& renderContext );
	ALResult UnlockWriting( Tr2RenderContextAL& renderContext );

	Tr2RenderContextEnum::LockType m_currentLock;
	std::vector<char> m_lockedData;
	uint32_t m_lockedLevel;
	uint32_t m_lockedRect[4];
	uint32_t m_lockedFace;

	GLenum m_internalFormat;
	GLenum m_targetFormat;
	GLenum m_targetType;

	Tr2SamplerStateAL::StateData m_currentSampler;

public:	//DEBUG
#ifdef __ANDROID__
    GLuint* m_texture;
#else
	std::shared_ptr<GLuint> m_texture;
#endif

	friend class Tr2RenderContextAL;
	friend class Tr2RenderTargetAL;
	friend class Tr2DepthStencilAL;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2TextureALGLES2_h_
