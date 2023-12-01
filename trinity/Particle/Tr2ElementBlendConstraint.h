////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2ElementBlendConstraint_H
#define Tr2ElementBlendConstraint_H

#include "ITr2GenericParticleConstraint.h"
#include "Tr2ParticleElementDeclaration.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ElementBlendConstraint is a constraint that blends particle element value with 
//   the provided constant value. Can be used to symulate "global" particle systems by 
//   shifting particle positions.
// See Also:
//   Tr2ParticleSystem, ITr2GenericParticleConstraint
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ElementBlendConstraint ):
	public ITr2GenericParticleConstraint
{
public:
	Tr2ElementBlendConstraint( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////
	// ITr2ParticleConstraint
	virtual void ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particle, unsigned* strides, unsigned count, float dt );
	virtual void Bind( Tr2ParticleSystem* system );
private:
	// Is valid flag (for debugging)
	bool m_isValid;

	// Particle element type/name
	Tr2ParticleElementDeclarationName m_name;

	// Bound particle element data
	Tr2ParticleElementData m_element;

	// Constant value to be blended with
	Vector4 m_value;

	// Factor for original element value
	float m_originalFactor;
};

TYPEDEF_BLUECLASS( Tr2ElementBlendConstraint );

#endif // Tr2ElementBlendConstraint_H