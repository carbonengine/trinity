// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRunJob.h"
#include "TriRenderJob.h"

BLUE_DEFINE( TriStepRunJob );

const Be::ClassInfo* TriStepRunJob::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRunJob, "" )
		MAP_INTERFACE( TriStepRunJob )

		MAP_ATTRIBUTE(
			"job",
			m_job,
			"The render job to run",
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			SetRenderJob,
			1,
			"Creates a render job that runs another renderjob\n"
			":param renderJob: the TriRenderJob to run" )

	EXPOSURE_CHAINTO( TriRenderStep )
}