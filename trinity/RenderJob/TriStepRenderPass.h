////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef TriStepRenderPass_H
#define TriStepRenderPass_H


#include "TriRenderStep.h"
#include "include/ITr2MultiPassScene.h"

// -------------------------------------------------------------
// Description:
//   A render step to render multi-pass scene. Takes the scene and
//   a pass as parameters.
// SeeAlso:
//   TriRenderStep, ITr2MultiPassScene
// -------------------------------------------------------------
BLUE_CLASS( TriStepRenderPass ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepRenderPass( IRoot* lockobj = 0 );
	~TriStepRenderPass(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void py__init__( ITr2MultiPassScene* scene, int passType );
private:
	ITr2MultiPassScene::PassType m_pass;
	ITr2MultiPassScenePtr m_scene;
};

TYPEDEF_BLUECLASS( TriStepRenderPass );

#endif // TriStepRenderPass_H