#pragma once
#ifndef Tr2RenderTargetALStub_h_
#define Tr2RenderTargetALStub_h_

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2HalHelperStructures.h"
#include "../include/Tr2TextureAL.h"


#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2RenderTargetAL : 
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
	const Tr2TextureAL& GetTexture() const;

	uintptr_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;
private:
	Tr2MsaaDesc m_msaa;
	Tr2TextureAL m_backingStore;

	friend class Tr2RenderContextAL;
};

#endif

#endif	// !Tr2RenderTargetALStub_h_
