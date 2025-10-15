////////////////////////////////////////////////////////////
//
//    Created:   October 2025
//    Copyright: CCP 2025
//

#include "StdAfx.h"
#include "ContinueOnMainThread.h"

namespace
{
std::vector<std::function<void()>> mainThreadActions;
std::mutex mainThreadActionsMutex;
}

void ContinueOnMainThread( std::function<void()>&& action )
{
	std::lock_guard<std::mutex> lock( mainThreadActionsMutex );
	mainThreadActions.push_back( std::move( action ) );
}

void ExecuteMainThreadActions()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	static std::vector<std::function<void()>> actionsToProcess;
	{
		std::lock_guard<std::mutex> lock( mainThreadActionsMutex );
		actionsToProcess.swap( mainThreadActions );
	}
	for( auto& action : actionsToProcess )
	{
		action();
	}
	actionsToProcess.clear();
}
