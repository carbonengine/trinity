#include "StdAfx.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Renderer.h"
#include "Tr2ImageIOHelpers.h"


using namespace Tr2RenderContextEnum;

Tr2TextureAtlas::Tr2TextureAtlas( IRoot* lockobj ) :
	m_format( PIXEL_FORMAT_B8G8R8A8_UNORM ),
	m_width( 2048 ),
	m_height( 2048 ),
	m_freeAreas( "Tr2TextureAtlas/m_freeAreas" ),
	m_texturesInAtlas( "Tr2TextureAtlas/m_texturesInAtlas" ),
	m_texturesOutsideAtlas( "Tr2TextureAtlas/m_texturesOutsideAtlas" ),
	m_optimizeOnRemoval( true ),
	m_paintEmptyAreas( false ),
	m_createOutsiders( true ),
	m_margin( 2 ),
	m_maxTextureArea( 512 * 512 ),
	m_freeMaxWidth( -1 ), 
	m_freeMaxHeight( -1 ),
	m_optimizationWarranted( false ),
	m_areasFreedSinceLastCollapse( 0 ),
	m_hasMipMaps( false ),
	m_freeTexels( 0 ),
	m_mipLevels( 0 )
{
}

Tr2TextureAtlas::~Tr2TextureAtlas()
{
}

void Tr2TextureAtlas::Initialize( PixelFormat fmt, unsigned int width, unsigned int height )
{
	ReleaseResources( TRISTORAGE_ALL );

	m_format = fmt;
	m_width = width;
	m_height = height;

	PrepareResources();
}

void Tr2TextureAtlas::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_MANAGEDMEMORY ) 
	{
		m_texture = Tr2TextureAL();

		for( AreaList_t::iterator it = m_freeAreas.begin(); it != m_freeAreas.end(); ++it )
		{
			CCP_DELETE *it;
		}
		m_freeAreas.clear();
		m_dirtyMipRegions.clear();

		m_onTextureChange();
	}
}

// This gets called after a device reset
bool Tr2TextureAtlas::OnPrepareResources()
{
	if( m_texture.IsValid() )
	{
		return true;
	}
	
	HRESULT hr;

	do
	{
		if( m_height <= 1 || m_width <= 1 )
		{
			return false;
		}
		
		m_mipLevels = m_hasMipMaps ? unsigned( 0.5 + log(double(std::max(m_height, m_width))) / log(2.0) ) : 1;
		USE_MAIN_THREAD_RENDER_CONTEXT();
		hr = m_texture.Create(
			Tr2BitmapDimensions( m_width, m_height, m_mipLevels, m_format ), 
			Tr2GpuUsage::SHADER_RESOURCE,
			Tr2CpuUsage::READ | Tr2CpuUsage::WRITE,
			renderContext );

		if( FAILED(hr) )
		{
			if( m_width > m_height )
			{
				m_width /= 2;
			}
			else
			{
				m_height /= 2;
			}
		}
	} while( FAILED(hr) );

	Tr2TextureAtlasArea* area = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
	area->type = Tr2TextureAtlasArea::FREE;
	area->rect.left = 0;
	area->rect.top = 0;
	area->rect.right = m_width;
	area->rect.bottom = m_height;
	area->tex = NULL;

	m_freeAreas.clear();
	FreeArea( area );

	m_freeTexels = m_width * m_height;
	m_freeMaxWidth = m_width;
	m_freeMaxHeight = m_height;

	m_onTextureChange();

	return true;
}

// This gets called when atlas textures are preparing themselves
bool Tr2TextureAtlas::DoPrepare( Tr2AtlasTexture* tex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	unsigned int width = tex->m_loadedBitmap->GetWidth();
	unsigned int height = tex->m_loadedBitmap->GetHeight();
	const unsigned areaWidth = width + m_margin * 2;
	const unsigned areaHeight = height + m_margin * 2;

	// Large textures are not good for atlassing, end up with a lot of small outsiders
	if( areaWidth * areaHeight > m_maxTextureArea )
	{
		return false;
	}

	Tr2TextureAtlasArea* area = GetFreeArea( areaWidth, areaHeight );

	if( !area )
	{
		// There isn't room in the atlas for this texture. This isn't an error per se,
		// we'll fall back to creating it as a standalone texture.
		return false;
	}

	area->type = Tr2TextureAtlasArea::IN_USE;
	area->tex = tex;
	tex->m_atlasArea = area;

	tex->m_x = area->rect.left + m_margin;
	tex->m_y = area->rect.top + m_margin;
	tex->m_width = width;
	tex->m_height = height;
	tex->m_textureWidth = m_width;
	tex->m_textureHeight = m_height;

	Tr2ImageIOHelpers::CopyToTexture( *tex->m_loadedBitmap, m_texture, area->rect.left, area->rect.top, m_margin, renderContext );

	if( m_hasMipMaps ) 
	{
		m_dirtyMipRegions.push_back( area->rect );
	}

	RegisterInsider( tex );

	return true;	
}

// Register an atlas texture that lives inside the atlas
void Tr2TextureAtlas::RegisterInsider( Tr2AtlasTexture* tex )
{
	m_texturesInAtlas.insert( tex );
}

// Register an atlas texture that lives outside the atlas. It will be pulled
// into the atlas if space becomes available.
void Tr2TextureAtlas::RegisterOutsider( Tr2AtlasTexture* tex )
{
	CCP_ASSERT( !tex->m_atlasArea );
	m_texturesOutsideAtlas.insert( tex );
}

void Tr2TextureAtlas::RemoveFromAtlas( Tr2AtlasTexture* tex )
{
	m_texturesInAtlas.erase( tex );
	m_texturesOutsideAtlas.erase( tex );

	Tr2TextureAtlasArea* area = tex->m_atlasArea;
	if( area )
	{
		m_freeTexels += (area->rect.bottom - area->rect.top) * (area->rect.right - area->rect.left);
		tex->m_atlasArea = NULL;

		area->type = Tr2TextureAtlasArea::FREE;
		FreeArea( area );		

		PaintEmptyArea( area );

		if( m_optimizeOnRemoval )
		{
			PullInOutsiders();
		}
		else
		{
			UpdateFreeMaxima();
		}
	}
}

