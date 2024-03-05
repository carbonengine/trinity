#include "StdAfx.h"

#include "TriDebugTextRenderer.h"
#include "Tr2Renderer.h"
#include "pdm.h"

#ifndef _WIN32
#include "TriDebugTextRendererBitmaps.h"
#endif

CCP_STATS_DECLARE( triDebugTextRenderersAlive, "Trinity/TriDebugTextRenderer", false, CST_COUNTER_LOW, "Count of TriDebugTextRenderers alive" );

TriDebugTextRenderer::TriDebugTextRenderer() :
	m_entries( "TriDebugTextRenderer/m_entries" )
#ifdef _WIN32
    , m_bitmapData( nullptr ),
	m_bitmapWidth( 0 ),
	m_bitmapHeight( 0 ),
	m_bitmap( nullptr )
#endif
{
#ifdef _WIN32
	const char* font = PDM::IsWine() ? "Menlo" : "Lucida Console";
	const char* fallbackFont = "Courier New";
	for( int i = 0; i < 3; ++i )
    {
        static int heights[3] = { -9, -12, -16 };
		m_dcFonts[i] = CreateFont( heights[i], 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, 0, font );
		if( m_dcFonts[i] == NULL )
		{
			m_dcFonts[i] = CreateFont( heights[i], 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, 0, fallbackFont );

			if( m_dcFonts[i] == NULL )
			{
				CCP_LOGERR( "Could not create debug text renderer font: %s or %s, %d", font, fallbackFont, heights[i] );
			}
		}
    }
	HDC screenDC = GetDC( NULL );
	m_dc = CreateCompatibleDC( screenDC );
	ReleaseDC( NULL, screenDC );
#endif
	CCP_STATS_INC( triDebugTextRenderersAlive );
}

TriDebugTextRenderer::~TriDebugTextRenderer()
{
    CCP_STATS_DEC( triDebugTextRenderersAlive );

#ifdef _WIN32
    for( int i = 0; i < 3; ++i )
    {
        DeleteObject( m_dcFonts[i] );
    }

    DeleteDC( m_dc );
    DeleteObject( m_bitmap );
#endif
}

void TriDebugTextRenderer::Vprintf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, va_list args )
{
	const int BUFFER_SIZE = 1024;
	static char buffer[BUFFER_SIZE];

	vsnprintf_s( buffer, BUFFER_SIZE, _TRUNCATE, msg, args );
	
	TextEntry e;
	e.m_font = font;
	e.m_rect = rect;
	e.m_format = format;
	e.m_color = color;
	e.m_text = CCP_STRDUP( "TriDebugTextRenderer/Vprintf/buffer", buffer );

	m_entries.push_back( e );
}

void TriDebugTextRenderer::VprintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, va_list args )
{
	const int BUFFER_SIZE = 1024;
	static char buffer[BUFFER_SIZE];

	vsnprintf_s( buffer, BUFFER_SIZE, _TRUNCATE, msg, args );
	DrawText( renderContext, font, buffer, rect, format, color );
}

void TriDebugTextRenderer::Printf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, ... )
{
    va_list args;
    va_start( args, msg );

    Vprintf( font, rect, format, color, msg, args );
	va_end( args );
}


void TriDebugTextRenderer::PrintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, const Vector4& color, const char* msg, ... )
{
	va_list args;
	va_start( args, msg );

	VprintfImmediate( renderContext, font, rect, format, color, msg, args );
	va_end( args );
}

void TriDebugTextRenderer::Render( Tr2RenderContext& renderContext )
{
    for( TrackableStdList<TextEntry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it )
    {
        DrawText( renderContext, it->m_font, it->m_text, it->m_rect, it->m_format, it->m_color );
	}
}

void TriDebugTextRenderer::Clear()
{
    for( TrackableStdList<TextEntry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it )
    {
        CCP_FREE( (void*)it->m_text );
    }
    m_entries.clear();
}

void TriDebugTextRenderer::ReleaseResources( TriStorage s )
{
	m_texture = Tr2TextureAL();
}

