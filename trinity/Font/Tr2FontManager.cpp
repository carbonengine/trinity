#include "StdAfx.h"

#include "Tr2FontManager.h"
#include "Tr2SBit.h"
#include "Tr2AtlasTexture.h"
#include "Tr2TextureAtlas.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2Renderer.h"

#include FT_MULTIPLE_MASTERS_H
#include FT_MODULE_H

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( fontMem, "Trinity/FontMemory", false, CST_MEMORY, "Memory used by the font system");

Tr2FontManager* g_fontManager = NULL;

// This ensures that the resource handles
// stay alive and open until Freetype
// calls FileClose
static CIResFileVector s_fpVector;

void* Tr2FreeTypeAlloc( FT_Memory memory, long size )
{
	void* block = CCP_MALLOC( "FreeType", size );
	if( block )
	{
		memset( block, 0, size );

		CCP_STATS_ADD( fontMem, CCP_MSIZE( block ) );
	}
	return block;
}

void Tr2FreeTypeFree( FT_Memory memory, void* block )
{
	int size = (unsigned int)CCP_MSIZE( block );
	CCP_STATS_ADD( fontMem, -size );
	CCP_FREE( block );
}

void* Tr2FreeTypeRealloc( FT_Memory memory, long curSize, long newSize, void* block )
{
	int size = (unsigned int)CCP_MSIZE( block );
	CCP_STATS_ADD( fontMem, -size );
	void* newBlock = CCP_REALLOC( "FreeType", block, newSize );
	CCP_STATS_ADD( fontMem, CCP_MSIZE( newBlock ) );
	return newBlock;
}

static struct FT_MemoryRec_ s_ft_memory = { NULL, Tr2FreeTypeAlloc, Tr2FreeTypeFree, Tr2FreeTypeRealloc };

static unsigned long FileRead( FT_Stream stream, unsigned long offset, unsigned char *buffer, unsigned long count );
static void FileClose( FT_Stream stream );

static const char* s_cookie = "fontMan";

