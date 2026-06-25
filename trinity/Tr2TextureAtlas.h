// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2TextureAtlas_h
#define Tr2TextureAtlas_h


#include "Tr2DeviceResource.h"
#include "ITr2TextureProvider.h"
#include "include/Rect.h"

BLUE_DECLARE( Tr2AtlasTexture );

struct Tr2TextureAtlasArea
{
	enum Type
	{
		FREE, // Area is free
		IN_USE, // Area is in active use
		IN_USE_NO_REF // Area was in use and still contains data, but there are no references to it
	};

	// Type of this area
	Type type;

	// Bounds of the area
	Tr2Rect rect;

	// Weak reference to a texture object for in-use areas
	Tr2AtlasTexture* tex;
};

BLUE_CLASS( Tr2TextureAtlas ) :
	public ITr2TextureProvider,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();
	Tr2TextureAtlas( IRoot* lockobj = NULL );
	~Tr2TextureAtlas();

	void Initialize( Tr2RenderContextEnum::PixelFormat fmt, unsigned int width, unsigned int height );
	void CollapseFreeAreas();
	void ConsolidateFreeAreas();
	void PullInOutsiders( bool optimiseInsertion = false );

	enum AtlasTextureType
	{
		ATT_DEFAULT,
		ATT_VIDEO
	};
	ALResult CreateTexture( unsigned int width, unsigned int height, AtlasTextureType type, Tr2AtlasTexture** result );

	Tr2RenderContextEnum::PixelFormat GetFormat() const;

	int GetTexturesInAtlasCount();
	int GetTexturesOutsideAtlasCount();

#if BLUE_WITH_PYTHON
	PyObject* GetTexturesInAtlas();
	PyObject* GetTexturesOutsideAtlas();
#endif

	//////////////////////////////////////////////////////////////////////////
	// ITr2TextureProvider
	Tr2TextureAL* GetTexture();
	OnTextureChangeEvent& OnTextureChange() override;

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

	unsigned GetWidth() const
	{
		return m_width;
	}
	unsigned GetHeight() const
	{
		return m_height;
	}
	unsigned GetMargin() const
	{
		return m_margin;
	}

	void SetMargin( unsigned int margin );
	void SetPaintEmptyAreas( bool paintEmptyAreas );
	void SetCreateOutsiders( bool createOutsiders );
	void SetMaxTextureArea( unsigned int maxTextureArea );

	//Return a copy of the area list to avoid users fiddling with our privates
	// (only used by the Atlas render step for debugging anyway)
	std::list<Tr2Rect> GetFreeAreas() const;
	std::list<Tr2Rect> GetUsedAreas() const;

	uint32_t GetMsaaSamples() const;
	uint32_t GetMsaaQuality() const;

private:
	bool OnPrepareResources();

private:
	friend class Tr2AtlasTexture;
	bool DoPrepare( Tr2AtlasTexture * tex );

	void RegisterInsider( Tr2AtlasTexture * tex );
	void RegisterOutsider( Tr2AtlasTexture * tex );

	void RemoveFromAtlas( Tr2AtlasTexture * tex );

	void PaintEmptyArea( Tr2TextureAtlasArea * area );
	bool CopyTextureIntoAtlas( Tr2AtlasTexture * tex );

	Tr2TextureAtlasArea* GetFreeArea( unsigned int width, unsigned int height );

	// Collapse areas into area1 if they are adjacent. Returns true if areas were collapsed.
	bool CollapseAreas( Tr2TextureAtlasArea * area1, Tr2TextureAtlasArea * area2 );

	bool HasALObject( int type, size_t object );

public:
	int GetFreeTexels() const
	{
		return m_freeTexels;
	}
	float GetFreeTexelPercentage() const
	{
		return m_freeTexels / float( m_width * m_height );
	}
	int GetFreeMaxWidth() const
	{
		return m_freeMaxWidth;
	}
	int GetFreeMaxHeight() const
	{
		return m_freeMaxHeight;
	}


	//kick all the insiders out of the atlas!
	void EjectAllTextures();

	//kick a texture out of the atlas
	bool EjectTexture( Tr2AtlasTexture * tex );

private:
	// Returns true if dimensions are deemed large, thus unsuitable for atlasing.
	bool IsLargeTexture( unsigned int width, unsigned int height );

	// Internal version of EjectTexture - does not remove the texture from the m_texturesInsideAtlas
	bool EjectTextureHelper( Tr2AtlasTexture * tex );

	void FreeArea( Tr2TextureAtlasArea* );
	void ReleasePendingFreeAreas();

	typedef TrackableStdList<Tr2TextureAtlasArea*> AreaList_t;
	AreaList_t m_freeAreas;
	// List of areas and frame numbers that are to be freed when GPU is done copying from them (driver bug workaround).
	std::vector<std::pair<Tr2TextureAtlasArea*, uint64_t>> m_pendingFreeAreas;

	Tr2TextureAL m_texture;
	OnTextureChangeEvent m_onTextureChange;

	Tr2RenderContextEnum::PixelFormat m_format;
	unsigned int m_width;
	unsigned int m_height;


	typedef TrackableStdSet<Tr2AtlasTexture*> Tr2AtlasTextureSet_t;
	Tr2AtlasTextureSet_t m_texturesInAtlas;
	Tr2AtlasTextureSet_t m_texturesOutsideAtlas;

	unsigned int m_margin;

	bool m_optimizeOnRemoval;
	bool m_paintEmptyAreas;
	bool m_createOutsiders;

	unsigned m_maxTextureArea;
	int m_freeTexels;
	mutable int m_freeMaxWidth, m_freeMaxHeight;
	void UpdateFreeMaxima() const;

	bool m_optimizationWarranted;
	int m_areasFreedSinceLastCollapse;

	//mipmaps
	bool m_hasMipMaps;
	unsigned m_mipLevels;
	std::vector<Tr2Rect> m_dirtyMipRegions;

public:
	//call manually to calculate lower mips
	void UpdateMipMaps( Tr2RenderContext & renderContext );
	//add a dirty region after changing the top level
	void DirtyRegion( const Tr2Rect& rect );
};

TYPEDEF_BLUECLASS( Tr2TextureAtlas );

#endif //Tr2TextureAtlas_h
