////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "TriStepRenderPass.h"


TriStepRenderPass::TriStepRenderPass( IRoot* lockobj )
:	m_pass( ITr2MultiPassScene::RP_BEGIN_RENDER )
{
}

TriStepRenderPass::~TriStepRenderPass(void)
{
}

TriStepResult TriStepRenderPass::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_scene )
	{
		ITr2MultiPassScene::RenderPassResult result = m_scene->RenderPass( m_pass, renderContext );
		if( result == ITr2MultiPassScene::PASS_RESULT_TERMINATE )
		{
			return RS_TERMINATE;
		}
	}
	return RS_OK;
}

void TriStepRenderPass::py__init__( ITr2MultiPassScene* scene, int passType )
{
	m_scene = scene;
	m_pass = ITr2MultiPassScene::PassType(passType);
}
