////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2FontMeasurer.h"
#include "Tr2FontManager.h"
#include "Tr2AtlasTexture.h"
#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"
#include "Sprite2d/Tr2Sprite2dScene.h"

using namespace Tr2RenderContextEnum;

//-----------------------------------------------------------------------------
// Buffer copy helpers
//-----------------------------------------------------------------------------
void SBit_To_RGBABuffer(void* destPtr, int dWidth, int dHeight, int dPitch, FTC_SBit sbit, int left, int top, int col);
void Underline_To_RGBABuffer(void* destPtr, int dWidth, int dHeight, int dPitch, int left, int top, int width, int thickness, int col);
static inline int GetBit( uint8_t* line, int bit );
struct RGBA;

// CharacterBitmap is a wrapper around the FreeType sbit structure, along with x,y coords.
struct CharacterBitmap
{
	~CharacterBitmap()
	{
		FTC_Node_Unref( node, g_fontManager->GetManager() );
	}

	USE_CACHED_ALLOCATOR( CharacterBitmap );

	std::unique_ptr<CharacterBitmap> next;

	wchar_t charCode;
	FTC_SBit sbit;
	FTC_Node node;
	int x;
	int y;
	Tr2AtlasTexturePtr atlasTexture;
};

//The disposer object function
template <class T>
struct delete_disposer
{
	void operator()( T* delete_this )
	{  delete delete_this;  }
};

// Tr2FontRenderData holds the data from one AddText call
struct Tr2FontRenderData
{
	// cppcheck-suppress uninitMemberVar
	Tr2FontRenderData()
		:tail( nullptr ), 
		bitmapCount( 0 ) 
	{ 
	}

	USE_CACHED_ALLOCATOR( Tr2FontRenderData )
	
	// cursorX value that was in effect before this text segment was added.
	// This is used to reset the cursor when canceling text.
	int prevCursorX;

	int ascender;
	int descender;

	// The bitmaps for each character
	std::unique_ptr<CharacterBitmap> sbits;
	CharacterBitmap* tail;
	uint32_t bitmapCount;

	// Is the text underlined?
	bool underline;

	// Underline position (vertical, relative to baseline)
	short underlinePosition;

	// Thickness of underline
	short underlineThickness;

	// Text color
	unsigned int color;
};

Tr2FontMeasurer::Tr2FontMeasurer( IRoot* lockobj /*= NULL */ ) :
	m_renderData( "Tr2FontMeasurer/m_renderData" ),
	m_committedRenderData( "Tr2FontMeasurer/m_committedRenderData" ),
	m_fallbackFace( 0 ),
	m_vertices( nullptr ),
	m_indices( nullptr ),
	m_fontSize( 12 ),
	m_letterSpace( 1 ),
	m_color( 0xffffffff ),
	m_underline( false ),
	m_fadeLeftStart( -FLT_MAX ),
	m_fadeLeftEnd( -FLT_MAX ),
	m_fadeRightStart( FLT_MAX ),
	m_fadeRightEnd( FLT_MAX ),
	m_fadeTopStart( -FLT_MAX ),
	m_fadeTopEnd( -FLT_MAX ),
	m_fadeBottomStart( FLT_MAX ),
	m_fadeBottomEnd( FLT_MAX ),
	m_currentFace( 0 ),
	m_currentAscender( 0 ),
	m_currentDescender( 0 ),
	m_drawCalls( "Tr2FontMeasurer/m_drawCalls" ),
	m_changeListeners( "Tr2FontMeasurer/m_changeListeners" )
{
	Reset();
}

Tr2FontMeasurer::~Tr2FontMeasurer()
{
	Reset();
}

DEFINE_CACHED_ALLOCATOR( CharacterBitmap, 1024, 256 );
DEFINE_CACHED_ALLOCATOR( Tr2FontRenderData, 512, 256 );

//////////////////////////////////////////////////////////////////////////
// Description:
//   Clears both committed and non-committed text.
//////////////////////////////////////////////////////////////////////////
void Tr2FontMeasurer::Reset()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_cursorX = 0;
	m_cursorY = 0;

	// Reset ascender/descender and font to get fresh sizes on next AddText
	m_ascender = 0;
	m_descender = 0;
	m_currentFace = 0;
	
	for( auto it = m_renderData.begin(); it != m_renderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		delete rd;
	}
	m_renderData.clear();

	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		delete rd;
	} 

	m_committedRenderData.clear();

	if( m_vertices )
	{
		CCP_DELETE [] m_vertices;
		m_vertices = nullptr;
		m_vertexCount = 0;

		CCP_DELETE [] m_indices;
		m_indices = nullptr;
		m_indexCount = 0;
	}

	NotifyListenersOfChange();
}

