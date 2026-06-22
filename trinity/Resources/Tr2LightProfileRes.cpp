// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#include "Tr2LightProfileRes.h"
#include "../Tr2LightManager.h"
#include "../Tr2HostBitmap.h"
#include <regex>
#include <sstream>


namespace
{

const uint32_t PROFILE_LIGHTMAP_SIZE = 1024;
const Tr2RenderContextEnum::PixelFormat PROFILE_LIGHTMAP_FORMAT = Tr2RenderContextEnum::PIXEL_FORMAT_R16_FLOAT;

bool IsCompatibleBitmap( const ImageIO::HostBitmap& bitmap )
{
	return bitmap.GetType() == Tr2RenderContextEnum::TEX_TYPE_2D &&
		bitmap.GetWidth() == PROFILE_LIGHTMAP_SIZE &&
		bitmap.GetHeight() == 1 &&
		bitmap.GetFormat() == PROFILE_LIGHTMAP_FORMAT &&
		bitmap.GetTrueMipCount() == 11;
}

template <typename T>
void Eat( std::istringstream& is )
{
	T t;
	is >> t;
}

bool IsIesExtension( const wchar_t* path )
{
	auto len = wcslen( path );
	if( len < 4 )
	{
		return false;
	}
	return path[len - 4] == '.' &&
		( path[len - 3] == 'i' || path[len - 3] == 'I' ) &&
		( path[len - 2] == 'e' || path[len - 2] == 'E' ) &&
		( path[len - 1] == 's' || path[len - 1] == 'S' );
}

}


Tr2LightProfileRes::Tr2LightProfileRes()
{
}

bool Tr2LightProfileRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2LightProfileRes::GetMemoryUsage()
{
	return IsGood() ? PROFILE_LIGHTMAP_SIZE : 0;
}

uint16_t Tr2LightProfileRes::GetTextureIndex() const
{
	return m_element.GetElementIndex();
}

Tr2LightProfileRes::LoadingResult Tr2LightProfileRes::DoLoad()
{
	if( !m_dataStream )
	{
		return LR_FAILED;
	}
	if( IsIesExtension( m_path.c_str() ) )
	{
		return ParseIes() ? LR_SUCCESS : LR_FAILED;
	}

	auto result = ImageIO::ReadImage( *m_dataStream, ImageIO::LoadParameters( m_path.c_str() ), m_bitmap );
	if( !result )
	{
		CCP_LOGWARN( "Tr2LightProfileRes: error reading '%S' - %s", GetPath(), result.GetErrorMessage().c_str() );
		return LR_FAILED;
	}
	if( !IsCompatibleBitmap( m_bitmap ) )
	{
		CCP_LOGWARN( "Tr2LightProfileRes: bitmap '%S' contains an uncompatible lightmap. The lightmap needs to be %ux1 texture with format R16F", GetPath(), PROFILE_LIGHTMAP_SIZE );
		m_bitmap.Destroy();
		return LR_FAILED;
	}

	return LR_SUCCESS;
}

bool Tr2LightProfileRes::DoPrepare()
{
	m_element = Tr2LightManager::GetLightProfileArray().AddElement( m_bitmap );
	return true;
}

bool Tr2LightProfileRes::ParseIes()
{
	std::string contents;
	contents.resize( m_dataStream->GetSize() );
	m_dataStream->Read( contents.data(), contents.size() );
	return ParseIes( contents, m_bitmap );
}