//////////////////////////////////////////////////////////////////////////
// Description:
//   Initializes the font library and sets up the singleton g_fontManager.
//////////////////////////////////////////////////////////////////////////
void Tr2FontManager::Initialize()
{
	g_fontManager = new OTr2FontManager;

	BeOS->RegisterForTicks( g_fontManager, (void*)s_cookie );
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Takes down the g_fontManager singleton and shuts down the font library.
//////////////////////////////////////////////////////////////////////////
void Tr2FontManager::Shutdown()
{
	if( BeOS )
	{
		BeOS->UnregisterForTicks( g_fontManager, (void*)s_cookie );
	}
	CCP_DELETE g_fontManager;
	g_fontManager = NULL;
}

Tr2FontManager::Tr2FontManager( IRoot* lockobj ) :    
	m_manager( NULL ),
	m_cmCache( NULL ),
	m_sbitCache( NULL ),
	m_ftLib( NULL ),
	m_loadflag( FT_LOAD_DEFAULT ),
	m_maxFaces( 15 ),
	m_maxSizes( 40 ),
	m_maxBytes( 40 * 1024 * 1024 ),
	m_faceMap( "Tr2FontManager/m_faceMap" ),
	m_reverseFaceMap( "Tr2FontManager/m_reverseFaceMap" ),
	m_sbitToTextureMap( "Tr2FontManager/m_sbitToTextureMap" ),
	m_sbitToCachedTextureMap( "Tr2FontManager/m_sbitToCachedTextureMap" ),
	m_textureToSbitMap( "Tr2FontManager/m_textureToSbitMap"),
	m_totalGlyphsCachedSize( 0 ),
	m_glyphCacheBudget( 512*1024 )
{
	FT_Error err = FT_New_Library( &s_ft_memory, &m_ftLib );
	if( err )
	{
		CCP_LOGERR( "Tr2FontManager: FT_New_Library failed - %d", err );
		return;
	}

	FT_Add_Default_Modules( m_ftLib );

	FT_Error e;
	e = FTC_Manager_New( m_ftLib, m_maxFaces, m_maxSizes, m_maxBytes, FaceRequester, reinterpret_cast<FT_Pointer>( this ), &m_manager );
	if( e )
	{
		return;
	}
	e = FTC_CMapCache_New( m_manager, &m_cmCache );
	if( e )
	{
		return;
	}
	e = FTC_SBitCache_New( m_manager, &m_sbitCache );
	if( e )
	{
		return;
	}

	// Put a dummy entry in the reverse face map to prevent 0 as a FaceID.
	m_reverseFaceMap.push_back( "dummy" );
}

Tr2FontManager::~Tr2FontManager()
{
	FTC_Manager_Done( m_manager );
	FT_Done_Library( m_ftLib );
}

//////////////////////////////////////////////////////////////////////////
// Description:
//  Gets a face id based on the font name and parameters. Any unique
//  combination of font name and relevant parameters gets a unique face
//  id - conversely, any previously seen combination is guaranteed to
//  get the same face id.
// Arguments:
//  font	 - Name of the font (resource path)
//  pyParams - PyObject with parameters as attributes. The relevant
//				parameters are 'bold', 'italic' and 'width'
// Returns:
//  Face id
//////////////////////////////////////////////////////////////////////////
FaceID Tr2FontManager::GetFaceID( const char* font )
{
	FaceMap_t::const_iterator foundIt = m_faceMap.find( font );
	unsigned int id;

	if( foundIt != m_faceMap.end() )
	{
		id = foundIt->second;
	}
	else
	{
		id = (unsigned int)m_reverseFaceMap.size();

		m_reverseFaceMap.push_back( font );

		m_faceMap[font] = id;
	}

	return id;
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Get a FreeType face structure from a face ID.
//
// Arguments:
//   faceID - the face ID, as returned by GetFaceID
//
// Returns:
//   FT_Face structure
//////////////////////////////////////////////////////////////////////////
FT_Face Tr2FontManager::LookupFace( const FaceID& faceID )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	FT_Face face = NULL;

	if( m_manager )
	{
		FTC_Manager_LookupFace( m_manager, (FTC_FaceID)faceID, &face );
	}
	return face;
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Get FreeType size metrics for the given face ID.
//
// Arguments:
//   faceID - the face ID, as returned by GetFaceID
//   width  - the width, in pixels
//   height - the height, in pixels
//
// Returns:
//   FT_Size_Metrics structure (see FreeType documentation for details)
//////////////////////////////////////////////////////////////////////////
FT_Size_Metrics Tr2FontManager::LookupMetrics( const FaceID& faceID, unsigned int width, unsigned int height )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	FTC_ScalerRec scaler = {(FTC_FaceID)faceID, width, height, 1, 0, 0 }; // 1 indicates to use pixel sizes
	FT_Size size;
	FT_Error err = FTC_Manager_LookupSize( m_manager, &scaler, &size );
	if( err )
	{
		// NOTE: Throw exception?
		return FT_Size_Metrics();
	}
	return size->metrics;
}


std::pair<int, int> Tr2FontManager::LookupMetricsFromScript( const FaceID& faceID, unsigned int width, unsigned int height )
{
	FT_Size_Metrics metrics = LookupMetrics( faceID, width, height );
	return std::make_pair( metrics.ascender >> 6, metrics.descender >> 6 );
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Get the kerning value for a pair of glyphs.
//
// Arguments:
//   faceID     - the face ID, as returned by GetFaceID
//   leftIndex  - left glyph index
//   rightIndex - right glyph index
//
// Returns:
//   Horizontal (x) kerning value, in pixels, for the given glyph pair.
//////////////////////////////////////////////////////////////////////////
FT_Pos Tr2FontManager::LookupKerningXP(const FaceID& faceID, int leftIndex, int rightIndex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	FT_Face face = NULL;
	if( m_manager )
	{
		FTC_Manager_LookupFace( m_manager, (FTC_FaceID)faceID, &face );
	}
	FT_Vector kerning;
	FT_Error err = FT_Get_Kerning( face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning );
	if( err )
	{
		return INT_MAX; // Kerning value in the millions of pixels, unlikely to happen...
	}
	return ( kerning.x >> 6 );
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Get a glyph index from a character code.
//
// Arguments:
//   faceID     - the face ID, as returned by GetFaceID
//   charCode   - Unicode character code
//
// Returns:
//   glyphIndex, that can be passed to LookupSBit
//////////////////////////////////////////////////////////////////////////
int Tr2FontManager::LookupGlyphIndex( const FaceID& faceID, int charCode )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	FT_Face face = LookupFace( faceID );
	FT_Error err = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
	if( err )
	{
		return -1;
	}
	int cmapIndex = FT_Get_Charmap_Index( face->charmap );

	int glyphIndex = -1;
	if( m_cmCache )
	{
		glyphIndex = FTC_CMapCache_Lookup( m_cmCache, (FTC_FaceID)faceID, cmapIndex, charCode );
	}
	return glyphIndex;
}

//////////////////////////////////////////////////////////////////////////
// Description:
//   Gets an sbit from the given face ID, dimensions and glyph index
//
// Arguments:
//   faceID     - the face ID, as returned by GetFaceID
//   width      - the width of the font, in pixels
//   height     - the height of the font, in pixels
//   glyphIndex  - glyph index as returned by LookupGlyphIndex
//////////////////////////////////////////////////////////////////////////
 Be::Result<std::string> Tr2FontManager::LookupSBit( const FaceID& faceID, int width, int height, int glyphIndex, Tr2SBitWrapper** result )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2SBitWrapper* img = new OTr2SBitWrapper;

	if( m_sbitCache )
	{
		FTC_ImageTypeRec imgType = { (FTC_FaceID)faceID, FT_UInt( width ), FT_UInt( height ), m_loadflag };
		FT_Error e = FTC_SBitCache_Lookup( m_sbitCache, &imgType, glyphIndex, &img->sbit, &img->node );
		if( e )
		{
			img->Unlock();
			return Be::Result<std::string>( "FTC_SBitCache_Lookup failed" );
		}

		img->manager = m_manager;
		img->faceId = faceID;
		img->x = 0;
		img->y = 0;
	}

	*result = img;
	return Be::Result<std::string>();
}

inline FT_Fixed Fix( float f )
{
	return (FT_Fixed) floor(f * float(1<<16) + 0.5f);
}

//-----------------------------------------------------------------------------
// Load font file from disk using a ResFile object.
//-----------------------------------------------------------------------------
FT_Face Tr2FontManager::LoadFromDisk( FTC_FaceID id )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_ftLib )
	{
		CCP_LOGERR( "Tr2FontManager::LoadFromDisk: FreeType has not been initialized" );
		return NULL;
	}

	uintptr_t index = reinterpret_cast<uintptr_t>( id );
	const char* fontPath = m_reverseFaceMap[index].c_str();

	// Create a new ResFile instance and add to the
	// opened ResFiles vector
	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr fp( resFileClsid );
	s_fpVector.Insert( -1, fp );
	// Open the file
	// TODO: Add error handling to close and remove
	// object from the opened ResFile vector
	if( !fp->Open( fontPath, true ) )
	{
		CCP_LOGERR( "Tr2FontManager::LoadFromDisk: File '%s' does not exist", fontPath );
		return NULL;
	}
	ssize_t fileLength = fp->GetSize();

	// Create our stream struct.
	// This looks like a memory leak since we are
	// creating a stream using new and not deleting
	// at the end of the function.
	// What we are doing is ensuring that the stream
	// stays alive as long as freetype may need it.
	// Freetype will then call FileClose with a pointer
	// to the stream when it is done, making it safe for
	// us to delete it there.
	FT_Stream stream = CCP_NEW( "Tr2FontManager/LoadFromDisk/stream" ) FT_StreamRec;
	if( !stream ) 
	{
		return NULL;
	}
	memset( stream, 0, sizeof( *stream ) );

	stream->descriptor.pointer = (void*)fp.p;
	
	stream->read = &FileRead;
	stream->close = &FileClose;
	stream->size = (unsigned int)fileLength;

	FT_Open_Args openargs;
	openargs.flags = FT_OPEN_STREAM;
	openargs.stream = stream;
	FT_Face face;
	FT_Error e = FT_Open_Face(m_ftLib, &openargs, 0, &face);
	if( e )
	{
		CCP_LOGERR( "Tr2FontManager::LoadFromDisk: FT_Open_Face failed for '%s'", fontPath );
		return NULL;
	}

	FT_Error err = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
	if( err )
	{
		CCP_LOGERR( "Tr2FontManager::LoadFromDisk - FT_Select_Charmap failed (%d)", err );
	}

	return face;
}

