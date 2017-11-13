#pragma once
#ifndef Tr2DepthStencilALStub_h_
#define Tr2DepthStencilALStub_h_

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2HalHelperStructures.h"
#include "../include/Tr2TextureAL.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )


class Tr2DepthStencilAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_DEPTH_STENCIL>
{
public:
	Tr2DepthStencilAL();
	~Tr2DepthStencilAL();

	ALResult Create(	
		uint32_t width, 
		uint32_t height, 
		Tr2RenderContextEnum::DepthStencilFormat format, 
		const Tr2MsaaDesc& msaa,
		Tr2RenderContextEnum::ExFlag flags,
		Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	void Destroy();

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	const Tr2MsaaDesc& GetMsaaDesc() const;
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const;

	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uint32_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	Tr2TextureAL m_backingStore;
	uint32_t m_width;
	uint32_t m_height;
	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::DepthStencilFormat m_format;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_STUB )

#endif //Tr2DepthStencilALStub_h_
