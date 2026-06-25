// Copyright © 2021 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuProfiler.h"

namespace
{
Tr2GpuProfiler* GetGlobalGpuProfiler()
{
	return &Tr2GpuProfiler::GetProfiler();
}

}

MAP_FUNCTION_AND_WRAP( "GetGpuProfiler", GetGlobalGpuProfiler, "Returns the global instance of GPU profiler" );


BLUE_DEFINE( Tr2GpuProfiler );

const Be::ClassInfo* Tr2GpuProfiler::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GpuProfiler, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2GpuProfiler )

		MAP_METHOD_AND_WRAP( "Capture", Capture, "Requests profile capture for the next frame" )
		MAP_METHOD_AND_WRAP(
			"GetFrameReport",
			GetFrameReport,
			"Returns captured data for zone tree. The data returned is a tree with each node\n"
			"being a tuple (owner, label, stats, children) where:\n"
			"owner - zone\'s owner object or None,\n"
			"label - text label for the zone,\n"
			"stats - a dict with captured stats,\n"
			"children - a list of child zone data (same tuple format).\n"
			"If the data is not ready the function raises a RuntimeError.\n"
			":rtype: (blue.IRoot|None, str, dict, list)\n" )
		MAP_METHOD_AND_WRAP( "IsReportReady", IsDataReady, "Check if the captured data is ready to be queried with GetFrameReport method" )
		MAP_METHOD_AND_WRAP( "ClearData", ClearData, "Clears any captured data" )

	EXPOSURE_END();
}