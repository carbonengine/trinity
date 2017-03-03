
#ifndef Tr2RenderTargetALGLES2_h_
#define Tr2RenderTargetALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "../Tr2AutoResetObjectAL.h"
#include "Tr2TextureALGL4.h"

class Tr2RenderTargetAL : 
	public Tr2AutoResetObjectAL, 
	public Tr2BitmapDimensions,
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RENDER_TARGET>
{
public:
	Tr2RenderTargetAL();
	~Tr2RenderTargetAL();

	ALResult Create(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextAL& renderContext );

	ALResult Create(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality,
		Tr2RenderContextAL& renderContext );

	ALResult CreateEx(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality,
		Tr2RenderContextEnum::BufferUsage usage,
		uint32_t flags,
		Tr2RenderContextAL& renderContext )
	{ 
		return Create( width, height, mipLevelCount, format, msaaType, msaaQuality, renderContext ); 
	}

	bool IsValid() const;
	bool IsReadable() const { return GetTexture().IsValid(); }
	void Destroy();
	ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext );
	ALResult Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( 
		uint32_t destX, 
		uint32_t destY, 
		Tr2RenderTargetAL& source, 
		uint32_t* ltrb, 
		Tr2RenderContextAL& renderContext );

	uint32_t GetMsaaType() const { return m_msaaType; }
	uint32_t GetMsaaQuality() const { return m_msaaQuality; }
	
	// This is not necessarily valid -- for example MSAA RTs cannot be directly textured from.
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	ALResult GetLockedRenderTarget( uint32_t mipLevel, uint32_t* ltrb, Tr2LockedRenderTargetAL& lockedRT, Tr2RenderContextAL& renderContext );

	// The lock is always read only
	ALResult Lock(	
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );
	void SetHintLockOften();

	ALResult CreateUAV( Tr2RenderContextAL& renderContext )	{ return E_FAIL; }

	uint32_t GetSharedHandle() const { return 0; }
	bool operator==( const Tr2RenderTargetAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

private:
	uint32_t							m_msaaType;
	uint32_t							m_msaaQuality;

	Tr2TextureAL						m_backingStore;
	GLuint m_msaaTarget;
	CcpMallocBuffer						m_lockedData;
	bool								m_lockedOften;
	bool								m_isLocked;

	struct TDeviceLost
	{
		Tr2RenderContextEnum::PixelFormat	m_format;
		uint32_t							m_width;
		uint32_t							m_height;
		uint32_t							m_mipCount;
		uint32_t							m_msaaType;
		uint32_t							m_msaaQuality;

		bool								m_valid;
	};
	TDeviceLost	m_deviceLost;

private:	
	ALResult Bind( uint32_t slot, Tr2RenderContextAL& renderContext ) const;
	ALResult GetRenderTargetData( uint32_t* ltrb, CcpMallocBuffer& buffer, uint32_t& pitch, Tr2RenderContextAL& renderContext );

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	friend class Tr2RenderContextAL;
	friend class Tr2RenderTarget;
	friend class Tr2TextureAL;

	Tr2RenderTargetAL( const Tr2RenderTargetAL& );
	Tr2RenderTargetAL& operator=( const Tr2RenderTargetAL& );
};

#endif

#endif	// !Tr2RenderTargetALGLES2_h_
