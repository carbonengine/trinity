#pragma once
#ifndef Tr2DepthStencilALGLES2_h_
#define Tr2DepthStencilALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../Tr2AutoResetObjectAL.h"
#include "Tr2TextureALGLES2.h"

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
		Tr2RenderContextAL& renderContext );

	ALResult CreateEx(	
		uint32_t width, 
		uint32_t height, 
		Tr2RenderContextEnum::DepthStencilFormat format, 
		uint32_t msaaType, 
		uint32_t msaaQuality, 
		uint32_t flags, 
		Tr2RenderContextAL& renderContext )
	{ 
		return Create( width, height, format, msaaType, msaaQuality, renderContext ); 
	}

	bool IsValid() const;
	void Destroy();

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	uint32_t GetMsaaType()	const { return m_msaaType; }
	uint32_t GetMsaaQuality()const { return m_msaaQuality; }
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const { return m_format; }

	bool IsReadable() const;
	Tr2TextureAL& GetTexture();
	const Tr2TextureAL& GetTexture() const;

	uint32_t GetSharedHandle() const { return 0; }
	bool operator==( const Tr2DepthStencilAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

private:
	uint32_t m_depthRenderBuffer;
	uint32_t m_stencilRenderBuffer;
	bool m_separateStencilBuffer;
	Tr2TextureAL m_readableDepth;

	uint32_t m_width;
	uint32_t m_height;
	Tr2RenderContextEnum::DepthStencilFormat m_format;
	uint32_t m_msaaType;
	uint32_t m_msaaQuality;

	struct TDeviceLost
	{
		uint32_t m_width;
		uint32_t m_height;
		Tr2RenderContextEnum::DepthStencilFormat m_format;
		uint32_t m_msaaType;
		uint32_t m_msaaQuality;

		bool m_valid;
	};
	TDeviceLost	m_deviceLost;

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	Tr2DepthStencilAL( const Tr2DepthStencilAL& ) /* = delete */;
	Tr2DepthStencilAL& operator=( const Tr2DepthStencilAL& ) /* = delete */;

	friend class Tr2RenderContextAL;
	friend class Tr2DepthStencil;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2DepthStencilALGLES2_h_
