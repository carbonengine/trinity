#ifndef Tr2RenderTargetALDx11_h_
#define Tr2RenderTargetALDx11_h_


#include "../Tr2AutoResetObjectAL.h"
#include "../Tr2HalHelperStructures.h"
#include "../include/Tr2TextureAL.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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
		const Tr2MsaaDesc& msaa,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::ExFlag flags,
		Tr2PrimaryRenderContextAL &renderContext );

	bool IsValid() const;
	void Destroy();

	bool operator==( const Tr2RenderTargetAL& other ) const;

	ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext );
	ALResult Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( 
		uint32_t destX, 
		uint32_t destY, 
		Tr2RenderTargetAL& source, 
		uint32_t* ltrb, 
		Tr2RenderContextAL& renderContext );

	ALResult Lock(	
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );
	void SetHintLockOften();

	Tr2MsaaDesc GetMsaaDesc() const;
	
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL&	GetTexture() const;

	uintptr_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	ALResult Attach( ID3D11Texture2D* texture, Tr2PrimaryRenderContextAL& renderContext );

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::BufferUsage m_usage;
	Tr2RenderContextEnum::ExFlag m_flags;

	Tr2TextureAL m_backingStore;
	
	CComPtr<ID3D11Texture2D> m_texture;
	CComPtr<ID3D11RenderTargetView> m_RTV;
	CComPtr<ID3D11RenderTargetView> m_RTVsRgb;

	// For auto-recreate after a device lost
	struct TDeviceLost
	{
		Tr2RenderContextEnum::PixelFormat	m_format;
		uint32_t							m_width;
		uint32_t							m_height;
		uint32_t							m_mipCount;
		Tr2MsaaDesc							m_msaa;
		Tr2RenderContextEnum::BufferUsage m_usage;
		Tr2RenderContextEnum::ExFlag m_flags;

		bool								m_valid;
	};
	TDeviceLost	m_deviceLost;
	bool		m_isAttached;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class Tr2SwapChainAL;
};

#endif

#endif	// !Tr2RenderTargetALDx11_h_