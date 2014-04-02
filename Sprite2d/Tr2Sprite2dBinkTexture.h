//////////////////////////////////////////////////////////////////////////
//
// Created: December 2010
// Copyright CCP 2010
//

#pragma once
#include "include/ITr2Updateable.h"
#include "ITr2Sprite2dTexture.h"

BLUE_DECLARE( Tr2AtlasTexture );
BLUE_DECLARE( Tr2Sprite2dBinkTexture );



// The Tr2Sprite2dBinkTexture wraps an atlas texture that is used for
// decompressing Bink movies.
class Tr2Sprite2dBinkTexture:
	public ITr2Sprite2dTexture,
	public ITr2Updateable
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dBinkTexture( IRoot* lockobj = NULL );
	~Tr2Sprite2dBinkTexture();

	// Set of created particle systems
	typedef std::set<Tr2Sprite2dBinkTexture *> Tr2Sprite2dBinkTexturesSet;
	// Gets a global list of 2D bink textures
	static Tr2Sprite2dBinkTexturesSet& GetTr2Sprite2dBinkTextures();
	static void UpdateAllBinkBideos();

	std::string GetResPath() const;
	void SetResPath( const std::string& path );

	void Pause();
	void Play();
	void Rewind();
	void MuteAudio();
	void UnmuteAudio();

	// Duration of video, in seconds
	float GetDuration();
	void StartBinkFrameAsyncDecompression();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Updateable
	void Update( Be::Time time );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Sprite2dTexture
	void Apply( Tr2Sprite2dScene* renderer, unsigned int ix );

	Tr2AtlasTexture* GetAtlasTexture() const;

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

	float GetSrcX() const { return m_srcX; }
	float GetSrcY() const { return m_srcY; }
	float GetSrcWidth() const { return m_srcWidth; }
	float GetSrcHeight() const { return m_srcHeight; }

	Matrix* GetTransform();

	bool GetDoDotProduct() const { return false; }
	int GetTextureRepeatMode() const { return 0; }

	bool IsLoading() const { return false; }
	bool IsGood() const { return m_isGood; }

	void RegisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p );
	void UnregisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value );

protected:
	void OpenBinkVideo();
	void SetDirty();

private:
	std::wstring m_name;
	std::string m_resPath;

	HBINK m_binkHandle;

	int m_outputChannel;

	bool m_isPaused;
	bool m_isMuted;
	bool m_isFinished;
	bool m_isGood;

	bool m_loop;

	Tr2AtlasTexturePtr m_atlasTexture;

	float m_srcX;
	float m_srcY;
	float m_srcWidth;
	float m_srcHeight;

	unsigned m_originalVideoWidth;
	unsigned m_originalVideoHeight;
	unsigned m_videoFps;
	unsigned m_currentFrame;

	// When the D3D texture is destroyed, force Bink to copy the entire frame
	bool m_copyAllNextFrame;

	TrackableStdSet<ITr2Sprite2dTextureNotifyTarget*> m_changeListeners;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dBinkTexture );

