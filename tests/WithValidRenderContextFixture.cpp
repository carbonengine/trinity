#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include <map>

#if defined(__ANDROID__)
#include <android/log.h>

extern volatile bool g_windowResized;
#endif

WithValidRenderContext::WithValidRenderContext()
	:m_madeScreenshot( false )
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
	presentParameters.depthStencilFormat = Tr2RenderContextEnum::DSFMT_D24S8;
	presentParameters.outputWindow = WithWindow::GetWindowHandle();
	presentParameters.windowed = true;
	presentParameters.software = false;
	presentParameters.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_ONE;

    
	CR( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );
    
#if defined(__ANDROID__)
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
	uint32_t dwSize;			// 4
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;	// 16
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;		// 32
};

struct DDS_HEADER
{
	uint32_t dwFourCC;				// 4
	uint32_t dwSize;
	uint32_t dwHeaderFlags;
	uint32_t dwHeight;				// 16
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;				// only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
	uint32_t dwMipMapCount;		// 32
	uint32_t dwReserved1[11];		// 76
	DDS_PIXELFORMAT ddspf;		// 108
	uint32_t dwSurfaceFlags;
	uint32_t dwCubemapFlags;		// 116
	uint32_t dwReserved2[3];		// 128
};


#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif


#define DDS_HEADER_FLAGS_TEXTURE    0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP     0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME     0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH      0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000  // DDSD_LINEARSIZE
#define DDS_RGB     0x00000040 // DDPF_RGB
#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE

void SaveReadableRenderTarget( Tr2RenderTargetAL& rt, const char* outFilePath, Tr2RenderContextAL& renderContext )
{
	ASSERT_FALSE( Tr2RenderContextEnum::IsCompressedFormat( rt.GetFormat() ) );

	void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.Lock( 0, nullptr, data, pitch, renderContext ) );

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
	header.dwPitchOrLinearSize = (header.ddspf.dwRGBBitCount * header.dwWidth * header.dwHeight) / 8;

	FILE* f;
	if( fopen_s( &f, outFilePath, "wb" ) )
	{
		rt.Unlock( renderContext );
		return;
	}
	EXPECT_EQ( 1, fwrite( &header, sizeof( header ), 1, f ) );
	EXPECT_EQ( 1, fwrite( data, pitch * rt.GetHeight(), 1, f ) );
	fclose( f );

	rt.Unlock( renderContext );
}

}

void WithValidRenderContext::MakeScreenShot( const char* outFilePath )
{
	Tr2RenderTargetAL& rt = renderContext->GetDefaultBackBuffer();
	ASSERT_TRUE( rt.IsValid() );
	if( rt.GetMsaaDesc().samples > 1 )
	{
		Tr2RenderTargetAL readable;
		ASSERT_HRESULT_SUCCEEDED( readable.Create( rt.GetWidth(), rt.GetHeight(), 1, rt.GetFormat(), Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( rt.Resolve( readable, *renderContext ) );
		SaveReadableRenderTarget( readable, outFilePath, *renderContext );
	}
	else
	{
		SaveReadableRenderTarget( rt, outFilePath, *renderContext );
	}
}

#ifdef _MSC_VER
#define mkdir( path ) _mkdir( path )
#elif defined(__ANDROID__)
#include <sys/stat.h>
#define mkdir( path ) mkdir( path, 0777 );
#else
#define mkdir( path ) mkdir( path, S_IRWXU|S_IRGRP|S_IXGRP )
#endif


namespace
{
std::map<std::string, std::string> s_madeScreenshots;
}

void WithValidRenderContext::MakeTestScreenShot()
{
	extern bool g_makeScreenShots;
	extern const char* g_screenshotFolder;

	if( !g_makeScreenShots || m_madeScreenshot )
	{
		return;
	}
	m_madeScreenshot = true;

	const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
	
	std::string path = g_screenshotFolder;
	mkdir( path.c_str() );
	path += std::string( "/" ) + test_info->test_case_name();
	mkdir( path.c_str() );
	path += std::string( "/" ) + test_info->name() + ".dds";

	MakeScreenShot( path.c_str() );

	std::string relPath = test_info->test_case_name();
	relPath += std::string( "/" ) + test_info->name() + ".dds";
	s_madeScreenshots[std::string( test_info->test_case_name() ) + std::string( "." ) + test_info->name()] = relPath;
}

Tr2PresentParametersAL WithValidRenderContext::presentParameters;
Tr2PrimaryRenderContextAL* WithValidRenderContext::renderContext = nullptr;
