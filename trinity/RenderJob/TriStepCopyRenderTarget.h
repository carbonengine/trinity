#pragma once
#ifndef TriStepCopyRenderTarget_H
#define TriStepCopyRenderTarget_H

#include "TriRenderStep.h"

BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( TriViewport );

// --------------------------------------------------------------------------------
// Description:
//   This render step is designed to copy (part of) one renderTarget to another that's
//   exactly the same format and Msaa type.
// SeeAlso:
//   TriRenderStep
// --------------------------------------------------------------------------------
BLUE_CLASS( TriStepCopyRenderTarget ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepCopyRenderTarget( IRoot* lockobj = 0 );
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

private:
#if BLUE_WITH_PYTHON
	static PyObject* PyInitLowLevel( PyObject* self, PyObject* args );
#endif

	Tr2RenderTargetPtr m_destinationRT;
	Tr2RenderTargetPtr m_sourceRT;

	TriTextureResPtr	m_destinationTexture;

	TriViewportPtr m_sourceViewport;
	TriViewportPtr m_destinationViewport;
};

TYPEDEF_BLUECLASS( TriStepCopyRenderTarget );

#endif