unsigned int Tr2FontMeasurer::AddText( const std::wstring& text )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const char* font;

	if( !m_font.empty() )
	{
		font = m_font.c_str();
	}
	else
	{
		font = TR2_FONT_FALLBACK;
	}

	while( !m_currentFace || !m_currentFtFace )
	{
		m_currentFace = g_fontManager->GetFaceID( font );
		m_currentFtFace = g_fontManager->LookupFace( m_currentFace );

		if( !m_currentFtFace )
		{
			if( font == TR2_FONT_FALLBACK )
			{
				// We've already tried the fallback, no point in continuing.
				// If we get here something must be wrong with the installation.

				CCP_LOGERR( "%s: fallback font %s not found - check your installation", __FUNCTION__, font );
				return 0;
			}

			CCP_LOGERR( "%s: font %s not found", __FUNCTION__, font );

			// Requested font not found, use the fallback instead.
			font = TR2_FONT_FALLBACK;
			m_currentFace = 0;
			continue;
		}

		FT_Size_Metrics metrics = g_fontManager->LookupMetrics( m_currentFace, m_fontSize, m_fontSize );
		int asc = int( metrics.ascender >> 6 );
		if( asc > m_ascender )
		{
			m_ascender = asc;
		}
		m_currentAscender = asc;

		int des = int( metrics.descender >> 6 );
		if( des < m_descender )
		{
			m_descender = des;
		}
		m_currentDescender = des;

		m_imgType.face_id = (FTC_FaceID)m_currentFace;
		m_imgType.width = m_fontSize;
		m_imgType.height = m_fontSize;
		m_imgType.flags = g_fontManager->GetLoadFlag();

		// We have a font - don't need to try again
		break;
	}

	CCP_ASSERT( m_currentFace );
	CCP_ASSERT( m_currentFtFace );

	Tr2FontRenderData* renderData = new Tr2FontRenderData;
	m_renderData.push_back( renderData );

	// Store cursor position for possible cancellation of text
	renderData->prevCursorX = m_cursorX;
	
	if( m_underline )
	{
		renderData->underline = true;
		renderData->underlinePosition = m_currentFtFace->underline_position >> 6;
		renderData->underlineThickness = m_currentFtFace->underline_thickness >> 6;
		if( renderData->underlineThickness < 1 )
		{
			renderData->underlineThickness = 1;
		}
	}
	else
	{
		renderData->underline = false;
	}

	renderData->color = m_color;

	int cmapIndex = FT_Get_Charmap_Index( m_currentFtFace->charmap );

	// Parameters used for finding the glyph in the font
	FTC_CMapCache cmCache = g_fontManager->GetCmCache();
	FTC_SBitCache sbitCache = g_fontManager->GetSbitCache();

	int charactersAdded = 0;
	const wchar_t* curChar = text.c_str();
	while( *curChar )
	{
		// Character bitmap is only allocated after we've found the glyph index. The
		// font may not have a glyph for the character we're asking for - in that case
		// we look for it in the fallback font.
		CharacterBitmap* characterBitmap = NULL;

		// Look for the glyph index for current character in the current font
		int glyphIndex = FTC_CMapCache_Lookup( cmCache, (FTC_FaceID)m_currentFace, cmapIndex, *curChar );
		if( glyphIndex )
		{
			// Current font has the character - construct the CharacterBitmap structure and get
			// the sbit from FreeType.
			characterBitmap = new CharacterBitmap;
			FT_Error e = FTC_SBitCache_Lookup( sbitCache, &m_imgType, glyphIndex, &characterBitmap->sbit, &characterBitmap->node );
			if( e )
			{
				delete characterBitmap;
				characterBitmap = NULL;
			}
		}

		if( !characterBitmap )
		{
			// Current font didn't have the character requested - try the fallback font
			if( !m_fallbackFace )
			{
				// Set up for handling fall back for characters not in the current font
				m_fallbackFace = g_fontManager->GetFaceID( TR2_FONT_FALLBACK );
				m_ftFallbackFace = g_fontManager->LookupFace( m_fallbackFace );

				if( !m_ftFallbackFace )
				{
					CCP_LOGERR( "%s: fallback font %s not found - check your installation", __FUNCTION__, TR2_FONT_FALLBACK );
					break;
				}

				m_fallbackCmapIndex = FT_Get_Charmap_Index( m_ftFallbackFace->charmap );
				
				m_imgTypeFallback.face_id = (FTC_FaceID)m_fallbackFace;
				m_imgTypeFallback.flags = g_fontManager->GetLoadFlag();
				m_imgTypeFallback.width = m_fontSize;
				m_imgTypeFallback.height = m_fontSize;
			}

			glyphIndex = FTC_CMapCache_Lookup( cmCache, (FTC_FaceID)m_fallbackFace, m_fallbackCmapIndex, *curChar );
			if( glyphIndex )
			{
				characterBitmap = new CharacterBitmap;
				FT_Error e = FTC_SBitCache_Lookup( sbitCache, &m_imgTypeFallback, glyphIndex, &characterBitmap->sbit, &characterBitmap->node );
				if( e )
				{
					delete characterBitmap;
					characterBitmap = NULL;
				}
			}
		}

		if( !characterBitmap )
		{
			// Neither current font nor fallback font has a glyph for curChar
			// Charcode 0x25a1 is a white box in ArialUni - use that instead
			glyphIndex = FTC_CMapCache_Lookup( cmCache, (FTC_FaceID)m_fallbackFace, m_fallbackCmapIndex, 0x25a1 );
			if( glyphIndex )
			{
				characterBitmap = new CharacterBitmap;
				FT_Error e = FTC_SBitCache_Lookup( sbitCache, &m_imgTypeFallback, glyphIndex, &characterBitmap->sbit, &characterBitmap->node );
				if( e )
				{
					delete characterBitmap;
					characterBitmap = NULL;
				}
			}
		}

		if( !characterBitmap )
		{
			// I give up
			break;
		}

		CCP_ASSERT( characterBitmap );

		characterBitmap->charCode = *curChar;

		int newCursorX = m_cursorX;
		newCursorX += characterBitmap->sbit->xadvance;
		newCursorX += m_letterSpace;

		if( m_limit && newCursorX > m_limit )
		{
			delete characterBitmap;
			break;
		}

		characterBitmap->x = m_cursorX;
		characterBitmap->y = m_cursorY;

		m_cursorX = newCursorX;

		if( renderData->tail )
		{
			renderData->tail->next.reset( characterBitmap );
			renderData->tail = characterBitmap;
		}
		else
		{
			renderData->sbits.reset( characterBitmap );
			renderData->tail = renderData->sbits.get();
		}
		++renderData->bitmapCount;

		++charactersAdded;
		++curChar;
	}

	return charactersAdded;
}