bool Tr2LightProfileRes::ParseIes( const std::string& contents, ImageIO::HostBitmap& bitmap )
{
	// Parsing according to:
	// http://lumen.iee.put.poznan.pl/kw/iesna.txt

	const std::regex tilt( "TILT\\s*=\\s*([^\\n]*)\\n" );
	std::smatch match;
	if( !std::regex_search( contents, match, tilt ) )
	{
		return false;
	}
	auto tiltType = match[1].str();
	size_t end = tiltType.find_last_not_of( " \r\t\f\v" );
	if( end != std::string::npos )
	{
		tiltType = tiltType.substr( 0, end + 1 );
	}
	if( tiltType != "NONE" )
	{
		return false;
	}

	std::istringstream is( match.suffix().str() );

	Eat<int>( is ); // # of lamps
	Eat<float>( is ); // lumens per lamp
	Eat<float>( is ); // candela multiplier
	size_t vAngleCount = 0;
	is >> vAngleCount;
	size_t hAngleCount = 0;
	is >> hAngleCount;
	Eat<int>( is ); // photometric type
	Eat<int>( is ); // units type
	Eat<float>( is ); // width
	Eat<float>( is ); // length
	Eat<float>( is ); // height
	Eat<float>( is ); // ballast factor
	Eat<float>( is ); // future use
	Eat<float>( is ); // input watts

	std::vector<float> vAngles( vAngleCount, 0.f );
	for( size_t i = 0; i < vAngleCount; ++i )
	{
		is >> vAngles[i];
	}
	std::vector<float> hAngles( hAngleCount, 0.f );
	for( size_t i = 0; i < hAngleCount; ++i )
	{
		is >> hAngles[i];
	}

	std::vector<float> intensities( vAngleCount, 0.f );
	for( size_t i = 0; i < vAngleCount; ++i )
	{
		is >> intensities[i];
	}

	float normalization = intensities[0];
	for( auto i : intensities )
	{
		normalization = std::max( normalization, i );
	}
	if( normalization > 0 )
	{
		for( auto& i : intensities )
		{
			i /= normalization;
		}
	}

	std::vector<float> strip( PROFILE_LIGHTMAP_SIZE, 0.f );
	for( size_t i = 0; i < strip.size(); ++i )
	{
		float angle = acos( -float( i ) / strip.size() * 2 + 1 ) / XM_PI * 180.f;

		float intensity = 0;
		for( size_t j = 0; j + 1 < vAngles.size(); ++j )
		{
			if( angle >= vAngles[j] && angle <= vAngles[j + 1] )
			{
				float t = ( angle - vAngles[j] ) / ( vAngles[j + 1] - vAngles[j] );
				intensity = intensities[j] + t * ( intensities[j + 1] - intensities[j] );
				break;
			}
		}
		strip[i] = intensity;
	}

	bitmap.Create( PROFILE_LIGHTMAP_SIZE, 1, 0, Tr2RenderContextEnum::PIXEL_FORMAT_R16_FLOAT );
	auto dest = reinterpret_cast<Float_16*>( bitmap.GetRawData() );
	for( size_t i = 0; i < strip.size(); ++i )
	{
		dest[i] = Float_16( strip[i] );
	}
	for( uint32_t mip = 1; mip < bitmap.GetTrueMipCount(); ++mip )
	{
		auto src = reinterpret_cast<const Float_16*>( bitmap.GetMipRawData( mip - 1 ) );
		auto dest = reinterpret_cast<Float_16*>( bitmap.GetMipRawData( mip ) );
		auto width = bitmap.GetMipWidth( mip );
		for( uint32_t i = 0; i < width; ++i )
		{
			*dest = Float_16( ( float( src[0] ) + float( src[1] ) ) * 0.5f );
			++dest;
			src += 2;
		}
	}
	return true;
}

BlueStdResult Tr2LightProfileRes::BakeLightProfile( const wchar_t* path, Tr2HostBitmapPtr& output )
{
	output = nullptr;

	if( !IsIesExtension( path ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "This function expects an .ies path" );
	}

	IBlueStreamPtr stream;
	auto success = BePaths->GetFileContentsWithYield( path, &stream );
	if( !BeIsSuccess( success ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_IO_ERROR, success.value.c_str() );
	}

	output.CreateInstance();

	std::string contents;
	contents.resize( stream->GetSize() );
	stream->Read( contents.data(), contents.size() );

	if( !ParseIes( contents, *output ) )
	{
		output = nullptr;
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "Failed to parse IES file" );
	}
	return BlueStdResult( BLUE_STD_RESULT_OK );
}