//Consolidates free areas, attempting to improve the 'squareness' of the free regions of the atlas
//to improve further usage.
void Tr2TextureAtlas::ConsolidateFreeAreas()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( m_freeAreas.size() < 2 )
	{
		return;
	}

	struct Squareness {
		static float Metric( const Tr2Rect& a ) {
			const int w = a.right - a.left;
			const int h = a.bottom - a.top;
			if( w == 0 || h == 0 ) return 0.f;
			const float d = float(w - h);
			return d * d / float( w * h );
		}
	};
	
	bool restartLoop = true;

	while( restartLoop )
	{
		restartLoop = false;
		for( AreaList_t::iterator i = m_freeAreas.begin(); i != m_freeAreas.end(); ++i )
		{
			const auto& rect_i = (*i)->rect;
			const float square_i = Squareness::Metric( rect_i );
			AreaList_t::iterator j = i;
			for( ++j; j != m_freeAreas.end(); ++j )
			{
				const auto& rect_j = (*j)->rect;
				const float square_j = Squareness::Metric( rect_j );
				const float squareStart = square_i + square_j;

				
				if( rect_i.bottom == rect_i.top ||
					rect_i.top == rect_j.bottom )
				{
					//areas are vertically adjacent
					if( rect_i.right == rect_j.right &&
						rect_i.left == rect_j.left )
					{
						//have an exact match, can just merge these two areas
						Tr2Rect sum;
						sum.left = rect_i.left;
						sum.right = rect_i.right;
						sum.top = std::min( rect_i.top, rect_j.top );
						sum.bottom = std::max( rect_i.bottom, rect_j.bottom );
						
						CCP_DELETE (*j);
						j = m_freeAreas.erase( j );
						(*i)->rect = sum;

						PaintEmptyArea( *i );

						restartLoop = true;
					}
					else if( rect_i.right == rect_j.right )
					{
						//areas are right aligned, can rotate the splitting plane
						Tr2Rect l,r;
						l.left = std::min( rect_i.left, rect_j.left );
						l.right = std::max( rect_i.left, rect_j.left );
						r.left = l.right;
						r.right = rect_i.right;
						r.top = std::min( rect_i.top, rect_j.top );
						r.bottom = std::max( rect_i.bottom, rect_j.bottom );
						if( rect_i.left < rect_j.left )
						{
							l.top = rect_i.top;
							l.bottom = rect_i.bottom;
						}
						else
						{
							l.top = rect_j.top;
							l.bottom = rect_j.bottom;
						}

						const float square_l = Squareness::Metric( l );
						const float square_r = Squareness::Metric( r );
						const float squareNew = square_l + square_r;

						if( squareNew < squareStart )
						{
							restartLoop = true;
							(*i)->rect = l;
							(*j)->rect = r;
							PaintEmptyArea( *i );
							PaintEmptyArea( *j );
						}
					}
					else if( rect_i.left == rect_j.left )
					{
						//left aligned, can rotate the splitting plane
						Tr2Rect l,r;
						r.left = std::min( rect_i.right, rect_j.right );
						r.right = std::max( rect_i.right, rect_j.right );
						l.left = rect_i.left;
						l.right = r.left;
						l.top = std::min( rect_i.top, rect_j.top );
						l.bottom = std::max( rect_i.bottom, rect_j.bottom );
						if( rect_i.right > rect_j.right )
						{
							r.top = rect_i.top;
							r.bottom = rect_i.bottom;
						}
						else
						{
							r.top = rect_j.top;
							r.bottom = rect_j.bottom;
						}

						const float square_l = Squareness::Metric( l );
						const float square_r = Squareness::Metric( r );
						const float squareNew = square_l + square_r;

						if( squareNew < squareStart )
						{
							restartLoop = true;
							(*i)->rect = l;
							(*j)->rect = r;
							PaintEmptyArea( *i );
							PaintEmptyArea( *j );
						}
					}
					else
					{
						//neither left nor right aligned, can split this into three areas or just skip it

					}
				}
				else if( rect_i.left == rect_j.right ||
						 rect_i.right == rect_j.left )
				{
					//areas are horizontally adjacent
					//don't need to test for left/right both aligned, caught by above
					if( rect_i.top == rect_j.top )
					{
						//areas are top aligned, can rotate the splitting plane
						Tr2Rect t,b;
						t.top = rect_i.top;
						t.bottom = std::min( rect_i.bottom, rect_j.bottom );
						t.left = std::min( rect_i.left, rect_i.bottom );
						t.right = std::max( rect_i.right, rect_j.right );
						b.top = t.bottom;
						b.bottom = std::max( rect_i.bottom, rect_j.bottom );
						
						if( rect_i.bottom > rect_j.bottom )
						{
							b.left = rect_i.left;
							b.right = rect_i.right;
						}
						else
						{
							b.left = rect_j.left;
							b.right = rect_j.right;
						}

						const float square_t = Squareness::Metric( t );
						const float square_b = Squareness::Metric( b );
						const float squareNew = square_t + square_b;

						if( squareNew < squareStart )
						{
							restartLoop = true;
							(*i)->rect = t;
							(*j)->rect = b;
							PaintEmptyArea( *i );
							PaintEmptyArea( *j );
						}
					}
					else if( rect_i.bottom == rect_j.bottom )
					{
						//bottom aligned, can rotate the splitting plane
						Tr2Rect t,b;
						t.top = std::min( rect_i.top, rect_j.top );
						t.bottom = std::max( rect_i.top, rect_j.top );
						b.left = std::min( rect_i.left, rect_i.bottom );
						b.right = std::max( rect_i.right, rect_j.right );
						b.top = t.bottom;
						b.bottom = rect_i.bottom;
						
						if( rect_i.top < rect_j.top )
						{
							t.left = rect_i.left;
							t.right = rect_i.right;
						}
						else
						{
							t.left = rect_j.left;
							t.right = rect_j.right;
						}

						const float square_t = Squareness::Metric( t );
						const float square_b = Squareness::Metric( b );
						const float squareNew = square_t + square_b;

						if( squareNew < squareStart )
						{
							restartLoop = true;
							(*i)->rect = t;
							(*j)->rect = b;
							PaintEmptyArea( *i );
							PaintEmptyArea( *j );
						}
					}
					else
					{
						//neither top nor bottom aligned, can split into three areas or skip
					}
				}
			}
		}
	}
}

