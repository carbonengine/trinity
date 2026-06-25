// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dScene.h"

Tr2Sprite2dTexture::Tr2Sprite2dTexture( IRoot* lockobj /*= NULL */ ) :
	m_hasTextureWindow( false ),
	m_srcX( 0 ),
	m_srcY( 0 ),
	m_srcWidth( 0 ),
	m_srcHeight( 0 ),
	m_settings( S2D_TS_NONE ),
	m_useTransform( false ),
	m_translation( 0.0f, 0.0f ),
	m_rotationCenter( 0.5f, 0.5f ),
	m_rotation( 0.0f ),
	m_scalingCenter( 0.5f, 0.5f ),
	m_scalingRotation( 0.0f ),
	m_scale( 1.0f, 1.0f ),
	m_changeListeners( "Tr2Sprite2dTexture/m_changeListeners" )
{
}

Tr2Sprite2dTexture::~Tr2Sprite2dTexture()
{
	if( m_atlasTexture )
	{
		m_atlasTexture->UnregisterForChangeNotification( this );
	}
}

Tr2AtlasTexture* Tr2Sprite2dTexture::GetAtlasTexture() const
{
	return m_atlasTexture;
}

void Tr2Sprite2dTexture::SetAtlasTexture( Tr2AtlasTexture* val )
{
	if( m_atlasTexture )
	{
		m_atlasTexture->UnregisterForChangeNotification( this );
	}
	m_atlasTexture = val;
	if( m_atlasTexture )
	{
		m_atlasTexture->RegisterForChangeNotification( this );
	}

	m_resPath = "";
	SetDirty();
}

std::string Tr2Sprite2dTexture::GetResPath() const
{
	return m_resPath;
}

void Tr2Sprite2dTexture::SetResPath( const std::string& path )
{
	m_resPath = path;

	if( m_atlasTexture )
	{
		m_atlasTexture->UnregisterForChangeNotification( this );
	}
	m_atlasTexture.Unlock();

	BeResMan->GetResource( m_resPath.c_str(), "atlas", m_atlasTexture );

	if( m_atlasTexture )
	{
		m_atlasTexture->RegisterForChangeNotification( this );
	}

	SetDirty();
}

bool Tr2Sprite2dTexture::Initialize()
{
	if( !m_resPath.empty() )
	{
		SetResPath( m_resPath );
	}

	return true;
}

void Tr2Sprite2dTexture::Apply( Tr2Sprite2dScene* renderer, unsigned int ix )
{
	renderer->SetTexture( ix, m_atlasTexture, m_settings );

	if( m_hasTextureWindow )
	{
		renderer->SetTextureWindow( ix, m_srcX, m_srcY, m_srcWidth, m_srcHeight );
	}
	renderer->SetTextureTransform( ix, GetTransform() );
	renderer->SetTileMode( m_settings );
}

unsigned int Tr2Sprite2dTexture::GetWidth() const
{
	if( m_atlasTexture )
	{
		return m_atlasTexture->GetWidth();
	}

	return 0;
}

unsigned int Tr2Sprite2dTexture::GetHeight() const
{
	if( m_atlasTexture )
	{
		return m_atlasTexture->GetHeight();
	}

	return 0;
}

Matrix* Tr2Sprite2dTexture::GetTransform()
{
	if( m_useTransform )
	{
		m_transform = Transformation2DMatrix( &m_scalingCenter, m_scalingRotation, &m_scale, &m_rotationCenter, m_rotation, &m_translation );

		return &m_transform;
	}

	return NULL;
}

bool Tr2Sprite2dTexture::IsLoading() const
{
	if( m_atlasTexture )
	{
		return m_atlasTexture->IsLoading();
	}

	return false;
}

bool Tr2Sprite2dTexture::IsGood() const
{
	if( m_atlasTexture )
	{
		return m_atlasTexture->IsGood();
	}

	return false;
}

float Tr2Sprite2dTexture::GetSrcX() const
{
	return m_srcX;
}

