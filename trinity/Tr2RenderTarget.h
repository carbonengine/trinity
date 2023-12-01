#pragma once
#ifndef Tr2RenderTarget_h_
#define Tr2RenderTarget_h_

#include "ITr2TextureProvider.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2RenderTarget );

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a renderTarget.
//   This class replaces TriSurface and TriTextureRes.
// -------------------------------------------------------------
BLUE_CLASS( Tr2RenderTarget ) : 
	public ITr2TextureProvider,
	public Tr2DeviceResource
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
		Tr2RenderContextEnum::ExFlag flags,
		Tr2RenderContextEnum::TextureType type );

	long Create( 
		unsigned width, 
		unsigned height, 
		unsigned mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		unsigned msaaType = 1, 
		unsigned msaaQuality = 0,
		Tr2RenderContextEnum::ExFlag flags = Tr2RenderContextEnum::EX_NONE,
		Tr2RenderContextEnum::TextureType type = Tr2RenderContextEnum::TEX_TYPE_2D );

	long CreateManual( 
		unsigned width, 
		unsigned height, 
		unsigned mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		unsigned msaaType,
		unsigned msaaQuality,
		Tr2RenderContextEnum::ExFlag flags,
		Tr2RenderContextEnum::TextureType type,
		Tr2CpuUsage::Type cpuUsage,
		Tr2GpuUsage::Type gpuUsage );

	virtual Tr2TextureAL* GetTexture();

	void Attach( const Tr2TextureAL& renderTarget, IRoot* owner );
	void Detach();
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
	Tr2RenderContextEnum::TextureType GetType() const;

	long GenerateMipMaps();
	long Resolve( Tr2RenderTarget* destination );
	
	Tr2TextureAL& GetRenderTarget();
	const Tr2TextureAL& GetRenderTarget() const;

	operator Tr2TextureAL&() { return GetRenderTarget(); }
	operator const Tr2TextureAL&() const { return GetRenderTarget(); }

	uintptr_t GetSharedHandle() const;

	void SetName( const char* name );
	std::string GetName() const;
	
protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();
private:
	Tr2TextureAL m_renderTarget;
	Tr2TextureAL m_attachedRenderTarget;
	BlueWeakRef<IRoot> m_attachedOwner;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipCount;
	Tr2RenderContextEnum::PixelFormat m_format;
	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::ExFlag m_flags;
	Tr2RenderContextEnum::TextureType m_type;
	Tr2CpuUsage::Type m_cpuUsage;
	Tr2GpuUsage::Type m_gpuUsage;
	std::string m_name;

	bool HasALObject( int type, size_t object );
};

TYPEDEF_BLUECLASS( Tr2RenderTarget );

#endif //Tr2RenderTarget_h_
