#include "StdAfx.h"

#if BINK_ENABLED

#include "Tr2Sprite2dBinkTexture.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Renderer.h"
#include "BinkOutputStream.h"

static bool isBinkInitialized = false;

Tr2Sprite2dBinkTexture::Tr2Sprite2dBinkTexture( IRoot* lockobj /*= NULL */ ) :
	m_binkHandle( NULL ),
	m_isPaused( false ),
	m_isMuted( false ),
	m_isFinished( false ),
	m_isGood( false ),
	m_loop( false ),
	m_srcX( 0.0f ),
	m_srcY( 0.0f ),
	m_srcWidth( 0.0f ),
	m_srcHeight( 0.0f ),
	m_originalVideoWidth( 0 ),
	m_originalVideoHeight( 0 ),
	m_videoFps( 0 ),
	m_currentFrame( 0 ),
	m_copyAllNextFrame( true ),
	m_outputChannel( -1 ),
	m_changeListeners( "Tr2Sprite2dBinkTexture/m_changeListeners" )
{
	GetTr2Sprite2dBinkTextures().insert( this );

}

Tr2Sprite2dBinkTexture::~Tr2Sprite2dBinkTexture()
{
	GetTr2Sprite2dBinkTextures().erase( this );
	BinkClose( m_binkHandle );
	m_binkHandle = 0;
}

std::string Tr2Sprite2dBinkTexture::GetResPath() const
{
	return m_resPath;
}

void Tr2Sprite2dBinkTexture::SetResPath( const std::string& path )
{
	m_resPath = path;
	// Open the video
	OpenBinkVideo();
}

void Tr2Sprite2dBinkTexture::Pause()
{
	if( m_binkHandle )
	{
		BinkPause(m_binkHandle, 1);
	}

	m_isPaused = true;
}

void Tr2Sprite2dBinkTexture::Play()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_binkHandle )
	{
		return;
	}

	if ( m_binkHandle->FrameNum == m_binkHandle->Frames )
	{
		BinkGoto( m_binkHandle, 0, NULL );
	}

	if( m_isPaused )
	{
		BinkPause( m_binkHandle, 0 );
	}

	m_isPaused = false;
	m_isFinished = false;
}

void Tr2Sprite2dBinkTexture::Rewind()
{
	if( !m_binkHandle )
	{
		return;
	}
	BinkGoto( m_binkHandle, 0, NULL );
}

void Tr2Sprite2dBinkTexture::MuteAudio()
{
	if( m_binkHandle)
	{
		BinkSetVolume( m_binkHandle, 0, 0 );
	}
	m_isMuted = true;
}

void Tr2Sprite2dBinkTexture::UnmuteAudio()
{
	if( m_binkHandle )
	{
		BinkSetVolume( m_binkHandle, 0, 32768 );
	}
	m_isMuted = false;
}

void Tr2Sprite2dBinkTexture::Update( Be::Time time )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_isPaused || !m_binkHandle || BinkWait( m_binkHandle ) )
	{
		return;
	}

	if( m_atlasTexture && ( !m_atlasTexture->GetTexture() || !m_atlasTexture->GetTexture()->IsValid() ) )
	{
		m_atlasTexture = nullptr;
	}

	if( !m_atlasTexture )
	{
		Tr2TextureAtlas* atlas = g_textureAtlasMan->FindAtlas( Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
		if( atlas )
		{
			atlas->CreateTexture( m_originalVideoWidth, m_originalVideoHeight, atlas->ATT_VIDEO, &m_atlasTexture );
		}
	}

	if( !m_atlasTexture )
	{
		return;
	}

	SetDirty();

	{
		CCP_STATS_ZONE( "Bink Frame Update" );
		// Wait for async completion for as long as it takes
		BinkDoFrameAsyncWait( m_binkHandle, -1 );
	}

	{
		CCP_STATS_ZONE( "Bink Buffer Update" );
		void* data;
		unsigned int pitch;
		if( m_atlasTexture->LockBuffer( data, pitch ) )
		{
			// Copy
			// Note that buffer locking is now always write discard. So use BINKCOPYALL.
			BinkCopyToBuffer( 
				m_binkHandle,
				data,
				pitch,
				m_binkHandle->Height,
				0, 0,
				BINKSURFACE32A | BINKCOPYNOSCALING | BINKNOSKIP | /*( m_copyAllNextFrame ? BINKCOPYALL : 0 )*/ BINKCOPYALL );

			m_atlasTexture->UnlockBuffer();
		}
	}

	m_copyAllNextFrame = false;
	m_currentFrame = m_binkHandle->FrameNum;

	if ( m_binkHandle->FrameNum == m_binkHandle->Frames && !m_loop )
	{
		m_isPaused = true;
		m_isFinished = true;
		BinkPause(m_binkHandle, 1);
		return;
	}

	BinkNextFrame( m_binkHandle );
}

void Tr2Sprite2dBinkTexture::Apply( Tr2Sprite2dScene* renderer, unsigned int ix )
{
	uint32_t settings = S2D_TS_NONE;

	renderer->SetTexture( ix, m_atlasTexture, (Tr2Sprite2dTextureSettings)settings );
	renderer->SetTextureWindow( ix, m_srcX, m_srcY, m_srcWidth, m_srcHeight );
	renderer->SetTextureTransform( ix, GetTransform() );
}