void Tr2FontMeasurer::CommitText( int shiftX, int shiftY )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Note that reference counts of sbits are not affected - we're just
	// moving the RenderData objects from vector to another.

	for( auto it = m_renderData.begin(); it != m_renderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		for( CharacterBitmap* sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
		{
			CharacterBitmap& cbit = *sbIter;

			cbit.x += shiftX;
			cbit.y += shiftY;
		}
		rd->ascender = m_ascender;
		rd->descender = m_descender;

		m_committedRenderData.push_back( rd );
	}

	m_renderData.clear();

	m_ascender = m_currentAscender;
	m_descender = m_currentDescender;

	NotifyListenersOfChange();
}

void Tr2FontMeasurer::CancelLastText()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_renderData.empty() )
	{
		return;
	}

	Tr2FontRenderData* rd = m_renderData.back();
	rd->sbits.reset();
	rd->tail = nullptr;
	rd->bitmapCount = 0;

	m_cursorX = rd->prevCursorX;
	m_renderData.pop_back();

	delete rd;
}

// TODO: This assumes only a single line of text
int Tr2FontMeasurer::GetIndexAtPos( int x )
{
	if( x < 0 )
	{
		return 0;
	}

	int ix = 0;
	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
		{
			CharacterBitmap& cbit = *sbIter;

			if( (x >= cbit.x) && (x <= cbit.x + cbit.sbit->xadvance) )
			{
				return ix;
			}

			++ix;
		}
	}

	return ix;
}

int Tr2FontMeasurer::GetWidthAtIndex( int ix )
{
	int curIx = 0;
	int accumulatedWidth = 0;

	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
		{
			CharacterBitmap& cbit = *sbIter;

			if( curIx == ix )
			{
				return accumulatedWidth;
			}

			accumulatedWidth += cbit.sbit->xadvance;

			++curIx;
		}
	}

	return accumulatedWidth;
}

void Tr2FontMeasurer::DrawToAtlasTexture( Tr2AtlasTexture* atlasTexture )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !atlasTexture )
	{
		return;
	}

	void* pData = NULL;
	unsigned int pitch;

	if( !atlasTexture->LockBuffer( pData, pitch ) )
	{
		return;
	}

	unsigned int destWidth = atlasTexture->GetWidth();
	unsigned int destHeight = atlasTexture->GetHeight();

	DrawToBuffer( destWidth, destHeight, pData, pitch );

	atlasTexture->UnlockBuffer();
}


void Tr2FontMeasurer::DrawToTexture( TriTextureRes* texture )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !texture )
	{
		return;
	}

	Tr2TextureAL* const tex = texture->GetTexture();
	if( !tex || !tex->IsValid() || GetBytesPerPixel( tex->GetFormat() ) != 4 )
	{
		return;
	}

	void* pBits;
	uint32_t Pitch;
	CR_RETURN( tex->MapForWriting( Tr2TextureSubresource( 0 ), pBits, Pitch, renderContext ) );
	ON_BLOCK_EXIT( [&]{ tex->UnmapForWriting( renderContext ); } );

	DrawToBuffer(	texture->GetWidth(), 
					texture->GetHeight(), 
					pBits, 
					Pitch );
}

void Tr2FontMeasurer::DrawToHostBitmap( Tr2HostBitmap* hostBitmap )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !hostBitmap || !hostBitmap->IsValid() || GetBytesPerPixel( hostBitmap->GetFormat() ) != 4 )
	{
		return;
	}

	DrawToBuffer(	hostBitmap->GetWidth(),
					hostBitmap->GetHeight(),
					hostBitmap->GetRawData(),
					hostBitmap->GetPitch() );
}

bool Tr2FontMeasurer::HasCommittedText()
{
	return !m_committedRenderData.empty();
}

bool Tr2FontMeasurer::HasUncommittedText()
{
	return !m_renderData.empty();
}

//A function to get the bit value from a b/w bitmap
inline int GetBit( uint8_t* line, int bit )
{
	const int c = (int)line[bit>>3];
	const int mask = 128 >> ( bit & 7 );
	return c & mask;
}


