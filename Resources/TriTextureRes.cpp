#include "StdAfx.h"
#include "TriTextureRes.h"
#include "TriSettingsRegistrar.h"

#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "Tr2HostBitmap.h"
#include "Tr2ImageIOHelpers.h"

#include "TriConstants.h"
#include "TriDevice.h"


using namespace Tr2RenderContextEnum;


CCP_STATS_DECLARE( textureResBytes, "Trinity/textureResBytes", false, CST_MEMORY, "Size of memory occupied by textures." );
#if DEPRECATED_ENABLED
BLUE_REGISTER_ENUM_EX( "TRIFORMAT", D3DFORMAT, TriFormat, ENUM_REG_VALUES_ON_MODULE | ENUM_REG_ENUM_OBJECT_ON_MODULE );
#endif

float g_imageWarnLoadTime = 1.00f;
TRI_REGISTER_SETTING( "imageWarnLoadTime", g_imageWarnLoadTime );

// If set, then textures loaded via the D3DX functions (fallback for non-dds textures)
// generate miplevels. Set to false to load with no mips for faster loading.
static bool s_generateMipsOnTextureLoad = true;
TRI_REGISTER_SETTING( "generateMipsOnTextureLoad", s_generateMipsOnTextureLoad );

namespace {

IBlueResource* CreateTextureResource( const wchar_t* name )
{
	TriTextureResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"dds", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"png", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"sdd", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"tga", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"jpg", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"jpeg", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"bmp", CreateTextureResource );

} // end of anonymous namespace



TriTextureRes::TriTextureRes(): 
	m_resourceRebuiltCounter( 0 ),
	m_isTextureLoadDisabled( false ),
	m_memoryUse( 0 ),
	m_cutoutX( 0.0f ),
	m_cutoutY( 0.0f ),
	m_cutoutWidth( 1.0f ),
	m_cutoutHeight( 1.0f ),
	m_mipLevelMaxCount( std::numeric_limits<uint32_t>::max() ),
	m_isTextureResizable( true )
{}

TriTextureRes::~TriTextureRes()
{
	// Note: ReleaseResources now happens in Unlock due to threading issues.
	// If a texture resource was deleted while it was still being loaded it would cause crash.
}

// --------------------------------------------------------------------------------------
// Description:
//   Figure out what the lowest-number (ie. highest quality) miplevel is that we should use, by
//   looking at
//   a. if this is not a texture that's excluded from global resizing, taking global GetMipLevelSkipCount
//      into account.
//   The computed value is returned but no state is being changed (ie. we don't act on it).
// --------------------------------------------------------------------------------------
unsigned TriTextureRes::ComputeMipSkipCount()
{
	// Try to lower detail (increase the number) further from the global setting, if allowed.
	if( m_isTextureResizable && gTriDev )
	{
		return gTriDev->GetMipLevelSkipCount();
	}	

	return 0;
}

void TriTextureRes::Initialize( const wchar_t* name, const wchar_t* ext )
{
	CancelPendingLoad();
	CleanupAsyncSave(false);

	m_mipLevelMaxCount = gTriDev ? gTriDev->GetMipLevelMaxCount() : 255;

	m_isTextureResizable = Tr2Renderer::IsTextureToResize( CW2A( name ) );

	m_isTextureLoadDisabled = Tr2Renderer::IsTextureLoadDisabled();

	BlueAsyncRes::Initialize( name, ext );
}

void TriTextureRes::OnShutdown()
{
	ReleaseResources( TRISTORAGE_ALL );
}

void TriTextureRes::ReleaseResources( TriStorage s )
{
	CancelPendingLoad();
	CleanupAsyncSave(false);

	if( (s & TRISTORAGE_MANAGEDMEMORY) || 
		((s & TRISTORAGE_VIDEOMEMORY) && m_texture.GetMemoryClass() == AL_MEMORY_VIDEO
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
		&& !g_usingEXDevice
#endif
		) )
	{
		CCP_STATS_ADD( textureResBytes, -( int )m_memoryUse );
		m_memoryUse = 0;

		m_texture.Destroy();
		m_wrappedRenderTarget = nullptr;
		m_wrappedDepthStencil = nullptr;
		SetPrepared( false );
	}
}

