////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2CompositedVectorTextureRes.h"
#include "Resources/Tr2CairoScriptSourceRes.h"
#include "Tr2ImageIOHelpers.h"
#include "Tr2Renderer.h"
#include "Tr2HostBitmap.h"


namespace
{
	class QueryTokens
	{
	public:
		QueryTokens( const wchar_t* query )
			:m_query( query )
		{
		}

		bool Next( std::wstring& token )
		{
			if( !m_query )
			{
				return false;
			}
			auto div = wcschr( m_query, L'&' );
			if( !div )
			{
				token = m_query;
				m_query = nullptr;
			}
			else
			{
				token = std::wstring( m_query, div );
				m_query = div + 1;
			}
			return true;
		}

		bool Next( float& token )
		{
			std::wstring str;
			if( !Next( str ) )
			{
				return false;
			}
			size_t index;
			try
			{
				token = std::stof( str, &index );
			}
			catch( ... )
			{
				return false;
			}
			if( index != str.length() )
			{
				return false;
			}
			return true;
		}

		bool Next( uint32_t& token )
		{
			std::wstring str;
			if( !Next( str ) )
			{
				return false;
			}
			size_t index;
			try
			{
				token = uint32_t( std::stoul( str, &index ) );
			}
			catch( ... )
			{
				return false;
			}
			if( index != str.length() )
			{
				return false;
			}
			return true;
		}

	private:
		const wchar_t* m_query;
	};

	class CompositedVectorTextureConstructor: public IBlueDynamicResourceConstructor
	{
	public:
		CompositedVectorTextureConstructor( const wchar_t* directory )
			:m_directory( directory )
		{
		}

		bool ParseQuery( const wchar_t* query, std::vector<Tr2CompositedVectorTextureRes::Source> &sources, uint32_t& width, uint32_t& height, bool& premultipliedAlpha )
		{
			QueryTokens tokens( query );

			uint32_t pma;
			if( !tokens.Next( pma ) )
			{
				return false;
			}
			premultipliedAlpha = pma != 0;
			if( !tokens.Next( width ) )
			{
				return false;
			}
			if( !tokens.Next( height ) )
			{
				return false;
			}

			while( true )
			{
				Tr2CompositedVectorTextureRes::Source source;

				std::wstring fileName;
				if( !tokens.Next( fileName ) )
				{
					break;
				}

				for( int32_t i = 0; i < 7; ++i )
				{
					if( !tokens.Next( ( &source.position.x )[i] ) )
					{
						return false;
					}
				}

				BeResMan->GetResource( m_directory + fileName + L".ecs", L"raw", source.source );

				sources.push_back( source );
			}
			return true;
		}

		IBlueResource* GetResource( const wchar_t* query )
		{
			Tr2CompositedVectorTextureResPtr texture;
			texture.CreateInstance();

			std::vector<Tr2CompositedVectorTextureRes::Source> sources;
			bool premultipliedAlpha;
			uint32_t width, height;

			if( ParseQuery( query, sources, width, height, premultipliedAlpha ) )
			{
				texture->ComposeAsync( width, height, premultipliedAlpha, sources );
			}
			else
			{
				CCP_LOGERR( "Invalid query for dynamic composited vector texture resource: %ls", query );
			}

			return texture.Detach();
		}
	private:
		std::wstring m_directory;
	};
}


Tr2CompositedVectorTextureRes::Tr2CompositedVectorTextureRes( IRoot* )
	:m_sources( "Tr2CompositedVectorTextureRes::m_sources" ),
	m_resourceLoadCbId( 0 ),
	m_resourcePrepCbId( 0 ),
	m_width( 0 ),
	m_height( 0 ),
	m_premultipliedAlpha( true )
{
}

Tr2CompositedVectorTextureRes::~Tr2CompositedVectorTextureRes()
{
	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
}

