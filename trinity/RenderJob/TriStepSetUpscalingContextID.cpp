#include "StdAfx.h"
#include "TriStepSetUpscalingContextID.h"
#include "Tr2Renderer.h"


TriStepSetUpscalingContextID::TriStepSetUpscalingContextID( IRoot* lockobj ) :
	m_upscalingContextID( 0 )
{
}

TriStepSetUpscalingContextID::~TriStepSetUpscalingContextID( void )
{
	Tr2Renderer::SetUpscalingContextID( 0 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepSetUpscalingContextID::py__init__( uint32_t upscalingContextID )
{
	m_upscalingContextID = upscalingContextID;
}

TriStepResult TriStepSetUpscalingContextID::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	Tr2Renderer::SetUpscalingContextID( m_upscalingContextID );

	return RS_OK;
}