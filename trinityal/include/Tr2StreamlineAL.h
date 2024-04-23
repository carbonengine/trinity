#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_DIRECTX11

#include <sl.h>
#include <sl_consts.h>

#define DECLARE_SL_FUNC( streamlineModule, func ) \
	PFun_##func* m_##func = reinterpret_cast<PFun_##func*>( GetProcAddress( streamlineModule, #func ) );\

#define INITIALIZE_SL_FUNC( streamlineModule, func ) \
	m_##func = reinterpret_cast<PFun_##func*>( GetProcAddress( streamlineModule, #func ) )

#define INITIALIZE_SL_FEATURE_FUNC( func, feature )   \
	if( SL_FAILED( res, m_slGetFeatureFunction( feature, #func, (void*&)m_##func ) ) )                                                   \
		CCP_LOGERR( "Unable to find function %s for feature %s Error code: %d", #func, ##feature, res ); \

namespace Tr2StreamlineAL
{	
	const char* GetPluginName( sl::Feature feature );
	sl::float4x4 F16AsFloat4x4( float f[16] );

	void Tr2StreamlineLog( sl::LogType type, const char* msg );

	HMODULE GetStreamlineModule();
	sl::Result InitializeStreamline( HMODULE streamlineModule );
	void ReleaseStreamline( HMODULE streamlineModule );
	sl::Result CheckForAvailability( HMODULE streamlineModule, sl::Feature feature, sl::AdapterInfo adapterInfo );
	}
#endif
