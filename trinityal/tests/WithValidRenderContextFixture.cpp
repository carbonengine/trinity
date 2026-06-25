// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include <map>

#if defined( __ANDROID__ )
#include <android/log.h>

extern volatile bool g_windowResized;
#endif

WithValidRenderContext::WithValidRenderContext() :
	m_madeScreenshot( false )
{
}

void WithValidRenderContext::SetUpTestCase()
{
	WithWindow::SetUpTestCase();

	renderContext = new Tr2PrimaryRenderContextAL();
	Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( renderContext );

	CR( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, presentParameters.mode ) );
	presentParameters.mode.width = 640;
	presentParameters.mode.height = 480;
	presentParameters.backBufferCount = 1;
	presentParameters.msaaType = 0;
	presentParameters.msaaQuality = 0;
	presentParameters.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	presentParameters.outputWindow = WithWindow::GetWindowHandle();
	presentParameters.windowed = true;
	presentParameters.software = false;
	presentParameters.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_ONE;


	CR( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );

#if defined( __ANDROID__ )
	__android_log_print( ANDROID_LOG_INFO, "TrinityALTest", "OpenGL extensions: %s", glGetString( GL_EXTENSIONS ) );
	while( !g_windowResized )
	{
	}
#endif
}

void WithValidRenderContext::TearDownTestCase()
{
	delete renderContext;
	Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
	renderContext = nullptr;

	WithWindow::TearDownTestCase();
}

namespace
{

struct DDS_PIXELFORMAT
{
	uint32_t dwSize; // 4
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount; // 16
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask; // 32
};

struct DDS_HEADER
{
	uint32_t dwFourCC; // 4
	uint32_t dwSize;
	uint32_t dwHeaderFlags;
	uint32_t dwHeight; // 16
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth; // only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
	uint32_t dwMipMapCount; // 32
	uint32_t dwReserved1[11]; // 76
	DDS_PIXELFORMAT ddspf; // 108
	uint32_t dwSurfaceFlags;
	uint32_t dwCubemapFlags; // 116
	uint32_t dwReserved2[3]; // 128
};


#ifndef MAKEFOURCC
#define MAKEFOURCC( ch0, ch1, ch2, ch3 )                                 \
	( (uint32_t)(uint8_t)( ch0 ) | ( (uint32_t)(uint8_t)( ch1 ) << 8 ) | \
	  ( (uint32_t)(uint8_t)( ch2 ) << 16 ) | ( (uint32_t)(uint8_t)( ch3 ) << 24 ) )
#endif


#define DDS_HEADER_FLAGS_TEXTURE 0x00001007 // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP 0x00020000 // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME 0x00800000 // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH 0x00000008 // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000 // DDSD_LINEARSIZE
#define DDS_RGB 0x00000040 // DDPF_RGB
#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE

void SaveReadableRenderTarget( Tr2TextureAL& rt, const char* outFilePath, Tr2RenderContextAL& renderContext )
{
	ASSERT_FALSE( Tr2RenderContextEnum::IsCompressedFormat( rt.GetFormat() ) );

	const void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.MapForReading( Tr2TextureSubresource( 0 ), data, pitch, renderContext ) );

	DDS_HEADER header;
	memset( &header, 0, sizeof( header ) );
	header.dwFourCC = MAKEFOURCC( 'D', 'D', 'S', ' ' );
	header.dwSize = sizeof( header ) - 4;
	header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE;
	header.dwWidth = rt.GetWidth();
	header.dwHeight = rt.GetHeight();
	header.dwDepth = 0;
	header.dwMipMapCount = 0;

	header.ddspf.dwSize = sizeof( header.ddspf );
	header.ddspf.dwFlags = DDS_RGB;
	header.ddspf.dwRGBBitCount = 32;
	header.ddspf.dwRBitMask = 0xFF0000;
	header.ddspf.dwGBitMask = 0xFF00;
	header.ddspf.dwBBitMask = 0xFF;

	header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	header.dwPitchOrLinearSize = ( header.ddspf.dwRGBBitCount * header.dwWidth * header.dwHeight ) / 8;

	auto sz = pitch * rt.GetHeight();
	std::unique_ptr<uint8_t[]> rgbx( new uint8_t[sz] );
	memcpy( rgbx.get(), data, sz );
	for( uint32_t i = 3; i < sz; i += 4 )
	{
		rgbx[i] = 0xff;
	}

	FILE* f;
	if( fopen_s( &f, outFilePath, "wb" ) )
	{
		rt.UnmapForReading( renderContext );
		return;
	}
	EXPECT_EQ( 1, fwrite( &header, sizeof( header ), 1, f ) );
	EXPECT_EQ( 1, fwrite( rgbx.get(), pitch * rt.GetHeight(), 1, f ) );
	fclose( f );

	rt.UnmapForReading( renderContext );
}

::testing::AssertionResult LoadDDS( const char* filePath, uint32_t& width, uint32_t& height, std::unique_ptr<uint8_t[]>& data )
{
	FILE* f;
	if( fopen_s( &f, filePath, "rb" ) )
	{
		return ::testing::AssertionFailure() << "failed to open file " << filePath;
	}
	ON_BLOCK_EXIT( [=] { fclose( f ); } );

	DDS_HEADER header;
	if( 1 != fread( &header, sizeof( header ), 1, f ) )
	{
		return ::testing::AssertionFailure() << "failed to read DDS header from " << filePath;
	}

	width = header.dwWidth;
	height = header.dwHeight;

	if( width * height * 4 != header.dwPitchOrLinearSize )
	{
		return ::testing::AssertionFailure() << "unexpected DDS file pixel format or pitch " << filePath;
	}

	data.reset( new uint8_t[header.dwPitchOrLinearSize] );

	if( header.dwPitchOrLinearSize != fread( data.get(), 1, header.dwPitchOrLinearSize, f ) )
	{
		return ::testing::AssertionFailure() << "failed to read DDS pixel values " << filePath;
	}
	return ::testing::AssertionSuccess();
}