class FreeAreasLeftToRight
{
public:
	bool operator()( Tr2TextureAtlasArea* area1, Tr2TextureAtlasArea* area2 )
	{
		if( area1->rect.left < area2->rect.left )
		{
			return true;
		}
		else if( area1->rect.left == area2->rect.left )
		{
			if( area1->rect.right < area2->rect.right )
			{
				return true;
			}
		}

		return false;
	}
};

class FreeAreasTopToBottom
{
public:
	bool operator()( Tr2TextureAtlasArea* area1, Tr2TextureAtlasArea* area2 )
	{
		if( area1->rect.top < area2->rect.top )
		{
			return true;
		}
		else if( area1->rect.top == area2->rect.top )
		{
			if( area1->rect.bottom < area2->rect.bottom )
			{
				return true;
			}
		}

		return false;
	}
};

void Tr2TextureAtlas::CollapseFreeAreas()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_freeAreas.size() < 2 )
	{
		return;
	}
	
	m_optimizationWarranted = false;
	m_areasFreedSinceLastCollapse = 0;

	bool restartLoop = true;

	while( restartLoop )
	{
		restartLoop = false;

		m_freeAreas.sort( FreeAreasLeftToRight() );

		for( AreaList_t::iterator outer = m_freeAreas.begin(); outer != m_freeAreas.end(); ++outer )
		{
			AreaList_t::iterator candidate = outer;
			Tr2TextureAtlasArea* area1 = *outer;

			for( ++candidate; candidate != m_freeAreas.end(); )
			{
				Tr2TextureAtlasArea* area2 = *candidate;

				if( area1->rect.right > area2->rect.left )
				{
					++candidate;
					continue;
				}

				if( area1->rect.right < area2->rect.left )
				{
					break;
				}

				Tr2Rect newRect;
				bool isAdjacent = false;
				if( (area1->rect.top == area2->rect.top) && (area1->rect.bottom == area2->rect.bottom) )
				{
					// Top/bottom matches, see if areas are adjacent left or right
					if( area1->rect.right == area2->rect.left )
					{
						newRect.left = area1->rect.left;
						newRect.right = area2->rect.right;
						newRect.top = area1->rect.top;
						newRect.bottom = area1->rect.bottom;
						isAdjacent = true;
					}
					else if( area1->rect.left == area2->rect.right )
					{
						newRect.left = area2->rect.left;
						newRect.right = area1->rect.right;
						newRect.top = area1->rect.top;
						newRect.bottom = area1->rect.bottom;
						isAdjacent = true;
					}
				}

				if( isAdjacent )
				{
					CCP_DELETE( area2 );
					// cppcheck-suppress uninitStructMember
					area1->rect = newRect;
					candidate = m_freeAreas.erase( candidate );

					restartLoop = true;
				}
				else
				{
					++candidate;
				}
			}
		}

		m_freeAreas.sort( FreeAreasTopToBottom() );

		for( AreaList_t::iterator outer = m_freeAreas.begin(); outer != m_freeAreas.end(); ++outer )
		{
			AreaList_t::iterator candidate = outer;
			Tr2TextureAtlasArea* area1 = *outer;

			for( ++candidate; candidate != m_freeAreas.end(); )
			{
				Tr2TextureAtlasArea* area2 = *candidate;

				if( area1->rect.bottom > area2->rect.top )
				{
					++candidate;
					continue;
				}

				if( area1->rect.bottom < area2->rect.top )
				{
					break;
				}

				Tr2Rect newRect;
				bool isAdjacent = false;
				if( (area1->rect.left == area2->rect.left) && (area1->rect.right == area2->rect.right) )
				{
					// Left/right matches, see if areas are adjacent top or bottom
					if( area1->rect.bottom == area2->rect.top )
					{
						newRect.left = area1->rect.left;
						newRect.right = area1->rect.right;
						newRect.top = area1->rect.top;
						newRect.bottom = area2->rect.bottom;
						isAdjacent = true;
					}
					else if( area1->rect.top == area2->rect.bottom )
					{
						newRect.left = area1->rect.left;
						newRect.right = area1->rect.right;
						newRect.top = area2->rect.top;
						newRect.bottom = area1->rect.bottom;
						isAdjacent = true;
					}
				}

				if( isAdjacent )
				{
					CCP_DELETE( area2 );
					// cppcheck-suppress uninitStructMember
					area1->rect = newRect;
					candidate = m_freeAreas.erase( candidate );

					restartLoop = true;
				}
				else
				{
					++candidate;
				}
			}
		}
	}

	//update our cached value for largest free area
	UpdateFreeMaxima();
}

