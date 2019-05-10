#include "StdAfx.h"
#include "TriTextureRes.h"
#include "TriSettingsRegistrar.h"

#include "Tr2RenderTarget.h"
#include "Tr2HostBitmap.h"
#include "Tr2ImageIOHelpers.h"
#include "Tr2ImageRes.h"

#include "TriConstants.h"
#include "TriDevice.h"

#include "TexturePipeline/Tr2TexturePipeline.h"


using namespace Tr2RenderContextEnum;


CCP_STATS_DECLARE( textureResBytes, "Trinity/textureResBytes", false, CST_MEMORY, "Size of memory occupied by textures." );

float g_imageWarnLoadTime = 1.00f;
TRI_REGISTER_SETTING( "imageWarnLoadTime", g_imageWarnLoadTime );

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
BLUE_REGISTER_RESOURCE_EXTENSION( L"ecs", CreateTextureResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"ctr", CreateTextureResource );


bool IsCtrPath( const wchar_t* name )
{
	auto length = wcslen( name );
	return length > 4 && wcscmp( name + length - 4, L".ctr" ) == 0;
}

} // end of anonymous namespace



TriTextureRes::TriTextureRes(): 
	m_resourceRebuiltCounter( 0 ),
	m_isTextureLoadDisabled( false ),
	m_texture( nullptr ),
	m_memoryUse( 0 ),
	m_cutoutX( 0.0f ),
	m_cutoutY( 0.0f ),
	m_cutoutWidth( 1.0f ),
	m_cutoutHeight( 1.0f ),
	m_mipLevelMaxCount( std::numeric_limits<uint32_t>::max() ),
	m_isTextureResizable( true ),
	m_data( nullptr ),
	m_dataSize( 0 ),
	m_resourceLoadCbId( 0 ),
	m_resourcePrepCbId( 0 ),
	m_averageColor( 0.0, 0.0, 0.0, 0.0 )
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
	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
	CancelPendingLoad();
	CleanupAsyncSave(false);

	m_pipeline = nullptr;
	m_pipelineInputs.clear();

	if( IsCtrPath( name ) )
	{
		m_pipeline = BlueCastPtr( BeResMan->LoadObjectW( name ) );
		if( m_pipeline )
		{
			std::set<std::wstring> resources;
			m_pipeline->GetResourceDependencies( resources );
			for( auto it = begin( resources ); it != end( resources ); ++it )
			{
				Tr2ImageResPtr image;
				BeResMan->GetResource( *it, L"raw", image );
				m_pipelineInputs[*it] = image;
			}
			m_isGood = false;
			m_isPrepared = false;
			m_isLoading = true;
			m_path = name;
			m_ext = ext;

			BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
			return;
		}
	}

	m_mipLevelMaxCount = 255;

	m_isTextureResizable = Tr2Renderer::IsTextureToResize( CW2A( name ) );

	m_isTextureLoadDisabled = Tr2Renderer::IsTextureLoadDisabled();

	BlueAsyncRes::Initialize( name, ext );
}

void TriTextureRes::StaticResourceLoadFinished( void* pContext )
{
	static_cast<TriTextureRes*>( pContext )->m_resourceLoadCbId = 0;
	BeResMan->AddToQueue( BRMQ_MAIN, StaticResourcePrepFinished, pContext, 0, &static_cast<TriTextureRes*>( pContext )->m_resourcePrepCbId );
}

void TriTextureRes::StaticResourcePrepFinished( void* pContext )
{
	static_cast<TriTextureRes*>( pContext )->m_resourcePrepCbId = 0;
	static_cast<TriTextureRes*>( pContext )->ResourcePrepFinished();
}