Tr2TextureAL* TriTextureRes::GetTexture()
{
	if( m_wrappedRenderTarget )
	{
		if( !m_texture.IsValid() )
		{
			SetTexture( m_wrappedRenderTarget->GetRenderTarget().GetTexture() );
		}
	}
	else
	if( m_wrappedDepthStencil )
	{
		if( !m_texture.IsValid() )
		{
			SetTexture( m_wrappedDepthStencil->m_depthStencil.GetTexture() );
		}
	}
	
	return m_texture.IsValid() ? &m_texture : nullptr;
}

const Tr2TextureAL* TriTextureRes::GetTexture() const
{
	return m_texture.IsValid() ? &m_texture : nullptr;
}

#if TRINITYDEV
void TriTextureRes::GetDescription( std::string& desc )
{
	desc = "TriTextureRes: '";
	desc += CW2A( m_path.c_str() );
	desc += "'";
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// Python thunkers for TriTextureRes
/////////////////////////////////////////////////////////////////////////////////////////

bool TriTextureRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t TriTextureRes::GetMemoryUsage()
{
	return m_memoryUse;
}

void TriTextureRes::ReloadResources() 
{
	CancelPendingLoad();

	gTriDev->QueueForReload( this );
}

// Called on background thread
bool TriTextureRes::DoOpenStream()
{
	m_reservedMemory = 0;

	// make sure the path string is not empty
	if( !GetPath()[0] )
	{
		return false;
	}

	if( m_isTextureLoadDisabled )
	{
		return false;
	}

	if( BePaths->GetStreamFromPathW( GetPath(), &m_dataStream ) )
	{
		m_reservedMemory = m_dataStream->GetSize();
		BeResMan->ReserveBackgroundLoadMemory( m_reservedMemory );
		return true;
	}

	return false;
}

void TriTextureRes::DoCloseStream()
{
	if( m_loadedBitmap )
	{
		m_cutoutX		= m_metadata.cutout.x;
		m_cutoutY		= m_metadata.cutout.y;
		m_cutoutWidth	= m_metadata.cutout.width;
		m_cutoutHeight	= m_metadata.cutout.height;
	}

	m_loadedBitmap.reset();

	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_data = NULL;
		m_dataSize = 0;
		m_dataStream = nullptr;
	}

	BeResMan->ReleaseBackgroundLoadMemory( m_reservedMemory );
	m_reservedMemory = 0;
}

bool TriTextureRes::OnPrepareResources()
{
	if( IsPrepared() || IsLoading() )
	{
		// todo: we get here a lot from TriTexture calling PrepareResources.
		// Those calls are probably not needed anymore - research a bit more.
		return true;
	}

	Initialize( m_path.c_str(), m_ext.c_str() );
	return true;
}

bool TriTextureRes::Save( const wchar_t* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// Only permit saving of 2d and cube textures
	if( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Texture save failed - only 2d and cubemap textures can be saved" );
		return false;
	}
	
	if( !ImageIO::IsSaveSupported( filename, *this ) )
	{
		CCP_LOGERR( "Unsupported format for saving (%S)", filename );
		return false;
	}
	
	Tr2HostBitmapPtr saveBitmap;

	if( m_type == TEX_TYPE_2D )
	{
		if( !saveBitmap.CreateInstance()										||
			!saveBitmap->Create( GetWidth(), GetHeight(), GetTrueMipCount(), GetFormat() )	||
			!saveBitmap->CopyFromTextureRes( *this, renderContext ) )
		{
			return false;
		}
	}
	else
	{
		if( !saveBitmap.CreateInstance()										||
			!saveBitmap->CreateCube( GetWidth(), GetTrueMipCount(), GetFormat() ) ||
			!saveBitmap->CopyFromTextureRes( *this, renderContext ) )
		{
			return false;
		}
	}

	return saveBitmap->Save( filename );
}

bool TriTextureRes::SaveAsync( const wchar_t* filename )
{
	// Only permit saving of 2d and cube textures
	if( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Texture save failed - only 2d and cubemap textures can be saved" );
		return false;
	}
	
	return StartAsyncSave( filename );
}

