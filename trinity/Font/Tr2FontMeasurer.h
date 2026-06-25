// Copyright © 2011 CCP ehf.

#pragma once

#ifndef Tr2FontMeasurer_h
#define Tr2FontMeasurer_h

#include "Tr2SBit.h"
#include "Tr2FontManager.h"
#include "Sprite2d/ITr2Sprite2dRenderer.h"

BLUE_DECLARE( Tr2FontMeasurer );
BLUE_DECLARE( Tr2AtlasTexture );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2HostBitmap );

struct Tr2FontRenderData;
class Tr2Sprite2dScene;

class ITr2FontMeasurerNotifyTarget
{
public:
	virtual void FontMeasurerChanged( Tr2FontMeasurer* p ) = 0;
};

// The Tr2FontMeasurer class is used to measure and render text. It is primarily
// intended to be used by Python, and is the workhorse for uicls.Label.
class Tr2FontMeasurer : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2FontMeasurer( IRoot* lockobj = NULL );
	~Tr2FontMeasurer();

	// Resets the measurer to the initial state
	void Reset();

	// Adds text to the measurer object, advancing the cursor horizontally
	unsigned int AddText( const std::wstring& text );

	// Commits text previously added with AddText, shifting it by the given x,y coordinates
	void CommitText( int shiftX, int shiftY );

	// Cancels the last added text. Can be called repeatedly to undo
	// multiple AddText calls. Text that has been committed cannot be canceled.
	void CancelLastText();

	// Returns true if any text has been added and committed
	bool HasCommittedText();

	// Returns true if any text has been added and not committed
	bool HasUncommittedText();

	int GetIndexAtPos( int x );
	int GetWidthAtIndex( int ix );

	// Renders the current contents of the measurer into the given atlas texture
	void DrawToAtlasTexture( Tr2AtlasTexture* atlasTexture );

	// Renders the current contents of the measurer into the given texture
	void DrawToTexture( TriTextureRes* texture );

	// Renders the current contents of the measurer into the given hostbitmap
	void DrawToHostBitmap( Tr2HostBitmap* hostBitmap );

	// Prepares sprites based on the current contents of the measurer for later
	// rendering with SubmitSprites.
	void PrepareSprites( Tr2Sprite2dScene* renderer, const Vector2& translation, const Color& color, Tr2SpriteObjectEffect sfx, Tr2SpriteObjectBlendMode blendMode, Tr2SpriteTarget target, float glowBrightness, bool dropShadow, const Vector2& shadowOffset, const Color& shadowColor, Tr2SpriteObjectEffect shadowSfx );

	// Submits sprites previously prepared with PrepareSprites for rendering.
	void SubmitSprites( Tr2Sprite2dScene* renderer );

	// Clears any sprites prepared with PrepareSprites.
	void ClearSprites();

	void SetFont( const std::string& font );
	const std::string GetFont() const;

	void SetFontSize( int size );
	int GetFontSize() const;

	float GetFadeLeftStart() const;
	void SetFadeLeftStart( float val );

	float GetFadeLeftEnd() const;
	void SetFadeLeftEnd( float val );

	float GetFadeRightStart() const;
	void SetFadeRightStart( float val );

	float GetFadeRightEnd() const;
	void SetFadeRightEnd( float val );

	float GetFadeTopStart() const;
	void SetFadeTopStart( float val );

	float GetFadeTopEnd() const;
	void SetFadeTopEnd( float val );

	float GetFadeBottomStart() const;
	void SetFadeBottomStart( float val );

	float GetFadeBottomEnd() const;
	void SetFadeBottomEnd( float val );

	unsigned int GetVertexCount() const;

	void RegisterForChangeNotification( ITr2FontMeasurerNotifyTarget* p );
	void UnregisterForChangeNotification( ITr2FontMeasurerNotifyTarget* p );

protected:
	void DrawToBuffer( uint32_t destWidth, uint32_t destHeight, void* pData, uint32_t pitch );

	void NotifyListenersOfChange();
	float CalcAlphaForHorizontal( float x );
	float CalcAlphaForVertical( float y );

	unsigned short* AdjustIndicesIfNeeded( unsigned int startSprite, unsigned short* adjustedIndices, unsigned int indexCount, unsigned int vertexOffset );

protected:
	std::string m_font;
	int m_fontSize;
	int m_cursorX;
	int m_cursorY;
	int m_letterSpace;
	int m_limit;

	// Ascender value for the current line (reset on CommitText)
	int m_ascender;

	// Descender value for the current line (reset on CommitText)
	int m_descender;

	// Ascender value for the current font
	int m_currentAscender;

	// Descender value for the current font
	int m_currentDescender;

	uint32_t m_color;
	int32_t GetColor();
	void SetColor( int32_t color );

	bool m_underline;

	float m_fadeLeftStart;
	float m_fadeLeftEnd;
	float m_fadeRightStart;
	float m_fadeRightEnd;

	float m_fadeTopStart;
	float m_fadeTopEnd;
	float m_fadeBottomStart;
	float m_fadeBottomEnd;

	TrackableStdVector<Tr2FontRenderData*> m_renderData;
	TrackableStdVector<Tr2FontRenderData*> m_committedRenderData;

	// Set to NULL when font or font size changes - this triggers a face lookup.
	FaceID m_currentFace;
	FT_Face m_currentFtFace;
	FTC_ImageTypeRec m_imgType;

	// Parameters for fallback font - only initialized when needed
	FaceID m_fallbackFace;
	FT_Face m_ftFallbackFace;
	int m_fallbackCmapIndex;
	FTC_ImageTypeRec m_imgTypeFallback;

	struct Tr2Sprite2dD3DVertex* m_vertices;
	unsigned short* m_indices;
	unsigned int m_vertexCount;
	unsigned int m_indexCount;

	struct DrawCallEntry
	{
		unsigned int startSprite;
		unsigned int spriteCount;
		Tr2AtlasTexturePtr texture;
	};

	TrackableStdVector<DrawCallEntry> m_drawCalls;

	TrackableStdSet<ITr2FontMeasurerNotifyTarget*> m_changeListeners;
};

TYPEDEF_BLUECLASS( Tr2FontMeasurer );

#endif // Tr2FontMeasurer_h