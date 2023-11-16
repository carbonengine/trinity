////////////////////////////////////////////////////////////
//
//    Created:   August 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef TriStepSetDebugRenderer_h
#define TriStepSetDebugRenderer_h


#include "TriRenderStep.h"

BLUE_DECLARE_INTERFACE( ITr2DebugRenderer );

// -------------------------------------------------------------
// Description:
//   TriStepSetDebugRenderer is a render step that assigns the 
//   given debug renderer to the global debug renderer variable.
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepSetDebugRenderer ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepSetDebugRenderer( IRoot* lockobj = 0 );
	~TriStepSetDebugRenderer(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void SetDebugRenderer( ITr2DebugRenderer* renderer );
private:
	ITr2DebugRendererPtr m_debugRenderer;
};

TYPEDEF_BLUECLASS( TriStepSetDebugRenderer );

#endif // TriStepSetDebugRenderer_h