void TriTextureRes::ResourcePrepFinished()
{
	if( m_pipeline != nullptr)
	{
		m_isLoading = false;
		m_isPrepared = true;
		m_isGood = false;

		ON_BLOCK_EXIT( [&] { m_pipelineInputs.clear(); m_pipeline = nullptr; } );

		std::unordered_map<std::wstring, const ImageIO::HostBitmap*> inputs;
		for( auto it = begin( m_pipelineInputs ); it != end( m_pipelineInputs ); ++it )
		{
			inputs[it->first] = ( it->second && it->second->IsGood() ) ? &it->second->GetBitmap() : nullptr;
		}
		ImageIO::HostBitmap result;
		if( m_pipeline->Execute( result, inputs, Tr2TexturePipelineParams() ) )
		{
			m_ownTexture = Tr2TextureAL();
			SetTexture( m_ownTexture );

			// No need to check for texture load disabled - we wouldn't have gotten here
			// if it were.

			if( !Tr2Renderer::IsResourceCreationAllowed() )
			{
				return;
			}

			Tr2TextureAL face;
			USE_MAIN_THREAD_RENDER_CONTEXT();
			uint32_t memoryUse;
			if( !Tr2ImageIOHelpers::CreateTexture( result, m_ownTexture,
				memoryUse,
				renderContext,
				USAGE_IMMUTABLE ) )
			{
				CCP_LOGWARN( "Tr2ImageHandler failed to create texture '%S'", GetPath() );
				return;
			}

			SetTexture( m_ownTexture );
			++m_resourceRebuiltCounter;
		}
		m_isGood = true;
		NotifyRebuildCachedData();
	}
}

Tr2TexturePipeline* TriTextureRes::GetPipeline() const
{
	return m_pipeline;
}

void TriTextureRes::OnShutdown()
{
	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
	ReleaseResources( TRISTORAGE_ALL );
}

void TriTextureRes::ReleaseResources( TriStorage s )
{
	if( ( m_ownTexture.GetMemoryClass() & s ) != 0 )
	{
		m_ownTexture = Tr2TextureAL();
	}

	if( !m_texture )
	{
		return;
	}

	if( (s & TRISTORAGE_MANAGEDMEMORY) || 
		((s & TRISTORAGE_VIDEOMEMORY) && m_texture->GetMemoryClass() == AL_MEMORY_VIDEO
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
		&& !g_usingEXDevice
#endif
		) )
	{
		CCP_STATS_ADD( textureResBytes, -(int)m_memoryUse );
		m_memoryUse = 0;

		CancelPendingLoad();
		CleanupAsyncSave(false);

		m_ownTexture = Tr2TextureAL();
		m_texture = nullptr;
		m_wrappedRenderTarget = nullptr;
		SetPrepared( false );
		SetGood( false );
	}
}

Tr2TextureAL* TriTextureRes::GetTexture()
{
	if( m_wrappedRenderTarget )
	{
		if( !m_texture || !m_texture->IsValid() )
		{
			SetTexture( m_wrappedRenderTarget->GetRenderTarget() );
		}
	}

	return ( m_texture && m_texture->IsValid() ) ? m_texture : nullptr;
}

