#pragma once
#ifndef Tr2RenderTarget_h_
#define Tr2RenderTarget_h_

#include "ITr2TextureProvider.h"

BLUE_DECLARE( Tr2RenderTarget );

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a renderTarget.
//   This class replaces TriSurface and TriTextureRes.
// -------------------------------------------------------------
BLUE_CLASS( Tr2RenderTarget ) : public ITr2TextureProvider
{
public:
	EXPOSE_TO_BLUE();

    Tr2RenderTarget( IRoot* = 0 );
	~Tr2RenderTarget();

	void py__init__( 
		unsigned width, 
		unsigned height, 
		unsigned mipCount, 
		Tr2RenderContextEnum::PixelFormat format,
		unsigned msaaType, 
		unsigned msaaQuality,
		Tr2RenderContextEnum::ExFlag flags );

	long Create( 
		unsigned width, 
		unsigned height, 
		unsigned mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		unsigned msaaType = 1, 
		unsigned msaaQuality = 0,
		Tr2RenderContextEnum::ExFlag flags = Tr2RenderContextEnum::EX_NONE );

	virtual Tr2TextureAL* GetTexture();

	void Attach( Tr2RenderTargetAL* renderTarget, IRoot* owner );
	bool IsAttached() const;

	bool IsValid() const;
	void Destroy();
	bool IsReadable() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetMipCount() const;
	uint32_t GetMsaaType() const;
	uint32_t GetMsaaQuality() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;

	long GenerateMipMaps();
	long Resolve( Tr2RenderTarget* destination );
	
	Tr2RenderTargetAL& GetRenderTarget();
	const Tr2RenderTargetAL& GetRenderTarget() const;

	operator Tr2RenderTargetAL&() { return GetRenderTarget(); }
	operator const Tr2RenderTargetAL&() const { return GetRenderTarget(); }

	uintptr_t GetSharedHandle() const;
	
	std::string m_name;
private:
	Tr2RenderTargetAL m_renderTarget;
	Tr2RenderTargetAL* m_attachedRenderTarget;
	BlueWeakRef<IRoot> m_attachedOwner;

	bool HasALObject( int type, size_t object );
};

TYPEDEF_BLUECLASS( Tr2RenderTarget );

#endif //Tr2RenderTarget_h_