::testing::AssertionResult CompareWithBitmap( const char* filePath, Tr2TextureAL& rt, Tr2RenderContextAL& renderContext )
{
	uint32_t width, height;
	std::unique_ptr<uint8_t[]> data;
	auto loaded = LoadDDS( filePath, width, height, data );
	if( !loaded )
	{
		return ::testing::AssertionSuccess();
	}

	if( width != rt.GetWidth() )
	{
		return ::testing::AssertionFailure() << "width or DDS image " << width << " is not equal to back buffer width " << rt.GetWidth();
	}
	if( height != rt.GetHeight() )
	{
		return ::testing::AssertionFailure() << "height or DDS image " << height << " is not equal to back buffer height " << rt.GetHeight();
	}

	const void* rtData;
	uint32_t rtPitch;
	auto hr = rt.MapForReading( Tr2TextureSubresource( 0 ), rtData, rtPitch, renderContext );
	if( FAILED( hr ) )
	{
		return ::testing::AssertionFailure() << "failed to map render target, HR=" << hr.GetResult();
	}
	ON_BLOCK_EXIT( [&] { rt.UnmapForReading( renderContext ); } );

	const uint8_t* rtRow = static_cast<const uint8_t*>( rtData );
	//const uint8_t* flRow = data.get();
	for( uint32_t i = 0; i < rt.GetHeight(); ++i )
	{
		for( uint32_t j = 0; j < width; ++j )
		{
			if( rtRow[j * 4] != rtRow[j * 4] || rtRow[j * 4 + 1] != rtRow[j * 4 + 1] || rtRow[j * 4 + 2] != rtRow[j * 4 + 2] )
			{
				return ::testing::AssertionFailure() << "pixel values in expected DDS and back buffer are different at (" << j << ", " << i << ") pixel";
			}
		}
		rtRow += rtPitch;
		//flRow += 4 * width;
	}
	return ::testing::AssertionSuccess();
}

Tr2TextureAL GetReadableBackBuffer( Tr2PrimaryRenderContextAL& renderContext )
{
	auto& rt = renderContext.GetDefaultBackBuffer();
	if( rt.GetMsaaDesc().samples > 1 )
	{
		Tr2TextureAL readable;
		CR_RETURN_VAL( readable.Create( Tr2BitmapDimensions( rt.GetWidth(), rt.GetHeight(), 1, rt.GetFormat() ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ, renderContext ), readable );
		CR_RETURN_VAL( rt.Resolve( readable, renderContext ), Tr2TextureAL() );
		return readable;
	}
	return rt;
}

size_t FindSlash( const std::string& path, size_t offset )
{
	auto pos1 = path.find( '/', offset );
	auto pos2 = path.find( '\\', offset );
	if( pos1 != std::string::npos )
	{
		if( pos2 != std::string::npos )
		{
			return std::min( pos1, pos2 );
		}
		return pos1;
	}
	return pos2;
}

#ifdef _MSC_VER
#define mkdir( path ) _mkdir( path )
#elif defined( __ANDROID__ )
#include <sys/stat.h>
#define mkdir( path ) mkdir( path, 0777 );
#else
#define mkdir( path ) mkdir( path, S_IRWXU | S_IRGRP | S_IXGRP )
#endif

void MkDirs( const std::string& path )
{
	size_t start = 0;
	size_t pos = 0;
	std::string part;
	pos = FindSlash( path, start );
	while( pos != std::string::npos )
	{
		mkdir( path.substr( 0, pos ).c_str() );
		start = pos + 1;
		pos = FindSlash( path, start );
	}
	mkdir( path.c_str() );
}

}

bool WithValidRenderContext::MachineHasGfxAdapter()
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount( count );
	return count > 0;
}

void WithValidRenderContext::MakeScreenShot( const char* outFilePath )
{
	auto rt = GetReadableBackBuffer( *renderContext );
	ASSERT_TRUE( rt.IsValid() );
	SaveReadableRenderTarget( rt, outFilePath, *renderContext );
}


void WithValidRenderContext::MakeTestScreenShot()
{
#if TRINITY_PLATFORM == TRINITY_STUB
	return;
#endif

	extern bool g_makeScreenShots;
	extern bool g_compareScreenShots;
	extern std::string g_screenshotFolder;

	if( ( !g_makeScreenShots && !g_compareScreenShots ) || m_madeScreenshot )
	{
		return;
	}
	m_madeScreenshot = true;

	const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();

	std::string path = g_screenshotFolder;
	path += std::string( "/" ) + test_info->test_case_name();
	MkDirs( path.c_str() );
	path += std::string( "/" ) + test_info->name() + ".dds";

	if( g_makeScreenShots )
	{
		MakeScreenShot( path.c_str() );
	}
	else
	{
		auto rt = GetReadableBackBuffer( *renderContext );
		ASSERT_TRUE( rt.IsValid() );
		ASSERT_TRUE( CompareWithBitmap( path.c_str(), rt, *renderContext ) );
	}
}

Tr2PresentParametersAL WithValidRenderContext::presentParameters;
Tr2PrimaryRenderContextAL* WithValidRenderContext::renderContext = nullptr;
