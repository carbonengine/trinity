// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2DriverUtilities.h"

namespace
{

struct DriverInfo
{
	Tr2VideoDriverInfo info;
	ALResult fetchResult;
};

TrackableStdUnorderedMap<uint32_t, DriverInfo> s_fetchedDriverInfo( "Tr2DriverUtilities::s_fetchedDriverInfo" );

#ifdef _WIN32

bool IsOptimus()
{
	static bool initialized = false;
	static bool isOptimus = false;
	if( !initialized )
	{
		initialized = true;
		isOptimus = GetModuleHandleW( L"nvd3d9wrap.dll" ) != nullptr;
	}
	return isOptimus;
}

bool GetHexIdFromDeviceId( const char* deviceId, uint32_t& deviceIdHex )
{
	const char* deviceIdPrefix = "DEV_";

	auto found = strstr( deviceId, deviceIdPrefix );
	if( !found )
	{
		return false;
	}
	return sscanf_s( found + strlen( deviceIdPrefix ), "%x", &deviceIdHex ) == 1;
}

const char* GetRegistryPathToLocalMachine( const char* registryPath )
{
	const char* rootPath = "\\Registry\\Machine\\";
	if( strncmp( registryPath, rootPath, strlen( rootPath ) ) == 0 )
	{
		return registryPath + strlen( rootPath );
	}
	else
	{
		return registryPath;
	}
}

bool GetDeviceRegistryKey( uint32_t deviceId, std::string& keyPath )
{
	DISPLAY_DEVICE dd;
	dd.cb = sizeof( DISPLAY_DEVICE );

	for( int i = 0; EnumDisplayDevices( nullptr, i, &dd, 0 ); ++i )
	{
		uint32_t device;
		if( GetHexIdFromDeviceId( dd.DeviceID, device ) && device == deviceId )
		{
			keyPath = GetRegistryPathToLocalMachine( dd.DeviceKey );
			return true;
		}
	}
	return false;
}

bool GetRegistryValue( HKEY key, const char* name, std::string& value )
{
	char buffer[256];
	DWORD dwcb_data = sizeof( buffer );

	LONG result = RegQueryValueEx( key, name, nullptr, nullptr, reinterpret_cast<LPBYTE>( buffer ), &dwcb_data );
	if( result == ERROR_SUCCESS )
	{
		value = buffer;
		return true;
	}
	value = "";
	return false;
}

bool DriverVersionToInt64( const char* driverVersion, int64_t& intVersion )
{
	unsigned parts[4];
	if( sscanf_s( driverVersion, "%u.%u.%u.%u", &parts[0], &parts[1], &parts[2], &parts[3] ) == 4 )
	{
		intVersion = ( int64_t( parts[0] ) << 48 ) | ( int64_t( parts[1] ) << 32 ) | ( int64_t( parts[2] ) << 16 ) | int64_t( parts[3] );
		return true;
	}
	return false;
}

ALResult DoGetDriverVersion( uint32_t deviceId, Tr2VideoDriverInfo& info )
{
	std::string keyPath;
	if( !GetDeviceRegistryKey( deviceId, keyPath ) )
	{
		return E_FAIL;
	}

	HKEY key;
	LONG result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_QUERY_VALUE, &key );
	if( result != ERROR_SUCCESS )
	{
		return E_FAIL;
	}
	ON_BLOCK_EXIT_WITH_UNUSED( [&] { RegCloseKey( key ); } );

	if( GetRegistryValue( key, "DriverVersion", info.driverVersionString ) )
	{
		DriverVersionToInt64( info.driverVersionString.c_str(), info.driverVersion );
	}
	GetRegistryValue( key, "DriverDate", info.driverDate );
	if( GetRegistryValue( key, "ProviderName", info.driverVendor ) )
	{
		info.isAmdDynamicSwitchable = info.driverVendor == "Advanced Micro Devices, Inc." || info.driverVendor == "ATI Technologies Inc.";
	}

	info.isOptimus = IsOptimus();
	info.isAmdDynamicSwitchable = false;

	return S_OK;
}

#else

ALResult DoGetDriverVersion( uint32_t deviceId, Tr2VideoDriverInfo& info )
{
	return E_FAIL;
}

#endif

}

namespace Tr2DriverUtilities
{
#if ( TRINITY_PLATFORM == TRINITY_STUB )
ALResult GetDriverVersion( uint32_t deviceId, Tr2VideoDriverInfo& info )
{
	if( deviceId == 0xffffffff )
	{
		return E_FAIL;
	}
	info.driverDate = "01/01/01";
	info.driverVendor = "Stub";
	info.driverVersion = 31337;
	info.driverVersionString = "31337";
	info.isAmdDynamicSwitchable = false;
	info.isOptimus = false;
	return S_OK;
}
#else
ALResult GetDriverVersion( uint32_t deviceId, Tr2VideoDriverInfo& info )
{
	auto found = s_fetchedDriverInfo.insert( std::make_pair( deviceId, DriverInfo() ) );
	if( found.second )
	{
		found.first->second.fetchResult = DoGetDriverVersion( deviceId, found.first->second.info );
	}

	info = found.first->second.info;
	return found.first->second.fetchResult;
}
#endif
}