const Tr2TextureAL* TriTextureRes::GetTexture() const
{
	return ( m_texture && m_texture->IsValid() ) ? m_texture : nullptr;
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

void TriTextureRes::OnCloseStream()
{
	if( m_loadedBitmap )
	{
		m_cutoutX		= m_metadata.cutout.x;
		m_cutoutY		= m_metadata.cutout.y;
		m_cutoutWidth	= m_metadata.cutout.width;
		m_cutoutHeight	= m_metadata.cutout.height;
	}

	m_loadedBitmap.reset();

	m_data = nullptr;
	m_dataSize = 0;
}

bool TriTextureRes::OnPrepareResources()
{
	if( IsGood() || IsLoading() )
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
	if( m_type == TEX_TYPE_INVALID )
	{
		CCP_LOGERR( "Texture save failed - texture is invalid" );
		return false;
	}
	else if( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
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

	if( !saveBitmap.CreateInstance() ||
		!saveBitmap->CreateFromBitmapDimensions( *this )	||
		!saveBitmap->CopyFromTextureRes( *this, renderContext ) )
	{
		return false;
	}

	return saveBitmap->Save( filename );
}

bool TriTextureRes::SaveAsync( const wchar_t* filename )
{
	// Only permit saving of 2d and cube textures
	if( m_type == TEX_TYPE_INVALID )
	{
		CCP_LOGERR( "Texture save failed - texture is invalid" );
		return false;
	}
	else if( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Texture save failed - only 2d and cubemap textures can be saved" );
		return false;
	}
	
	return StartAsyncSave( filename );
}

bool TriTextureRes::DoPrepareAsyncSave( void )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_asyncSaveImage)
	{
		CCP_LOGERR( "Unsupported extension for saving (%S)", m_saveFilename.c_str() );
		return false;
	}
	if( !ImageIO::IsSaveSupported( m_saveFilename.c_str(), *this ) )
	{
		CCP_LOGERR( "Unsupported format for saving (%S)", m_saveFilename.c_str() );
		return false;
	}
	
	if( !m_asyncSaveBitmap.CreateInstance()																||
		!m_asyncSaveBitmap->CreateFromBitmapDimensions( *this )	||
		!m_asyncSaveBitmap->CopyFromTextureRes( *this, renderContext ) )
	{
		CCP_LOGERR( "Failed to save (%S)", m_saveFilename.c_str() );
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

	if( m_isTextureLoadDisabled )
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

	ImageIO::Result result;
	if( Tr2ImageIOHelpers::IsCairoScriptPath( GetFilePath().c_str() ) )
	{
		result = Tr2ImageIOHelpers::RasterizeCairoScript( *m_loadedBitmap, m_dataStream, m_queryArguments );
	}
	else
	{
		ImageIO::LoadParameters params( m_path.c_str(), ComputeMipSkipCount(), m_mipLevelMaxCount );
		result = ImageIO::ReadImage( *m_dataStream, params, *m_loadedBitmap, &m_metadata );
	}
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
	if( result )
	{
		m_loadedBitmap->GetAverageColor( m_averageColor.r, m_averageColor.g, m_averageColor.b, m_averageColor.a );
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

	m_ownTexture = Tr2TextureAL();
	SetTexture( m_ownTexture );

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
		uint32_t memoryUse;
		if( !Tr2ImageIOHelpers::CreateTexture( *m_loadedBitmap, m_ownTexture, 
												memoryUse, 
												renderContext, 
												USAGE_IMMUTABLE ) )
		{
			CCP_LOGWARN( "Tr2ImageHandler failed to create texture '%S'", GetPath() );
			return false;
		}
		
		isOK = true;
		SetTexture( m_ownTexture );
		++m_resourceRebuiltCounter; 
	}


	return isOK;
}

long TriTextureRes::UpdateSubresource( unsigned left, unsigned top, unsigned right, unsigned bottom, const void* source, unsigned sourcePitch )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( Tr2TextureAL* texture = GetTexture() )
	{
		uint32_t slicePitch = 0;
		return (HRESULT)texture->UpdateSubresource( Tr2TextureSubresource( 0 ).SetRect( left, top, right, bottom ), source, sourcePitch, slicePitch, renderContext );
	}
	return E_FAIL;
}

void TriTextureRes::SetAverageColor( float red, float green, float blue, float alpha )
{
	m_averageColor = Color( red, green, blue, alpha );
}

// ---------------------------------------------------------------


bool TriTextureRes::SetTextureFromRT( Tr2RenderTarget* renderTarget )
{
	m_ownTexture = Tr2TextureAL();
	SetTexture( m_ownTexture );
	m_wrappedRenderTarget = renderTarget;

	if( renderTarget && renderTarget->IsValid() )
	{
		m_isTextureResizable = false;
		SetTexture( renderTarget->GetRenderTarget() );		
	}

	return true;
}

