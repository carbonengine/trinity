#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorLightSet.h"
#include "include/ITr2Interior.h"

// --------------------------------------------------------------------------------------
// Description:
//   Default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorLightSet::Tr2InteriorLightSet() :
	m_lightInstances(),
	m_isSorted( true )
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
	// Ignore non-primary lights & lights with marginal intensity
	if( lightSource->UseWithPrimaryLighting() )
	{
		// Setup the light instance
		InternalLightInstance instance =
		{
			Matrix( 1.0f, 0.0f, 0.0f, 0.0f, 
					0.0f, 1.0f, 0.0f, 0.0f, 
					0.0f, 0.0f, 1.0f, 0.0f, 
					0.0f, 0.0f, 0.0f, 1.0f ),
			lightSource,
			0.0f,
			false
		};

		// Determine the importance
		ComputeLightImportance( instance, viewPosition );

		// Insert the light instance into the list
		m_lightInstances.push_back( instance );

		// Flag the list as unsorted
		m_isSorted = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds an instanced light source and computes the view importance
// Arguments:
//   lightSource        - The light source to add
//   viewPosition       - The current view position, used to determine view importance
//   mirrorToWorld		- The mirror to world space transform
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::AddInstancedLight( ITr2InteriorLight* lightSource, 
											 const Vector3& viewPosition,
								             const Matrix& mirrorToWorld )
{
	// Ignore non-primary lights & lights with marginal intensity
	if( lightSource->UseWithPrimaryLighting() )
	{
		// Setup the light instance
		InternalLightInstance instance = 
		{
			mirrorToWorld,
			lightSource,
			0.0f,
			false
		};

		// Determine the importance
		ComputeLightImportance( instance, viewPosition );

		// Insert the light instance into the list
		m_lightInstances.push_back( instance );

		// Flag the list as unsorted
		m_isSorted = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a light from the light set
// Arguments:
//   lightSource - The light to remove
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::RemoveLight( ITr2InteriorLight* lightSource )
{
	// This would be more awesome with lambdas.
	m_lightInstances.remove_if( LightPointerComp( lightSource ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Clears the light set
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::Clear( void )
{
	m_lightInstances.clear();
	m_isSorted = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Populates per-object data with the most important lights in the light set
// Arguments:
//   perObjectPSData - The per-object data to populate
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::PopulateLightData( Tr2InteriorPerObjectPSData* perObjectPSData )
{
	// Sort the list, if needed
	if( !m_isSorted )
	{
		m_lightInstances.sort();
		m_isSorted = true;
	}

	// set each pointlight data in target array
	unsigned int i = 0;
	std::list<InternalLightInstance>::const_iterator it = m_lightInstances.begin();
	while( (i < MAX_INTERIOR_LIGHTS_PER_OBJECT) && (it != m_lightInstances.end()) )
	{
		// Put standard lightsource data in target
		if( !it->lightDataValid )
		{
			it->lightSource->PopulateLightData( &it->lightData, it->mirrorToWorldMatrix );
			it->lightDataValid = true;
		}
		memcpy( &perObjectPSData->pointLights[i], &it->lightData, sizeof( it->lightData ) );

		// Increment counter & iterator
		++i;
		++it;
	}
	perObjectPSData->shadowCaster0 = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
	perObjectPSData->shadowCaster1 = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the world-space position of all the lights in the set for "light spider" tool
// Arguments:
//   positions - A vector of float-3 light positions, filled by the function
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::GetLightPositions( std::vector<Vector3>& positions ) const
{
	positions.clear();

	for( std::list<InternalLightInstance>::const_iterator it = m_lightInstances.begin(); 
		 it != m_lightInstances.end(); ++it )
	{
		Matrix worldToMirror;
		D3DXMatrixTranspose( &worldToMirror, &it->mirrorToWorldMatrix );
		D3DXMatrixInverse( &worldToMirror, NULL, &worldToMirror );
		positions.push_back( it->lightSource->GetPositionForLightSpider( worldToMirror ) );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Computes the light importance for a light
// Arguments:
//   instance     - The light instance
//   viewPosition - The current view position, for determining view importance
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSet::ComputeLightImportance( InternalLightInstance& instance, 
												  const Vector3& viewPosition )
{
	Matrix mirrorMat;
	D3DXMatrixTranspose( &mirrorMat, &instance.mirrorToWorldMatrix );

	Vector3 position;
	D3DXVec3TransformCoord( &position, &viewPosition, &mirrorMat );
	instance.importance = instance.lightSource->GetCurrentViewImportance( position );
}

#endif