bool Tr2TextureAtlas::CollapseAreas( Tr2TextureAtlasArea* area1, Tr2TextureAtlasArea* area2 )
{
	Tr2Rect newRect;
	bool isAdjacent = false;

	if( (area1->rect.left == area2->rect.left) && (area1->rect.right == area2->rect.right) )
	{
		// Left/right matches, see if areas are adjacent top or bottom
		if( area1->rect.bottom == area2->rect.top )
		{
			newRect.left = area1->rect.left;
			newRect.right = area1->rect.right;
			newRect.top = area1->rect.top;
			newRect.bottom = area2->rect.bottom;
			isAdjacent = true;
		}
		else if( area1->rect.top == area2->rect.bottom )
		{
			newRect.left = area1->rect.left;
			newRect.right = area1->rect.right;
			newRect.top = area2->rect.top;
			newRect.bottom = area1->rect.bottom;
			isAdjacent = true;
		}
	}
	else if( (area1->rect.top == area2->rect.top) && (area1->rect.bottom == area2->rect.bottom) )
	{
		// Top/bottom matches, see if areas are adjacent left or right
		if( area1->rect.right == area2->rect.left )
		{
			newRect.left = area1->rect.left;
			newRect.right = area2->rect.right;
			newRect.top = area1->rect.top;
			newRect.bottom = area1->rect.bottom;
			isAdjacent = true;
		}
		else if( area1->rect.left == area2->rect.right )
		{
			newRect.left = area2->rect.left;
			newRect.right = area1->rect.right;
			newRect.top = area1->rect.top;
			newRect.bottom = area1->rect.bottom;
			isAdjacent = true;
		}
	}

	if( isAdjacent )
	{
		// cppcheck-suppress uninitStructMember
		area1->rect = newRect;
	}

	return isAdjacent;
}

Tr2TextureAtlasArea* Tr2TextureAtlas::GetFreeArea( unsigned int width, unsigned int height )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Check if the largest free area we have is smaller than requested
	if( IsLargeTexture( width, height ) )
	{
		return NULL;
	}

	// Align to 8pixels to prevent thin slivers as free areas
	width = (width + 7) & ~7;
	height = (height + 7) & ~7;

	// First search through free areas.
	// Keep track of an area that fits, but keep looking for a perfect fit.
	AreaList_t::iterator anyFit = m_freeAreas.end();
	AreaList_t::iterator perfectFit = m_freeAreas.end();
	int bestPrimaryMetric = m_width+m_height;
	int bestSecondaryMetric = m_width+m_height;
	for( AreaList_t::iterator it = m_freeAreas.begin(); it != m_freeAreas.end(); ++it )
	{
		Tr2TextureAtlasArea* area = *it;
		unsigned int candidateWidth = area->rect.right - area->rect.left;
		unsigned int candidateHeight = area->rect.bottom - area->rect.top;

		if( (width == candidateWidth) && (height == candidateHeight)  )
		{
			perfectFit = it;
			break;
		}

		if( (width <= candidateWidth) && (height <= candidateHeight)  )
		{
			int metric = std::min( candidateWidth - width, candidateHeight - height );
			int altMetric = std::max( candidateWidth - width, candidateHeight - height );
			if( metric < bestPrimaryMetric || (bestPrimaryMetric == metric && altMetric < bestSecondaryMetric) )
			{
				bestPrimaryMetric = metric;
				bestSecondaryMetric = altMetric;
				// todo: largest fit/smallest fit/first/last?
				anyFit = it;
			}
		}
	}

	if( perfectFit != m_freeAreas.end() )
	{
		// Found a perfect fit
		Tr2TextureAtlasArea* freeArea = *perfectFit;
		m_freeTexels -= (freeArea->rect.bottom - freeArea->rect.top) * (freeArea->rect.right - freeArea->rect.left);
		m_freeAreas.erase( perfectFit );

		//update cached largest free area
		if( (freeArea->rect.right - freeArea->rect.left) >= m_freeMaxWidth ||
			(freeArea->rect.bottom - freeArea->rect.top) >= m_freeMaxHeight )
		{
			UpdateFreeMaxima();
		}

		return freeArea;
	}
	
	if( anyFit != m_freeAreas.end() )
	{
		Tr2TextureAtlasArea* freeArea = *anyFit;
		m_freeAreas.erase( anyFit );

		unsigned int candidateWidth = freeArea->rect.right - freeArea->rect.left;
		unsigned int candidateHeight = freeArea->rect.bottom - freeArea->rect.top;

		//if we're allocating a small bit out of a large area, first split it into squares
		const unsigned minNewRegion = 128 + m_margin * 4;
		
		if( height < minNewRegion && width < minNewRegion )
		{
			//(if we've found a small region, it is already within a square so no need to do this)
			//vertical split
			if( candidateHeight > minNewRegion )
			{
				Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
				newArea->rect.left = freeArea->rect.left;
				newArea->rect.top = freeArea->rect.top + minNewRegion;
				newArea->rect.right = freeArea->rect.right;
				newArea->rect.bottom = freeArea->rect.bottom;
				PaintEmptyArea( newArea );

				FreeArea( newArea );
				freeArea->rect.bottom = freeArea->rect.top + minNewRegion;
			}
			//horizontal split
			if( candidateWidth > minNewRegion )
			{
				Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
				newArea->rect.left = freeArea->rect.left + minNewRegion;
				newArea->rect.top = freeArea->rect.top;
				newArea->rect.right = freeArea->rect.right;
				newArea->rect.bottom = freeArea->rect.bottom;
				PaintEmptyArea( newArea );

				FreeArea( newArea );
				freeArea->rect.right = freeArea->rect.left + minNewRegion;
			}
		}
		
		// Must split the area
		if( candidateWidth == width )
		{
			// Split area vertically
			Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
			newArea->rect.left = freeArea->rect.left;
			newArea->rect.top = freeArea->rect.top + height;
			newArea->rect.right = freeArea->rect.right;
			newArea->rect.bottom = freeArea->rect.bottom;
			PaintEmptyArea( newArea );

			FreeArea( newArea );
		}
		else if( candidateHeight == height )
		{
			// Split area horizontally
			Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
			newArea->rect.left = freeArea->rect.left + width;
			newArea->rect.top = freeArea->rect.top;
			newArea->rect.right = freeArea->rect.right;
			newArea->rect.bottom = freeArea->rect.bottom;
			PaintEmptyArea( newArea );

			FreeArea( newArea );
		}
		else
		{
			// Need two new areas
			Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
			newArea->rect.left = freeArea->rect.left + width;
			newArea->rect.top = freeArea->rect.top;
			newArea->rect.right = freeArea->rect.right;
			newArea->rect.bottom = freeArea->rect.top + height;
			PaintEmptyArea( newArea );

			FreeArea( newArea );

			newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
			newArea->rect.left = freeArea->rect.left;
			newArea->rect.top = freeArea->rect.top + height;
			newArea->rect.right = freeArea->rect.right;
			newArea->rect.bottom = freeArea->rect.bottom;
			PaintEmptyArea( newArea );

			FreeArea( newArea );
		}

		//update cached largest free area (freeArea not yet resized but has been erased from free list)
		if( (freeArea->rect.right - freeArea->rect.left) >= m_freeMaxWidth ||
			(freeArea->rect.bottom - freeArea->rect.top) >= m_freeMaxHeight )
		{
			UpdateFreeMaxima();
		}

		freeArea->rect.right = freeArea->rect.left + width;
		freeArea->rect.bottom = freeArea->rect.top + height;

		m_freeTexels -= (freeArea->rect.bottom - freeArea->rect.top) * (freeArea->rect.right - freeArea->rect.left);

		return freeArea;
	}
	

	// Couldn't find anything!
	return NULL;

}

