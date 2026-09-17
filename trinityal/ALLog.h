// Copyright © 2013 CCP ehf.

#pragma once
#ifndef ALLog_H
#define ALLog_H

#include <atomic>
#include <cstdint>

namespace CCP
{
inline CcpLogChannel_t& GetTrinityALChannel()
{
	static CcpLogChannel_t s_moduleChannel = CCP_LOG_DEFINE_CHANNEL( "TrinityAL" );
	return s_moduleChannel;
}
}

#define CCP_AL_LOG( ... ) CCP_LOG_CH( CCP::GetTrinityALChannel(), __VA_ARGS__ )
#define CCP_AL_LOGERR( ... ) CCP_LOGERR_CH( CCP::GetTrinityALChannel(), __VA_ARGS__ )
#define CCP_AL_LOGNOTICE( ... ) CCP_LOGNOTICE_CH( CCP::GetTrinityALChannel(), __VA_ARGS__ )
#define CCP_AL_LOGWARN( ... ) CCP_LOGWARN_CH( CCP::GetTrinityALChannel(), __VA_ARGS__ )

// Per-call-site cap for warnings that can fire every draw or every frame
#define CCP_AL_LOG_LIMIT 32
#define CCP_AL_LOGWARN_LIMITED( ... ) \
	do \
	{ \
		static std::atomic<uint32_t> s_ccpAlLogCount = 0; \
		if( s_ccpAlLogCount.fetch_add( 1 ) < CCP_AL_LOG_LIMIT ) \
		{ \
			CCP_AL_LOGWARN( __VA_ARGS__ ); \
		} \
	} while( false )

#endif