#include "StdAfx.h"
#include "WithWindowFixture.h"
#include "WithRenderContextFixture.h"
#include "WithValidRenderContextFixture.h"
#include "TrinityAL/Tr2DriverUtilities.h"

// Needed by CcpCore
const char* g_moduleName = "TrinityALTest";

// Needed by TrinityAL
bool g_usingEXDevice = false;
bool g_useManagedDX9Buffers = true;

// Interactive test flag (set by --interactive option)
bool g_interactive = false;
// Make screenshots for the first frame of interactive tests (set by --screenshots option)
bool g_makeScreenShots = false;
// Compare with existing screenshots
bool g_compareScreenShots = false;
// Folder to save screenshots (set by --screenshotdir option)
std::string g_screenshotFolder = "screenshots/" TRINITY_PLATFORM_NAME;

void PrintAdapterInfo( unsigned index )
{
	Tr2AdapterInfo info;
	if( FAILED( Tr2VideoAdapterInfo::GetAdapterInfo( index, info ) ) )
	{
		fprintf( stderr, "Failed to get video adapter information for adapter %u\n", index );
		return;
	}

	printf( 
		"Device name: %s\nDescription: %ls\nVendor ID: %u\nDevice ID: %u\n", 
		info.deviceName.c_str(), 
		info.description.c_str(), 
		info.vendorID, 
		info.deviceID );

	Tr2VideoDriverInfo driverInfo;
	if( FAILED( Tr2DriverUtilities::GetDriverVersion( info.deviceID, driverInfo ) ) )
	{
		fprintf( stderr, "Failed to get video driver information for adapter %u\n", index );
		return;
	}
	printf(
		"Driver version: %s\nDriver date: %s\nDriver vendor: %s\nIs Optimus: %s\nIs AMD Dynamic Switchable: %s\n\n",
		driverInfo.driverVersionString.c_str(),
		driverInfo.driverDate.c_str(),
		driverInfo.driverVendor.c_str(),
		driverInfo.isOptimus ? "yes" : "no",
		driverInfo.isAmdDynamicSwitchable ? "yes" : "no" );
}

void PrintAllAdapterInfo()
{
	unsigned count = 0;
	if( FAILED( Tr2VideoAdapterInfo::GetAdapterCount( count ) ) )
	{
		fprintf( stderr, "Failed to get video adapter count\n" );
		return;
	}
	for( unsigned i = 0; i < count; ++i )
	{
		PrintAdapterInfo( i );
	}
}

int main( int argc, char **argv ) 
{
	CCP::SetLogMainThreadId();

	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "--interactive" ) == 0 )
		{
			g_interactive = true;
		}
		else if( strcmp( argv[i], "--screenshots" ) == 0 )
		{
			g_makeScreenShots = true;
		}
		else if( strcmp( argv[i], "--compare" ) == 0 )
		{
			g_compareScreenShots = true;
		}
		else if( strcmp( argv[i], "--screenshotdir" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				printf( "Error parsing arguments: --screenshotdir should be followed by directory path" );
				return 1;
			}
			g_screenshotFolder = argv[++i];
			g_screenshotFolder += "/" TRINITY_PLATFORM_NAME;
		}
		else if( strcmp( argv[i], "--adapterinfo" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				printf( "Error parsing arguments: --adapterinfo should be followed by adapter index or \"all\"" );
				return 1;
			}
			if( strcmp( argv[i + 1], "all" ) == 0 )
			{
				PrintAllAdapterInfo();
			}
			else
			{
				PrintAdapterInfo( atoi( argv[i + 1] ) );
			}
		}
	}
	::testing::InitGoogleTest( &argc, argv );
	int result = RUN_ALL_TESTS();
	return result;
}

#if defined(__ANDROID__)

#include <android/native_activity.h>

ANativeActivity* g_androidActivity = nullptr;
ANativeWindow* g_androidWindow = nullptr;
bool g_windowResized = false;

uint32_t mainThread( void* )
{
    char* args[] = {
        strdup( "TrinityALTest" ),
        strdup( "--interactive" ),
    };
    main( 2, args );
    ANativeActivity_finish( g_androidActivity );
    return 0;
}

static void onNativeWindowCreated( ANativeActivity*, ANativeWindow* window )
{
    g_androidWindow = window;
    CcpCreateThread( &mainThread, nullptr, CCP_THREAD_PRIORITY_NORMAL );
    
}

static void onNativeWindowResized( ANativeActivity*, ANativeWindow* window )
{
    g_windowResized = true;
}

void ANativeActivity_onCreate( ANativeActivity* activity, void*, size_t )
{
    g_androidActivity = activity;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    
}

#endif