Tr2AtlasTexture* Tr2Sprite2dBinkTexture::GetAtlasTexture() const
{
	return m_atlasTexture;
}

unsigned int Tr2Sprite2dBinkTexture::GetWidth() const
{
	return m_originalVideoWidth;
}

unsigned int Tr2Sprite2dBinkTexture::GetHeight() const
{
	return m_originalVideoHeight;
}

void Tr2Sprite2dBinkTexture::OpenBinkVideo()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_binkHandle )
	{
		BinkClose( m_binkHandle );
		m_binkHandle = NULL;
	}

	if( !isBinkInitialized )
	{
		// Start a worker thread to decompress frames
		BinkStartAsyncThread( 0, NULL );

		isBinkInitialized = true;
	}

	// Reset the other settings
	m_isPaused = true;
	m_isFinished = false;
	m_isGood = false;

	if( !m_resPath.empty() )
	{
		SetBinkOutputChannel( m_outputChannel );

		m_originalVideoWidth = 0;
		m_originalVideoHeight = 0;
		m_videoFps = 0;
		m_isFinished = true;


		std::wstring resPathW = CA2W( m_resPath.c_str() );

		std::wstring filename = BePaths->ResolvePathW( resPathW );

		// open filename as a relative path by explicitly changing the working directory first.
		// This allows unicode in the path.

		std::wstring unicodePath( filename.c_str() );
		std::wstring relativeFile( unicodePath );
		size_t slash = unicodePath.find_last_of( L"/\\" );
		if( slash != unicodePath.npos )
		{
			unicodePath.erase( slash );
			relativeFile.erase( 0, slash+1 );
		}
		
		const unsigned MAX_FILENAME_SIZE = 260;
		wchar_t originalCwd[ MAX_FILENAME_SIZE ];

		if( !unicodePath.empty() )
		{
			if( !_wgetcwd( originalCwd, MAX_FILENAME_SIZE ) )
			{
				return;
			}

			_wchdir( unicodePath.c_str() );
		}
		ON_BLOCK_EXIT( [&]{ if( !unicodePath.empty() ) { _wchdir( originalCwd ); } } );

		m_binkHandle = BinkOpen( CW2A(relativeFile.c_str()), BINKSNDTRACK | BINKALPHA );
		if( !m_binkHandle )
		{
			CCP_LOGERR( "Video not found or not valid: '%S'", filename.c_str() );
			return;
		}

		m_isGood = true;

		m_originalVideoWidth = m_binkHandle->Width;
		m_originalVideoHeight = m_binkHandle->Height;
		m_videoFps = m_binkHandle->FrameRate / m_binkHandle->FrameRateDiv;
		m_currentFrame = 0;

		// Retain any mute settings
		if( m_isMuted )
		{
			MuteAudio();
		}
	}

}

Matrix* Tr2Sprite2dBinkTexture::GetTransform()
{
	// TODO: Do we want to support full transformation here?
	return NULL;
}

float Tr2Sprite2dBinkTexture::GetDuration()
{
	if( m_binkHandle )
	{
		float framerate = (float)m_binkHandle->FrameRate/(float)m_binkHandle->FrameRateDiv;
		return (float)m_binkHandle->Frames / framerate;
	}
	else
	{
		return 0.0f;
	}
}

void Tr2Sprite2dBinkTexture::UpdateAllBinkBideos()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2Sprite2dBinkTexturesSet& binkTextureSet = GetTr2Sprite2dBinkTextures();
	for( Tr2Sprite2dBinkTexturesSet::iterator it = binkTextureSet.begin(); it != binkTextureSet.end(); ++it )
	{
		Tr2Sprite2dBinkTexture* binkTexture = (*it);
		binkTexture->StartBinkFrameAsyncDecompression();
	}
}

void Tr2Sprite2dBinkTexture::StartBinkFrameAsyncDecompression()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_isPaused || !m_binkHandle )
	{
		return;
	}

	U32 lastFrame = m_binkHandle->FrameNum - 1;

	while( BinkShouldSkip( m_binkHandle ) && lastFrame != m_binkHandle->FrameNum )
	{
		BinkNextFrame( m_binkHandle );

		if ( m_binkHandle->FrameNum > m_binkHandle->Frames )
			return;

		lastFrame = m_binkHandle->FrameNum;
	}
	
	BinkDoFrameAsync( m_binkHandle, 0, 0 );
}

Tr2Sprite2dBinkTexture::Tr2Sprite2dBinkTexturesSet& Tr2Sprite2dBinkTexture::GetTr2Sprite2dBinkTextures()
{
	static NeverEndingSingleton<Tr2Sprite2dBinkTexture::Tr2Sprite2dBinkTexturesSet> UIBinkTextures;
	return UIBinkTextures.GetInstance();
}

void Tr2Sprite2dBinkTexture::RegisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p )
{
	m_changeListeners.insert( p );
}

void Tr2Sprite2dBinkTexture::UnregisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p )
{
	m_changeListeners.erase( p );
}

bool Tr2Sprite2dBinkTexture::OnModified( Be::Var* value )
{
	SetDirty();
	return true;
}

void Tr2Sprite2dBinkTexture::SetDirty()
{
	for( auto it = m_changeListeners.begin(); it != m_changeListeners.end(); ++it )
	{
		(*it)->Sprite2dTextureChanged( this );
	}
}

#endif
