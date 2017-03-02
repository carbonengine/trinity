#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

bool g_preloadTextureToDeviceOnPrepare = true;

#pragma warning( disable: 4189 )	// Scopeguard
#include "Tr2TextureALDx11.h"
#include "ALLog.h"
using namespace Tr2RenderContextEnum;


Tr2TextureAL::Tr2TextureAL()	
	: m_isAlias( false )
{
	Destroy();
}

void Tr2TextureAL::Destroy()
{
	m_usage			= 0;
	
	Tr2BitmapDimensions::Destroy();
	//m_overrideSrgb  = false;

	m_texture		= nullptr;
	m_view[0]		= nullptr;
	m_view[1]		= nullptr;
	m_staging		= nullptr;
	m_arraySize = 0;
	m_uav = nullptr;

	//m_writeStaging.swap( TrackableStdVector<char>() );
	m_writeStaging.clear();

	m_currentLock	= LOCK_INVALID;
	m_currentLockMipLevel = 0;
}

Tr2TextureAL::~Tr2TextureAL()
{
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL&& other )
{
	CCP_ASSERT( m_currentLock == LOCK_INVALID );
	CCP_ASSERT( other.m_currentLock == LOCK_INVALID );
	if( m_currentLock != LOCK_INVALID || other.m_currentLock != LOCK_INVALID )
	{
		return *this;
	}

	if( this != &other )
	{
		m_texture.Attach( other.m_texture.Detach() );
		m_view[0].Attach( other.m_view[0].Detach() );
		m_view[1].Attach( other.m_view[1].Detach() );
		m_uav.Attach( other.m_uav.Detach() );
		
		m_format		= other.m_format;
		m_usage			= other.m_usage;
		m_type			= other.m_type;
		m_width			= other.m_width;
		m_height		= other.m_height;
		m_volumeDepth	= other.m_volumeDepth;
		m_mipCount		= other.m_mipCount;
		m_isAlias		= other.m_isAlias;
		m_arraySize = other.m_arraySize;
		ChangeObjectId();
	}
	return *this;
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL& other )
{
	CCP_ASSERT( m_currentLock == LOCK_INVALID );
	CCP_ASSERT( other.m_currentLock == LOCK_INVALID );
	if( m_currentLock != LOCK_INVALID || other.m_currentLock != LOCK_INVALID )
	{
		return *this;
	}

	if( this != &other )
	{
		m_format		= other.m_format;
		m_usage			= other.m_usage;
		m_type			= other.m_type;
		m_width			= other.m_width;
		m_height		= other.m_height;
		m_volumeDepth	= other.m_volumeDepth;
		m_mipCount		= other.m_mipCount;
		m_isAlias		= other.m_isAlias;
		m_arraySize = other.m_arraySize;

		m_texture		= other.m_texture;
		m_view[0]		= other.m_view[0];
		m_view[1]		= other.m_view[1];
		m_uav			= other.m_uav;
		ChangeObjectId();
	}

	return *this;
}

namespace {

	void ConvertUsage( BufferUsage usage, D3D11_USAGE& d3dUsage, uint32_t& CPUAccessFlags )
	{
		if( usage & USAGE_IMMUTABLE )
		{
			d3dUsage		= D3D11_USAGE_IMMUTABLE;
			CPUAccessFlags	= 0u;
		}
		else if( usage & USAGE_CPU_WRITE )
		{
			d3dUsage		= D3D11_USAGE_DYNAMIC;
			CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			d3dUsage		= D3D11_USAGE_DEFAULT;
			CPUAccessFlags	= 0u;
		}
	}

	std::vector<D3D11_SUBRESOURCE_DATA> ConvertInitialData( uint32_t numInit, Tr2SubresourceData* initialData )
	{
		std::vector<D3D11_SUBRESOURCE_DATA> init( numInit );
		if( initialData )
		{
			for( size_t i = 0; i != init.size(); ++i )
			{
				init[i].pSysMem			 = initialData[i].m_sysMem;
				init[i].SysMemPitch		 = initialData[i].m_sysMemPitch;
				init[i].SysMemSlicePitch = initialData[i].m_sysMemSlicePitch;
			}
		}

		return init;
	}

}