bool TriTextureRes::DoPrepareAsyncSave( void )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_asyncSaveImage )
	{
		CCP_LOGERR( "Unsupported extension for saving (%S)", m_saveFilename.c_str() );
		return false;
	}
	if( !ImageIO::IsSaveSupported( m_saveFilename.c_str(), *this ) )
	{
		CCP_LOGERR( "Unsupported format for saving (%S)", m_saveFilename.c_str() );
		return false;
	}
	
	switch( GetType() )
	{
	case TEX_TYPE_2D:
		if( !m_asyncSaveBitmap.CreateInstance()																||
			!m_asyncSaveBitmap->Create( GetWidth(), GetHeight(), GetTrueMipCount(), GetFormat() )	||
			!m_asyncSaveBitmap->CopyFromTextureRes( *this, renderContext ) )
		{
			CCP_LOGERR( "Failed to save (%S)", m_saveFilename.c_str() );
			return false;
		}
		break;
	case TEX_TYPE_CUBE:
		if( !m_asyncSaveBitmap.CreateInstance()														||
			!m_asyncSaveBitmap->CreateCube( GetWidth(), GetTrueMipCount(), GetFormat() )	||
			!m_asyncSaveBitmap->CopyFromTextureRes( *this, renderContext ) )
		{
			return false;
		}
		break;
	default:
		CCP_LOGERR( "Unsupported texture type for saving (%S)", m_saveFilename.c_str() );
		return false;
	}

	return true;
}

bool TriTextureRes::DoExecuteAsyncSave()
{
	if( m_asyncSaveBitmap && m_asyncSaveBitmap->IsValid() )
	{
		return m_asyncSaveBitmap->Save( m_saveFilename.c_str() );
	}

	return false;
}

void TriTextureRes::DoCleanupAsyncSave()
{
	m_asyncSaveBitmap = nullptr;
	m_asyncSaveImage.reset();
}

static bool IsTga( const wchar_t* filename )
{
	size_t len = wcslen( filename );
	if( len > 3 )
	{
		const wchar_t* ext = filename + len - 3;
		return ( ext[0] == L't' || ext[0] == L'T' ) &&
			( ext[1] == L'g' || ext[1] == L'G' ) &&
			( ext[2] == L'a' || ext[2] == L'A' );
	}
	return false;
}

// Called on background thread
BlueAsyncRes::LoadingResult TriTextureRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BeTimer t;

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	// check if this is a cube map texture or not
	if( m_path.find( L"_cube") != m_path.npos )
	{
		m_type = TEX_TYPE_CUBE;
	}
	else if( m_path.find( L"_volume") != m_path.npos )
	{
		m_type = TEX_TYPE_3D;
	}
	else
	{
		m_type = TEX_TYPE_2D;
	}

	m_loadedBitmap.reset( CCP_NEW( "TriTextureRes::m_loadedBitmap" ) ImageIO::HostBitmap );
	CCP_ASSERT( m_loadedBitmap != nullptr );

	ImageIO::LoadParameters params( m_path.c_str(), ComputeMipSkipCount(), m_mipLevelMaxCount );
	auto result = ImageIO::ReadImage( *m_dataStream, params, *m_loadedBitmap, &m_metadata );
	if( !result )
	{
		CCP_LOGERR( "Tr2ImageHandler failed to load texture '%S': %s", GetPath(), result.GetErrorMessage().c_str() );
		m_loadedBitmap.reset();
	}
	else if( IsTga( GetPath() ) )
	{
		if( !m_loadedBitmap->GenerateMipMaps() )
		{
			CCP_LOGERR( "Tr2ImageHandler failed to generate mipmaps for texture '%S'", GetPath() );
			m_loadedBitmap.reset();
		}
	}

	const float secs = (float)t.GetSeconds();
	if( secs > g_imageWarnLoadTime )
	{
		CCP_LOGWARN( "TriTextureRes - image read '%S' took %f seconds", GetPath(), secs );
	}

	return m_loadedBitmap ? LR_SUCCESS : LR_FAILED;
}

// Called on main thread
bool TriTextureRes::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if ( m_texture.IsValid() )
	{
		CCP_STATS_ADD( textureResBytes, -(int)m_memoryUse );
		m_memoryUse = 0;

		m_texture.Destroy();
	}

	// No need to check for texture load disabled - we wouldn't have gotten here
	// if it were.

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	bool isOK = false;
	if( m_loadedBitmap )
	{
		Tr2TextureAL face;
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( !Tr2ImageIOHelpers::CreateTexture( *m_loadedBitmap, m_texture, 
												m_memoryUse, 
												renderContext, 
												USAGE_IMMUTABLE ) )
		{
			CCP_LOGWARN( "Tr2ImageHandler failed to create texture '%S'", GetPath() );
			return false;
		}
		
		isOK = true;
		SetTexture( m_texture );		
		++m_resourceRebuiltCounter; 
	}


	return isOK;
}

