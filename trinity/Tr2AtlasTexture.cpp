// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Renderer.h"
#include "Tr2TextureAtlas.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2HostBitmap.h"
#include "Tr2ImageIOHelpers.h"
#include "Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;

Tr2AtlasTexture::Tr2AtlasTexture( IRoot* lockobj ) :
	m_x( 0 ),
	m_y( 0 ),
	m_width( 0 ),
	m_height( 0 ),
	m_widthReciprocal( 0.0f ),
	m_heightReciprocal( 0.0f ),
	m_textureWidth( 0 ),
	m_textureHeight( 0 ),
	m_textureWidthReciprocal( 0.0f ),
	m_textureHeightReciprocal( 0.0f ),
	m_atlasArea( NULL ),
	m_data( NULL ),
	m_dataSize( 0 ),
	m_renderTarget( nullptr ),
	m_memoryUsage( 0 ),
	m_isLocked( false ),
	m_isStandAlone( false ),
	m_changeListeners( "Tr2AtlasTexture/m_changeListeners" )
{
}

Tr2AtlasTexture::~Tr2AtlasTexture()
{
	if( m_isLocked )
	{
		CCP_LOGWARN( "Tr2AtlasTexture locked upon destruction - unlock before destroying" );
		UnlockBuffer();
	}
}

Tr2TextureAL* Tr2AtlasTexture::GetTexture()
{
	// are we part of an atlas? If so, go get the texture from that to guarantee pointer identity.
	if( m_textureAtlas && m_atlasArea )
	{
		return m_textureAtlas->GetTexture();
	}

	if( m_texture.IsValid() )
	{
		return &m_texture;
	}

	if( m_textureRes && m_textureRes->IsGood() )
	{
		return m_textureRes->GetTexture();
	}

	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns texture atlas render target if the atlas was created for render targets or
//   nullptr otherwise.
// Return Value:
//   texture atlas render target
// --------------------------------------------------------------------------------------
Tr2TextureAL* Tr2AtlasTexture::GetRenderTarget()
{
	return m_renderTarget;
}

void Tr2AtlasTexture::SetRenderTarget( Tr2TextureAL* rt )
{
	m_renderTarget = rt;
	m_isGood = 1;
	m_isPrepared = 1;
	if( m_renderTarget )
	{
		m_width = rt->GetWidth();
		m_textureWidth = m_width;
		m_height = rt->GetHeight();
		m_textureHeight = m_height;

		CalcReciprocals();

		m_textureWindow.x = 0.0f;
		m_textureWindow.y = 0.0f;
		m_textureWindow.z = 1.0f;
		m_textureWindow.w = 1.0f;
	}
}

void Tr2AtlasTexture::GetTextureWindow( Vector4& tw )
{
	tw = m_textureWindow;
}

void Tr2AtlasTexture::CalcTextureWindow()
{
	CalcSubTextureWindow( m_textureWindow, 0, 0, 0, 0 );
}

void Tr2AtlasTexture::CalcSubTextureWindow( Vector4& tw, float rectX, float rectY, float rectWidth, float rectHeight )
{
	Tr2TextureAL* tex = GetTexture();

	if( !tex )
	{
		tw.x = 0.0f;
		tw.y = 0.0f;
		tw.w = 1.0f;
		tw.z = 1.0f;

		return;
	}

	if( rectWidth == 0.0f )
	{
		rectWidth = (float)m_width;
	}

	if( rectHeight == 0 )
	{
		rectHeight = (float)m_height;
	}

	tw.x = (float)( m_x + rectX ) * m_textureWidthReciprocal;
	tw.y = (float)( m_y + rectY ) * m_textureHeightReciprocal;
	tw.z = (float)rectWidth * m_textureWidthReciprocal;
	tw.w = (float)rectHeight * m_textureHeightReciprocal;
}

void Tr2AtlasTexture::OnShutdown()
{
	if( m_textureAtlas )
	{
		m_textureAtlas->RemoveFromAtlas( this );
	}
	ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2AtlasTexture::OnPrepareResources()
{
	if( IsPrepared() || IsLoading() )
	{
		return true;
	}

	Initialize( m_path.c_str(), m_ext.c_str() );
	return true;
}

void Tr2AtlasTexture::ReleaseResources( TriStorage s )
{
	Tr2TextureAL* texture = nullptr;
	if( m_textureAtlas && m_atlasArea )
	{
		texture = m_textureAtlas->GetTexture();
	}
	else if( m_texture.IsValid() )
	{
		texture = &m_texture;
	}

	if( texture && texture->GetMemoryClass() & s )
	{
		// When managed memory is freed, both standalone textures and the
		// ones resident in an atlas are purged.
		CancelPendingLoad();

		m_texture = Tr2TextureAL();
		CCP_DELETE m_atlasArea;
		m_atlasArea = NULL;

		SetPrepared( false );
	}
}

BlueAsyncRes::LoadingResult Tr2AtlasTexture::DoLoad()
{
	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	m_loadedBitmap.reset( CCP_NEW( "Tr2AtlasTexture::m_loadedBitmap" ) ImageIO::HostBitmap );

	ImageIO::Result result = ImageIO::ReadImage( *m_dataStream, ImageIO::LoadParameters( m_path.c_str() ), *m_loadedBitmap );

	if( result )
	{
		m_loadedBitmap->ConvertFormat( PIXEL_FORMAT_B8G8R8A8_UNORM );
	}
	else
	{
		CCP_LOGWARN( "Tr2AtlasTexture: failed to load '%S' - %s", m_path.c_str(), result.GetErrorMessage().c_str() );
	}
	return result ? LR_SUCCESS : LR_FAILED;
}

bool Tr2AtlasTexture::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_textureAtlas )
	{
		// In case we're reloading
		m_textureAtlas->RemoveFromAtlas( this );
	}
	// Allow loading to a specific atlas by setting this before DoLoad
	if( !m_textureAtlas )
	{
		m_textureAtlas = g_textureAtlasMan->FindAtlas( m_loadedBitmap->GetFormat() );
	}

	m_texture = Tr2TextureAL();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	bool isOK = false;
	if( m_textureAtlas && !m_isStandAlone )
	{
		isOK = m_textureAtlas->DoPrepare( this );

		// Estimate memory usage based on 32-bit textures (the average case for atlas textures)
		m_memoryUsage = m_width * m_height * 4;
	}

	if( !isOK )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( Tr2ImageIOHelpers::Create2DTexture( *m_loadedBitmap, m_texture, m_memoryUsage, renderContext ) )
		{
			m_x = 0;
			m_y = 0;
			m_width = m_loadedBitmap->GetWidth();
			m_height = m_loadedBitmap->GetHeight();
			m_textureWidth = m_width;
			m_textureHeight = m_height;

			isOK = true;

			if( m_textureAtlas )
			{
				// An appropriate texture atlas was found but it didn't have space.
				// Register with it, and it may pull the texture in at some point.
				m_textureAtlas->RegisterOutsider( this );
			}
		}
	}

	FinalizePrepare();

	SetGood( isOK );

	m_loadedBitmap.reset();

	return isOK;
}

