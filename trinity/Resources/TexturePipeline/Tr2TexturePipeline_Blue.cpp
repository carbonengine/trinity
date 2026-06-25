// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2TexturePipeline.h"
#include "Tr2HostBitmap.h"


BLUE_DEFINE_INTERFACE( ITr2TexturePipelineStep );


BLUE_DEFINE( Tr2TexturePipeline );

const Be::ClassInfo* Tr2TexturePipeline::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipeline, "" )
		MAP_INTERFACE( Tr2TexturePipeline )

		MAP_ATTRIBUTE( "pipelineType", m_pipelineType, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "steps", m_steps, "Pipeline steps", Be::READ | Be::PERSIST )
		MAP_METHOD_AND_WRAP(
			"GetResourceDependencies",
			GetResourceDependenciesFromScript,
			"Returns a list of res file paths of resource dependencies for this pipeline" )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"Execute",
			ExecuteFromScript,
			2,
			"Executes the pipeline and returns the resulting bitmap.\n"
			":param maxWidth: Max resulting texture width\n"
			":param maxHeight: Max resulting texture width\n" )

	EXPOSURE_END()
}
