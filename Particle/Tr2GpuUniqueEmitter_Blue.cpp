////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2GpuUniqueEmitter.h"


BLUE_DEFINE( Tr2GpuUniqueEmitter );

const Be::ClassInfo* Tr2GpuUniqueEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GpuUniqueEmitter, "Emitter with unique per-instance data for GPU particle systems" )
		MAP_INTERFACE( Tr2GpuUniqueEmitter )

        MAP_ATTRIBUTE( 
            "scaledByParent", 
            m_scaledByParent, 
            "Apply parent scaling to particles", 
            Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"attractorPosition", 
			m_attractorPosition, 
			"Attractor force center (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( 
			"attractorStrength", 
			m_params.attractorStrength, 
			"Attractor force coefficient (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
	EXPOSURE_CHAINTO( Tr2GpuSharedEmitter )
}
