#include "StdAfx.h"
#include <Tr2DriverUtilities.h>

using namespace Tr2RenderContextEnum;

TEST( VideoAdapterInfo, HasAtLeastOneAdapter )
{
	unsigned count = 0;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterCount( count ) );
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }
	EXPECT_GT( count, 0u );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterInfo )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

	Tr2AdapterInfo info;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterInfo( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, info ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterMonitor )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

	void* monitor;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMonitor( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, monitor ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterDisplayMode )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	EXPECT_GT( mode.width, 0u );
	EXPECT_GT( mode.height, 0u );
	EXPECT_GT( mode.format, 0 );
}

TEST( VideoAdapterInfo, CanEnumerateModesForDefaultAdapter )
{
	unsigned adapter_count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(adapter_count);
	if (!adapter_count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

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

TEST( VideoAdapterInfo, DefaultAdapterSupportsItsCurrentBackBufferFormat )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	
	EXPECT_TRUE( Tr2VideoAdapterInfo::SupportsBackBufferFormat( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode.format ) );
}

TEST( VideoAdapterInfo, SameAdaptersAreNotDifferent )
{
	EXPECT_FALSE( Tr2VideoAdapterInfo::AreAdaptersDifferent( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, Tr2VideoAdapterInfo::DEFAULT_ADAPTER ) );
}

TEST( VideoAdapterInfo, CanQueryMsaaSupport )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

	unsigned quality;
	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		PIXEL_FORMAT_B8G8R8A8_UNORM, 
		1, 
		quality ) );
}

TEST( VideoAdapterInfo, DefaultAdapterSupports32bppRenderTarget )
{
	EXPECT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::SupportsRenderTargetFormat( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
		PIXEL_FORMAT_B8G8R8A8_UNORM ) );
}

#ifdef _WIN32
TEST( VideoAdapterInfo, CanGetDriverInfo )
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount(count);
	if (!count) { GTEST_SKIP() << "Test Skipped as no adapters present on machine."; }

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