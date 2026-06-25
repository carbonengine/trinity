// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2AtlasTexture_h
#define Tr2AtlasTexture_h

#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2AtlasTexture );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2TextureAtlas );
BLUE_DECLARE( Tr2HostBitmap );

class Tr2ImageHandler;

class ITr2AtlasTextureNotifyTarget
{
public:
	virtual void AtlasTextureChanged( Tr2AtlasTexture* p ) = 0;
};

// A Tr2AtlasTexture is a texture that usually lives in a texture atlas. This means
// that is a part of a larger texture, residing within a given texture window in that
// larger texture. Sometimes an atlas texture lives in its own D3D texture, while
// still being associated with a texture atlas. This can happen if the atlas was
// full at the time of creation - the texture may be pulled into the atlas at a later
// point.
BLUE_CLASS( Tr2AtlasTexture ) :
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();
	using BlueAsyncRes::Lock;
	using BlueAsyncRes::Unlock;

	Tr2AtlasTexture( IRoot* lockobj = NULL );
	~Tr2AtlasTexture();

	// Get the D3D texture behind the atlas texture. Use GetTextureWindow
	// to get the uv-offsets to apply when rendering.
	Tr2TextureAL* GetTexture();
	Tr2TextureAL* GetRenderTarget();
	void SetRenderTarget( Tr2TextureAL * rt );

	// Get texture window for rendering this texture out of the atlas
	void GetTextureWindow( Vector4 & tw );

	// Calculate the texture coordinates for a region within this atlas texture.
	void CalcSubTextureWindow( Vector4 & tw, float rectX, float rectY, float rectWidth, float rectHeight );

	unsigned int GetX() const;
	unsigned int GetY() const;
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	unsigned int GetTextureWidth() const;
	unsigned int GetTextureHeight() const;
	float GetWidthReciprocal() const;
	float GetHeightReciprocal() const;

	// Lock the rectangle in the D3D texture that corresponds to this atlas texture.
	bool LockBuffer( void*& pData, unsigned int& pitch );
	bool LockBufferAndMargin( void*& data, unsigned& pitch, unsigned& margin );

	// Unlock a rectangle previously locked with LockBuffer.
	void UnlockBuffer();

	void RegisterForChangeNotification( ITr2AtlasTextureNotifyTarget * p );
	void UnregisterForChangeNotification( ITr2AtlasTextureNotifyTarget * p );

	// A stand-alone atlas texture is really not an atlas texture - it's a permanent outsider.
	// This is to support textures that need texture transformations - clamping and repeat
	// is currently too expensive to support for atlas textures.
	void SetStandAlone( bool b );
	bool IsStandAlone() const;

	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	void OnShutdown();

	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	virtual bool IsMemoryUsageKnown();
	virtual size_t GetMemoryUsage();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	// For general use, we want to be able to load directly to a target atlas
	void SetTargetAtlasBeforeLoad( Tr2TextureAtlas * atlas );

private:
	bool OnPrepareResources();

	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

private:
	friend class Tr2TextureAtlas;

	void FinalizePrepare();

	void NotifyListenersOfChange();

	// Calculate the texture window - get the cached results with GetTextureWindow.
	void CalcTextureWindow();

	// Calculate reciprocals for width/height
	void CalcReciprocals();

	void SetTextureRes( TriTextureRes * p );
	TriTextureRes* GetTextureRes();

private:
	// Name of the object, for programmer convenience
	std::string m_name;

	// Resource path this texture was loaded from
	std::string m_resPath;

	void* m_data;
	uint32_t m_dataSize;

	// Image handler used for loading
	std::unique_ptr<ImageIO::HostBitmap> m_loadedBitmap;

	// Used for dynamic textures.
	TriTextureResPtr m_textureRes;

	// AL texture, used if texture is stand-alone (didn't fit in atlas
	// or was requested stand-alone)
	Tr2TextureAL m_texture;

	// It is ok to have raw pointer to Tr2AtlasTexture::m_renderTarget since
	// since T2AtlasTexture lifetime is bound to it.
	Tr2TextureAL* m_renderTarget;

	// Bounds of this texture within the D3D texture
	unsigned int m_x;
	unsigned int m_y;
	unsigned int m_width;
	unsigned int m_height;
	float m_widthReciprocal;
	float m_heightReciprocal;

	// Dimensions of the D3D texture atlas this texture lives in
	unsigned int m_textureWidth;
	unsigned int m_textureHeight;

	float m_textureWidthReciprocal;
	float m_textureHeightReciprocal;

	// Texture window to use when rendering
	Vector4 m_textureWindow;

	Tr2TextureAtlasPtr m_textureAtlas;
	struct Tr2TextureAtlasArea* m_atlasArea;

	unsigned int m_memoryUsage;

	bool m_isStandAlone;
	bool m_isLocked;

	TrackableStdSet<ITr2AtlasTextureNotifyTarget*> m_changeListeners;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2AtlasTexture );
#endif //Tr2AtlasTexture_h