void Tr2CompositedVectorTextureRes::ComposeSync( uint32_t width, uint32_t height, bool premultipliedAlpha, const std::vector<Source>& sources )
{
	if( !width || !height )
	{
		CCP_LOGERR( "Tr2CompositedVectorTextureRes::ComposeSync called with invalid dimensions" );
		return;
	}

	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
	CancelPendingLoad();

	m_sources.clear();
	m_sources.insert( begin( m_sources ), begin( sources ), end( sources ) );
	m_width = width;
	m_height = height;
	m_premultipliedAlpha = premultipliedAlpha;

	for( auto it = begin( m_sources ); it != end( m_sources ); ++it )
	{
		if( !it->source )
		{
			CCP_LOGERR( "Tr2CompositedVectorTextureRes::ComposeSync called with None source" );
			m_sources.clear();
			return;
		}
		if( !it->source->IsGood() )
		{
			CCP_LOGERR( "Tr2CompositedVectorTextureRes::ComposeSync called with not fully loaded source" );
			m_sources.clear();
			return;
		}
	}

	m_isForcedSynchronous = true;
	StartLoading();
	m_isForcedSynchronous = false;
}

void Tr2CompositedVectorTextureRes::ComposeAsync( uint32_t width, uint32_t height, bool premultipliedAlpha, const std::vector<Source>& sources )
{
	if( !width || !height )
	{
		CCP_LOGERR( "Tr2CompositedVectorTextureRes::ComposeAsync called with invalid dimensions" );
		return;
	}

	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}
	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
	CancelPendingLoad();

	m_sources.clear();
	m_sources.insert( begin( m_sources ), begin( sources ), end( sources ) );
	m_width = width;
	m_height = height;
	m_premultipliedAlpha = premultipliedAlpha;

	bool waitForLoads = false;

	for( auto it = begin( m_sources ); it != end( m_sources ); ++it )
	{
		if( !it->source )
		{
			CCP_LOGERR( "Tr2CompositedVectorTextureRes::ComposeAsync called with None source" );
			m_sources.clear();
			return;
		}
		if( !it->source->IsGood() )
		{
			waitForLoads = true;
		}
	}

	if( waitForLoads )
	{
		m_isLoading = true;
		BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
	}
	else
	{
		StartLoading();
	}
}

Tr2TextureAL* Tr2CompositedVectorTextureRes::GetTexture()
{
	return &m_texture;
}

void Tr2CompositedVectorTextureRes::StaticResourceLoadFinished( void* context )
{
	Tr2CompositedVectorTextureRes* pThis = static_cast<Tr2CompositedVectorTextureRes*>( context );
	pThis->m_resourceLoadCbId = 0;
	BeResMan->AddToQueue( BRMQ_MAIN, StaticResourcePrepFinished, pThis, 0, &pThis->m_resourcePrepCbId );
}

void Tr2CompositedVectorTextureRes::StaticResourcePrepFinished( void* context )
{
	Tr2CompositedVectorTextureRes* pThis = static_cast<Tr2CompositedVectorTextureRes*>( context );
	pThis->m_resourcePrepCbId = 0;
	pThis->StartLoading();
}

void Tr2CompositedVectorTextureRes::StartLoading()
{
	Initialize( L" ", L"" );
}

