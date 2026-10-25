// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2BackgroundWorker_h
#define Tr2BackgroundWorker_h

#include <IBlueCallbackMan.h>

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BackgroundWorker is a small class to offer asynchronous execution of tasks.
// See Also:
//   Tr2ProceduralGeometryProvider
// --------------------------------------------------------------------------------------
class Tr2BackgroundWorker
{
public:
	static IBlueCallbackMan& Instance();
};

#endif // Tr2BackgroundWorker_h