// -------------------------------------------
// Arguments:
//   initialData - array of at least mipLevelCount entries, 1 entry if mipLevelCount is zero.
// -------------------------------------------
ALResult Tr2TextureAL::Create2D(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	PixelFormat format,
	BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	HRESULT hr = Create2DImpl( width, height, mipLevelCount, 1, 0, format, usage, initialData, renderContext );
	if( SUCCEEDED( hr ) )
	{
		m_type = TEX_TYPE_2D;
	}
	return hr;
}

ALResult Tr2TextureAL::Create2DArray(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	uint32_t arrayCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	HRESULT hr = Create2DImpl( width, height, mipLevelCount, arrayCount, 0, format, usage, initialData, renderContext );
	if( SUCCEEDED( hr ) )
	{
		m_type = TEX_TYPE_2D;
	}
	return hr;
}

// -------------------------------------------
// Arguments:
//   initialData - array of at least 6 * mipLevelCount entries, 6 entries if mipLevelCount is zero. All miplevels
//				   for a given face need to be in order, highest resolution to lowest resolution, then the next
//				   face all mips high to low, etc repeated six times.
// -------------------------------------------
ALResult Tr2TextureAL::CreateCube(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	PixelFormat format,
	BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	HRESULT hr = Create2DImpl( width, height, mipLevelCount, 6, D3D11_RESOURCE_MISC_TEXTURECUBE, format, usage, initialData, renderContext );
	if( SUCCEEDED( hr ) )
	{
		m_type = TEX_TYPE_CUBE;
	}
	return hr;
}