//-----------------------------------------------------------------------------
// Font reading callbacks.
//-----------------------------------------------------------------------------
FT_Error Tr2FontManager::FaceRequester( FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face* aface )
{
	Tr2FontManager* fm = reinterpret_cast<Tr2FontManager*>( request_data );
	
	*aface = fm->LoadFromDisk( face_id );
	if( *aface == NULL )
	{
		// Font wasn't found, or otherwise failed to load
		return -1;
	}

	return 0;
}

//A function to get the bit value from a b/w bitmap
inline int GetBit( uint8_t* line, int bit )
{
	const int c = (int)line[bit>>3];
	const int mask = 128 >> ( bit & 7 );
	return c & mask;
}

bool Tr2FontManager::GetAtlasTextureForSbit( FTC_SBit sbit, Tr2AtlasTexture** at )
{
	auto foundItInUse = m_sbitToTextureMap.find( sbit );
	if( foundItInUse != m_sbitToTextureMap.end() )
	{
		// CCP_LOG( __FUNCTION__ " found sbit (%d in map)", m_sbitToTextureMap.size() );

		IWeakObject* weakObj = foundItInUse->second;
		weakObj->QueryInterface( BlueInterfaceIID<Tr2AtlasTexture>(), (void**)at );
		return true;
	}

	auto foundItInCache = m_sbitToCachedTextureMap.find( sbit );
	if( foundItInCache != m_sbitToCachedTextureMap.end() )
	{
		// CCP_LOG( __FUNCTION__ " found sbit in cache (%d in cache)", m_sbitToCachedTextureMap.size() );

		FTC_SBit sbit = foundItInCache->first;
		GlyphCacheEntry& entry = foundItInCache->second;
		IWeakObject* weakObj = entry.glyphObject;
		m_totalGlyphsCachedSize -= entry.memoryUsage;

		// Move it from the cache to the main map
		m_sbitToCachedTextureMap.erase( foundItInCache );

		// entry is no longer valid - do not reference it below

		m_sbitToTextureMap[sbit] = weakObj;
		m_textureToSbitMap[weakObj] = sbit;

		// Register it for future about-to-die notifications.
		weakObj->WeakRefRegister( this );
		weakObj->QueryInterface( BlueInterfaceIID<Tr2AtlasTexture>(), (void**)at );

		// Drop the strong reference the cached map used to hold.
		weakObj->Unlock();
		return true;
	}

	// First time we see this sbit - create an atlas texture and copy the pixels over
	*at = nullptr;

	Tr2TextureAtlas* atlas = g_textureAtlasMan->FindAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM );
	if( !atlas )
	{
		return false;
	}

	// We add one pixel to allow for potential drop-shadow

	Tr2AtlasTexturePtr tex;
	ALResult al = atlas->CreateTexture( sbit->width, sbit->height + 1, Tr2TextureAtlas::ATT_DEFAULT, &tex );
	if( !tex )
	{
		return false;
	}

	void* pDest;
	unsigned int pitch;

	if( !tex->LockBuffer( pDest, pitch ) )
	{
		return false;
	}

	if( sbit->format == 1 )
	{
		for( int l = 0; l < sbit->height; ++l ) 
		{
			uint32_t* pDestLine = (uint32_t*)( (char*)pDest + l * pitch );
			uint8_t* pSrcLine = (uint8_t*)sbit->buffer + l * sbit->pitch;

			for( int c = 0; c < sbit->width; ++c )
			{
				uint32_t destColor;
				if( GetBit( pSrcLine, c ) )
				{
					destColor = 0xffffffff;
				}
				else
				{
					destColor = 0;
				}

				uint32_t* dp = pDestLine + c;
				*dp = destColor;
			}
		}
	}
	else
	{
		for( int l = 0; l < sbit->height; ++l ) 
		{
			uint32_t* pDestLine = (uint32_t*)( (uint8_t*)pDest + l * pitch );
			uint8_t* pSrcLine = (uint8_t*)sbit->buffer + l * sbit->pitch;

			for( int c = 0; c < sbit->width; ++c )
			{
				uint8_t* sp = pSrcLine + c;
				uint8_t gray = *sp;

				uint32_t destColor = 0x00ffffff;
				destColor |= gray << 24;

				uint32_t* dp = pDestLine + c;
				*dp = destColor;
			}
		}
	}

	uint32_t* pLastLine = (uint32_t*)( (uint8_t*)pDest + sbit->height * pitch );
	CCP_ASSERT( pitch >= sbit->width * 4u );
	memset( pLastLine, 0, sbit->width * 4 );
	
	tex->UnlockBuffer();

	BluePtr<IWeakObject> weakObj( BlueCastPtr( tex->GetRawRoot() ) );

	weakObj->WeakRefRegister( this );

	m_sbitToTextureMap[sbit] = weakObj;
	m_textureToSbitMap[weakObj] = sbit;

	*at = tex.Detach();

	// CCP_LOG( __FUNCTION__ " created sbit (%d in map)", m_sbitToTextureMap.size() );

	return true;
}

