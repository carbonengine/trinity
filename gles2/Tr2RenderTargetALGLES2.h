
#ifndef Tr2RenderTargetALGLES2_h_
#define Tr2RenderTargetALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../Tr2AutoResetObjectAL.h"
#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2HalHelperStructures.h"
#include "../include/Tr2TextureAL.h"


class Tr2RenderContextAL;


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
		Tr2RenderContextAL& renderContext );

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

	// The lock is always read only
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
	const Tr2TextureAL& GetTexture() const;

	uintptr_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	ALResult Bind( uint32_t slot, Tr2RenderContextAL& renderContext ) const;
	ALResult GetRenderTargetData( uint32_t* ltrb, CcpMallocBuffer& buffer, uint32_t& pitch, Tr2RenderContextAL& renderContext );

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	Tr2MsaaDesc m_msaa;

	Tr2TextureAL m_backingStore;
	GLuint m_msaaTarget;
	CcpMallocBuffer m_lockedData;

	struct TDeviceLost
	{
		Tr2RenderContextEnum::PixelFormat m_format;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_mipCount;
		Tr2MsaaDesc m_msaa;

		bool m_valid;
	};
	TDeviceLost	m_deviceLost;

	bool m_lockedOften;
	bool m_isLocked;

	friend class Tr2RenderContextAL;
	friend class Tr2SwapChainAL;
	friend class Tr2TextureAL;
};

#endif

#endif	// !Tr2RenderTargetALGLES2_h_