long TriTextureRes::UpdateSubresource( unsigned left, unsigned top, unsigned right, unsigned bottom, const void* source, unsigned sourcePitch )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( Tr2TextureAL* texture = GetTexture() )
	{
		return (HRESULT)texture->UpdateSubresource( left, top, right, bottom, source, sourcePitch, renderContext );
	}
	return E_FAIL;
}

// ---------------------------------------------------------------


bool TriTextureRes::SetTextureFromRT( Tr2RenderTarget* renderTarget )
{
	m_wrappedRenderTarget = renderTarget;
	m_texture.Destroy();

	if( renderTarget && renderTarget->IsValid() )
	{
		m_isTextureResizable = false;
		SetTexture( renderTarget->GetRenderTarget().GetTexture() );		
	}

	return true;
}

bool TriTextureRes::CreateFromRT( Tr2RenderTarget* renderTarget, unsigned width, unsigned height )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_texture.Destroy();
	ON_BLOCK_EXIT( [&]{ SetTexture( m_texture ); } );

	if( !renderTarget || !renderTarget->IsValid() )
	{
		return false;
	}

	auto& rt = renderTarget->GetRenderTarget();

	if( !width ) 
	{
		width = rt.GetWidth();
	}
	if( !height )
	{
		height = rt.GetHeight();
	}
	
	// With mipmaps there may be staging resources involved, so just defer the problem to Tr2TextureAL::CopySubresourcRegion
	if( rt.GetMipCount() != 1 )
	{		
		Tr2BitmapDimensions bd( width, height, 0, rt.GetFormat() );	// use this to compute true mipcount of new texture

		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( 
				m_texture.Create2D(		width, 
										height, 
										bd.GetTrueMipCount(), 
										rt.GetFormat(), 
										USAGE_CPU_READ, 
										nullptr, 
										renderContext )
				, false );
		}

		Tr2TextureSubresource dst;
		dst.m_right  = width;
		dst.m_bottom = height;
		CR_RETURN_VAL( m_texture.CopySubresourceRegion( Tr2TextureSubresource(), *renderTarget, dst, renderContext ), false );
		m_isTextureResizable = false;
		return S_OK;
	}

	// No mipmaps, just locking the RT and initializing a new texture with its contents using initialData should work.
	Tr2SubresourceData srd;
	if( FAILED( rt.Lock( 0, nullptr, srd.m_sysMem, srd.m_sysMemPitch, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&]{ rt.Unlock( renderContext ); } );
	
	srd.m_height = rt.GetHeight();
	srd.m_sysMemSlicePitch = rt.GetHeight() * srd.m_sysMemPitch;

	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL( 
				m_texture.Create2D(		width, 
										height, 
										1, 
										rt.GetFormat(), 
										USAGE_CPU_READ,
										&srd, 
										renderContext )
				, false );	
	}

	m_isTextureResizable = false;
	SetTexture( m_texture );
	return true;	
}

bool TriTextureRes::CreateFromHostBitmap( Tr2HostBitmap* bitmap )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_texture.Destroy();

	if( !bitmap || !bitmap->IsValid() || !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	std::vector<Tr2SubresourceData> srd;
	srd.resize( bitmap->GetTrueMipCount() );
	for( unsigned i = 0; i != bitmap->GetTrueMipCount(); ++i )
	{
		srd[i].m_height = bitmap->GetMipWidth( i );
		srd[i].m_sysMem = bitmap->GetMipRawData( i );
		srd[i].m_sysMemPitch = bitmap->GetMipPitch( i );
		srd[i].m_sysMemSlicePitch = srd[i].m_height * srd[i].m_sysMemPitch;
	}
	CR_RETURN_VAL( 
		m_texture.Create2D(		bitmap->GetWidth(), 
								bitmap->GetHeight(), 
								bitmap->GetTrueMipCount(), 
								bitmap->GetFormat(), 
								USAGE_IMMUTABLE, 
								&srd[0], 
								renderContext )
		, false );

	m_isTextureResizable = false;
	SetTexture( m_texture );
	return true;	
}