bool TriTextureRes::CreateFromRT( Tr2RenderTarget* renderTarget, unsigned width, unsigned height )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_ownTexture = Tr2TextureAL();
	ON_BLOCK_EXIT( [&]{ SetTexture( m_ownTexture ); } );

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
			CR_RETURN_VAL( m_ownTexture.Create( 
				Tr2BitmapDimensions( width, height, bd.GetTrueMipCount(), rt.GetFormat() ), 
				Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::COPY_DESTINATION,
				Tr2CpuUsage::READ | Tr2CpuUsage::WRITE,
				renderContext ), false );
		}

		Tr2TextureSubresource dst;
		dst.SetRect( 0, 0, width, height );
		if( FAILED( m_ownTexture.CopySubresourceRegion( Tr2TextureSubresource(), *renderTarget, dst, renderContext ) ) )
		{
			m_ownTexture = Tr2TextureAL();
			return false;
		}
	}
	else
	{
		// No mipmaps, just locking the RT and initializing a new texture with its contents using initialData should work.
		Tr2SubresourceData srd;
		if( FAILED( rt.MapForReading( Tr2TextureSubresource( 0 ), srd.m_sysMem, srd.m_sysMemPitch, renderContext ) ) )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { rt.UnmapForReading( renderContext ); } );
	
		srd.m_sysMemSlicePitch = rt.GetHeight() * srd.m_sysMemPitch;

		CR_RETURN_VAL( m_ownTexture.Create( 
			Tr2BitmapDimensions( width, height, 1, rt.GetFormat() ), 
			Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::COPY_DESTINATION,
			Tr2CpuUsage::READ, 
			&srd, 
			renderContext ), false );	
	}

	m_isTextureResizable = false;
	return true;	
}

bool TriTextureRes::CreateFromHostBitmap( Tr2HostBitmap* bitmap )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_ownTexture = Tr2TextureAL();
	SetTexture( m_ownTexture );

	if( !bitmap || !bitmap->IsValid() || !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	uint32_t memoryUse;
	if( !Tr2ImageIOHelpers::CreateTexture( *bitmap, m_ownTexture, memoryUse, renderContext, USAGE_IMMUTABLE ) )
	{
		return false;
	}
	m_isTextureResizable = false;
	SetTexture( m_ownTexture );
	return true;	
}

BlueStdResult TriTextureRes::CreateFromTexture( TriTextureRes* texture )
{
	if( this == texture )
	{
		return BLUE_STD_RESULT_OK;
	}
	if( !texture || !texture->IsGood() || !texture->GetTexture() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "invalid texture" );
	}
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "resource creation is not allowed at this time" );
	}

	auto& other = *texture->GetTexture();

	m_ownTexture = Tr2TextureAL();
	ON_BLOCK_EXIT( [&]{ SetTexture( m_ownTexture ); } );

	auto width = other.GetWidth();
	auto height = other.GetHeight();

	USE_MAIN_THREAD_RENDER_CONTEXT();
	
	Tr2BitmapDimensions bd( other.GetDesc() );

	CR_RETURN_VAL( m_ownTexture.Create( 
		Tr2BitmapDimensions( width, height, other.GetTrueMipCount(), other.GetFormat() ), 
			Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::COPY_DESTINATION,
			Tr2CpuUsage::READ, 
			renderContext ),
		BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "could not create a texture" ) );

	CR_RETURN_VAL( 
		m_ownTexture.CopySubresourceRegion( Tr2TextureSubresource(), other, Tr2TextureSubresource(), renderContext ),
		BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "could not copy a texture" ) );
	m_isTextureResizable = false;
	return BLUE_STD_RESULT_OK;
}

