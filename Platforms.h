#pragma once


enum Platform
{
	PLATFORM_INVALID = 0,

	PLATFORM_DX9 = 1,
	PLATFORM_DX11 = 2,
	PLATFORM_GL2 = 3,

	PLATFORM_DX12 = 6,
	PLATFORM_VULKAN = 7,

	PLATFORM_GL3 = 8,
	PLATFORM_GL4 = 9,

	_PLATFORM_END,
};

inline bool IsValidPlatform( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX9:
		return true;
	case PLATFORM_DX11:
		return true;
	case PLATFORM_GL2:
		return true;
	case PLATFORM_DX12:
		return true;
	case PLATFORM_VULKAN:
		return true;
	case PLATFORM_GL3:
		return true;
	case PLATFORM_GL4:
		return true;
	default:
		return false;
	}
}

inline const char* GetPlatformShortName( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX9:
		return "dx9";
	case PLATFORM_DX11:
		return "dx11";
	case PLATFORM_GL2:
		return "gl2";
	case PLATFORM_DX12:
		return "dx12";
	case PLATFORM_VULKAN:
		return "vulkan";
	case PLATFORM_GL3:
		return "gl3";
	case PLATFORM_GL4:
		return "gl4";
	default:
		return "INVALID";
	}
}

inline const char* GetPlatformLongName( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX9:
		return "DirectX 9";
	case PLATFORM_DX11:
		return "DirectX 11";
	case PLATFORM_GL2:
		return "GLES 2";
	case PLATFORM_DX12:
		return "DirectX 12";
	case PLATFORM_VULKAN:
		return "Vulkan";
	case PLATFORM_GL3:
		return "OpenGL 3";
	case PLATFORM_GL4:
		return "OpenGL 4";
	default:
		return "INVALID";
	}
}

inline const char* GetPlatformIdString( Platform platform )
{
	switch( platform )
	{
	case PLATFORM_DX9:
		return "1";
	case PLATFORM_DX11:
		return "2";
	case PLATFORM_GL2:
		return "3";
	case PLATFORM_DX12:
		return "6";
	case PLATFORM_VULKAN:
		return "7";
	case PLATFORM_GL3:
		return "8";
	case PLATFORM_GL4:
		return "9";
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