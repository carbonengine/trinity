#pragma once
#ifndef Tr2TextureALDx11_h_
#define Tr2TextureALDx11_h_

struct Tr2SubresourceData;

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#ifdef TRINITY_AL_GUARD_LOCKS
#include "Tr2LockGuard.h"
#endif

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a GPU
// texture.
//   Prefer to use TriTextureRes, Tr2ImageHandler etc. over working
//  with this class directly.
//  32bit - no support for textures whose dimensions or locked pitch
//  exceed 32 bits
// SeeAlso:
//   Tr2SubresourceData
// -------------------------------------------------------------
class Tr2TextureAL: 
	public Tr2BitmapDimensions,
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TEXTURE>
{
public:
    Tr2TextureAL();
	~Tr2TextureAL();

	Tr2TextureAL& operator=( Tr2TextureAL&& );
	Tr2TextureAL& operator=( Tr2TextureAL& ) throw();

	ALResult Create2D(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult Create2DArray(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		uint32_t arrayCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult CreateCube(
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult CreateVolume(
		uint32_t width, 
		uint32_t height, 
		uint32_t depth, 
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	bool IsValid() const 
	{ 
		return m_texture != nullptr && m_view[0] != nullptr && m_view[1] != nullptr; 
	}
	bool IsAlias() const 
	{ 
		return m_isAlias; 
	}
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage() const 
	{ 
		return m_usage; 
	}
	
	ALResult UpdateSubresource(	
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom, 
		const void* source, 
		uint32_t sourcePitch, 
		Tr2RenderContextAL& renderContext );

	ALResult CopySubresourceRegion( 
		const Tr2TextureSubresource& destSubresource,
		Tr2TextureAL& source, 
		const Tr2TextureSubresource& sourceSubresource,
		Tr2RenderContextAL& renderContext );

	ALResult CopySubresourceRegion( 
		const Tr2TextureSubresource& destSubresource,
		Tr2RenderTargetAL& source, 
		const Tr2TextureSubresource& sourceSubresource,
		Tr2RenderContextAL& renderContext );

	ALResult Lock(	
		uint32_t mipLevel, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL& renderContext );
	ALResult Lock(	
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL& renderContext );
	ALResult Lock(	
		uint32_t face, 
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );

	bool operator==( const Tr2TextureAL& other ) const { return m_texture == other.m_texture; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	Tr2RenderContextEnum::BufferUsage	m_usage;
	// Texture is owned by other AL object (render target, depth stencil)
	bool								m_isAlias;
	
	CComPtr<ID3D11Resource>				m_texture;
	CComPtr<ID3D11ShaderResourceView>	m_view[2];
	CComPtr<ID3D11UnorderedAccessView>	m_uav;

private:
	friend class Tr2RenderContextAL;

	// hokey pokey
	friend class TriStepRunComputeShader;

	ALResult Create2DImpl(	
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevelCount,
		uint32_t arraySize,
		uint32_t miscFlags,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2SubresourceData* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult LockReading(	
		uint32_t face, 
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextAL & renderContext );
	ALResult UnlockReading( Tr2RenderContextAL & renderContext );
	ALResult LockWriting(	
		uint32_t face, 
		uint32_t mipLevel, 
		uint32_t* ltrb, 
		void*& data, 
		uint32_t& pitch, 
		Tr2RenderContextAL & renderContext );
	ALResult UnlockWriting( Tr2RenderContextAL & renderContext );

	Tr2RenderContextEnum::LockType	m_currentLock;
	uint32_t						m_currentLockMipLevel;
	CComPtr<ID3D11Texture2D>		m_staging;

	CcpMallocBuffer					m_writeStaging;
	uint32_t						m_writeLtrb[4];
	
	friend class Tr2RenderTargetAL;
	friend class Tr2DepthStencilAL;

	static ALResult CopySubresourceRegion( 
		CComPtr<ID3D11Texture2D> dest, 
		uint32_t destX, 
		uint32_t destY, 
		CComPtr<ID3D11Texture2D> source, 
		uint32_t* ltrb, 
		Tr2RenderContextAL& renderContext );

#if TRINITY_AL_GUARD_LOCKS
	Tr2LockGuard m_lockGuard;
#endif
private:
	Tr2TextureAL( const Tr2TextureAL& );
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2TextureALDx11_h_