Tr2RenderContextEnum::PixelFormat Tr2TextureAtlas::GetFormat() const
{
	return m_format;
}

void Tr2TextureAtlas::PaintEmptyArea( Tr2TextureAtlasArea* area )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !area || !m_paintEmptyAreas || !m_texture.IsValid() )
	{
		return;
	}

	if( m_texture.GetType() != TEX_TYPE_2D ||
		m_format != PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		return;
	}

	const bool isValidArea = 
				area->rect.left   >= 0					&&
				area->rect.top    >= 0					&&
				area->rect.right  >= 0					&&
				area->rect.bottom >= 0					&&
				area->rect.right  >= area->rect.left	&&
				area->rect.bottom >= area->rect.top		&&
				uint32_t( area->rect.right ) <= m_texture.GetWidth() &&
				uint32_t( area->rect.bottom) <= m_texture.GetHeight();

	CCP_ASSERT( isValidArea );
	if( !isValidArea )
	{
		return;
	}


	void* rgba = nullptr;
	uint32_t pitch = 0;
	CR_RETURN( m_texture.MapForWriting( Tr2TextureSubresource( 0 ), rgba, pitch, renderContext ) );
	ON_BLOCK_EXIT( [&]{ m_texture.UnmapForWriting( renderContext ); } );

	uint8_t *dst = (uint8_t*)rgba;

	static unsigned int s_fillValue = 0;
	s_fillValue += 0xabcdef;
	s_fillValue &= 0xffffff;

	const uint32_t width  = uint32_t( area->rect.right  - area->rect.left );
	const uint32_t height = uint32_t( area->rect.bottom - area->rect.top );

	if( IsCompressedFormat( m_format ) )
	{
		const uint32_t blockSize = GetBlockByteSize( m_format );

		const uint32_t blocksX = width / 4;

		// Have to copy one line at a time since the target area is not linearly laid out.
		for( uint32_t line = 0; line < height; line += 4 )
		{
			uint32_t* p = (uint32_t*)dst;
			for( uint32_t i = 0; i < blocksX * blockSize / 4; ++i )
			{
				*p++ = s_fillValue | 0xff000000;
			}
			dst += pitch;
		}
	}
	else
	{
		const uint32_t byteCount = GetBytesPerPixel( m_format );

		// Have to copy one line at a time since the target area is not linearly laid out.
		for( uint32_t line = 0; line < height; ++line )
		{
			uint32_t* p = (uint32_t*)dst;
			for( uint32_t i = 0; i < width * byteCount / 4; ++i )
			{
				*p++ = s_fillValue | 0xff000000;
			}
			dst += pitch;
		}
	}
}

Tr2TextureAL* Tr2TextureAtlas::GetTexture()
{
	return &m_texture;
}

Tr2TextureAtlas::OnTextureChangeEvent& Tr2TextureAtlas::OnTextureChange()
{
	return m_onTextureChange;
}

int Tr2TextureAtlas::GetTexturesInAtlasCount()
{
	return (int)m_texturesInAtlas.size();
}

int Tr2TextureAtlas::GetTexturesOutsideAtlasCount()
{
	return (int)m_texturesOutsideAtlas.size();
}

void Tr2TextureAtlas::PullInOutsiders( bool optimiseInsertion )
{	
	std::list< Tr2AtlasTexture * > sorted( m_texturesOutsideAtlas.begin(), m_texturesOutsideAtlas.end() );

	if( optimiseInsertion )
	{
		//todo: implement more options for sorting the list (squareness, power-of-two pref.)
		struct SmallestFirst 
		{
			bool operator()( const Tr2AtlasTexture *a, const Tr2AtlasTexture *b ) 
			{
				const int widthA = a->m_width;
				const int heightA = a->m_height;
				const int widthB = b->m_width;
				const int heightB = b->m_height;

				return (widthA * heightA) < (widthB * heightB);
			}
		};		 

		SmallestFirst smallestFirst;
		sorted.sort( smallestFirst );
	}

	CollapseFreeAreas();

	for( std::list<Tr2AtlasTexture*>::iterator it = sorted.begin(); it != sorted.end(); ++it )
	{
		Tr2AtlasTexture* tex = *it;

		if( tex->IsStandAlone() )
		{
			// Texture prefers to be an outsider
			continue;
		}

		CCP_ASSERT( !tex->m_atlasArea );

		Tr2TextureAtlasArea* area = GetFreeArea( tex->m_width + m_margin*2, tex->m_height + m_margin*2 );
		if( area )
		{
			area->tex = tex;
			area->type = Tr2TextureAtlasArea::IN_USE;
			tex->m_atlasArea = area;
			if( CopyTextureIntoAtlas( tex ) )
			{
				// Set up the texture window into the atlas
				tex->m_texture = Tr2TextureAL();
				tex->m_renderTarget = nullptr;
				tex->m_x = area->rect.left + m_margin;
				tex->m_y = area->rect.top + m_margin;
				tex->m_textureWidth = m_width;
				tex->m_textureHeight = m_height;
				
				tex->FinalizePrepare();

				m_texturesOutsideAtlas.erase( tex );				
				m_texturesInAtlas.insert( tex );
			}
			else
			{
				// Oops, failed. Continue to use as an outsider, remembering to return free area to the pool.
				tex->m_atlasArea = NULL;
				FreeArea( area );
			}
		}
	}
}

