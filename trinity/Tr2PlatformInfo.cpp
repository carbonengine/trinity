////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2PlatformInfo.h"


std::string Tr2PlatformInfo::GetPlatformName() const
{
	return TRINITY_PLATFORM_NAME;
}

uint32_t Tr2PlatformInfo::GetPlatformID() const
{
	return TRINITY_PLATFORM;
}

bool Tr2PlatformInfo::GetStaticCap( StaticCap cap ) const
{
#define HANDLE_PLATFORM_CAP(x) case x: return TRINITY_PLATFORM_SUPPORTS_##x != 0;
#define HANDLE_APPLICATION_CAP(x) case x: return TRINITY_SUPPORTS_##x != 0;

	switch( cap )
	{
	HANDLE_PLATFORM_CAP( BUFFER_SHADER_RESOURCES );
	HANDLE_PLATFORM_CAP( UNORDERED_ACCESS );
	HANDLE_PLATFORM_CAP( COMPUTE );
	HANDLE_PLATFORM_CAP( TEXTURE_ARRAYS );
	HANDLE_PLATFORM_CAP( MSAA_SAMPLE );

	default:
		return false;
	}
#undef HANDLE_PLATFORM_CAP
}

bool Tr2PlatformInfo::IsLowPerformance() const
{
	return TRINITY_PLATFORM_IS_LOW_PERFORMACE != 0;
}
