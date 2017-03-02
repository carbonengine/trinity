#pragma once
#ifndef Tr2DepthStencilALDx11_h_
#define Tr2DepthStencilALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2AutoResetObjectAL.h"

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a classic depth-and-stencil-buffer setup.
//   This class replaces TriSurface for those use-cases where the
//   surface was just a depth stencil surface.
// -------------------------------------------------------------
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
		Tr2PrimaryRenderContextAL& renderContext );

	ALResult CreateEx(	
		uint32_t width, 
		uint32_t height, 
		Tr2RenderContextEnum::DepthStencilFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality, 
		uint32_t flags, 
		Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const { return m_depthStencil != nullptr && m_depthStencilView != nullptr; }
	void Destroy();

	CComPtr<ID3D11Texture2D>		m_depthStencil;
	CComPtr<ID3D11DepthStencilView>	m_depthStencilView;
	CComPtr<ID3D11DepthStencilView>	m_depthStencilViewReadOnly;

	uint32_t	GetWidth()  const		{ return m_width; }
	uint32_t	GetHeight() const		{ return m_height; }
	uint32_t	GetMsaaType()	const	{ return m_msaaType; }
	uint32_t	GetMsaaQuality()const	{ return m_msaaQuality; }
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const { return m_format; }

	bool IsReadable() const;
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uint32_t	GetSharedHandle() const;
	bool operator==( const Tr2DepthStencilAL& other ) const { return m_depthStencil == other.m_depthStencil; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	static DXGI_FORMAT	ConvertDepthStencilFormatToDxgi( 
					Tr2RenderContextEnum::DepthStencilFormat format );

private:
	uint32_t	m_width;
	uint32_t	m_height;
	Tr2RenderContextEnum::DepthStencilFormat	m_format;
	uint32_t	m_msaaType;
	uint32_t	m_msaaQuality;

	Tr2TextureAL	m_backingStore;

	struct TDeviceLost
	{
		uint32_t	m_width;
		uint32_t	m_height;
		Tr2RenderContextEnum::DepthStencilFormat	m_format;
		uint32_t	m_msaaType;
		uint32_t	m_msaaQuality;

		bool		m_valid;
	};
	TDeviceLost	m_deviceLost;

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	friend class Tr2DepthStencil;	// Blue

	Tr2DepthStencilAL( const Tr2DepthStencilAL& ) /* = delete */;
	Tr2DepthStencilAL& operator=( const Tr2DepthStencilAL& ) /* = delete */;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2DepthStencilALDx11_h_
