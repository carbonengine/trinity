////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2GpuSharedEmitter.h"


BLUE_DEFINE( Tr2GpuSharedEmitter );

const Be::ClassInfo* Tr2GpuSharedEmitter::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2GpuSharedEmitter, "Emitter with shared per-instance data for GPU particle systems" )
        MAP_INTERFACE( Tr2GpuSharedEmitter )
        MAP_INTERFACE( IInitialize )
        MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2GenericEmitter )

        MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

        MAP_ATTRIBUTE( 
            "continuousEmitter", 
            m_continuousEmitter, 
            "Emit particles continuously while in parent emitter list", 
            Be::READWRITE | Be::PERSIST )
		
        MAP_ATTRIBUTE( 
            "rate", 
            m_rate, 
            "Number of particles emitted per second", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
            "emissionDensity", 
            m_emissionDensity, 
            "Number of particles emitted per one meter movement of the emitter", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
            "maxEmissionDensity", 
            m_maxDensity, 
            "Maximum number of particles emitted per one meter movement of the emitter in a single frame", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
            "maxDisplacement", 
            m_maxDisplacement, 
            "Maximum emitter displacement in a single frame that turns the emitter off", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
            "position", 
            m_position, 
            "Emitter position", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
            "direction", 
            m_direction, 
            "Emitter primary direction", 
            Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"angle", 
			m_emitter.angle, 
			"Particle spread angle (in radians) around primary direction; if greater than pi/2, the emitter emits in all directions", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"innerAngle", 
			m_emitter.innerAngle, 
			"Exclude angle (in radians) around primary direction: particles are spawned in a hollow cone between innerAngle and angle", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"radius", 
			m_emitter.radius, 
			"Emitter radius", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"inheritVelocity", 
			m_inheritVelocity, 
			"Coefficient of how much of emitter velocity is transfered to particle velocities", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"minSpeed", 
			m_emitter.minSpeed, 
			"Minimum particle initial speed", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"maxSpeed", 
			m_emitter.maxSpeed, 
			"Maximum particle initial speed", 
			Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( 
			"minLifeTime", 
			m_params.minLifeTime, 
			"Minimum particle life time in seconds (stored on GPU)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"maxLifeTime", 
			m_params.maxLifeTime, 
			"Maximum particle life time in seconds (stored on GPU)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"sizes", 
			m_params.sizes, 
			"Particle sizes at 0, 50%, 100% of its age\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"sizeVariance", 
			m_params.sizeVariance, 
			"Particle size variance (0 to 1)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"color0", 
			m_params.colors[0], 
			"Particle color at 0% age\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"color1", 
			m_params.colors[1], 
			"Particle color at 33% age\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"color2", 
			m_params.colors[2], 
			"Particle color at 66% age\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"color3", 
			m_params.colors[3], 
			"Particle color at 100% age\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"textureIndex", 
			m_params.textureIndex, 
			"Particle atlas texture index (stored on GPU)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"colorMidpoint", 
			m_params.colorMidpoint, 
			"Color curve midpoint from 0 to 1 (stored on GPU)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"velocityStretchRotation", 
			m_params.velocityStretchRotation, 
			"Particle maximum rotation speed or velocity stretch coefficient if negative (stored on GPU)\n"
			":jessica-group: Particle", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"drag", 
			m_params.drag, 
			"Drag force coefficient (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"turbulenceAmplitude", 
			m_params.turbulenceAmplitude, 
			"Turbulence force amplitude (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"turbulenceFrequency", 
			m_params.turbulenceFrequency, 
			"Turbulence force frequency (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
        MAP_ATTRIBUTE( 
			"gravity", 
			m_params.gravity, 
			"Gravity force along Y axis (stored on GPU)\n"
			":jessica-group: Dynamics", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
    EXPOSURE_END()
}