BlueStdResult Tr2LightProfileRes::GetThumbnail( uint32_t width, uint32_t height, Tr2HostBitmapPtr& bitmap ) const
{
	bitmap = nullptr;

	IBlueStreamPtr stream;
	auto success = BePaths->GetFileContentsWithYield( m_path.c_str(), &stream );
	if( !BeIsSuccess( success ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_IO_ERROR, success.value.c_str() );
	}

	ImageIO::HostBitmap lightmap;

	if( IsIesExtension( m_path.c_str() ) )
	{
		std::string contents;
		contents.resize( stream->GetSize() );
		stream->Read( contents.data(), contents.size() );

		if( !ParseIes( contents, lightmap ) )
		{
			return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "Failed to parse IES file" );
		}
	}
	else
	{
		auto result = ImageIO::ReadImage( *stream, ImageIO::LoadParameters( m_path.c_str() ), lightmap );
		if( !result )
		{
			return BlueStdResult( BLUE_STD_RESULT_IO_ERROR, result.GetErrorMessage().c_str() );
		}
		if( !IsCompatibleBitmap( lightmap ) )
		{
			return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "The input texture has incompatible dimensions/format" );
		}
	}

	bitmap.CreateInstance();
	bitmap->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM );
	if( !bitmap )
	{
		bitmap = nullptr;
		return BlueStdResult( BLUE_STD_RESULT_MEMORY_ERROR, "Ran out of memory when creating the thumbnail bitmap" );
	}

	auto PlaneRay = []( const Plane& plane, const Vector3& origin, const Vector3& direction ) {
		float t = -( plane.d + origin.x * plane.a + origin.y * plane.b + origin.z * plane.c ) / ( direction.x * plane.a + direction.y * plane.b + direction.z * plane.c );
		return std::make_pair( t, origin + direction * t );
	};


	float aspectRatio = float( width ) / float( height );

	Matrix rotation = RotationMatrix( Vector3( 1, 0, 0 ), 0.3f );

	Vector3 light = Vector3( 0, 0.7f, 0.98f );

	for( uint32_t j = 0; j < height; ++j )
	{
		uint8_t* row = reinterpret_cast<uint8_t*>( bitmap->GetRawData() ) + j * bitmap->GetPitch();

		for( uint32_t i = 0; i < width; ++i )
		{
			float x = float( i ) / width * 2 - 1;
			float y = -float( j ) / height * 2 + 1;

			x *= 1.3f * aspectRatio;
			y *= 1.3f;

			Vector3 origin = Vector3( x, y - 0.2f, -1 );
			Vector3 direction = Vector3( 0, 0, 1 );

			origin = TransformCoord( origin, rotation );
			direction = TransformNormal( direction, rotation );

			Plane back = Plane( 0, 0, 1, -1 );
			Plane bottom = Plane( 0, 1, 0, 1 );

			auto i0 = PlaneRay( back, origin, direction );
			auto i1 = PlaneRay( bottom, origin, direction );

			auto p = i0.first < i1.first ? i0.second : i1.second;

			auto lightDir = Normalize( p - light );

			auto cs = -lightDir.y;

			float lmx = std::max( 0.f, ( -cs * 0.5f + 0.5f ) * lightmap.GetWidth() );
			auto weight = lmx - floor( lmx );
			uint32_t coord0 = std::min( uint32_t( lmx ), lightmap.GetWidth() - 1 );
			uint32_t coord1 = std::min( coord0 + 1, lightmap.GetWidth() - 1 );

			float intensity0 = float( reinterpret_cast<Float_16*>( lightmap.GetRawData() )[coord0] );
			float intensity1 = float( reinterpret_cast<Float_16*>( lightmap.GetRawData() )[coord1] );

			float intensity = intensity0 * ( 1 - weight ) + intensity1 * weight;
			intensity = pow( intensity, 1.f / 2.4f );

			*row++ = uint8_t( intensity * 255 );
			*row++ = uint8_t( intensity * 255 );
			*row++ = uint8_t( intensity * 255 );
			*row++ = uint8_t( intensity * 255 );
		}
	}
	return BlueStdResult( BLUE_STD_RESULT_OK );
}
