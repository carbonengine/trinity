#pragma once
#ifndef Tr2DepthStencilALDx9_h_
#define Tr2DepthStencilALDx9_h_


#include "../Tr2HalHelperStructures.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a classic depth-and-stencil-buffer setup.
//   This class replaces TriSurface for those use-cases where the
//   surface was just a depth stencil surface.
// -------------------------------------------------------------
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

	bool operator==( const Tr2DepthStencilAL& other ) const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	const Tr2MsaaDesc& GetMsaaDesc() const;
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const;

	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uintptr_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	CComPtr<IDirect3DSurface9>		m_depthStencil;

	// READABLE specific, special case for readable depth in DX9, which makes it more of a renderTarget.
	// It's either this, or messing up renderTarget to make it more of a depthStencil.
	CComPtr<IDirect3DTexture9>		m_depthStencilREADABLE;
	Tr2TextureAL					m_backingStore;

	ALResult CreateReadableDepth( Tr2RenderContextAL& renderContext );

	uint32_t m_width;
	uint32_t m_height;
	Tr2RenderContextEnum::DepthStencilFormat m_format;
	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::ExFlag m_exFlags;
	Tr2MemoryCounterAL m_memory;

	friend class Tr2RenderContextAL;

	HANDLE m_sharedHandle;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#endif //Tr2DepthStencilALDx9_h_