bool TriDebugTextRenderer::OnPrepareResources()
{
    return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Draws text into current render target.  
// Arguments:
//   font - Font to use
//   string - NULL-terminated string to render
//   rect - Rectangle to use in the render target
//   format - Bitfield of TRI_DFS_... constants
//   color - Color to use for rendering (alpha component is ignored)
// --------------------------------------------------------------------------------------
void TriDebugTextRenderer::DrawText( Tr2RenderContext& renderContext, TriDebugFont font, const char* string, const Tr2Rect& rect, uint32_t format, const Vector4& vecColor)
{
	uint32_t color = Color( vecColor.x, vecColor.y, vecColor.z, vecColor.w );
	
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return;
	}

#ifdef WIN32
	if( !m_dc )
	{
		return;
	}
    
	// Measure actual text size
	HGDIOBJ oldFont = SelectObject( m_dc, m_dcFonts[font] );
	ON_BLOCK_EXIT( [&] { SelectObject( m_dc, oldFont ); } );
	RECT realRect = { rect.left, rect.top, rect.right, rect.bottom };
	::DrawText( m_dc, string, -1, &realRect, format | DT_CALCRECT );
	RECT size = realRect;
	OffsetRect( &size, -size.left, -size.top );
	unsigned width = size.right;
	unsigned height = size.bottom;
	if( width == 0 || height == 0 )
	{
		return;
	}

	if( width > m_bitmapWidth || height > m_bitmapHeight )
	{
		DeleteObject( m_bitmap );
		m_bitmapData = nullptr;
		m_bitmapWidth = max( width, m_bitmapWidth );
		m_bitmapHeight = max( height, m_bitmapHeight );
		HDC screenDC = GetDC( NULL );

		BITMAPINFO info;
		memset( &info, 0, sizeof( info ) );
		info.bmiHeader.biSize = sizeof( info );
		info.bmiHeader.biWidth = m_bitmapWidth;    
		info.bmiHeader.biHeight = -int( m_bitmapHeight );  
		info.bmiHeader.biPlanes = 1;    
		info.bmiHeader.biBitCount = 32;    
		info.bmiHeader.biCompression = BI_RGB;    
		info.bmiHeader.biSizeImage = 0;  
		info.bmiHeader.biXPelsPerMeter = 0;    
		info.bmiHeader.biYPelsPerMeter = 0;    
		info.bmiHeader.biClrUsed = 0;    
		info.bmiHeader.biClrImportant = 0;

		m_bitmap = CreateDIBSection( screenDC, &info, DIB_RGB_COLORS, (void**)&m_bitmapData, nullptr, 0 );
		if( !m_bitmapData )
		{
			DeleteObject( m_bitmap );
			m_bitmapWidth = 0;
			m_bitmapHeight = 0;
			m_bitmap = nullptr;
		}
		ReleaseDC( NULL, screenDC );
	}

	if( !m_bitmap || !m_bitmapData )
	{
		return;
	}

	// Render into offscreen GDI bitmap
	HGDIOBJ bkBitmap = SelectObject( m_dc, m_bitmap );
	FillRect( m_dc, &size, (HBRUSH) GetStockObject( BLACK_BRUSH ) );
	SetTextColor( m_dc, RGB( 255, 255, 255 ) );
	SetBkMode( m_dc, TRANSPARENT );
	::DrawText( m_dc, string, -1, &size, format );
	SelectObject( m_dc, bkBitmap );

	// Flush GDI commands before accessing bitmap data
	GdiFlush();

	if( !m_texture.IsValid()					|| 
		m_texture.GetWidth()  < m_bitmapWidth	|| 
		m_texture.GetHeight() < m_bitmapHeight )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN( m_texture.Create(		
			Tr2BitmapDimensions( m_bitmapWidth, m_bitmapHeight, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), 
			Tr2GpuUsage::SHADER_RESOURCE,
			Tr2CpuUsage::WRITE_OFTEN,
			renderContext ) );
	}

	// Copy to texture
	uint32_t b = color & 0xff;
	uint32_t g = ( color >> 8 ) & 0xff;
	uint32_t r = ( color >> 16 ) & 0xff;
	void* textureData;
	unsigned pitch;
	CR_RETURN( m_texture.MapForWriting( Tr2TextureSubresource( 0 ), textureData, pitch, renderContext ) );
	for( int j = 0; j < size.bottom; ++j )
	{
		uint32_t* outRow = reinterpret_cast<uint32_t*>( reinterpret_cast<char*>( textureData ) + j * pitch );
		for( int i = 0; i < size.right; ++i )
		{
			uint32_t alpha = m_bitmapData[j * m_bitmapWidth * 4 + i * 4] & 0xff;
			outRow[i] = ( ( b * alpha ) >> 8 ) |
				( ( ( g * alpha ) >> 8 ) << 8 ) |
				( ( ( r * alpha ) >> 8 ) << 16 ) |
				( alpha << 24 );
		}
	}
	m_texture.UnmapForWriting( renderContext );