// shared implementation for Create2D and CreateCube. Sets everything except m_type.
ALResult Tr2TextureAL::Create2DImpl(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	uint32_t arraySize,
	uint32_t miscFlags,
	PixelFormat format,
	BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	Destroy();

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2TextureAL Create function" );
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create2DImpl: Trying to create an immutable texture without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof (desc) );
	desc.Width		= width;
	desc.Height		= height;
	desc.MipLevels	= /*mipLevelCount*/ trueMipLevelCount;	// see below, no auto-gen yet
	desc.ArraySize  = arraySize;
	desc.Format     = static_cast<DXGI_FORMAT>( MakeTypeless( format ) );
	desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
	// this only works with rendertargets, and calling GenerateMips. Probably not what we want
	//desc.MiscFlags  = mipLevelCount ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS;

	ConvertUsage( usage, desc.Usage, desc.CPUAccessFlags );
	
	desc.SampleDesc.Count = 1;
	desc.MiscFlags  = miscFlags;
	if( usage & USAGE_UNORDERED_ACCESS )
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	auto init = ConvertInitialData( trueMipLevelCount * arraySize, initialData );

	ID3D11Texture2D* texture2D;
	HRESULT HR = renderContext.m_d3dDevice11->CreateTexture2D( &desc, initialData ? &init[0] : nullptr, &texture2D );

	if( FAILED( HR ) )
	{
		return HR;
	}

	m_texture.Attach( texture2D );

	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	memset( &srvDesc, 0, sizeof( srvDesc ) );

	srvDesc.Format	= static_cast<DXGI_FORMAT>( format );

	if( miscFlags == D3D11_RESOURCE_MISC_TEXTURECUBE )
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;

		srvDesc.TextureCube.MipLevels = desc.MipLevels;
	}
	else
	{
		srvDesc.ViewDimension = arraySize > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels		 = desc.MipLevels;
		srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
	}
	
	for( unsigned loop = 0; loop != 2; ++loop )
	{
		HR = renderContext.m_d3dDevice11->CreateShaderResourceView( m_texture, &srvDesc, &m_view[loop] );
		if( FAILED( HR ) )
		{
			CCP_AL_LOGERR( "Create2DImpl: CreateShaderResourceView/%d failed: 0x%x", loop, HR );
			m_texture = nullptr;
			return HR;
		}

		srvDesc.Format = static_cast<DXGI_FORMAT>( MakeSrgb( format ) );
	}

	m_format		= format;
	m_usage			= usage;	
	m_width			= width;
	m_height		= height;
	m_volumeDepth	= 1;
	m_mipCount		= mipLevelCount;
	m_isAlias		= false;
	m_arraySize = arraySize;
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2TextureAL::CreateVolume(	
	uint32_t width, 
	uint32_t height, 
	uint32_t depth, 
	uint32_t mipLevelCount,
	PixelFormat format,
	BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	Destroy();

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2TextureAL Create function" );
		return E_INVALIDARG;
	}

	if( !initialData )
	{
		CCP_AL_LOGERR( "CreateVolume: Trying to create volume texture without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	D3D11_TEXTURE3D_DESC desc;
	memset( &desc, 0, sizeof (desc) );
	desc.Width		= width;
	desc.Height		= height;
	desc.Depth		= depth;
	desc.MipLevels	= trueMipLevelCount;
	desc.Format     = static_cast<DXGI_FORMAT>( MakeTypeless( format ) );
	desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
	
	ConvertUsage( usage, desc.Usage, desc.CPUAccessFlags );
	
	auto init = ConvertInitialData( trueMipLevelCount, initialData );

	ID3D11Texture3D* texture3D = nullptr;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateTexture3D( &desc, init.empty() ? nullptr : &init[0], &texture3D ) );

	m_texture.Attach( texture3D );

	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	memset( &srvDesc, 0, sizeof( srvDesc ) );

	srvDesc.Format	= static_cast<DXGI_FORMAT>( format );

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

	srvDesc.Texture3D.MipLevels = desc.MipLevels;
	
	for( unsigned loop = 0; loop != 2; ++loop )
	{
		HRESULT HR = renderContext.m_d3dDevice11->CreateShaderResourceView( m_texture, &srvDesc, &m_view[loop] );
		if( FAILED( HR ) )
		{
			CCP_AL_LOGERR( "CreateVolume: CreateShaderResourceView/%d failed: 0x%x", loop, HR );
			m_texture = nullptr;
			return HR;
		}

		srvDesc.Format = static_cast<DXGI_FORMAT>( MakeSrgb( format ) );
	}

	m_format		= format;
	m_usage			= usage;	
	m_width			= width;
	m_height		= height;
	m_volumeDepth	= depth;
	m_mipCount		= trueMipLevelCount;
	m_isAlias		= false;
	m_arraySize = 1;

	m_type			= TEX_TYPE_3D;
	ChangeObjectId();
	
	return S_OK;
}