void Tr2FontMeasurer::DrawToBuffer(		uint32_t destWidth, 
										uint32_t destHeight, 
										void* pData, 
										uint32_t pitch )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( uint32_t i = 0; i < destHeight; ++i )
	{
		memset( (uint8_t*)pData + i*pitch, 0, destWidth*4 );
	}

	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;

		for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
		{
			CharacterBitmap& cbit = *sbIter;
			FTC_SBit sbit = cbit.sbit;

			//add the left and top from the SBit
			int left = cbit.x + sbit->left;

			//specify top top-down
			int top = cbit.y - sbit->top;

			int sbitLeft = 0;
			if( left < 0 )
			{
				sbitLeft = -left;
			}

			int sbitRight = sbit->width;
			if( left + sbitRight >= (int)destWidth )
			{
				sbitRight = destWidth - left;
			}

			int sbitTop = 0;
			if( top + sbitTop < 0 )
			{
				sbitTop = -top;
			}

			int sbitBottom = sbit->height;
			if( top + sbitBottom >= (int)destHeight )
			{
				sbitBottom = destHeight - top;
			}


			uint8_t alphaFromColor = rd->color >> 24;

			if( sbit->format == 1 )
			{
				// Top as 1 being the bottom line!
				for( int l = sbitTop; l < sbitBottom; ++l ) 
				{
					int destLine = top + l;
					uint32_t* pDestLine = (uint32_t*)( (char*)pData + destLine * pitch );
					uint8_t* pSrcLine = (uint8_t*)sbit->buffer + l * sbit->pitch;

					for( int c = sbitLeft; c < sbitRight; ++c )
					{
						if( !GetBit( pSrcLine, c ) )
						{
							continue;
						}

						int destColumn = left + c;

						uint32_t destColor = rd->color & 0x00ffffff;
						destColor |= alphaFromColor << 24;

						uint32_t* dp = pDestLine + destColumn;
						*dp = destColor;
					}
				}
			}
			else
			{
				// Top as 1 being the bottom line!
				for( int l = sbitTop; l < sbitBottom; ++l ) 
				{
					int destLine = top + l;
					uint32_t* pDestLine = (uint32_t*)( (char*)pData + destLine * pitch );
					uint8_t* pSrcLine = (uint8_t*)sbit->buffer + l * sbit->pitch;

					for( int c = sbitLeft; c < sbitRight; ++c )
					{
						uint8_t* sp = pSrcLine + c;
						uint8_t gray = *sp;
						if( !gray )
						{
							continue;
						}

						int destColumn = left + c;

						uint32_t destColor = rd->color & 0x00ffffff;
						uint8_t a = (gray * (alphaFromColor + 1)) >> 8;
						destColor |= a << 24;

						uint32_t* dp = pDestLine + destColumn;
						*dp = destColor;
					}
				}
			}
		}

		if( rd->underline )
		{
			for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
			{
				CharacterBitmap& cbit = *sbIter;

				Underline_To_RGBABuffer(
					pData,
					destWidth, 
					destHeight, 
					pitch,
					cbit.x,
					destHeight - cbit.y + rd->underlinePosition,
					cbit.sbit->xadvance,
					rd->underlineThickness,
					rd->color );
			}
		}
	}
}

void Tr2FontMeasurer::SetFont( const std::string& font )
{
	if( strcmp( font.c_str(), m_font.c_str() ) != 0 )
	{
		m_font = font;
		m_currentFace = 0;
		m_currentFtFace = 0;
		m_currentAscender = 0;
		m_currentDescender = 0;
	}
}

const std::string Tr2FontMeasurer::GetFont() const
{
	return m_font;
}

void Tr2FontMeasurer::SetFontSize( int size )
{
	if( size != m_fontSize )
	{
		m_fontSize = size;
		m_currentFace = 0;
		m_currentFtFace = 0;
		m_currentAscender = 0;
		m_currentDescender = 0;
	}
}

int Tr2FontMeasurer::GetFontSize() const
{
	return m_fontSize;
}

