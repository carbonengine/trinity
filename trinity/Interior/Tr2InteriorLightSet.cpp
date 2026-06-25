// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2InteriorLightSet.h"
#include "include/ITr2Interior.h"
#include "Tr2InteriorLightSource.h"

// --------------------------------------------------------------------------------------
// Description:
//   Default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorLightSet::Tr2InteriorLightSet() :
	m_lightInstances()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Default destructor
// --------------------------------------------------------------------------------------
Tr2InteriorLightSet::~Tr2InteriorLightSet()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a non-instanced light source and computes the view importance
// Arguments:
//   lightSource  - The light source to add
//   viewPosition - The view position, used to determine view importance
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::AddLight( ITr2InteriorLight* lightSource,
									const Vector3& viewPosition )
{
	// Setup the light instance
	InternalLightInstance instance = {
		lightSource,
		false
	};

	// Insert the light instance into the list
	m_lightInstances.push_back( instance );
}

// --------------------------------------------------------------------------------------
// Description:
//   Clears the light set
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::Clear( void )
{
	m_lightInstances.clear();
}

// --------------------------------------------------------------------------------------
// Description:
//   Populates per-object data with the most important lights in the light set
// Arguments:
//   perObjectPSData - The per-object data to populate
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::PopulateLightData( Tr2InteriorPerObjectPSData* perObjectPSData )
{
	perObjectPSData->lightCount = GetNumOfActiveLights();

	// set each pointlight data in target array
	unsigned int i = 0;
	std::list<InternalLightInstance>::const_iterator it = m_lightInstances.begin();
	while( ( i < MAX_INTERIOR_LIGHTS_PER_OBJECT ) && ( it != m_lightInstances.end() ) )
	{
		if( i < 4 )
		{
			perObjectPSData->spotLights[i] = dynamic_cast<Tr2InteriorLightSource*>( it->lightSource )->m_viewProjection;
		}

		// Put standard lightsource data in target
		if( !it->lightDataValid )
		{
			it->lightSource->PopulateLightData( &it->lightData );
			it->lightDataValid = true;
		}
		memcpy( &perObjectPSData->pointLights[i], &it->lightData, sizeof( it->lightData ) );

		// Increment counter & iterator
		++i;
		++it;
	}
}