bool Tr2TextureAtlas::CopyTextureIntoAtlas( Tr2AtlasTexture* tex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_texture.IsValid() || !tex || !tex->GetTexture() )
	{
		// assuming that RTs don't use this, seems nonsensical
		CCP_LOGERR( "CopyTextureIntoAtlas failed, target or source textures invalid" );
		return false;
	}
	
	const auto& r = tex->m_atlasArea->rect;

	if( !m_margin )
	{
		Tr2TextureSubresource dest( 0, 0 );
		dest.m_box.left = r.left;
		dest.m_box.top = r.top;
		return SUCCEEDED( m_texture.CopySubresourceRegion( dest, *tex->GetTexture(), Tr2TextureSubresource( 0, 0 ), renderContext ) );
	}

	if( m_hasMipMaps )
	{
		m_dirtyMipRegions.push_back( r );
	}

	const void* srcData = nullptr;
	unsigned srcPitch = 0;
	CR_RETURN_VAL( tex->GetTexture()->MapForReading( Tr2TextureSubresource( 0 ), srcData, srcPitch, renderContext ), false );
	ON_BLOCK_EXIT( [&]{ tex->GetTexture()->UnmapForReading( renderContext ); } );

	std::vector<unsigned char> pixels;
	unsigned pitch = 0;
	Tr2ImageIOHelpers::AddMargin( m_texture.GetFormat(), (const unsigned char*)srcData, tex->GetWidth(), tex->GetHeight(), m_margin, pixels, pitch );

	// Area may be larger than texture due to alignment
	uint32_t right = r.left + tex->GetWidth() + 2*m_margin;
	uint32_t bottom = r.top + tex->GetHeight() + 2*m_margin;
	return SUCCEEDED( m_texture.UpdateSubresource( Tr2TextureSubresource( 0 ).SetRect( r.left, r.top, right, bottom ), &pixels[0], pitch, 0, renderContext ) );
}

