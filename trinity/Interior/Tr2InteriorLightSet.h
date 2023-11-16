#pragma once
#ifndef Tr2InteriorLightSet_H
#define Tr2InteriorLightSet_H

#include "Tr2InteriorConstantBufferFormats.h"

BLUE_DECLARE_INTERFACE( ITr2InteriorLight );
struct Tr2InteriorPerObjectPSData;

//---------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSet is a collection of ITr2InteriorLights accumulated during 
//   scene traversal.  It can handle ordinary lights as well as light instances.  
// See Also:
//   Tr2InteriorScene, ITr2InteriorLight
//---------------------------------------------------------------------------------------
class Tr2InteriorLightSet
{
public:
	// Constructor
	Tr2InteriorLightSet();
	// Destructor
	~Tr2InteriorLightSet();

	// Add light (non-instanced)
	void AddLight( ITr2InteriorLight* lightSource, const Vector3& viewPosition );
	// Clear all light sources
	void Clear( void );

	// How many light instances are in this set?
	unsigned int GetNumOfActiveLights( void ) const { return (unsigned int)m_lightInstances.size(); }

	// Populate constant buffer
	void PopulateLightData( Tr2InteriorPerObjectPSData* perObjectPSData );

private:
	// Internal helper structure for managing light instances
	struct InternalLightInstance
	{
		// Pointer to the underlying light source
		ITr2InteriorLight* lightSource;
		// Is lightData valid
		mutable bool lightDataValid;
		// Cached light data
		mutable Tr2InteriorPerObjectLightData lightData;
	};

private:
	// List of light instances
	std::list<InternalLightInstance> m_lightInstances;
};

#endif // Tr2InteriorLightSet_H