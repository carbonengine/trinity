// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPCLEAR_H_
#define _TRISTEPCLEAR_H_


#include "TriRenderStep.h"

BLUE_CLASS( TriStepClear ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepClear( IRoot* lockobj = 0 );
	~TriStepClear( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

#if BLUE_WITH_PYTHON
	// Python __init__ constructor
	static PyObject* py__init__( PyObject * self, PyObject * args );
#endif

	Color m_color;
	float m_depth;
	uint32_t m_stencil;

	bool m_isColorCleared;
	bool m_isDepthCleared;
	bool m_isStencilCleared;
};

TYPEDEF_BLUECLASS( TriStepClear );

#endif