ALResult Tr2TextureAtlas::CreateTexture( unsigned int width, unsigned int height, AtlasTextureType type, Tr2AtlasTexture** result )
{
	Tr2AtlasTexturePtr tex;
	tex.CreateInstance();
	tex->m_textureAtlas = this;

	Tr2TextureAtlasArea* area = NULL;
	
	if( type == ATT_DEFAULT && !IsLargeTexture( width, height ) )
	{
		area = GetFreeArea( width + m_margin*2, height + m_margin*2 );	

		if( !area && m_optimizationWarranted )
		{
			CollapseFreeAreas();
			area = GetFreeArea( width + m_margin*2, height + m_margin*2 );
		}
	}

	if( area )
	{
		area->type = Tr2TextureAtlasArea::IN_USE;
		area->tex = tex;
		tex->m_atlasArea = area;

		// Set up the texture window into the atlas
		tex->m_renderTarget = nullptr;
		tex->m_x = area->rect.left + m_margin;
		tex->m_y = area->rect.top + m_margin;
		tex->m_width = width;
		tex->m_height = height;
		tex->m_textureWidth = m_width;
		tex->m_textureHeight = m_height;

		RegisterInsider( tex );
		PaintEmptyArea( area );
	}
	else if( m_createOutsiders )
	{
		if( !Tr2Renderer::IsResourceCreationAllowed() )
		{
			*result = nullptr;
			return E_DEVICELOST;
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();
		HRESULT hr = tex->m_texture.Create(
			Tr2BitmapDimensions( width, height, 1, m_format ), 
			Tr2GpuUsage::SHADER_RESOURCE,
			Tr2CpuUsage::READ | Tr2CpuUsage::WRITE,
			renderContext ).GetResult();
		if( FAILED( hr ) )
		{
			*result = nullptr;
			return hr;
		}

		if( type == ATT_VIDEO )
		{
			tex->SetStandAlone( true );
		}

		tex->m_x = 0;
		tex->m_y = 0;
		tex->m_width = width;
		tex->m_height = height;
		tex->m_textureWidth = width;
		tex->m_textureHeight = height;

		RegisterOutsider( tex );
	}
	else
	{
		*result = nullptr;
		return S_OK;
	}

	tex->m_memoryUsage = width * height * GetBytesPerPixel( m_format );
	tex->FinalizePrepare();
	tex->SetPrepared( true );
	tex->SetGood( false );

	*result = tex.Detach();
	return S_OK;
}

#if BLUE_WITH_PYTHON
PyObject* Tr2TextureAtlas::GetTexturesInAtlas()
{
	PyObject* list = PyList_New( m_texturesInAtlas.size() );

	Tr2AtlasTextureSet_t::iterator it;
	int i;
	for( i = 0, it = m_texturesInAtlas.begin(); it != m_texturesInAtlas.end(); ++i, ++it )
	{
		PyList_SET_ITEM( list, i, PyOS->WrapBlueObject( (*it)->GetRawRoot() ) );
	}

	return list;
}

PyObject* Tr2TextureAtlas::GetTexturesOutsideAtlas()
{
	PyObject* list = PyList_New( m_texturesOutsideAtlas.size() );

	Tr2AtlasTextureSet_t::iterator it;
	int i;
	for( i = 0, it = m_texturesOutsideAtlas.begin(); it != m_texturesOutsideAtlas.end(); ++i, ++it )
	{
		PyList_SET_ITEM( list, i, PyOS->WrapBlueObject( (*it)->GetRawRoot() ) );
	}

	return list;
}
#endif

void Tr2TextureAtlas::SetMargin( unsigned int margin )
{
	m_margin = margin;
}

void Tr2TextureAtlas::SetPaintEmptyAreas( bool paintEmptyAreas )
{
	m_paintEmptyAreas = paintEmptyAreas;
}

void Tr2TextureAtlas::SetCreateOutsiders( bool createOutsiders )
{
	m_createOutsiders = createOutsiders;
}

void Tr2TextureAtlas::SetMaxTextureArea( unsigned int maxTextureArea )
{
	m_maxTextureArea = maxTextureArea;
}

std::list<Tr2Rect> Tr2TextureAtlas::GetFreeAreas() const
{
	std::list<Tr2Rect> areas;
	for( AreaList_t::const_iterator i = m_freeAreas.begin(); i != m_freeAreas.end(); ++i ) 
	{
		areas.push_back( (*i)->rect );
	}
	return areas;
}

std::list<Tr2Rect> Tr2TextureAtlas::GetUsedAreas() const
{
	std::list<Tr2Rect> areas;
	for( Tr2AtlasTextureSet_t::const_iterator i = m_texturesInAtlas.begin(); i != m_texturesInAtlas.end(); ++i ) 
	{
		Tr2AtlasTexture *t = *i;

		// After a device reset, the texture may not have a valid area. Texture is still
		// registered with the atlas, but it might be in the queue for reloading.
		if( t->m_atlasArea )
		{
			areas.push_back( t->m_atlasArea->rect );
		}

	}
	return areas;
}


void Tr2TextureAtlas::UpdateFreeMaxima() const
{
	m_freeMaxWidth = m_freeMaxHeight = 0;
	for( AreaList_t::const_iterator i = m_freeAreas.begin(); i != m_freeAreas.end(); ++i ) 
	{
		const int w = (*i)->rect.right - (*i)->rect.left;
		const int h = (*i)->rect.bottom - (*i)->rect.top;
		if( w > m_freeMaxWidth ) m_freeMaxWidth = w;
		if( h > m_freeMaxHeight ) m_freeMaxHeight = h;
	}
}

bool Tr2TextureAtlas::EjectTexture( Tr2AtlasTexture *tex )
{
	CCP_ASSERT( tex );

	if( !tex->m_textureAtlas || !tex->m_atlasArea )
	{
		return false;
	}

	if( EjectTextureHelper( tex) )
	{
		for( auto i = m_texturesInAtlas.begin(); i != m_texturesInAtlas.end(); ++i ) 
		{
			Tr2AtlasTexture *t = *i;
			if( t == tex )
			{
				m_texturesInAtlas.erase( i );
				return true;
			}
		}
	}

	return false;
}

bool Tr2TextureAtlas::EjectTextureHelper( Tr2AtlasTexture *tex )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	CCP_ASSERT( tex );
	if( !tex )
	{
		return false;
	}
	CCP_ASSERT( tex->m_textureAtlas == this );
	CCP_ASSERT( tex->m_atlasArea );
	
	Tr2TextureAtlasArea *texArea = tex->m_atlasArea;
	
	Tr2Rect copyRect;
	copyRect.left = texArea->rect.left + m_margin;
	copyRect.top = texArea->rect.top + m_margin;
	copyRect.right = copyRect.left + tex->GetWidth();
	copyRect.bottom = copyRect.top + tex->GetHeight();
	const int width = tex->GetWidth();
	const int height = tex->GetHeight();


	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		CCP_LOGERR( "EjectTexture: There is no AL device available" );
		return false;
	}

	HRESULT hr = tex->m_texture.Create( Tr2BitmapDimensions( width, height, 1, m_format ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, renderContext );

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2TextureAtlas::EjectTexture - couldn't create D3D texture (%d)", hr );
		return false;
	}

	Tr2TextureSubresource subrect;
	subrect.m_endMipLevel = 1;
	subrect.m_box.left = copyRect.left;
	subrect.m_box.right = copyRect.right;
	subrect.m_box.top = copyRect.top;
	subrect.m_box.bottom = copyRect.bottom;
	if( SUCCEEDED( tex->m_texture.CopySubresourceRegion( Tr2TextureSubresource(), m_texture, subrect, renderContext ) ) )
	{
		tex->m_x = 0;
		tex->m_y = 0;
		tex->m_width = width;
		tex->m_height = height;
		tex->m_textureWidth = width;
		tex->m_textureHeight = height;
		tex->m_atlasArea = NULL;

		tex->FinalizePrepare();

		FreeArea( texArea );

		RegisterOutsider( tex );
		return true;
	}
	else
	{
		tex->m_texture = Tr2TextureAL();
	}
	
	return false;	
}

void Tr2TextureAtlas::EjectAllTextures()
{
	for( auto i = m_texturesInAtlas.begin(); i != m_texturesInAtlas.end();  ) 
	{
		Tr2AtlasTexture *t = *i;
		if( EjectTextureHelper( t ) )
		{
			i = m_texturesInAtlas.erase(i);
		}
		else
		{
			++i;
		}
	}

	if( m_texturesInAtlas.empty() )
	{
		m_freeAreas.clear();

		Tr2TextureAtlasArea* newArea = CCP_NEW( "Tr2TextureAtlas/Area" ) Tr2TextureAtlasArea;
		newArea->type = Tr2TextureAtlasArea::FREE;
		newArea->rect.left = 0;
		newArea->rect.top = 0;
		newArea->rect.right = m_width;
		newArea->rect.bottom = m_height;
		PaintEmptyArea( newArea );

		FreeArea( newArea );
	}
	else if( m_optimizeOnRemoval )
	{
		CollapseFreeAreas();
	}

}
void Tr2TextureAtlas::FreeArea( Tr2TextureAtlasArea *area )
{
	if( area == NULL || 
		(area->rect.right - area->rect.left) == 0 ||
		(area->rect.bottom - area->rect.top) == 0 )
	{
		return;
	}
	
	area->type = Tr2TextureAtlasArea::FREE;
	area->tex = NULL;
	m_freeAreas.push_back( area );

	++m_areasFreedSinceLastCollapse;
	if( m_areasFreedSinceLastCollapse > 32 )
	{
		m_optimizationWarranted = true;
	}
}

