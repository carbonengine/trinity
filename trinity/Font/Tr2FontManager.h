// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2FontManager_H
#define Tr2FontManager_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SIZES_H
#include FT_CACHE_H

#include "Tr2DeviceResource.h"

BLUE_DECLARE_IVECTOR( IResFile );

BLUE_DECLARE( Tr2FontManager );
BLUE_DECLARE( Tr2SBitWrapper );
BLUE_DECLARE( Tr2AtlasTexture );

typedef uintptr_t FaceID;

extern const char* TR2_FONT_FALLBACK;

//////////////////////////////////////////////////////////////////////////
// The Tr2FontManager class manages font handling in Trinity.
// It is currently implemented as a thin wrapper around FreeType.
// The intent is to make it more abstract, possibly even allow us to
// try out other font libraries.
//
// Tr2FontManager is a singleton, accessed from C++ via g_fontManager.
// It exposes some functions to Python as Trinity module functions.
//////////////////////////////////////////////////////////////////////////
class Tr2FontManager : public IRoot,
					   public IBlueEvents,
					   public IWeakRef,
					   public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2FontManager( IRoot* lockobj = NULL );
	~Tr2FontManager();

	static void Initialize();
	static void Shutdown();

	static FT_Error FaceRequester( FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face* aface );

	// Gets a face ID from font name
	FaceID GetFaceID( const char* font );

	// Returns the FreeType FT_Face structure behind the face ID
	FT_Face LookupFace( const FaceID& faceID );

	// Returns size metrics for the given font
	FT_Size_Metrics LookupMetrics( const FaceID& faceID, unsigned int width, unsigned int height );
	std::pair<int, int> LookupMetricsFromScript( const FaceID& faceID, unsigned int width, unsigned int height );

	// Returns glyph index, or -1 in case of error
	int LookupGlyphIndex( const FaceID& faceID, int charCode );

	std::pair<int, int> LookupFaceIDAndGlyphIndex( const std::string& faceName, int charCode );

	// Returns kerning value for the glyph pair
	FT_Pos LookupKerningXP( const FaceID& faceID, int leftIndex, int rightIndex );

	// Returns an sbit (small bitmap) for the given glyph index
	Be::Result<std::string> LookupSBit( const FaceID& faceID, int width, int height, int glyphIndex, Tr2SBitWrapper** result );

	FTC_Manager GetManager() const
	{
		return m_manager;
	}
	FTC_CMapCache GetCmCache() const
	{
		return m_cmCache;
	}
	FTC_SBitCache GetSbitCache() const
	{
		return m_sbitCache;
	}

	unsigned int GetLoadFlag() const
	{
		return m_loadflag;
	}

	bool GetAtlasTextureForSbit( FTC_SBit sbit, Tr2AtlasTexture** at );

	// Returns the number of glyphs in use
	int GetNumGlyphsInUse();

	// Returns the number of glyphs cached by the font manager
	int GetNumGlyphsCached();

	// Clears any glyphs cached
	void ClearCachedGlyphs();

	// Trims the glyph cache to the given size
	void TrimGlyphCache( size_t cacheSize );

	//////////////////////////////////////////////////////////////////////////
	// IWeakRef
	//
	void WeakRefNotify( IWeakObject* p );

	//////////////////////////////////////////////////////////////////////////
	// IBlueEvents
	virtual void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

protected:
	FT_Face LoadFromDisk( FTC_FaceID id );


private:
	int PyLookupKerningXP( int faceID, int charCode1, int charCode2 );

	int m_loadflag;

	unsigned int m_maxFaces;
	unsigned int m_maxSizes;
	unsigned int m_maxBytes;

	typedef TrackableStdMap<std::string, unsigned int> FaceMap_t;
	FaceMap_t m_faceMap;

	TrackableStdVector<std::string> m_reverseFaceMap;

	FT_Library m_ftLib;
	FTC_Face_Requester m_requesterCallback;

	// Cache Managers
	FTC_Manager m_manager;
	FTC_CMapCache m_cmCache;
	FTC_SBitCache m_sbitCache;

	typedef TrackableStdMap<FTC_SBit, IWeakObject*> SbitToTextureMap_t;
	typedef TrackableStdMap<IWeakObject*, FTC_SBit> TextureToSbitMap_t;

	struct GlyphCacheEntry
	{
		IWeakObject* glyphObject;
		unsigned int lastFrameUsed;
		size_t memoryUsage;
	};
	typedef TrackableStdMap<FTC_SBit, GlyphCacheEntry> SbitToCachedTextureMap_t;

	SbitToTextureMap_t m_sbitToTextureMap;
	SbitToCachedTextureMap_t m_sbitToCachedTextureMap;
	TextureToSbitMap_t m_textureToSbitMap;
	size_t m_totalGlyphsCachedSize;
	size_t m_glyphCacheBudget;
};

TYPEDEF_BLUECLASS( Tr2FontManager );

extern Tr2FontManager* g_fontManager;

#endif