// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2StepExecuteRenderNode.h"
#include "../Tr2RenderTarget.h"
#include "../ITr2RenderNode.h"


BLUE_DEFINE_INTERFACE( ITr2RenderNode );


BLUE_DEFINE( Tr2StepExecuteRenderNode );

const Be::ClassInfo* Tr2StepExecuteRenderNode::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StepExecuteRenderNode, "" )
		MAP_INTERFACE( TriRenderStep )


		MAP_ATTRIBUTE(
			"destinationTarget",
			m_destinationTarget,
			"Destination render target where the final rendered output is to be stored",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"node",
			m_node,
			"Render node to execute",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"clearTargetOnFailure",
			m_clearTargetOnFailure,
			"If enabled, the destination target will be cleared to black if the render node is not valid or fails validation",
			Be::READWRITE )
	EXPOSURE_CHAINTO( TriRenderStep )
}
