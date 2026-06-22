// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetUpscalingContextID.h"
#include "Tr2Renderer.h"


TriStepSetUpscalingContextID::TriStepSetUpscalingContextID( IRoot* lockobj ) :
	m_upscalingContextID( Tr2UpscalingAL::INVALID_CONTEXT_ID )
{
}

TriStepSetUpscalingContextID::~TriStepSetUpscalingContextID( void )
{
	Tr2Renderer::SetUpscalingContextID( Tr2UpscalingAL::INVALID_CONTEXT_ID );
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepSetUpscalingContextID::py__init__( Be::OptionalWithDefaultValue<uint32_t, Tr2UpscalingAL::INVALID_CONTEXT_ID> upscalingContextID )
{
	m_upscalingContextID = upscalingContextID;
}

TriStepResult TriStepSetUpscalingContextID::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	Tr2Renderer::SetUpscalingContextID( m_upscalingContextID );

	return RS_OK;
}