void Tr2Sprite2dTexture::SetSrcX( float srcX )
{
	m_srcX = srcX;

	CheckTextureWindow();
	SetDirty();
}

float Tr2Sprite2dTexture::GetSrcY() const
{
	return m_srcY;
}

void Tr2Sprite2dTexture::SetSrcY( float srcY )
{
	m_srcY = srcY;

	CheckTextureWindow();
	SetDirty();
}

float Tr2Sprite2dTexture::GetSrcWidth() const
{
	return m_srcWidth;
}

void Tr2Sprite2dTexture::SetSrcWidth( float srcWidth )
{
	m_srcWidth = srcWidth;

	CheckTextureWindow();
	SetDirty();
}

float Tr2Sprite2dTexture::GetSrcHeight() const
{
	return m_srcHeight;
}

void Tr2Sprite2dTexture::SetSrcHeight( float srcHeight )
{
	m_srcHeight = srcHeight;

	CheckTextureWindow();
	SetDirty();
}

void Tr2Sprite2dTexture::CheckTextureWindow()
{
	if( ( m_srcX == 0.0f ) && ( m_srcY == 0.0f ) && ( m_srcWidth == 0.0f ) && ( m_srcHeight == 0.0f ) )
	{
		m_hasTextureWindow = false;
		return;
	}

	if( m_atlasTexture &&
		( m_srcX == 0.0f ) &&
		( m_srcY == 0.0f ) &&
		( m_srcWidth == m_atlasTexture->GetWidth() ) &&
		( m_srcHeight == m_atlasTexture->GetHeight() ) )
	{
		m_hasTextureWindow = false;
		return;
	}

	m_hasTextureWindow = true;
}

int Tr2Sprite2dTexture::GetTextureRepeatMode() const
{
	int repeatMode = m_settings & ( S2D_TS_REPEAT_MIRROR | S2D_TS_REPEAT_CLAMP );
	return repeatMode >> 1;
}

void Tr2Sprite2dTexture::SetTextureRepeatMode( int repeatMode )
{
	int rest = m_settings & ~( S2D_TS_REPEAT_MIRROR | S2D_TS_REPEAT_CLAMP );
	m_settings = (Tr2Sprite2dTextureSettings)( rest | repeatMode );
	SetDirty();
}

bool Tr2Sprite2dTexture::GetTileX() const
{
	return ( m_settings & S2D_TS_TILE_X ) != 0;
}

void Tr2Sprite2dTexture::SetTileX( bool val )
{
	int settings = m_settings;
	if( val )
	{
		settings |= S2D_TS_TILE_X;
	}
	else
	{
		settings &= ~S2D_TS_TILE_X;
	}

	m_settings = (Tr2Sprite2dTextureSettings)settings;
	SetDirty();
}

bool Tr2Sprite2dTexture::GetTileY() const
{
	return ( m_settings & S2D_TS_TILE_Y ) != 0;
}

void Tr2Sprite2dTexture::SetTileY( bool val )
{
	int settings = m_settings;
	if( val )
	{
		settings |= S2D_TS_TILE_Y;
	}
	else
	{
		settings &= ~S2D_TS_TILE_Y;
	}
	m_settings = (Tr2Sprite2dTextureSettings)settings;
	SetDirty();
}

void Tr2Sprite2dTexture::RegisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p )
{
	m_changeListeners.insert( p );
}

void Tr2Sprite2dTexture::UnregisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p )
{
	m_changeListeners.erase( p );
}

bool Tr2Sprite2dTexture::OnModified( Be::Var* value )
{
	SetDirty();
	return true;
}

void Tr2Sprite2dTexture::SetDirty()
{
	for( auto it = m_changeListeners.begin(); it != m_changeListeners.end(); ++it )
	{
		( *it )->Sprite2dTextureChanged( this );
	}
}

void Tr2Sprite2dTexture::AtlasTextureChanged( Tr2AtlasTexture* p )
{
	SetDirty();
}