bool TriTextureRes::Create(	uint32_t width, 
							uint32_t height, 
							uint32_t mipCount, 
							PixelFormat format, 
							BufferUsageFlags usage,
							Tr2PrimaryRenderContext& renderContext )
{
	m_ownTexture = Tr2TextureAL();
	SetTexture( m_ownTexture );


	auto gpuUsage = Tr2GpuUsage::SHADER_RESOURCE;
	auto cpuUsage = Tr2CpuUsage::NONE;

	if( ( usage & Tr2RenderContextEnum::USAGE_IMMUTABLE ) != 0 )
	{
		cpuUsage = cpuUsage | Tr2CpuUsage::READ;
	}
	else if( ( usage & Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY ) != 0 )
	{
		cpuUsage = cpuUsage | Tr2CpuUsage::WRITE_OFTEN;
	}
	else
	{
		cpuUsage = cpuUsage | Tr2CpuUsage::WRITE;
	}
	if( ( usage & Tr2RenderContextEnum::USAGE_CPU_READ ) != 0 )
	{
		cpuUsage = cpuUsage | Tr2CpuUsage::READ;
	}
	if( ( usage & Tr2RenderContextEnum::USAGE_UNORDERED_ACCESS ) != 0 )
	{
		gpuUsage = gpuUsage | Tr2GpuUsage::UNORDERED_ACCESS;
	}
	if( ( usage & Tr2RenderContextEnum::USAGE_SHADER_RESOURCE ) != 0 )
	{
		gpuUsage = gpuUsage | Tr2GpuUsage::SHARED;
	}

	CR_RETURN_VAL( m_ownTexture.Create(		
		Tr2BitmapDimensions( width, height, mipCount, format ), 
		gpuUsage,
		cpuUsage,
		renderContext ), false );

	*static_cast<Tr2BitmapDimensions*>( this ) = m_ownTexture.GetDesc();

	m_memoryUse = 0;
	auto trueMipCount = GetTrueMipCount();
	for( uint32_t i = 0; i < trueMipCount; ++i )
	{
		m_memoryUse += GetMipSize( i );
	}
	m_memoryUse *= std::max( 1u, GetArraySize() );

	CCP_STATS_ADD( textureResBytes, m_memoryUse );
	SetTexture( m_ownTexture );
	SetPrepared( true );
	SetGood( true );
	return true;
}

ALResult TriTextureRes::OpenShared( uintptr_t handle )
{
	m_ownTexture = Tr2TextureAL();
	SetTexture( m_ownTexture );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	CR_RETURN_HR( m_ownTexture.OpenShared( handle, Tr2GpuUsage::SHADER_RESOURCE, renderContext ) );

	*static_cast<Tr2BitmapDimensions*>( this ) = m_ownTexture.GetDesc();

	m_memoryUse = 0;
	auto trueMipCount = GetTrueMipCount();
	for( uint32_t i = 0; i < trueMipCount; ++i )
	{
		m_memoryUse += GetMipSize( i );
	}
	m_memoryUse *= std::max( 1u, GetArraySize() );

	CCP_STATS_ADD( textureResBytes, m_memoryUse );
	SetTexture( m_ownTexture );
	SetPrepared( true );
	SetGood( true );
	return S_OK;
}

// ---------------------------------------------------------------
bool TriTextureRes::SetTexture( Tr2TextureAL& texture )
{
	if( m_memoryUse )
	{
		CCP_STATS_ADD( textureResBytes, -int( m_memoryUse ) );
		m_memoryUse = 0;
	}

	if( !texture.IsValid()  )
	{
		m_texture = nullptr;
		SetPrepared( false );
		SetGood( false );

		Tr2BitmapDimensions::Destroy();
		return true;
	}

	*static_cast<Tr2BitmapDimensions*>( this ) = texture.GetDesc();
	m_texture = &texture;
		
	// Estimate memory use for the resource cache.
	m_memoryUse = 0;
	auto mipCount = GetTrueMipCount();
	for( uint32_t i = 0; i < mipCount; ++i )
	{
		m_memoryUse += GetMipSize( i );
	}
	m_memoryUse *= std::max( 1u, GetArraySize() );
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
	return false;
}
