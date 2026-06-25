// Copyright © 2013 CCP ehf.

#pragma once
#ifndef ALLog_H
#define ALLog_H

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


#endif