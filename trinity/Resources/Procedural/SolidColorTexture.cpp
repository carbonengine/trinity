// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "SolidColorTexture.h"
#include "../TriTextureRes.h"
#include <sstream>
#include <string_view>


namespace
{

constexpr std::string_view colorPrefix = "dynamic:/color/";
constexpr std::wstring_view colorPrefixW = L"dynamic:/color/";


struct SolidColorTextureConstructor : public IBlueDynamicResourceConstructor
{
	SolidColorTextureConstructor()
	{
		BeResMan->RegisterResourceConstructor( L"color", this );
	}
	IBlueResource* GetResource( const wchar_t* query ) override
	{
		TriTextureResPtr tex;
		tex.CreateInstance();
		tex->Initialize( ( std::wstring( colorPrefixW ) + query ).c_str(), L"" );
		return tex.Detach();
	}
};

SolidColorTextureConstructor s_solidColorTextureConstructor;

std::optional<Color> ParseColor( const std::string_view& query )
{
	std::stringstream stream = std::stringstream( std::string( query ) );
	stream.imbue( std::locale( "C" ) );

	Color color;
	char coma;

	stream >> color.r;
	stream >> coma;
	if( coma != ',' )
	{
		CCP_LOGERR( "Failed to parse dynamic:/color/%s texture path", std::string( query ).c_str() );
		return {};
	}
	stream >> color.g;
	stream >> coma;
	if( coma != ',' )
	{
		CCP_LOGERR( "Failed to parse dynamic:/color/%s texture path", std::string( query ).c_str() );
		return {};
	}
	stream >> color.b;
	stream >> coma;
	if( coma != ',' )
	{
		CCP_LOGERR( "Failed to parse dynamic:/color/%s texture path", std::string( query ).c_str() );
		return {};
	}
	stream >> color.a;
	if( !stream.eof() )
	{
		CCP_LOGERR( "Failed to parse dynamic:/color/%s texture path", std::string( query ).c_str() );
		return {};
	}
	return color;
}

bool IsSolidColorTexturePath( const std::string_view& path )
{
	return colorPrefix == path.substr( 0, colorPrefix.size() );
}


Color ColorPathToColor( const char* path )
{
	std::string_view pathView = std::string_view( path );
	if( !IsSolidColorTexturePath( pathView ) )
	{
		return {};
	}
	pathView.remove_prefix( colorPrefix.size() );
	auto color = ParseColor( pathView );
	if( !color )
	{
		return {};
	}
	return color.value();
}

std::string ColorToColorPath( const Color& color )
{
	std::stringstream stream;
	stream.imbue( std::locale( "C" ) );
	stream << colorPrefix << color.r << "," << color.g << "," << color.b << "," << color.a;
	return stream.str();
}

}

void RasterizeSolidColor( const std::string_view& path, ImageIO::HostBitmap& bitmap )
{
	bitmap.Destroy();
	if( !IsSolidColorTexturePath( path ) )
	{
		return;
	}

	auto color = ParseColor( std::string_view( path.data() + colorPrefix.size(), path.size() - colorPrefix.size() ) );
	if( !color )
	{
		return;
	}

	bitmap.Create( 1, 1, 1, ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT );
	if( bitmap.IsValid() )
	{
		reinterpret_cast<Float_16*>( bitmap.GetRawData() )[0] = Float_16( color->r );
		reinterpret_cast<Float_16*>( bitmap.GetRawData() )[1] = Float_16( color->g );
		reinterpret_cast<Float_16*>( bitmap.GetRawData() )[2] = Float_16( color->b );
		reinterpret_cast<Float_16*>( bitmap.GetRawData() )[3] = Float_16( color->a );
	}
}

bool IsSolidColorTexturePath( const wchar_t* path )
{
	return wcsncmp( path, colorPrefixW.data(), colorPrefixW.size() ) == 0;
}

MAP_FUNCTION_AND_WRAP(
	"ColorPathToColor",
	ColorPathToColor,
	"Returns a color represented by a dynamic:/color/... path. If the path is malformed, returns (0, 0, 0, 0) blank color.\n\n"
	":param path: dynamic:/color/... path" );

MAP_FUNCTION_AND_WRAP(
	"ColorToColorPath",
	ColorToColorPath,
	"Returns a dynamic:/color/... path for a color\n\n"
	":param color: Color to convert to a path" );