unsigned int Tr2AtlasTexture::GetX() const
{
	return m_x;
}

unsigned int Tr2AtlasTexture::GetY() const
{
	return m_y;
}

unsigned int Tr2AtlasTexture::GetWidth() const
{
	return m_width;
}

float Tr2AtlasTexture::GetWidthReciprocal() const
{
	return m_widthReciprocal;
}

unsigned int Tr2AtlasTexture::GetHeight() const
{
	return m_height;
}

float Tr2AtlasTexture::GetHeightReciprocal() const
{
	return m_heightReciprocal;
}

unsigned int Tr2AtlasTexture::GetTextureWidth() const
{
	return m_textureWidth;
}

unsigned int Tr2AtlasTexture::GetTextureHeight() const
{
	return m_textureHeight;
}

bool Tr2AtlasTexture::LockBuffer( void*& pData, unsigned int& pitch )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2TextureAL* texture = nullptr;
	if( m_textureAtlas && m_atlasArea )
	{
		texture = m_textureAtlas->GetTexture();
	}
	else if( m_texture.IsValid() )
	{
		texture = &m_texture;
	}

	if( !texture || !texture->IsValid() )
	{
		return false;
	}

	if( m_isLocked )
	{
		CCP_LOGERR( "Tr2AtlasTexture::LockBuffer failed - texture is already locked" );
		return false;
	}

	long hr = texture->MapForWriting(
						 Tr2TextureSubresource( 0 ).SetRect( uint32_t( m_x ), uint32_t( m_y ), uint32_t( m_x + m_width ), uint32_t( m_y + m_height ) ),
						 pData,
						 pitch,
						 renderContext )
				  .GetResult();

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2AtlasTexture::LockBuffer failed - another atlas texture may be locked already (HR: %08lx)", hr );
		return false;
	}

	m_isLocked = true;

	return true;
}