// -------------------------------------------------------------
// Description:
// For updating subrectangles in a texture, the fastest way across platforms is to use this UpdateSubresource function.
// It requires that the texture was created with USAGE_DEFAULT. In exchange it actually works without the need for staging
// resources or other inefficient access flags.  Source must be in exactly the same format as this texture; however the
// dimensions can be smaller, so sourcePitch must be set up accordingly to be right-left times the byteCount, taking BC
// formats into account.
// -------------------------------------------------------------
ALResult Tr2TextureAL::UpdateSubresource(	
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom, 
	const void* source, 
	uint32_t sourcePitch, 
	Tr2RenderContextAL& renderContext )
{
	if( !m_texture || !renderContext.m_context || m_type != TEX_TYPE_2D ) 
	{
		return E_FAIL;
	}

	if( ( m_usage & USAGE_CPU_WRITE || m_usage & USAGE_IMMUTABLE ) )
	{
		CCP_AL_LOGWARN( "UpdateSubResource only works with no USAGE_CPU_WRITE and no USAGE_IMMUTABLE flags" );
		return E_INVALIDARG;
	}

	D3D11_BOX box;
	box.left	= std::min( left, m_width );
	box.top		= std::min( top, m_height );
	box.right	= std::min( right, m_width );
	box.bottom	= std::min( bottom, m_height );
	box.front	= 0;
	box.back	= 1;

	if( box.right <= box.left || box.bottom <= box.top )
	{
		return S_OK;
	}
	renderContext.m_context->UpdateSubresource( m_texture, 0, &box, source, sourcePitch, 0 );

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies part of the source texture into this texture. Textures must have the same 
//   format, they can't be immutable (safest is probably to use USAGE_DEFAULT on
//   both), subresources must have the same dimensions, 3D textures are not supported.  
//   There's no clipping so destination position must be able to fit the source texture, 
//   or its subrectangle. The subrectangle can't exceed the source's bounds.
//   The call may be asynchronous
// Arguments:
//   destSubresource - Desription of destination texture area
//   source - Source texture
//   sourceSubresource - Description of source texture area
//   renderContext - Current render context
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2TextureAL::CopySubresourceRegion( 
	const Tr2TextureSubresource& destSubresource,
	Tr2TextureAL& source, 
	const Tr2TextureSubresource& sourceSubresource,
	Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( m_texture );
	CCP_ASSERT( source.m_texture );

	if( !m_texture || !source.m_texture || !renderContext.m_context )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );

	if( destSubresource.IsSubresourceFull( *this ) && sourceSubresource.IsSubresourceFull( source ) )
	{
		renderContext.m_context->CopyResource( m_texture, source.m_texture );
		return S_OK;
	}

	Tr2TextureSubresource src = sourceSubresource;
	Tr2TextureSubresource dst = destSubresource;	
	
	if( !Crop( src, source, dst, *this ) )
	{
		return E_FAIL;
	}

	D3D11_BOX box = { src.m_left, src.m_top, src.m_front, src.m_right, src.m_bottom, src.m_back };

	const uint32_t srcMipCount = source.GetTrueMipCount();
	const uint32_t dstMipCount = GetTrueMipCount();

	const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

	for( uint32_t mip = 0; mip != mipCount; ++mip )
	{
		for( uint32_t face = 0; face != src.GetFaceCount(); ++face )
		{
			renderContext.m_context->CopySubresourceRegion( 
				m_texture,
				D3D10CalcSubresource( dst.m_startMipLevel + mip, dst.m_startFace + face, dstMipCount ),
				dst.m_left,
				dst.m_top,
				dst.m_front,
				source.m_texture,
				D3D10CalcSubresource( src.m_startMipLevel + mip, src.m_startFace + face, srcMipCount ),
				&box );
		}

		if( mip + 1 != src.GetMipCount() )
		{
			AdvanceMip( src, source, mip );
			AdvanceMip( dst, *this,  mip );
		}

		box.left = box.left >> 1;
		box.right = std::max( box.right >> 1, box.left + 1 );
		box.top = box.top >> 1;
		box.bottom = std::max( box.bottom >> 1, box.top + 1 );
		box.front = box.front >> 1;
		box.back = std::max( box.back >> 1, box.front + 1 );
	}

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies part of the source renderTarget into this texture. They both must have the same 
//   format, they can't be immutable (safest is probably to use USAGE_DEFAULT on
//   both), subresources must have the same dimensions, 3D textures are not supported.  
//   There's no clipping so destination position must be able to fit the source texture, 
//   or its subrectangle. The subrectangle can't exceed the source's bounds.
//   The call may be asynchronous
// Arguments:
//   destSubresource - Desription of destination texture area
//   source - Source renderTarget
//   sourceSubresource - Description of source renderTarget area
//   renderContext - Current render context
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2TextureAL::CopySubresourceRegion(	
	const Tr2TextureSubresource& destSubresource,
	Tr2RenderTargetAL& source, 
	const Tr2TextureSubresource& sourceSubresource,
	Tr2RenderContextAL& renderContext )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );

	// Looks like DX11 has no issues with treating the RT as a regular texture, so just use that one.
	return CopySubresourceRegion( destSubresource, source.GetTexture(), sourceSubresource, renderContext );
}

