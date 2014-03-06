#include "StdAfx.h"
#include "TrinityAL/Tr2DriverUtilities.h"

using namespace Tr2RenderContextEnum;

TEST( VideoAdapterInfo, HasAtLeastOneAdapter )
{
	unsigned count = 0;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterCount( count ) );
	EXPECT_GT( count, 0u );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterInfo )
{
	Tr2AdapterInfo info;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterInfo( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, info ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterMonitor )
{
	void* monitor;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMonitor( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, monitor ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterDisplayMode )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	EXPECT_GT( mode.width, 0u );
	EXPECT_GT( mode.height, 0u );
	EXPECT_GT( mode.format, 0 );
}

TEST( VideoAdapterInfo, CanEnumerateModesForDefaultAdapter )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	
	PixelFormat backBufferFormat = mode.format;
	unsigned count = 0;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterModeCount( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, backBufferFormat, count ) );

	EXPECT_GT( count, 0u );

	for( unsigned i = 0; i < count; ++i )
	{
		memset( &mode, 0, sizeof( mode ) );
		ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, backBufferFormat, i, mode ) );
		EXPECT_GT( mode.width, 0u );
		EXPECT_GT( mode.height, 0u );
		EXPECT_GT( mode.format, 0 );
	}
}

TEST( VideoAdapterInfo, CanGetShaderVersionForDefaultAdapter )
{
	unsigned version = 0xDeadBeef;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterShaderVersion( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, version ) );
	ASSERT_NE( version, 0xDeadBeef );
}

TEST( VideoAdapterInfo, DefaultAdapterSupportsItsCurrentBackBufferFormat )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	
	EXPECT_TRUE( 
		Tr2VideoAdapterInfo::SupportsBackBufferFormat( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode.format, false ) ||
		Tr2VideoAdapterInfo::SupportsBackBufferFormat( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode.format, true ) );
}

TEST( VideoAdapterInfo, SameAdaptersAreNotDifferent )
{
	EXPECT_FALSE( Tr2VideoAdapterInfo::AreAdaptersDifferent( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, Tr2VideoAdapterInfo::DEFAULT_ADAPTER ) );
}

TEST( VideoAdapterInfo, CanQueryMsaaSupport )
{
	unsigned quality;
	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		PIXEL_FORMAT_B8G8R8A8_UNORM, 
		true, 
		1, 
		quality ) );
	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		DSFMT_D16, 
		true, 
		1, 
		quality ) );
}

TEST( VideoAdapterInfo, DefaultAdapterSupports32bppRenderTarget )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );

	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::SupportsRenderTargetFormat( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		mode.format, 
		PIXEL_FORMAT_B8G8R8A8_UNORM, 
		false ) );
}

TEST( VideoAdapterInfo, DefaultAdapterSupports16bppDepthStencil )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );

	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::SupportsDepthStencilFormat( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		mode.format, 
		DSFMT_D16 ) );
}

#ifdef _WIN32
TEST( VideoAdapterInfo, CanGetDriverInfo )
{
	Tr2AdapterInfo info;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterInfo( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, info ) );

	Tr2VideoDriverInfo driverInfo;
	ASSERT_HRESULT_SUCCEEDED( Tr2DriverUtilities::GetDriverVersion( info.deviceID, driverInfo ) );

	EXPECT_FALSE( driverInfo.driverVersionString.empty() );
	EXPECT_FALSE( driverInfo.driverVendor.empty() );
	EXPECT_FALSE( driverInfo.driverDate.empty() );
	EXPECT_GT( driverInfo.driverVersion, 0 );
}

TEST( VideoAdapterInfo, GettingDriverInfoForInvalidVendorFails )
{
	Tr2VideoDriverInfo driverInfo;
	ASSERT_HRESULT_FAILED( Tr2DriverUtilities::GetDriverVersion( 0xffffffff, driverInfo ) );
}

#endif