void Tr2FontMeasurer::PrepareSprites( Tr2Sprite2dScene* renderer, const Vector2& translation, const Color& color, Tr2SpriteObjectEffect sfx, Tr2SpriteObjectBlendMode blendMode, Tr2SpriteTarget target, 
	float glowBrightness, bool dropShadow, const Vector2& shadowOffset, const Color& shadowColor, Tr2SpriteObjectEffect shadowSfx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_vertices )
	{
		CCP_DELETE [] m_vertices;
		m_vertices = nullptr;
		m_vertexCount = 0;

		CCP_DELETE [] m_indices;
		m_indices = nullptr;
		m_indexCount = 0;
	}

	unsigned int spriteCount = 0;
	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		spriteCount += rd->bitmapCount;
		if( rd->underline )
		{
			++spriteCount;
		}
	}

	if( spriteCount == 0 )
	{
		return;
	}

	if( dropShadow )
	{
		spriteCount *= 2;
	}

	m_drawCalls.clear();

	m_vertexCount = spriteCount * 4;
	m_vertices = CCP_NEW( "Tr2FontMeasurer/m_vertices" ) Tr2Sprite2dD3DVertex[m_vertexCount];

	m_indexCount = spriteCount * 6;
	m_indices = CCP_NEW( "Tr2FontMeasurer/m_indices" ) unsigned short[m_indexCount];
	unsigned short* curIndex = m_indices;

	Tr2AtlasTexture* currentTexture = nullptr;

	// Index of current sprite in the loop
	unsigned int spriteIx = 0;

	// Index of start of run with current texture
	unsigned int startIx = 0;

	// Index of vertex. This is reset for each drawcall, as the vertex buffer
	// is segmented for each drawcall.
	int vertexIx = 0;

	auto addSprites = [&]( const Vector2& offset, const Color& currentColor, Tr2SpriteObjectEffect currentSfx ) {
		for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
		{
			Tr2FontRenderData* rd = *it;

			Color finalColor( rd->color );
			finalColor.r *= currentColor.r;
			finalColor.g *= currentColor.g;
			finalColor.b *= currentColor.b;
			finalColor.a *= currentColor.a;

			for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
			{
				CharacterBitmap& cbit = *sbIter;
				FTC_SBit sbit = cbit.sbit;

				// Spaces don't have any actual pixels so we don't get a texture for them
				// and may as well skip them.
				if( ( sbit->width == 0 ) || ( sbit->height == 0 ) )
				{
					m_vertexCount -= 4;
					m_indexCount -= 6;
					continue;
				}

				Tr2AtlasTexture* at = cbit.atlasTexture;

				if( !at )
				{
					g_fontManager->GetAtlasTextureForSbit( cbit.sbit, &cbit.atlasTexture );
					at = cbit.atlasTexture;
				}

				float textureWidthReciprocal = 1.f;
				float textureHeightReciprocal = 1.f;

				Vector4 tw;
				if( at )
				{
					if( !currentTexture || ( currentTexture->GetTexture() != at->GetTexture() ) )
					{
						if( spriteIx != startIx )
						{
							// This glyph does not come from the same D3D texture as the one
							// before. This can happen if the texture atlas fills up. Rather
							// than failing horribly at rendering the text, we break it up
							// into more draw calls.
							DrawCallEntry entry;
							entry.startSprite = startIx;
							entry.spriteCount = spriteIx - startIx;
							entry.texture = currentTexture;

							m_drawCalls.push_back( entry );

							startIx = spriteIx;
							vertexIx = 0;
						}
						currentTexture = at;
					}

					at->GetTextureWindow( tw );
					textureWidthReciprocal = 1.f / at->GetTextureWidth();
					textureHeightReciprocal = 1.f / at->GetTextureHeight();
				}
				else
				{
					// This shouldn't really happen
					tw = Vector4( 0, 0, 0, 0 );
				}

				for( unsigned int i = 0; i < 4; ++i )
				{
					Tr2Sprite2dD3DVertex& curVertex = m_vertices[spriteIx * 4 + i];

					curVertex.color = finalColor;
					curVertex.glowBrightness = glowBrightness;
					curVertex.blendMode = PackBlendMode( blendMode, target );
					curVertex.spriteEffect = currentSfx;
					curVertex.transformIndex = 0;
					curVertex.tileMode = 0;
				}

				float xZero = tw.x;
				float yZero = tw.y;
				float xOne = xZero + tw.z;
				float yOne = yZero + tw.w;

				float xLeft = (float)( cbit.x + sbit->left ) + offset.x;
				float width = (float)sbit->width;
				float yTop = (float)( cbit.y - sbit->top ) + offset.y;

				// We add one pixel vertically to account for possible drop-shadow
				float height = (float)sbit->height + 1.0f;

				float alphaLeft = CalcAlphaForHorizontal( xLeft );
				float alphaRight = CalcAlphaForHorizontal( xLeft + width );
				float alphaTop = CalcAlphaForVertical( yTop );
				float alphaBottom = CalcAlphaForVertical( yTop + height );

				float xZeroInitial = xZero;
				float yZeroInitial = yZero;
				float xOneInitial = xOne;
				float yOneInitial = yOne;

				if( ( currentSfx == TR2_SFX_BLUR ) || ( currentSfx == TR2_SFX_GLOW ) )
				{
					xLeft -= 1;
					yTop -= 1;
					width += 2;
					height += 2;

					xZero -= textureWidthReciprocal;
					yZero -= textureHeightReciprocal;
					xOne += textureWidthReciprocal;
					yOne += textureHeightReciprocal;
				}

				Tr2Sprite2dD3DVertex& tl = m_vertices[spriteIx * 4];
				tl.position.x = translation.x + xLeft;
				tl.position.y = translation.y + yTop;
				tl.position.z = 0.0f;
				tl.texCoord[0] = Vector2( xZero, yZero );
				tl.texCoord[1] = Vector2( xOneInitial, yOneInitial );
				tl.color.a = alphaLeft * alphaTop * finalColor.a;

				Tr2Sprite2dD3DVertex& tr = m_vertices[spriteIx * 4 + 1];
				tr.position.x = tl.position.x + width;
				tr.position.y = tl.position.y;
				tr.position.z = 0.0f;
				tr.texCoord[0] = Vector2( xOne, yZero );
				tr.texCoord[1] = Vector2( xZeroInitial, yOneInitial );
				tr.color.a = alphaRight * alphaTop * finalColor.a;

				Tr2Sprite2dD3DVertex& br = m_vertices[spriteIx * 4 + 2];
				br.position.x = tl.position.x + width;
				br.position.y = tl.position.y + height;
				br.position.z = 0.0f;
				br.texCoord[0] = Vector2( xOne, yOne );
				br.texCoord[1] = Vector2( xZeroInitial, yZeroInitial );
				br.color.a = alphaRight * alphaBottom * finalColor.a;

				Tr2Sprite2dD3DVertex& bl = m_vertices[spriteIx * 4 + 3];
				bl.position.x = tl.position.x;
				bl.position.y = tl.position.y + height;
				bl.position.z = 0.0f;
				bl.texCoord[0] = Vector2( xZero, yOne );
				bl.texCoord[1] = Vector2( xOneInitial, yZeroInitial );
				bl.color.a = alphaLeft * alphaBottom * finalColor.a;

				curIndex[0] = vertexIx;
				curIndex[1] = vertexIx + 1;
				curIndex[2] = vertexIx + 3;
				curIndex[3] = vertexIx + 3;
				curIndex[4] = vertexIx + 1;
				curIndex[5] = vertexIx + 2;

				++spriteIx;
				curIndex += 6;
				vertexIx += 4;
			}

			if( rd->underline && rd->sbits )
			{
				CharacterBitmap& cbitFirst = *rd->sbits;
				FTC_SBit sbitFirst = cbitFirst.sbit;

				CharacterBitmap& cbitLast = *rd->tail;
				FTC_SBit sbitLast = cbitLast.sbit;

				Vector2 ulTopLeft;
				ulTopLeft.x = cbitFirst.x + (float)sbitFirst->left;
				ulTopLeft.y = cbitFirst.y - (float)rd->underlinePosition;

				float ulWidth = cbitLast.x + sbitLast->xadvance - ulTopLeft.x;

				ulTopLeft += translation + offset;

				float ulHeight = (float)rd->underlineThickness;

				for( unsigned int i = 0; i < 4; ++i )
				{
					Tr2Sprite2dD3DVertex& curVertex = m_vertices[spriteIx * 4 + i];

					curVertex.color = finalColor;
					curVertex.glowBrightness = glowBrightness;
					curVertex.blendMode = PackBlendMode( blendMode, target );
					curVertex.spriteEffect = TR2_SFX_FILL;
					curVertex.transformIndex = 0;
					curVertex.tileMode = 0;
				}

				Tr2Sprite2dD3DVertex& tl = m_vertices[spriteIx * 4];
				tl.position.x = ulTopLeft.x;
				tl.position.y = ulTopLeft.y;

				Tr2Sprite2dD3DVertex& tr = m_vertices[spriteIx * 4 + 1];
				tr.position.x = tl.position.x + ulWidth;
				tr.position.y = tl.position.y;

				Tr2Sprite2dD3DVertex& br = m_vertices[spriteIx * 4 + 2];
				br.position.x = tl.position.x + ulWidth;
				br.position.y = tl.position.y + ulHeight;

				Tr2Sprite2dD3DVertex& bl = m_vertices[spriteIx * 4 + 3];
				bl.position.x = tl.position.x;
				bl.position.y = tl.position.y + ulHeight;

				curIndex[0] = vertexIx;
				curIndex[1] = vertexIx + 1;
				curIndex[2] = vertexIx + 3;
				curIndex[3] = vertexIx + 3;
				curIndex[4] = vertexIx + 1;
				curIndex[5] = vertexIx + 2;

				++spriteIx;
				curIndex += 6;
				vertexIx += 4;
			}
		}
	};

	if( dropShadow )
	{
		addSprites( shadowOffset, shadowColor, shadowSfx );
	}
	addSprites( Vector2( 0.f, 0.f ), color, sfx );

	if( spriteIx != startIx )
	{
		// Hopefully we only get one entry most of the time - it's more efficient.
		// This gathers up whatever is left - that would be all the text unless
		// there is a glyph that didn't fit into the texture atlas.
		DrawCallEntry entry;
		entry.startSprite = startIx;
		entry.spriteCount = spriteIx - startIx;
		entry.texture = currentTexture;

		m_drawCalls.push_back( entry );
	}

	// Spaces are skipped - they don't have a texture
	CCP_ASSERT( curIndex <= &m_indices[m_indexCount] );
	CCP_ASSERT( spriteIx <= spriteCount );
}