bool Tr2TextureAtlas::IsLargeTexture( unsigned int width, unsigned int height )
{
	// Large textures are not good for atlasing, end up with a lot of small outsiders
	if( width * height > m_maxTextureArea )
	{
		return true;
	}

	return false;
}

//Call this manually to update mipmaps when appropriate
void Tr2TextureAtlas::UpdateMipMaps( Tr2RenderContext& renderContext ) 
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_hasMipMaps || m_dirtyMipRegions.empty() )
	{
		return;
	}
	if( m_format != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ) 
	{
		CCP_LOGERR( "Texture format not supported for mipmapped atlases: %08x [%d]", int(m_format), int(m_format) );
		m_dirtyMipRegions.clear();
		return;
	}

	if( !m_texture.IsValid() ) 
	{
		m_dirtyMipRegions.clear();
		return;
	}

	// Get a copy of the first mip level into system memory
	uint32_t sourcePitch;
	const void* sourceData;
	if( FAILED( m_texture.MapForReading( Tr2TextureSubresource( 0 ), sourceData, sourcePitch, renderContext ) ) )
	{
		CCP_LOGERR( "Tr2TextureAtlas::UpdateMipMaps: Failed to read texture" );
		return;
	}
	std::vector<unsigned char> buffer0( m_texture.GetMipSize( 0 ) );
	const unsigned char* row = reinterpret_cast<const unsigned char*>( sourceData );
	auto destRow = buffer0.begin();
	for( unsigned i = 0; i < m_texture.GetHeight(); ++i )
	{
		std::copy( row, row + m_texture.GetWidth() * 4, destRow );
		destRow += m_texture.GetWidth() * 4;
		row += sourcePitch;
	}
	m_texture.UnmapForReading( renderContext );
	sourcePitch = m_texture.GetWidth() * 4;

	uint32_t destPitch = sourcePitch / 2;

	std::vector<unsigned char>* prevMip = &buffer0;
	std::vector<unsigned char> buffer1( m_texture.GetMipSize( 1 ) );
	std::vector<unsigned char>* nextMip = &buffer1;

	for( unsigned i = 1; i < m_mipLevels; ++i )
	{
		// Construct next mip level data for dirty regions using previous mip level copy in system memory
		// and update rectangles in the texture
		bool updated = false;
		for( auto it = m_dirtyMipRegions.begin(); it != m_dirtyMipRegions.end(); ++it ) 
		{
			const Tr2Rect source = { it->left >> ( i - 1 ), it->top >> ( i - 1 ), it->right >> ( i - 1 ), it->bottom >> ( i - 1 ) };
			Tr2Rect dest = { it->left >> i, it->top >> i, it->right >> i, it->bottom >> i };
			const int height = dest.bottom - dest.top;
			const int width = dest.right - dest.left;
			if( height == 0 || width == 0 ) 
			{
				continue;
			}
			const unsigned char *src = &( *prevMip )[0];
			src += sourcePitch * source.top + source.left * 4;
			unsigned char *dst = &( *nextMip )[0];
			dst += destPitch * dest.top + dest.left * 4;

			uint32_t sysDestPitch;
			void* destData;
			if( SUCCEEDED( m_texture.MapForWriting( Tr2TextureSubresource( i ).SetRect( reinterpret_cast<uint32_t*>( &dest ) ), destData, sysDestPitch, renderContext ) ) )
			{
				unsigned char *dstRow = reinterpret_cast<unsigned char*>( destData );

				for( int y = 0; y < height; ++y ) 
				{
					for( int x = 0; x < width; ++x ) 
					{
						for( int ch = 0; ch < 4; ++ch ) 
						{
							unsigned s00 = src[y * 2 * sourcePitch + x * 2 * 4 + ch];
							unsigned s01 = src[y * 2 * sourcePitch + (x * 2 + 1) * 4 + ch];
							unsigned s10 = src[(y * 2 + 1) * sourcePitch + x * 2 * 4 + ch];
							unsigned s11 = src[(y * 2 + 1) * sourcePitch + (x * 2 + 1) * 4 + ch];
							dst[x * 4 + ch] = (unsigned char)( (s00 + s01 + s10 + s11) >> 2 );
							dstRow[x * 4 + ch] = dst[x * 4 + ch];
						}
					}
					dst += destPitch;
					dstRow += sysDestPitch;
				}
				m_texture.UnmapForWriting( renderContext );
			}
			updated = true;
		}
		if( !updated )
		{
			break;
		}
		sourcePitch = destPitch;
		destPitch >>= 1;
		std::swap( prevMip, nextMip );
	}

	m_dirtyMipRegions.clear();
}

void Tr2TextureAtlas::DirtyRegion( const Tr2Rect& rect )
{
	if( m_hasMipMaps )
		m_dirtyMipRegions.push_back( rect );
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the object contains a reference to given AL object. This method is exposed
//   to Python and is used for debugging.
// Arguments:
//   type - Tr2RenderContextEnum::ObjectType, type of AL object
//   object - pointer to an AL object (passed as a number)
// Return Value:
//   true If object contains a reference to the given AL object
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2TextureAtlas::HasALObject( int type, size_t object )
{
	switch( type )
	{
	case OT_TEXTURE:
		return m_texture == *reinterpret_cast<Tr2TextureAL*>( object );
	}
	return false;
}

uint32_t Tr2TextureAtlas::GetMsaaSamples() const 
{ 
	return 1; 
}

uint32_t Tr2TextureAtlas::GetMsaaQuality() const 
{ 
	return 0; 
}
