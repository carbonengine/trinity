// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "GradientTexture.h"
#include "Base64Encoding.h"
#include "../TriTextureRes.h"
#include "../../Curves/Tr2CurveScalar.h"
#include "../../Curves/Tr2CurveColor.h"

namespace
{
struct Channel
{
	uint16_t keyCount;
	uint8_t extrapolationBefore;
	uint8_t extrapolationAfter;
};
struct Header
{
	uint32_t width;
	Channel r;
	Channel g;
	Channel b;
	Channel a;
};

constexpr std::string_view gradientPrefix = "dynamic:/gradient_1d/";
constexpr std::wstring_view gradientPrefixW = L"dynamic:/gradient_1d/";


struct GradientTextureConstructor : public IBlueDynamicResourceConstructor
{
	GradientTextureConstructor()
	{
		BeResMan->RegisterResourceConstructor( L"gradient_1d", this );
	}
	IBlueResource* GetResource( const wchar_t* query ) override
	{
		TriTextureResPtr tex;
		tex.CreateInstance();
		tex->Initialize( ( std::wstring( gradientPrefixW ) + query ).c_str(), L"" );
		return tex.Detach();
	}
};
GradientTextureConstructor s_gradientTextureConstructor;


bool IsGradientTexturePath( const std::string_view& path )
{
	return gradientPrefix == path.substr( 0, gradientPrefix.size() );
}


std::pair<IRootPtr, uint32_t> GradientPathToCurve( const char* path )
{
	std::string_view pathView = std::string_view( path );
	if( !IsGradientTexturePath( pathView ) )
	{
		return {};
	}
	pathView.remove_prefix( gradientPrefix.size() );

	auto data = Base64::Decode( pathView );
	if( data.size() < sizeof( Header ) )
	{
		return {};
	}
	Header* header = reinterpret_cast<Header*>( data.data() );
	auto keyCount = header->r.keyCount + header->g.keyCount + header->b.keyCount + header->a.keyCount;
	if( data.size() != sizeof( Header ) + keyCount * sizeof( Tr2CurveScalarKey ) )
	{
		return {};
	}
	if( header->width == 0 )
	{
		return {};
	}
	auto keys = reinterpret_cast<Tr2CurveScalarKey*>( data.data() + sizeof( Header ) );
	Tr2CurveScalarDefinition r = { keys, header->r.keyCount, Tr2CurveExtrapolation::Type( header->r.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->r.extrapolationAfter ) };
	keys += header->r.keyCount;
	Tr2CurveScalarDefinition g = { keys, header->g.keyCount, Tr2CurveExtrapolation::Type( header->g.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->g.extrapolationAfter ) };
	keys += header->g.keyCount;
	Tr2CurveScalarDefinition b = { keys, header->b.keyCount, Tr2CurveExtrapolation::Type( header->b.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->b.extrapolationAfter ) };
	keys += header->b.keyCount;
	Tr2CurveScalarDefinition a = { keys, header->a.keyCount, Tr2CurveExtrapolation::Type( header->a.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->a.extrapolationAfter ) };

	Tr2CurveColorPtr curve;
	curve.CreateInstance();
	curve->m_r.SetDefinition( r );
	curve->m_g.SetDefinition( g );
	curve->m_b.SetDefinition( b );
	curve->m_a.SetDefinition( a );

	return { curve->GetRawRoot(), header->width };
}


std::string CurveToGradientPath( const Tr2CurveColor* curve, uint32_t width )
{
	std::string result;
	if( width > 0 && curve )
	{
		auto r = curve->m_r.GetDefinition();
		auto g = curve->m_g.GetDefinition();
		auto b = curve->m_b.GetDefinition();
		auto a = curve->m_a.GetDefinition();

		size_t size = sizeof( Header );
		size += r.keyCount * sizeof( Tr2CurveScalarKey );
		size += g.keyCount * sizeof( Tr2CurveScalarKey );
		size += b.keyCount * sizeof( Tr2CurveScalarKey );
		size += a.keyCount * sizeof( Tr2CurveScalarKey );

		std::vector<uint8_t> data( size );

		Header header;
		header.width = width;
		header.r = { uint16_t( r.keyCount ), uint8_t( r.extrapolationAfter ), uint8_t( r.extrapolationBefore ) };
		header.g = { uint16_t( g.keyCount ), uint8_t( g.extrapolationAfter ), uint8_t( g.extrapolationBefore ) };
		header.b = { uint16_t( b.keyCount ), uint8_t( b.extrapolationAfter ), uint8_t( b.extrapolationBefore ) };
		header.a = { uint16_t( a.keyCount ), uint8_t( a.extrapolationAfter ), uint8_t( a.extrapolationBefore ) };

		memcpy( data.data(), &header, sizeof( Header ) );
		auto keys = reinterpret_cast<Tr2CurveScalarKey*>( data.data() + sizeof( Header ) );
		memcpy( keys, r.keys, r.keyCount * sizeof( Tr2CurveScalarKey ) );
		keys += r.keyCount;
		memcpy( keys, g.keys, g.keyCount * sizeof( Tr2CurveScalarKey ) );
		keys += g.keyCount;
		memcpy( keys, b.keys, b.keyCount * sizeof( Tr2CurveScalarKey ) );
		keys += b.keyCount;
		memcpy( keys, a.keys, a.keyCount * sizeof( Tr2CurveScalarKey ) );

		result = std::string( gradientPrefix ) + Base64::Encode( data.data(), data.size() );
	}
	return result;
}

}