void Tr2FontMeasurer::SubmitSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_vertices || !m_indices )
	{
		// It's possible this measurer holds an empty string
		return;
	}

	if( m_indexCount == 0 )
	{
		// This could happen if the measurer only holds spaces
		return;
	}

	// We may need to submit the vertices in batches as there is a limit to the
	// vertex buffer size in the sprite renderer. When it is capturing we have
	// counted the vertices beforehand so we don't need to break up the draw calls.
	unsigned int maxSpriteCount;

	if( renderer->IsCapturing() )
	{
		maxSpriteCount = std::numeric_limits<uint32_t>::max();
	}
	else
	{
		maxSpriteCount = renderer->GetMaxVertexCountPerDrawCall() / 4;
	}

	Tr2Sprite2dD3DVertex* vertices;
	unsigned int vertexCount;
	unsigned short* indices;
	unsigned int indexCount;
	unsigned int vertexOffset = 0;

	for( auto it = m_drawCalls.begin(); it != m_drawCalls.end(); ++it )
	{
		auto entry = *it;
		renderer->SetTexture( 0, entry.texture, S2D_TS_NONE );

		unsigned int startSprite = entry.startSprite;
		unsigned int spriteCount = entry.spriteCount;
		unsigned short* adjustedIndices = nullptr;

		if( spriteCount > maxSpriteCount )
		{
			adjustedIndices = CCP_NEW( "adjustedIndices" ) unsigned short[maxSpriteCount * 6];
		}

		// Ensure we don't submit too many vertices at once
		while( spriteCount > maxSpriteCount )
		{
			vertices = &m_vertices[startSprite * 4];
			vertexCount = maxSpriteCount * 4;

			indexCount = maxSpriteCount * 6;
			indices = AdjustIndicesIfNeeded( startSprite, adjustedIndices, indexCount, vertexOffset );

			renderer->RenderTriangleVerts( vertices, vertexCount, indices, indexCount );

			startSprite += maxSpriteCount;
			spriteCount -= maxSpriteCount;
			vertexOffset += vertexCount;
		}

		// Submit any leftovers (or the whole thing when we're capturing)
		vertices = &m_vertices[startSprite * 4];
		vertexCount = spriteCount * 4;

		indexCount = spriteCount * 6;
		indices = AdjustIndicesIfNeeded( startSprite, adjustedIndices, indexCount, vertexOffset );

		renderer->RenderTriangleVerts( vertices, vertexCount, indices, indexCount );

		if( adjustedIndices )
		{
			CCP_DELETE [] adjustedIndices;
		}
	}
}

void Tr2FontMeasurer::ClearSprites()
{
	if( m_vertices )
	{
		CCP_DELETE [] m_vertices;
		m_vertices = nullptr;
		m_vertexCount = 0;

		CCP_DELETE [] m_indices;
		m_indices = nullptr;
		m_indexCount = 0;
	}

	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;
		for( auto sbIter = rd->sbits.get(); sbIter != nullptr; sbIter = sbIter->next.get() )
		{
			CharacterBitmap& cbit = *sbIter;

			cbit.atlasTexture = nullptr;
		}
	}

	m_drawCalls.clear();
}