void Tr2FontManager::WeakRefNotify( IWeakObject* p )
{
	// Object is about to die - we resurrect it, but move it to a different map.
	FTC_SBit sbit = m_textureToSbitMap[p];
	
	// Remove the object from the main map
	m_sbitToTextureMap.erase( sbit );
	m_textureToSbitMap.erase( p );

	Tr2AtlasTexturePtr at;
	p->QueryInterface( BlueInterfaceIID<Tr2AtlasTexture>(), (void**)&at );

	size_t memoryUsage = at->GetMemoryUsage();
	// Store a strong reference to it in the cached map. If it is reused it's
	// again moved from there to the main map as a weak reference.
	GlyphCacheEntry entry;
	entry.glyphObject = p;
	entry.lastFrameUsed = unsigned( Tr2Renderer::GetCurrentFrameCounter() );
	entry.memoryUsage = memoryUsage;
	m_sbitToCachedTextureMap[ sbit ] = entry;
	p->Lock();

	m_totalGlyphsCachedSize += memoryUsage;
}

void Tr2FontManager::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		for( auto it = m_sbitToTextureMap.begin(); it != m_sbitToTextureMap.end(); ++it )
		{
			it->second->WeakRefUnregister( this );
		}

		m_sbitToTextureMap.clear();
		m_textureToSbitMap.clear();

		for( auto it = m_sbitToCachedTextureMap.begin(); it != m_sbitToCachedTextureMap.end(); ++it )
		{
			it->second.glyphObject->Unlock();
		}
		m_sbitToCachedTextureMap.clear();
		m_totalGlyphsCachedSize = 0;
	}
}

