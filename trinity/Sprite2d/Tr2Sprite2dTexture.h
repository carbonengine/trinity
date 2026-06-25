// Copyright © 2010 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dTexture_h
#define Tr2Sprite2dTexture_h

#include "ITr2Sprite2dTexture.h"
#include "ITr2Sprite2dRenderer.h"
#include "Tr2AtlasTexture.h"

BLUE_DECLARE( Tr2AtlasTexture );

// The Tr2Sprite2dTexture wraps an atlas texture resource. It holds
// the resource path name and settings such as rectangle to use from
// the texture, filter and dot product flags.
class Tr2Sprite2dTexture : public ITr2Sprite2dTexture,
						   public IInitialize,
						   public INotify,
						   public ITr2AtlasTextureNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dTexture( IRoot* lockobj = NULL );
	~Tr2Sprite2dTexture();

	void SetAtlasTexture( Tr2AtlasTexture* val );

	std::string GetResPath() const;
	void SetResPath( const std::string& path );

	bool GetTileX() const;
	void SetTileX( bool val );

	bool GetTileY() const;
	void SetTileY( bool val );

	void CheckTextureWindow();

	//////////////////////////////////////////////////////////////////////////
	// ITr2Sprite2dTexture
	void Apply( Tr2Sprite2dScene* renderer, unsigned int ix );

	Tr2AtlasTexture* GetAtlasTexture() const;

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

	float GetSrcX() const;
	void SetSrcX( float srcX );


	float GetSrcY() const;
	void SetSrcY( float srcY );
	float GetSrcWidth() const;
	void SetSrcWidth( float srcWidth );
	float GetSrcHeight() const;
	void SetSrcHeight( float srcHeight );

	Matrix* GetTransform();

	int GetTextureRepeatMode() const;
	void SetTextureRepeatMode( int repeatMode );

	bool IsLoading() const;
	bool IsGood() const;

	void RegisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p );
	void UnregisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////
	// ITr2AtlasTextureNotifyTarget
	void AtlasTextureChanged( Tr2AtlasTexture* p );

protected:
	void SetDirty();

private:
	std::wstring m_name;
	std::string m_resPath;

	Tr2AtlasTexturePtr m_atlasTexture;

	// This is set to true if any of m_srcX, m_srcY, m_srcWidth or m_srcHeight
	// are set to non-zero values. This is used to determine if texture window
	// needs to be set when texture is applied.
	bool m_hasTextureWindow;
	float m_srcX;
	float m_srcY;
	float m_srcWidth;
	float m_srcHeight;

	Tr2Sprite2dTextureSettings m_settings;

	// If set, GetTransform returns an affine 2D transformation built
	// up from the translation, rotation and scaling values.
	// Otherwise NULL is returned and no transformation should be applied.
	bool m_useTransform;

	// Translation applied to texture coordinates
	Vector2 m_translation;

	// Center of rotation applied to texture coordinates
	Vector2 m_rotationCenter;

	// Rotation applied to texture coordinates
	float m_rotation;

	// Center of scaling applied to texture coordinates
	Vector2 m_scalingCenter;

	// Rotation of scaling applied to texture coordinates
	float m_scalingRotation;

	// Scale applied to texture coordinates
	Vector2 m_scale;

	Matrix m_transform;

	TrackableStdSet<ITr2Sprite2dTextureNotifyTarget*> m_changeListeners;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dTexture );

#endif // Tr2Sprite2dTexture_h
