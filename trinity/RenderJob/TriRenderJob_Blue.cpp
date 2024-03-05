#include "StdAfx.h"
#include "TriRenderJob.h"

BLUE_DEFINE( TriRenderJob );

const Be::VarChooser TriRenderJobStatusChooser[] =
{
	// Name					Value						Docstring
	{ "RJ_INIT",			BeCast( RJ_INIT ),			"Render job is in its initial state" }, 
	{ "RJ_IN_PROGRESS",		BeCast( RJ_IN_PROGRESS ),	"Render job is in progress" },
	{ "RJ_DONE",			BeCast( RJ_DONE ),			"Render job is done" },
	{ "RJ_FAILED",			BeCast( RJ_FAILED ),		"Render job failed" },
	{0}
};

BLUE_REGISTER_ENUM( "RENDERJOB_STATUS", TriRenderJobStatus, TriRenderJobStatusChooser );

const Be::ClassInfo* TriRenderJob::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriRenderJob, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( TriRenderJob )

		MAP_ATTRIBUTE
		( 
			"name", 
			m_name, 
			"Name of this render job", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"enabled", 
			m_enabled, 
			"Is this job enabled? If not, it does nothing when run", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"stackGuard", 
			m_stackGuard, 
			"Check and repair RT/DS stacks when needed", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE_WITH_CHOOSER
		( 
			"status", 
			m_status, 
			"Current status of this render job",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			TriRenderJobStatusChooser
		)
		
		MAP_ATTRIBUTE
		( 
			"steps", 
			m_renderSteps, 
			"RenderJob steps", 
			Be::READ | Be::PERSIST
		)

	EXPOSURE_END()
}