void RasterizeGradient( const std::string_view& path, ImageIO::HostBitmap& bitmap )
{
	bitmap.Destroy();

	if( !IsGradientTexturePath( path ) )
	{
		return;
	}

	auto data = Base64::Decode( std::string_view( path.data() + gradientPrefix.size(), path.size() - gradientPrefix.size() ) );
	if( data.size() < sizeof( Header ) )
	{
		CCP_LOGERR( "Failed to parse dynamic:/gradient_1d/ texture path: the path %s is invalid", std::string( path ).c_str() );
		return;
	}
	Header* header = reinterpret_cast<Header*>( data.data() );
	auto keyCount = header->r.keyCount + header->g.keyCount + header->b.keyCount + header->a.keyCount;
	if( data.size() != sizeof( Header ) + keyCount * sizeof( Tr2CurveScalarKey ) )
	{
		CCP_LOGERR( "Failed to parse dynamic:/gradient_1d/ texture path: the path %s is invalid", std::string( path ).c_str() );
		return;
	}
	if( header->width == 0 )
	{
		CCP_LOGERR( "The texture path %s has texture width of zero: the path is invalid", std::string( path ).c_str() );
		return;
	}
	auto keys = reinterpret_cast<Tr2CurveScalarKey*>( data.data() + sizeof( Header ) );
	Tr2CurveScalarDefinition r = { keys, header->r.keyCount, Tr2CurveExtrapolation::Type( header->r.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->r.extrapolationAfter ) };
	keys += header->r.keyCount;
	Tr2CurveScalarDefinition g = { keys, header->g.keyCount, Tr2CurveExtrapolation::Type( header->g.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->g.extrapolationAfter ) };
	keys += header->g.keyCount;
	Tr2CurveScalarDefinition b = { keys, header->b.keyCount, Tr2CurveExtrapolation::Type( header->b.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->b.extrapolationAfter ) };
	keys += header->b.keyCount;
	Tr2CurveScalarDefinition a = { keys, header->a.keyCount, Tr2CurveExtrapolation::Type( header->a.extrapolationBefore ), Tr2CurveExtrapolation::Type( header->a.extrapolationAfter ) };

	bitmap.Create( header->width, 1, 1, ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT );
	auto pixels = reinterpret_cast<Float_16*>( bitmap.GetRawData() );

	CTr2CurveScalar curve;
	curve.SetDefinition( r );
	curve.Rasterize( { header->width, 4, pixels } );
	curve.SetDefinition( g );
	curve.Rasterize( { header->width, 4, pixels + 1 } );
	curve.SetDefinition( b );
	curve.Rasterize( { header->width, 4, pixels + 2 } );
	curve.SetDefinition( a );
	curve.Rasterize( { header->width, 4, pixels + 3 } );
}


bool IsGradientTexturePath( const wchar_t* path )
{
	return wcsncmp( path, gradientPrefixW.data(), gradientPrefixW.size() ) == 0;
}


MAP_FUNCTION_AND_WRAP(
	"GradientPathToCurve",
	GradientPathToCurve,
	"Constructs a Tr2CurveColor object with curves set up to represent the given dynamic:/gradient_1d/... res path.\n"
	"Returns either a tuple with Tr2CurveColor object and texture width or (None, 0) if the path is malformed.\n\n"
	":param path: dynamic:/gradient_1d/... res path\n" );

MAP_FUNCTION_AND_WRAP(
	"CurveToGradientPath",
	CurveToGradientPath,
	"Constructs a dynamic:/gradient_1d/... res path from the given Tr2CurveColor object\n\n"
	":param curve: Tr2CurveColor object\n"
	":param width: width of the gradient texture\n" );