unsigned int Tr2FontMeasurer::GetVertexCount() const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_vertices )
	{
		return m_vertexCount;
	}

	unsigned int spriteCount = 0;
	for( auto it = m_committedRenderData.begin(); it != m_committedRenderData.end(); ++it )
	{
		Tr2FontRenderData* rd = *it;

		spriteCount += rd->bitmapCount;
		if( rd->underline )
		{
			++spriteCount;
		}
	}

	return spriteCount * 4;
}

void Tr2FontMeasurer::RegisterForChangeNotification( ITr2FontMeasurerNotifyTarget* p )
{
	m_changeListeners.insert( p );
}

void Tr2FontMeasurer::UnregisterForChangeNotification( ITr2FontMeasurerNotifyTarget* p )
{
	m_changeListeners.erase( p );
}

void Tr2FontMeasurer::NotifyListenersOfChange()
{
	for( auto it = m_changeListeners.begin(); it != m_changeListeners.end(); ++it )
	{
		(*it)->FontMeasurerChanged( this );
	}
}

float Tr2FontMeasurer::CalcAlphaForHorizontal( float x )
{
	if( (m_fadeLeftStart > 0.0f) && (x < m_fadeLeftStart) )
	{
		return 0.0f;
	}

	if( x > m_fadeRightEnd )
	{
		return 0.0f;
	}

	if( (x >= m_fadeLeftEnd) && (x <= m_fadeRightStart) )
	{
		return 1.0f;
	}

	if( (x >= m_fadeLeftStart) && (x <= m_fadeLeftEnd) )
	{
		return (x - m_fadeLeftStart) / (m_fadeLeftEnd - m_fadeLeftStart);
	}

	if( (x >= m_fadeRightStart) && (x <= m_fadeRightEnd) )
	{
		return 1.0f - (x - m_fadeRightStart) / (m_fadeRightEnd - m_fadeRightStart);
	}

	return 1.0f;
}

float Tr2FontMeasurer::CalcAlphaForVertical( float y )
{
	if( y < m_fadeTopStart )
	{
		return 0.0f;
	}

	if( y > m_fadeBottomEnd )
	{
		return 0.0f;
	}

	if( (y >= m_fadeTopEnd) && (y <= m_fadeBottomStart) )
	{
		return 1.0f;
	}

	if( (y >= m_fadeTopStart) && (y <= m_fadeTopEnd) )
	{
		return (y - m_fadeTopStart) / (m_fadeTopEnd - m_fadeTopStart);
	}

	if( (y >= m_fadeBottomStart) && (y <= m_fadeBottomEnd) )
	{
		return 1.0f - (y - m_fadeBottomStart) / (m_fadeBottomEnd - m_fadeBottomStart);
	}

	return 1.0f;
}

float Tr2FontMeasurer::GetFadeLeftStart() const
{
	return m_fadeLeftStart;
}