bool Tr2AtlasTexture::LockBufferAndMargin( void*& data, unsigned& pitch, unsigned& margin )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2TextureAL* texture = nullptr;
	if( m_textureAtlas && m_atlasArea )
	{
		texture = m_textureAtlas->GetTexture();
	}
	else if( m_texture.IsValid() )
	{
		texture = &m_texture;
	}

	if( !m_textureAtlas || !m_atlasArea || m_isLocked || !texture || !texture->IsValid() )
	{
		margin = 0;
		return LockBuffer( data, pitch );
	}

	margin = m_textureAtlas->GetMargin();
	long hr = texture->MapForWriting(
						 Tr2TextureSubresource( 0 ).SetRect( uint32_t( m_x - margin ), uint32_t( m_y - margin ), uint32_t( m_x + m_width + margin ), uint32_t( m_y + m_height + margin ) ),
						 data,
						 pitch,
						 renderContext )
				  .GetResult();

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2AtlasTexture::LockBufferAndMargin failed - another atlas texture may be locked already (HR: %08lx)", hr );
		return false;
	}

	m_isLocked = true;

	return true;
}

void Tr2AtlasTexture::UnlockBuffer()
{
	if( !m_isLocked )
	{
		CCP_LOGWARN( "Tr2AtlasTexture::UnlockBuffer failed - texture is not locked" );
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2TextureAL* texture = nullptr;
	if( m_textureAtlas && m_atlasArea )
	{
		texture = m_textureAtlas->GetTexture();
	}
	else if( m_texture.IsValid() )
	{
		texture = &m_texture;
	}

	if( !texture )
	{
		return;
	}

	texture->UnmapForWriting( renderContext );

	m_isLocked = false;
	SetGood( true );

	NotifyListenersOfChange();
}

bool Tr2AtlasTexture::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2AtlasTexture::GetMemoryUsage()
{
	return m_memoryUsage;
}

void Tr2AtlasTexture::CalcReciprocals()
{
	if( m_width )
	{
		m_widthReciprocal = 1.0f / (float)m_width;
	}
	if( m_height )
	{
		m_heightReciprocal = 1.0f / (float)m_height;
	}
	if( m_textureWidth )
	{
		m_textureWidthReciprocal = 1.0f / (float)m_textureWidth;
	}
	if( m_textureHeight )
	{
		m_textureHeightReciprocal = 1.0f / (float)m_textureHeight;
	}
}

void Tr2AtlasTexture::FinalizePrepare()
{
	CalcReciprocals();
	CalcTextureWindow();

	NotifyListenersOfChange();
}

void Tr2AtlasTexture::RegisterForChangeNotification( ITr2AtlasTextureNotifyTarget* p )
{
	m_changeListeners.insert( p );
}

void Tr2AtlasTexture::UnregisterForChangeNotification( ITr2AtlasTextureNotifyTarget* p )
{
	m_changeListeners.erase( p );
}

void Tr2AtlasTexture::SetStandAlone( bool b )
{
	m_isStandAlone = b;

	if( m_isStandAlone )
	{
		if( m_textureAtlas )
		{
			m_textureAtlas->EjectTexture( this );
		}
	}
}

bool Tr2AtlasTexture::IsStandAlone() const
{
	return m_isStandAlone;
}

void Tr2AtlasTexture::NotifyListenersOfChange()
{
	for( auto it = m_changeListeners.begin(); it != m_changeListeners.end(); ++it )
	{
		( *it )->AtlasTextureChanged( this );
	}
}

void Tr2AtlasTexture::SetTargetAtlasBeforeLoad( Tr2TextureAtlas* atlas )
{
	if( m_textureAtlas )
	{
		return;
	}
	m_textureAtlas = atlas;
}

void Tr2AtlasTexture::SetTextureRes( TriTextureRes* p )
{
	// TODO: What if the texture has already been loaded?
	// This is intended to be used on a newly created object
	m_textureRes = p;
	SetPrepared( true );
	SetGood( true );

	m_x = 0;
	m_y = 0;
	m_width = m_textureRes->GetWidth();
	m_textureWidth = m_width;
	m_height = m_textureRes->GetHeight();
	m_textureHeight = m_height;

	CalcReciprocals();
	CalcTextureWindow();
}

TriTextureRes* Tr2AtlasTexture::GetTextureRes()
{
	return m_textureRes;
}