bool Tr2FontManager::OnPrepareResources()
{
	return true;
}

int Tr2FontManager::GetNumGlyphsInUse()
{
	return (int)m_sbitToTextureMap.size();
}

int Tr2FontManager::GetNumGlyphsCached()
{
	return (int)m_sbitToCachedTextureMap.size();
}

void Tr2FontManager::ClearCachedGlyphs()
{
	for( auto it = m_sbitToCachedTextureMap.begin(); it != m_sbitToCachedTextureMap.end(); ++it )
	{
		it->second.glyphObject->Unlock();
	}
	m_sbitToCachedTextureMap.clear();
	m_totalGlyphsCachedSize = 0;
}

void Tr2FontManager::TrimGlyphCache( size_t cacheSize )
{
	if( cacheSize >= m_totalGlyphsCachedSize )
	{
		return;
	}

	if( cacheSize == 0 )
	{
		ClearCachedGlyphs();
		return;
	}

	struct Entry
	{
		unsigned int age;
		FTC_SBit sbit;

		bool operator<( const Entry& other ) const
		{
			return age < other.age;
		}
	};

	// Set up a list with age of all entries
	std::vector<Entry> entries;
	entries.reserve( m_sbitToCachedTextureMap.size() );

	unsigned int now = unsigned( Tr2Renderer::GetCurrentFrameCounter() );
	for( auto it = m_sbitToCachedTextureMap.begin(); it != m_sbitToCachedTextureMap.end(); ++it )
	{
		GlyphCacheEntry& gcEntry = it->second;

		Entry entry;
		entry.age = now - gcEntry.lastFrameUsed;
		entry.sbit = it->first;

		entries.push_back( entry );
	}

	// Sort by age
	std::sort( entries.begin(), entries.end() );

	// Delete oldest entries until cache is trimmed to size
	for( auto toDeleteIt = entries.rbegin(); toDeleteIt != entries.rend(); ++toDeleteIt )
	{
		Entry& entry = *toDeleteIt;

		auto foundIt = m_sbitToCachedTextureMap.find( entry.sbit );
		CCP_ASSERT( foundIt != m_sbitToCachedTextureMap.end() );

		GlyphCacheEntry& gcEntry = foundIt->second;
		m_totalGlyphsCachedSize -= gcEntry.memoryUsage;
		gcEntry.glyphObject->Unlock();

		m_sbitToCachedTextureMap.erase( foundIt );

		if( m_totalGlyphsCachedSize < cacheSize )
		{
			break;
		}
	}
}