ALResult Tr2TextureAL::CopySubresourceRegion( 
	CComPtr<ID3D11Texture2D> dest, 
	uint32_t destX, 
	uint32_t destY, 
	CComPtr<ID3D11Texture2D> source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( source );
	
	if( !renderContext.m_context || !dest || !source )
	{
		return E_FAIL;
	}

	if( destX == 0 && destY == 0 && ltrb == nullptr )
	{
		renderContext.m_context->CopyResource( dest, source );
		return S_OK;
	}

	D3D11_BOX box;
	if( ltrb )
	{
		box.left	= ltrb[0];
		box.top		= ltrb[1];
		box.right	= ltrb[2];
		box.bottom	= ltrb[3];
		box.front	= 0;
		box.back	= 1;
	}

	renderContext.m_context->CopySubresourceRegion( dest, 0, destX, destY, 0, source, 0, ltrb ? &box : nullptr );
	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Lock a texture. This may be extremely expensive and involve a copy of the texture from the GPU to the CPU, each and every time you
//  lock.
// -------------------------------------------------------------
ALResult Tr2TextureAL::Lock(	
	uint32_t mipLevel, 
	void*& data, 
	uint32_t& pitch, 
	LockType lockType, 
	Tr2RenderContextAL& renderContext )
{
	return Lock( mipLevel, nullptr, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock(	
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	LockType lockType, 
	Tr2RenderContextAL& renderContext )
{
	return Lock( 0, mipLevel, ltrb, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock(	
	uint32_t face, 
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	LockType lockType, 
	Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( m_currentLock == LOCK_INVALID );
	if( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Tr2TextureAL - Attempting to lock already locked texture" );
		return E_FAIL;
	}

	// cancel out requests for a rectangle where it turns out that the rectangle is the full texture.
	// there is a fast path for that case (map/unmap instead of staging resources)
	if( ltrb					&&
		ltrb[0] == 0			&&
		ltrb[1] == 0			&&
		ltrb[2] == GetWidth()	&&
		ltrb[3] == GetHeight()	)
	{
		ltrb = nullptr;
	}

	switch( lockType )
	{
	case LOCK_READONLY:
		return LockReading( face, mipLevel, ltrb, data, pitch, renderContext );
	case LOCK_WRITEONLY:
		return LockWriting( face, mipLevel, ltrb, data, pitch, renderContext );
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL & renderContext )
{
	switch( m_currentLock )
	{
	case LOCK_READONLY:
		{
			return UnlockReading( renderContext );
		}
	case LOCK_WRITEONLY:
		{
			return UnlockWriting( renderContext );
		}
	}

	CCP_AL_LOGERR( "Trying to Unlock buffer that's not locked" );
	return E_FAIL;
}

ALResult Tr2TextureAL::LockReading( 
	uint32_t face, 
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL & renderContext )
{
	data = nullptr;

	if( ( m_usage & USAGE_CPU_READ ) == 0 && ( m_usage & USAGE_IMMUTABLE ) == 0 )
	{
		return E_INVALIDARG;
	}

	if( !m_texture || !renderContext.m_context || ( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE ) || ( m_mipCount > 0 && mipLevel >= m_mipCount ) )
	{
		return E_FAIL;
	}

	if( !m_writeStaging.empty() )		// Currently locked for writing?
	{
		return E_FAIL;
	}

	if( !m_staging )
	{
		const uint32_t width  = ltrb ? ltrb[2] - ltrb[0] : m_width;
		const uint32_t height = ltrb ? ltrb[3] - ltrb[1] : m_height;

		D3D11_TEXTURE2D_DESC desc;
		memset( &desc, 0, sizeof (desc) );
		desc.Width		= width >> mipLevel;
		desc.Height		= height >> mipLevel;
		if( IsCompressedFormat( GetFormat() ) )
		{
			desc.Width  = std::max( desc.Width , 4u );
			desc.Height = std::max( desc.Height, 4u );
		}
		else
		{
			desc.Width  = std::max( desc.Width , 1u );
			desc.Height = std::max( desc.Height, 1u );
		}
		desc.MipLevels	= 1;
		desc.ArraySize  = 1;
		desc.Format     = static_cast<DXGI_FORMAT>( MakeTypeless( m_format ) );
		
		desc.Usage			= D3D11_USAGE_STAGING;
		desc.CPUAccessFlags	= D3D11_CPU_ACCESS_READ;
	
		desc.SampleDesc.Count = 1;
		
		if( !renderContext.m_secondaryDevice11 )
		{
			return E_FAIL;
		}
		CR_RETURN_HR( renderContext.m_secondaryDevice11->CreateTexture2D( &desc, nullptr, &m_staging ) );
		if( !m_staging )
		{
			return E_FAIL;
		}
	}

	//renderContext.m_context->CopyResource( m_staging, m_texture );
	if( ltrb )
	{
		D3D11_BOX box = { ltrb[0], ltrb[1], 0, ltrb[2], ltrb[3], 1 };
		renderContext.m_context->CopySubresourceRegion( m_staging, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( mipLevel, face, GetTrueMipCount() ), &box );
	}
	else
	{
		renderContext.m_context->CopySubresourceRegion( m_staging, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( mipLevel, face, GetTrueMipCount() ), nullptr );
	}
	
	D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
	HRESULT hr = renderContext.m_context->Map( m_staging, 0, D3D11_MAP_READ, 0, &ms );
	data = ms.pData;
	pitch = ms.RowPitch;
	if( !ms.pData )
	{
		return E_FAIL;
	}
	if( SUCCEEDED( hr ) )
	{
		m_currentLock = LOCK_READONLY;
		m_currentLockMipLevel = mipLevel;
	}
    return hr;
}

ALResult Tr2TextureAL::UnlockReading( Tr2RenderContextAL & renderContext )
{
	renderContext.m_context->Unmap( m_staging, 0 );
	m_currentLock = LOCK_INVALID;
	m_currentLockMipLevel = 0;
	m_staging = nullptr;	//TODO keep a cache?
	return S_OK;
}

ALResult Tr2TextureAL::LockWriting( 
	uint32_t face, 
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL & renderContext )
{
	// Note -- not sure if there is any way to lock the texture in a way that preserves what is already there.
	// The documentation suggests that usage_dynamic + map_write should do the trick, but at runtime this gives:
	//
	// D3D11: ERROR: ID3D11DeviceContext::Map: Map cannot be called with MAP_WRITE access, because the Resource
	// was created as D3D11_USAGE_DYNAMIC. D3D11_USAGE_DYNAMIC Resources must use either MAP_WRITE_DISCARD or 
	// MAP_WRITE_NO_OVERWRITE with Map. [ RESOURCE_MANIPULATION ERROR #2097210: RESOURCE_MAP_INVALIDMAPTYPE ]
	//
	// So when you lock the texture, you need to re-write the whole thing?... If the user is trying to lock a subrectangle,
	// use CPU hosted memory and UpdateSubresource instead.

	if( !m_texture )
	{
		data = nullptr;
		return E_FAIL;
	}

	if( ltrb || !( m_usage & USAGE_CPU_WRITE ) )
	{
		if( !m_writeStaging.empty() )
		{
			return E_FAIL;	// already locked for writing
		}

		if( IsCompressedFormat( GetFormat() ) )
		{
			return E_FAIL;
		}
		if( ltrb && ( ltrb[2] <= ltrb[0] || ltrb[3] <= ltrb[1] ) )
		{
			return E_FAIL;
		}
		const uint32_t w = ltrb ? ltrb[2] - ltrb[0] : m_width;
		const uint32_t h = ltrb ? ltrb[3] - ltrb[1] : m_height;
		pitch = w * GetBytesPerPixel( GetFormat() );

		m_writeStaging.resize( "Tr2TextureAL::m_writeStaging", pitch * h );

		if( ltrb )
		{
			m_writeLtrb[0] = ltrb[0];
			m_writeLtrb[1] = ltrb[1];
			m_writeLtrb[2] = ltrb[2];
			m_writeLtrb[3] = ltrb[3];
		}
		else
		{
			m_writeLtrb[0] = 0;
			m_writeLtrb[1] = 0;
			m_writeLtrb[2] = m_width;
			m_writeLtrb[3] = m_height;
		}

		CCP_ASSERT( m_writeLtrb[2] <= GetMipWidth( mipLevel ) && m_writeLtrb[3] <= GetMipHeight( mipLevel ) );
		if( m_writeLtrb[2] > GetMipWidth( mipLevel ) && m_writeLtrb[3] > GetMipHeight( mipLevel ) )
		{
			CCP_LOGERR( "TrinityAL: invalid rectangle in Tr2TextureAL::LockWriting" );
			m_writeStaging.clear();
			return E_FAIL;
		}

		CCP_ASSERT( !m_writeStaging.empty() );
		data = m_writeStaging.get();

		m_currentLock = LOCK_WRITEONLY;
		m_currentLockMipLevel = mipLevel;
		return S_OK;
	}

	D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
	HRESULT hr = renderContext.m_context->Map( m_texture, D3D10CalcSubresource( mipLevel, face, GetTrueMipCount() ), D3D11_MAP_WRITE_DISCARD, 0, &ms );
	if( FAILED( hr ) )
	{
		data = nullptr;
		pitch = 0;
		return hr;
	}
	if( !ms.pData )
	{
		data = nullptr;
		pitch = 0;
		return E_FAIL;
	}
#if TRINITY_AL_GUARD_LOCKS
	m_lockGuard.Lock( ms.RowPitch * GetMipHeight( mipLevel ), ms.pData );
	data = m_lockGuard.GetMemory();
	pitch = ms.RowPitch;
#else
	data = ms.pData;
	pitch = ms.RowPitch;
#endif
	m_currentLock = LOCK_WRITEONLY;
	m_currentLockMipLevel = mipLevel;
    return hr;
}

ALResult Tr2TextureAL::UnlockWriting( Tr2RenderContextAL & renderContext )
{
	if( !m_texture )
	{
		return E_FAIL;
	}

	ON_BLOCK_EXIT( [&]{ 
		m_currentLock = LOCK_INVALID;
		m_currentLockMipLevel = 0;
	} );

	if( !m_writeStaging.empty() )
	{
		const uint32_t w = m_writeLtrb[2] - m_writeLtrb[0];
		const uint32_t pitch = w * GetBytesPerPixel( GetFormat() );

		if( renderContext.m_context ) 
		{
			D3D11_BOX box;
			box.left	= m_writeLtrb[0];
			box.top		= m_writeLtrb[1];
			box.right	= m_writeLtrb[2];
			box.bottom	= m_writeLtrb[3];
			box.front	= 0;
			box.back	= 1;

			CCP_ASSERT( m_writeLtrb[2] <= GetMipWidth( m_currentLockMipLevel ) && m_writeLtrb[3] <= GetMipHeight( m_currentLockMipLevel ) );
			if( m_writeLtrb[2] > GetMipWidth( m_currentLockMipLevel ) && m_writeLtrb[3] > GetMipHeight( m_currentLockMipLevel ) )
			{
				CCP_LOGERR( "TrinityAL: invalid rectangle in Tr2TextureAL::UnlockWriting" );
				return E_FAIL;
			}

			renderContext.m_context->UpdateSubresource( m_texture, m_currentLockMipLevel, &box, m_writeStaging.get(), pitch, 0 );
		}

		m_writeStaging.clear();
		m_writeLtrb[0] = m_writeLtrb[1] = m_writeLtrb[2] = m_writeLtrb[3] = 0;

		return S_OK;
	}

#if TRINITY_AL_GUARD_LOCKS
	m_lockGuard.Unlock();
#endif
	renderContext.m_context->Unmap( m_texture, m_currentLockMipLevel );
	return S_OK;
}

ALResult Tr2TextureAL::CreateUAV( Tr2PrimaryRenderContextAL &renderContext )
{
	if( !IsValid() || !renderContext.IsValid() /*|| GetBytesPerPixel( m_format ) != 4 */ )
	{
		return E_FAIL;
	}

	// Create a UAV on the backbuffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
	//descUAV.Format				= DXGI_FORMAT_R32_FLOAT;	// has to be R32?
	descUAV.Format				= static_cast<DXGI_FORMAT>( m_format );
	descUAV.ViewDimension		= D3D11_UAV_DIMENSION_TEXTURE2D;
	descUAV.Texture2D.MipSlice	= 0;
	m_uav = nullptr;
	return renderContext.m_d3dDevice11->CreateUnorderedAccessView( m_texture, &descUAV, &m_uav );
}

#endif