bool TriTextureRes::SetTextureFromDS( Tr2DepthStencil* depthStencil )
{
	m_wrappedDepthStencil = depthStencil;
	m_texture.Destroy();

	if( depthStencil && depthStencil->IsValid() )
	{
		m_isTextureResizable = false;
		SetTexture( depthStencil->m_depthStencil.GetTexture() );		
	}

	return true;
}

bool TriTextureRes::Create(	uint32_t width, 
							uint32_t height, 
							uint32_t mipCount, 
							PixelFormat format, 
							BufferUsageFlags usage,
							Tr2PrimaryRenderContext& renderContext )
{
	m_texture.Destroy();

	CR_RETURN_VAL( 
			m_texture.Create2D(		width, 
									height, 
									mipCount, 
									format, 
									usage, 
									nullptr, 
									renderContext )
			, false );

	m_mipCount	= m_texture.GetMipCount();
	m_type		= m_texture.GetType();

	m_width		= m_texture.GetWidth();
	m_height	= m_texture.GetHeight();
	m_volumeDepth = m_texture.GetDepth();
	m_format	= m_texture.GetFormat();

	if( !m_memoryUse )
	{
		// Estimate memory use for the resource cache.
		// Ignoring format, reporting based on uncompressed textures.
		// It's better to overestimate here rather than underestimate.
		unsigned w = m_width;
		unsigned h = m_height;
		m_memoryUse = w * h * 4;
		for( unsigned i = 1; i < m_mipCount; ++i )
		{			
			w /= 2;
			h /= 2;
			m_memoryUse += w * h * 4;
		}
	}

	CCP_STATS_ADD( textureResBytes, m_memoryUse );
	SetPrepared( true );
	SetGood( true );
	return true;
}

// ---------------------------------------------------------------
bool TriTextureRes::SetTexture( Tr2TextureAL& texture )
{
	if( !texture.IsValid()  )
	{
		CCP_STATS_ADD( textureResBytes, -int( m_memoryUse ) );
		m_memoryUse = 0;
		SetPrepared( false );
		SetGood( false );

		Tr2BitmapDimensions::Destroy();
		return true;
	}

	m_texture = texture;
	m_mipCount = texture.GetMipCount();
	m_type = texture.GetType();

	m_width  = texture.GetWidth();
	m_height = texture.GetHeight();
	m_volumeDepth  = texture.GetDepth();
	m_format = texture.GetFormat();
		
	if( !m_memoryUse )
	{
		// Estimate memory use for the resource cache.
		// Ignoring format, reporting based on uncompressed textures.
		// It's better to overestimate here rather than underestimate.
		unsigned w = m_width;
		unsigned h = m_height;
		m_memoryUse = w * h * 4;
		for( unsigned i = 1; i < m_mipCount; ++i )
		{			
			w /= 2;
			h /= 2;
			m_memoryUse += w * h * 4;
		}
	}

	CCP_STATS_ADD( textureResBytes, m_memoryUse );
	SetPrepared( true );
	SetGood( true );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the object contains a reference to given AL object. This method is exposed
//   to Python and is used for debugging.
// Arguments:
//   type - Tr2RenderContextEnum::ObjectType, type of AL object
//   object - pointer to an AL object (passed as a number)
// Return Value:
//   true If object contains a reference to the given AL object
//   false Otherwise
// --------------------------------------------------------------------------------------
bool TriTextureRes::HasALObject( int type, size_t object )
{
	switch( type )
	{
	case OT_TEXTURE:
		return m_texture == *reinterpret_cast<Tr2TextureAL*>( object );
	case OT_RENDER_TARGET:
		if( m_wrappedRenderTarget )
		{
			return m_wrappedRenderTarget->GetRenderTarget() == *reinterpret_cast<Tr2RenderTargetAL*>( object );
		}
	case OT_DEPTH_STENCIL:
		if( m_wrappedDepthStencil )
		{
			return m_wrappedDepthStencil->m_depthStencil == *reinterpret_cast<Tr2DepthStencilAL*>( object );
		}
	}
	return false;
}
