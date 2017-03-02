#ifndef Tr2RenderTargetALDx11_h_
#define Tr2RenderTargetALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2AutoResetObjectAL.h"
#include "Tr2TextureALDx11.h"

// -------------------------------------------------------------
// Description:
//	Class representing the concept of a renderTarget, regardless of what type of GPU object and how
//  many of them are needed to implement the ability to render, to do MSAA, and/or to texture back
//  from the renderTarget.
// -------------------------------------------------------------
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
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult Create(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult CreateEx(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality,
		Tr2RenderContextEnum::BufferUsage usage,
		uint32_t flags,
		Tr2PrimaryRenderContextAL &renderContext );	

	ALResult Attach( ID3D11Texture2D* texture, Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const { return m_RTV != nullptr; }
	bool IsReadable() const { return GetTexture().IsValid() && m_msaaType <= 1 && m_msaaQuality == 0; }
	void Destroy();
	ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext );
	ALResult Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( 
		uint32_t destX, 
		uint32_t destY, 
		Tr2RenderTargetAL& source, 
		uint32_t* ltrb, 
		Tr2RenderContextAL& renderContext );

	uint32_t GetMsaaType()	const { return m_msaaType; }
	uint32_t GetMsaaQuality() const { return m_msaaQuality; }
	
	// This is not necessarily valid -- for example MSAA RTs cannot be directly textured from.
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL&	GetTexture() const;

	ALResult GetLockedRenderTarget( uint32_t mipLevel, uint32_t* ltrb, Tr2LockedRenderTargetAL& lockedRT, Tr2RenderContextAL& renderContext );

	ALResult Lock(	
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );
	void SetHintLockOften();

	ALResult CreateUAV( Tr2PrimaryRenderContextAL &renderContext );

	uint32_t GetSharedHandle() const;
	bool operator==( const Tr2RenderTargetAL& other ) const { return m_texture == other.m_texture; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	uint32_t m_msaaType;
	uint32_t m_msaaQuality;

	Tr2TextureAL m_backingStore;
	
	CComPtr<ID3D11Texture2D> m_texture;
	CComPtr<ID3D11RenderTargetView> m_RTV;
	CComPtr<ID3D11RenderTargetView> m_RTVsRgb;

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );
	
	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class Tr2RenderTarget;

	// For auto-recreate after a device lost
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
	bool		m_isAttached;

	Tr2RenderTargetAL( const Tr2RenderTargetAL& );
	Tr2RenderTargetAL& operator=( const Tr2RenderTargetAL& );
};

#endif

#endif	// !Tr2RenderTargetALDx11_h_