void Tr2FontManager::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	TrimGlyphCache( m_glyphCacheBudget );
}

int Tr2FontManager::PyLookupKerningXP( int faceID, int charCode1, int charCode2 )
{
	int kerning = int( g_fontManager->LookupKerningXP( faceID, charCode1, charCode2 ) );

	return kerning >> 6;
}

std::pair<int, int> Tr2FontManager::LookupFaceIDAndGlyphIndex( const std::string& faceName, int charCode )
{
	std::string faceNameUsed;
	if( faceName.empty() )
	{
		faceNameUsed = TR2_FONT_FALLBACK;
		CCP_LOGWARN( "trinity.fontMan.LookupGlyphIndex - no font given, using fallback font" );
	}
	else
	{
		faceNameUsed = faceName;
	}

	FaceID faceID = g_fontManager->GetFaceID( faceNameUsed.c_str() );

	int glyphIndex = g_fontManager->LookupGlyphIndex( faceID, charCode );

	if( glyphIndex <= 0 )
	{
		faceID = g_fontManager->GetFaceID( TR2_FONT_FALLBACK );
		glyphIndex = g_fontManager->LookupGlyphIndex( faceID, charCode );
	}

	return std::make_pair( (int)faceID, glyphIndex );
}

static unsigned long FileRead( FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count )
{
	IResFile* fp = (IResFile*)stream->descriptor.pointer;
	
	if( count == 0 )
	{
		ssize_t ret = fp->Seek( offset, ICcpStream::SO_BEGIN );
		if( ret == -1 )
		{
			return -1;
		}
		return 0;
	}
	if( offset != fp->GetPosition() )
	{
		fp->Seek( offset, ICcpStream::SO_BEGIN );
	}

	// TODO: See if I can't skip the memcpy there and just read straight into buffer
	unsigned char* tmpData = CCP_NEW( "Tr2FontCacheManager/FileRead" ) unsigned char[count];
	ssize_t bytesRead = 0;
	// Freetype guarantees that count is never bigger that the stream length
	while( (unsigned long)bytesRead < count )
	{
		bytesRead += fp->Read( (void*)tmpData, count - bytesRead );
	
		memcpy( buffer, tmpData, bytesRead );
		buffer += bytesRead;
	}

	CCP_DELETE[] tmpData;
	return (unsigned long)bytesRead;
}

static void FileClose( FT_Stream stream )
{
	IResFile* fp = ((IResFile*)stream->descriptor.pointer);
	
	// Close the file handle and remove the last reference
	// to that ResFile instance, effectively deleting it.
	fp->Close();
	ssize_t index = s_fpVector.FindKey( fp );
	s_fpVector.Remove( index );

	// Delete the stream we created in LoadFromDisk
	delete stream;
}

