// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRunComputeShader.h"
#include "Shader/Tr2Material.h"
#include "include/ITr2GpuBuffer.h"

BLUE_DEFINE( TriStepRunComputeShader );

const Be::ClassInfo* TriStepRunComputeShader::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRunComputeShader, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRunComputeShader )

		MAP_ATTRIBUTE( "effect", m_effect, "Effect containing a compute shader", Be::READWRITE )
		MAP_ATTRIBUTE( "groupDimX", m_groupDimX, "Dispatch group dimension.X", Be::READWRITE )
		MAP_ATTRIBUTE( "groupDimY", m_groupDimY, "Dispatch group dimension.Y", Be::READWRITE )
		MAP_ATTRIBUTE( "groupDimZ", m_groupDimZ, "Dispatch group dimension.Z", Be::READWRITE )

		MAP_ATTRIBUTE( "indirectionBuffer", m_indirectionBuffer, "If set, use DispatchIndirect with this buffer holding the arguments", Be::READWRITE )
		MAP_ATTRIBUTE( "offsetForArgs", m_offsetForArgs, "The argument offset in indirectionBuffer for an indirect dispatch.", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			4,
			"Run a compute shader in renderStep.Execute\n"
			"\n:param effect: A Tr2Effect containing a compute shader (default None)"
			"\n:param dimX: unsigned int, dispatch group dimension.X, default 1"
			"\n:param dimY: unsigned int, dispatch group dimension.Y, default 1"
			"\n:param dimZ: unsigned int, dispatch group dimension.Z, default 1" )

	EXPOSURE_CHAINTO( TriRenderStep )
}