#include "StdAfx.h"
#include "TriStepRunComputeShader.h"
#include "Shader/Tr2Material.h"
#include "include/ITr2GpuBuffer.h"
#include "Tr2Renderer.h"


#include "Tr2ConstantBufferFormats.h"	// hack

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( rsRunComputeShaderCount, "Trinity/RenderStep/RunComputeShaderCount", true, CST_COUNTER_LOW, "Calls to TriStepRunComputeShader::Execute per frame" );

TriStepRunComputeShader::TriStepRunComputeShader( IRoot* lockobj )
	: m_groupDimX( 1 )
	, m_groupDimY( 1 )
	, m_groupDimZ( 1 )
	, m_offsetForArgs( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void TriStepRunComputeShader::py__init__( 
	Tr2Material* effect,
	Be::OptionalWithDefaultValue<unsigned, 1> groupDimX,
	Be::OptionalWithDefaultValue<unsigned, 1> groupDimY,
	Be::OptionalWithDefaultValue<unsigned, 1> groupDimZ )
{
	m_effect = effect;
	m_groupDimX = groupDimX;
	m_groupDimY = groupDimY;
	m_groupDimZ = groupDimZ;
}

TriStepResult TriStepRunComputeShader::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_STATS_INC( rsRunComputeShaderCount );

	if( m_indirectionBuffer )
	{
		auto buffer = m_indirectionBuffer->GetGpuBuffer( 0 );
		if( buffer )
		{
			Tr2Renderer::RunComputeShaderIndirect( m_effect, *buffer, m_offsetForArgs, renderContext );
		}
	}
	else
	{
		Tr2Renderer::RunComputeShader( m_effect, m_groupDimX, m_groupDimY, m_groupDimZ, renderContext );
	}

	return RS_OK;
}
