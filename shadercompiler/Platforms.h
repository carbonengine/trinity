// Copyright © 2023 CCP ehf.

#pragma once


enum Platform
{
	PLATFORM_INVALID = 0,

	PLATFORM_DX11 = 2,

	PLATFORM_DX12 = 6,

	PLATFORM_METAL = 10,

	_PLATFORM_END,
};

inline bool IsValidPlatform( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX11:
		return true;
	case PLATFORM_DX12:
		return true;
	case PLATFORM_METAL:
		return true;
	default:
		return false;
	}
}

inline const char* GetPlatformShortName( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX11:
		return "dx11";
	case PLATFORM_DX12:
		return "dx12";
	case PLATFORM_METAL:
		return "mtl";
	default:
		return "INVALID";
	}
}

inline const char* GetPlatformLongName( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX11:
		return "DirectX 11";
	case PLATFORM_DX12:
		return "DirectX 12";
	case PLATFORM_METAL:
		return "Metal";
	default:
		return "INVALID";
	}
}

inline const char* GetPlatformIdString( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX11:
		return "2";
	case PLATFORM_DX12:
		return "6";
	case PLATFORM_METAL:
		return "10";
	default:
		return "INVALID";
	}
}

inline Platform ParsePlatform( const char* name )
{
	for( int32_t i = 0; i < _PLATFORM_END; ++i )
	{
		if( IsValidPlatform( Platform( i ) ) )
		{
			if( strcmp( GetPlatformIdString( Platform( i ) ), name ) == 0 )
			{
				return Platform( i );
			}
			if( _stricmp( GetPlatformShortName( Platform( i ) ), name ) == 0 )
			{
				return Platform( i );
			}
		}
	}
	return PLATFORM_INVALID;
}