void Tr2FontMeasurer::SetFadeLeftStart( float val )
{
	m_fadeLeftStart = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeLeftEnd() const
{
	return m_fadeLeftEnd;
}

void Tr2FontMeasurer::SetFadeLeftEnd( float val )
{
	m_fadeLeftEnd = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeRightStart() const
{
	return m_fadeRightStart;
}

void Tr2FontMeasurer::SetFadeRightStart( float val )
{
	m_fadeRightStart = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeRightEnd() const
{
	return m_fadeRightEnd;
}

void Tr2FontMeasurer::SetFadeRightEnd( float val )
{
	m_fadeRightEnd = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeTopStart() const
{
	return m_fadeTopStart;
}

void Tr2FontMeasurer::SetFadeTopStart( float val )
{
	m_fadeTopStart = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeTopEnd() const
{
	return m_fadeTopEnd;
}

void Tr2FontMeasurer::SetFadeTopEnd( float val )
{
	m_fadeTopEnd = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeBottomStart() const
{
	return m_fadeBottomStart;
}

void Tr2FontMeasurer::SetFadeBottomStart( float val )
{
	m_fadeBottomStart = val;
	NotifyListenersOfChange();
}

float Tr2FontMeasurer::GetFadeBottomEnd() const
{
	return m_fadeBottomEnd;
}

void Tr2FontMeasurer::SetFadeBottomEnd( float val )
{
	m_fadeBottomEnd = val;
	NotifyListenersOfChange();
}

unsigned short* Tr2FontMeasurer::AdjustIndicesIfNeeded( unsigned int startSprite, unsigned short* adjustedIndices, unsigned int indexCount, unsigned int vertexOffset )
{
	unsigned short* indices;
	if( startSprite > 0 && adjustedIndices )
	{
		indices = adjustedIndices;
		unsigned int srcIx = startSprite * 6;
		for( unsigned int indexIx = 0; indexIx < indexCount; ++indexIx )
		{
			adjustedIndices[indexIx] = m_indices[srcIx] - vertexOffset;
			++srcIx;
		}
	}
	else
	{
		indices = &m_indices[startSprite * 6];
	}
	return indices;
}

//-----------------------------------------------------------------------------
// Implementation of buffer copy helpers.
//-----------------------------------------------------------------------------

struct RGBA {

	static unsigned char Max(unsigned char l, unsigned char r)
	{
		return l > r ? l : r;
	}

	void BlendMax( const RGBA& other )
	{
		r = Max( r, other.r );
		g = Max( g, other.g );
		b = Max( b, other.b );
		a = Max( a, other.a );
	}

	void ModulateAlphaNum( int m, int num_grays )
	{
		//specifying num_grays
		if( num_grays == 256 )
		{
			//modulate.  use m+1 and >>8 instead of /255, same results
			a = ( a * ( m + 1 ) ) >> 8;
		}
		else
		{
			//a more generic thing
			a = a * m / ( num_grays - 1 );
		}
	}
	void ModulateAlphaMax( int m, int max_grays )
	{
		//specifying max_gray
		if( max_grays == 255 )
		{
			//modulate.  use m+1 and >>8 instead of /255, same results
			a = ( a * ( m + 1 ) ) >> 8;
		}
		else
		{
			//a more generic thing
			a = a * m / max_grays;
		}
	}

	//blending alpha.  1-(1-a1)(1-a2), combining transparencies, can be rewritten as a1+a2-a1*a2.  Since
	//this is scaled by 255, we have to divide by 255, but  k*a/255 is the same as (k*(a+1))>>8 due to precision
	static inline int BlendAlpha( int a1, int a2 )
	{
		return a1 + a2 - ( ( a1 * ( a2 + 1 ) ) >> 8) ;
	}

	void BlendOverAlpha( const RGBA& other )
	{
		if( !a || other.a == 0xff )
		{
			*this = other;
		}
		else if( other.a )
		{
			const int ia = (int)a, oa = (int)other.a;
			const int sa = 255 * ( ia + oa ) - ia * oa; //scaled alpha (by 255)
			r = ( r * ia * ( 255 - oa ) + 255 * other.r * oa ) / sa;
			g = ( g * ia * ( 255 - oa ) + 255 * other.g * oa ) / sa;
			b = ( b * ia * ( 255 - oa ) + 255 * other.b * oa ) / sa;
			a = sa / 255;
		}
	}

	union 
	{
		unsigned int col;
		unsigned char c[4];
		struct 
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
	};
};

void SBit_To_RGBABuffer(
	void *destPtr, 
	int dWidth, 
	int dHeight, 
	int dPitch, 
	FTC_SBit sbit, 
	int left, 
	int top, 
	int col)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	//set up the color in integers.
	RGBA color;
	color.col = col;

	//add the left and top from the SBit
	left += sbit->left;
	top += sbit->top;

	//specify top top-down
	top = dHeight - top; //Note the absence of the -1 here.  FreeType insists on this strange transform, it numbers

	if( sbit->format == 1 )
	{
		// Black and white
		
		// Top as 1 being the bottom line!
		for( int l = 0; l < sbit->height; l++ ) 
		{
			int dLine = top + l;
			if( dLine < 0 || dLine >= dHeight )
			{
				continue;
			}
			RGBA* dl = (RGBA*)( (char*)destPtr + dLine * dPitch );

			for( int c = 0; c < sbit->width; c++ )
			{
				uint8_t* line = (uint8_t*)sbit->buffer + l * sbit->pitch;
				if( !GetBit( line, c ) )
				{
					continue;
				}

				int dCol = left + c;
				if( dCol<0 || dCol >= dWidth )
				{
					continue;
				}

				RGBA* dp = dl + dCol;
				RGBA src = color;
				dp->BlendOverAlpha( src );
			}
		}
	}
	else
	{
		// Grayscale

		int sbitLeft = 0;
		if( left < 0 )
		{
			sbitLeft = -left;
		}

		int sbitRight = sbit->width;
		if( left + sbitRight >= dWidth )
		{
			sbitRight = dWidth - left;
		}

		int sbitTop = 0;
		if( top + sbitTop < 0 )
		{
			sbitTop = -top;
		}

		int sbitBottom = sbit->height;
		if( top + sbitBottom >= dHeight )
		{
			sbitBottom = dHeight - top;
		}

		// Top as 1 being the bottom line!
		for( int l = sbitTop; l < sbitBottom; ++l ) 
		{
			int dLine = top + l;
			RGBA* dl = (RGBA*)( (char*)destPtr + dLine * dPitch );

			for( int c = sbitLeft; c < sbitRight; ++c )
			{
				const unsigned char* sp = ( (const unsigned char*)sbit->buffer ) + l * sbit->pitch + c;
				const int gray = (int)*sp;
				if( !gray )
				{
					continue;
				}

				int dCol = left + c;
				RGBA* dp = dl + dCol;
				RGBA src = color;
				src.ModulateAlphaMax( gray, sbit->max_grays );
				dp->BlendOverAlpha( src );
			}
		}
	}
}

void Underline_To_RGBABuffer(void* destPtr, int dWidth, int dHeight, int dPitch, int left, int top, int width, int thickness, int col)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	//set up the color in integers.
	RGBA p;
	p.col = col;

	//specify top top-down
	top = dHeight - top; //Note the absence of the -1 here.  FreeType insists on this strange transform, it numbers
	//top as 1 being the bottom line!
	for( int l = 0; l < thickness; l++ ) 
	{
		int dLine = top + l;
		if( dLine < 0 || dLine >= dHeight )
		{
			continue;
		}
		RGBA* dl = (RGBA*)( (char*)destPtr + dLine * dPitch );
		for( int pixel = 0; pixel < width; ++pixel )
		{
			int dCol = left + pixel;
			if( dCol<0 || dCol >= dWidth )
			{
				continue;
			}

			RGBA src = p;
			RGBA* dp = dl + dCol;
			dp->BlendOverAlpha( src );
		}
	}
}
