////////////////////////////////////////////////////////////
//
//    Created:   February 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef TriStepClearUav_H
#define TriStepClearUav_H

#include "TriRenderStep.h"

BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );

// --------------------------------------------------------------------------------------
// Description:
//   TriStepClearUav render step clears a writable Tr2UavBuffer contents to a specified
//   value.
// See Also:
//   TriRenderStep
// --------------------------------------------------------------------------------------
BLUE_CLASS( TriStepClearUav ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepClearUav( IRoot* lockobj = 0 );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
private:
#if BLUE_WITH_PYTHON
	// Python __init__ constructor
	static PyObject* py__init__( PyObject* self, PyObject* args );
#endif

	// Buffer to clear
	ITr2GpuBufferPtr m_buffer;
	// Sould we clear with floating point values?
	bool m_clearWithFloat;
	// Floating point clear value (used when m_clearWithFloat is true)
	Vector4 m_floatValue;
	// uint32_t clear value
	uint32_t m_uintValue[4];
};

TYPEDEF_BLUECLASS( TriStepClearUav );

#endif