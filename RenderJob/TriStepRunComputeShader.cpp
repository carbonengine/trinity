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
	, m_logDispatchTime( false )
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

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	static ID3D11Query	*timingQuery = nullptr;
	if( m_logDispatchTime && !timingQuery )
	{
		D3D11_QUERY_DESC pQueryDesc;
		pQueryDesc.Query = D3D11_QUERY_EVENT;
		pQueryDesc.MiscFlags = 0;
		renderContext.m_secondaryDevice11->CreateQuery( &pQueryDesc, &timingQuery );
	}

	if( m_logDispatchTime )
	{
		renderContext.m_context->End( timingQuery );
		while( renderContext.m_context->GetData( timingQuery, NULL, 0, 0 ) == S_FALSE ) 
		{
		}
	}
#endif
	Be::Time start = BeOS->GetActualTime();

	if( m_indirectionBuffer )
	{
		Tr2GpuBufferAL* buffer = m_indirectionBuffer->GetGpuBuffer( 0 );
		if( buffer )
		{
			Tr2Renderer::RunComputeShaderIndirect( m_effect, *buffer, m_offsetForArgs, renderContext );
		}
	}
	else
	{
		Tr2Renderer::RunComputeShader( m_effect, m_groupDimX, m_groupDimY, m_groupDimZ, renderContext );
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	if( m_logDispatchTime )
	{
		renderContext.m_context->End( timingQuery );
		while( renderContext.m_context->GetData( timingQuery, NULL, 0, 0 ) == S_FALSE ) 
		{
		}
		Be::Time stop = BeOS->GetActualTime();

		CCP_LOG( "Dispatch call took %f sec", TimeAsFloat(stop-start) );
	}
#endif

	return RS_OK;
}