#else
	Tr2Rect size = { 0, 0, 0, s_charHeight[font] };
	int rowWidth = 0;
	for( const char* ch = string; *ch; ++ch )
	{
		if( *ch > 0 )
		{
			if( *ch == '\n' )
			{
				size.bottom += s_charHeight[font];
				if( size.right < rowWidth )
				{
					size.right = rowWidth;
				}
				rowWidth = 0;
			}
			else
			{
				rowWidth += s_charWidths[font][static_cast<int>( *ch )];
			}
		}
	}
	if( size.right < rowWidth )
	{
		size.right = rowWidth;
	}

	if( !m_texture.IsValid()					|| 
		m_texture.GetWidth()  < unsigned( size.right )	|| 
		m_texture.GetHeight() < unsigned( size.bottom ) )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN( m_texture.Create(
			Tr2BitmapDimensions( size.right, size.bottom, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ),
			Tr2GpuUsage::SHADER_RESOURCE,
			Tr2CpuUsage::WRITE_OFTEN,
			renderContext ) );
	}

	// Copy to texture
	uint32_t b = color & 0xff;
	uint32_t g = ( color >> 8 ) & 0xff;
	uint32_t r = ( color >> 16 ) & 0xff;
	void* textureData;
	unsigned pitch;
	CR_RETURN( m_texture.MapForWriting( Tr2TextureSubresource( 0 ), textureData, pitch, renderContext ) );
	int offsetX = 0;
	int offsetY = 0;
	int pixelWidths = int( s_charPixelsArraySizes[font] / s_charHeight[font] );
	for( const char* ch = string; *ch; ++ch )
	{
		int index = *ch;
		if( *ch > 0 )
		{
			if( *ch == '\n' )
			{
				offsetY += s_charHeight[font];
				offsetX = 0;
			}
			else
			{
				for( int j = 0; j < s_charHeight[font]; ++j )
				{
					uint32_t* outRow = reinterpret_cast<uint32_t*>( reinterpret_cast<char*>( textureData ) + ( j + offsetY ) * pitch + offsetX * 4 );
					for( int i = 0; i < s_charWidths[font][index]; ++i )
					{
						uint32_t alpha = s_charPixels[font][pixelWidths * j + i + s_charOffsets[font][index]];
						outRow[i] = ( ( b * alpha ) >> 8 ) |
							( ( ( g * alpha ) >> 8 ) << 8 ) |
							( ( ( r * alpha ) >> 8 ) << 16 ) |
							( alpha << 24 );
					}
				}
			}
			offsetX += s_charWidths[font][index];
		}
	}
	m_texture.UnmapForWriting( renderContext );
#endif
	// Calculate actual text position in render target
	int x, y;
	if( format & TRI_DFS_RIGHT )
	{
		x = rect.right - size.right;
	}
	else if( format & TRI_DFS_CENTER )
	{
		x = ( rect.right + rect.left - size.right ) / 2; 
	}
	else
	{
		x = rect.left;
	}
	if( format & TRI_DFS_BOTTOM )
	{
		y = rect.bottom - size.bottom;
	}
	else if( format & TRI_DFS_VCENTER )
	{
		y = ( rect.bottom + rect.top - size.bottom ) / 2; 
	}
	else
	{
		y = rect.top;
	}

	// Render texture
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_SPRITE2D );
	renderContext.m_esm.PushViewport();
	renderContext.m_esm.SetViewport( size.right, size.bottom, x, y, 0.f, 1.f );
	Tr2Renderer::DrawTexture( renderContext, m_texture, Vector2( 0.f, 0.f ), Vector2( float( size.right ) / m_texture.GetWidth(), float( size.bottom ) / m_texture.GetHeight() ) );
	renderContext.m_esm.PopViewport();
}

