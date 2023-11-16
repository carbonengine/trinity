#include "StdAfx.h"
#include "TriStepPresentSwapChain.h"

BLUE_DEFINE( TriStepPresentSwapChain );

const Be::ClassInfo* TriStepPresentSwapChain::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriStepPresentSwapChain, "" )
        MAP_INTERFACE( TriStepPresentSwapChain )
		MAP_INTERFACE( TriRenderStep )

		MAP_ATTRIBUTE
		( 
			"swapChain", 
			m_swapChain, 
			"The swap chain that the step should set", 
			Be::READWRITE 
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			1,
			"Create a step that will set a swap chain as active on the device.\n"
			":param swapChain: swap chain"
		)

    EXPOSURE_CHAINTO( TriRenderStep )
}
