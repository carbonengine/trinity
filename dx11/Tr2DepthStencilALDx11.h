#pragma once
#ifndef Tr2DepthStencilALDx11_h_
#define Tr2DepthStencilALDx11_h_


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2HalHelperStructures.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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
		Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const;
	void Destroy();

	bool operator==( const Tr2DepthStencilAL& other ) const;

	uint32_t GetWidth()  const;
	uint32_t GetHeight() const;
	const Tr2MsaaDesc& GetMsaaDesc() const;
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const;

	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uintptr_t GetSharedHandle() const;

	Tr2ALMemoryType GetMemoryClass() const;

	static DXGI_FORMAT ConvertDepthStencilFormatToDxgi( Tr2RenderContextEnum::DepthStencilFormat format );

private:

	CComPtr<ID3D11Texture2D>		m_depthStencil;
	CComPtr<ID3D11DepthStencilView>	m_depthStencilView;
	CComPtr<ID3D11DepthStencilView>	m_depthStencilViewReadOnly;

	uint32_t	m_width;
	uint32_t	m_height;
	Tr2RenderContextEnum::DepthStencilFormat m_format;
	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::ExFlag m_exFlag;
	Tr2MemoryCounterAL m_memory;

	Tr2TextureAL	m_backingStore;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2DepthStencilALDx11_h_
