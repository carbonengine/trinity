#pragma once
#ifndef Tr2DepthStencilALStub_h_
#define Tr2DepthStencilALStub_h_

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "../Tr2AutoResetObjectAL.h"
#include "Tr2TextureALStub.h"


class Tr2DepthStencilAL : 
	public Tr2AutoResetObjectAL, 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_DEPTH_STENCIL>
{
public:
	Tr2DepthStencilAL();
	~Tr2DepthStencilAL();

	ALResult Create(	
		uint32_t width, 
		uint32_t height, 
		Tr2RenderContextEnum::DepthStencilFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality, 
		Tr2RenderContextAL& renderContext );

	ALResult CreateEx(	
		uint32_t width, 
		uint32_t height, 
		Tr2RenderContextEnum::DepthStencilFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality, 
		uint32_t flags, 
		Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	void Destroy();

	uint32_t GetWidth() const { return m_backingStore.GetWidth(); }
	uint32_t GetHeight() const { return m_backingStore.GetHeight(); }
	uint32_t GetMsaaType() const { return m_msaaType; }
	uint32_t GetMsaaQuality()const { return m_msaaQuality; }
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const { return m_format; }

	bool IsReadable() const;
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uint32_t GetSharedHandle() const;
	

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

private:
	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );
	Tr2TextureAL m_backingStore;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_msaaType;
	uint32_t m_msaaQuality;
	Tr2RenderContextEnum::DepthStencilFormat m_format;

	struct TDeviceLost
	{
		uint32_t m_width;
		uint32_t m_height;
		Tr2RenderContextEnum::DepthStencilFormat m_format;
		uint32_t m_msaaType;
		uint32_t m_msaaQuality;

		bool		m_valid;
	};
	TDeviceLost	m_deviceLost;

	friend class Tr2DepthStencil;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_STUB )

#endif //Tr2DepthStencilALStub_h_