BlueAsyncRes::LoadingResult Tr2CompositedVectorTextureRes::DoLoad()
{
	m_bitmap.Destroy();

	std::vector<std::unique_ptr<std::string>> modifiedSources;
	std::vector<Tr2ImageIOHelpers::CairoScript> sources;
	for( auto it = begin( m_sources ); it != end( m_sources ); ++it )
	{
		if( !it->source || !it->source->IsGood() )
		{
			return LR_FAILED;
		}

		std::unique_ptr<std::string> modifiedSource( new std::string() );
		it->source->ApplyColor( *modifiedSource, it->color );

		Tr2ImageIOHelpers::CairoScript src;
		src.script = modifiedSource->c_str();
		src.length = modifiedSource->length();
		src.scale = it->scale / std::max( 1u, it->source->GetWidth() ) * m_width;
		src.rotation = it->rotation;
		src.translation = Vector2( it->position.x * m_width, it->position.y * m_height );
		sources.push_back( src );

		modifiedSources.push_back( std::move( modifiedSource ) );
	}
	if( sources.empty() )
	{
		if( !m_bitmap.Create( m_width, m_height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ) )
		{
			CCP_LOGERR( "Failed to create image '%S'", GetPath() );
			return LR_FAILED;
		}
		memset( m_bitmap.GetRawData(), 0, m_bitmap.GetRawDataSize() );
	}
	else
	{
		Tr2ImageIOHelpers::RasterizationOptions options;
		options.premultipliedAlpha = m_premultipliedAlpha;
		if( !Tr2ImageIOHelpers::RasterizeCairoScripts( m_bitmap, sources, m_width, m_height, options ) )
		{
			CCP_LOGERR( "Failed to rasterize composited vector image '%S'", GetPath() );
			m_bitmap.Destroy();
			return LR_FAILED;
		}
	}
	return LR_SUCCESS;
}

bool Tr2CompositedVectorTextureRes::DoPrepare()
{
	ON_BLOCK_EXIT( [&] {m_bitmap.Destroy(); } );

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	uint32_t memoryUse;
	if( !Tr2ImageIOHelpers::CreateTexture( m_bitmap, m_texture, memoryUse, renderContext, Tr2RenderContextEnum::USAGE_IMMUTABLE ) )
	{
		CCP_LOGERR( "Failed to create texture for rasterized composited vector image '%S'", GetPath() );
		return false;
	}

	return true;
}

bool Tr2CompositedVectorTextureRes::DoOpenStream()
{
	return true;
}

bool Tr2CompositedVectorTextureRes::IsMemoryUsageKnown()
{
	return m_texture.IsValid();
}

size_t Tr2CompositedVectorTextureRes::GetMemoryUsage()
{
	return m_texture.GetMipSize( 0 );
}

void Tr2CompositedVectorTextureRes::ReleaseResources( TriStorage )
{
}

bool Tr2CompositedVectorTextureRes::OnPrepareResources()
{
	if( IsGood() || IsLoading() )
	{
		return true;
	}

	if( m_width && m_height )
	{
		StartLoading();
	}
	return true;
}

Tr2RenderContextEnum::TextureType Tr2CompositedVectorTextureRes::GetTextureType() const
{
	return m_texture.GetType();
}

Tr2RenderContextEnum::PixelFormat Tr2CompositedVectorTextureRes::GetFormat() const
{
	return m_texture.GetFormat();
}

uint32_t Tr2CompositedVectorTextureRes::GetWidth() const
{
	return m_texture.GetWidth();
}

uint32_t Tr2CompositedVectorTextureRes::GetHeight() const
{
	return m_texture.GetHeight();
}

uint32_t Tr2CompositedVectorTextureRes::GetOne() const
{
	return 1;
}

bool Tr2CompositedVectorTextureRes::Save( const wchar_t* path )
{
	if( !IsGood() )
	{
		return false;
	}

	if( !ImageIO::IsSaveSupported( path, m_texture.GetDesc() ) )
	{
		CCP_LOGERR( "Unsupported format for saving (%S)", path );
		return false;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2HostBitmapPtr saveBitmap;

	if( !saveBitmap.CreateInstance() ||
		!saveBitmap->CreateFromBitmapDimensions( m_texture.GetDesc() ) ||
		!saveBitmap->CopyFromTexture( m_texture, renderContext ) )
	{
		return false;
	}

	return saveBitmap->Save( path );
}

void Tr2CompositedVectorTextureRes::RegisterDynamicConstructor( const wchar_t* name, const wchar_t* directory )
{
	BeResMan->RegisterResourceConstructor( name, new CompositedVectorTextureConstructor( directory ) );
}
