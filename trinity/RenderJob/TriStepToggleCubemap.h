////////////////////////////////////////////////////////////
//
//    Created:   January 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef TriStepToggleCubemap_h
#define TriStepToggleCubemap_h


#include "TriRenderStep.h"

BLUE_DECLARE( Tr2InteriorScene );

BLUE_DECLARE_INTERFACE( ITr2VisualizationModeRenderer );

// -------------------------------------------------------------
// Description:
//   TriStepToggleCubemap is a render step that 
//   either toggles the background cubemap on or off
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepToggleCubemap ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepToggleCubemap( IRoot* lockobj = 0 );
	~TriStepToggleCubemap(void);

	void py__init__( Be::OptionalWithDefaultValue<bool, true> showCubemap, Tr2InteriorScene* scene );

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

private:
	bool m_showCubemap;
	Tr2InteriorScene *m_scene;
};

TYPEDEF_BLUECLASS( TriStepToggleCubemap );

#endif // TriStepToggleCubemap_h