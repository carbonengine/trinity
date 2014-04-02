#pragma once
#ifndef Tr2InteriorLightSet_H
#define Tr2InteriorLightSet_H

#include "Tr2InteriorConstantBufferFormats.h"

BLUE_DECLARE_INTERFACE( ITr2InteriorLight );
struct Tr2InteriorPerObjectPSData;

//---------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSet is a collection of ITr2InteriorLights accumulated during 
//   scene traversal in Umbra.  It can handle ordinary lights as well as light instances.  
//   A light instance is produced when Umbra traverses through a virtual portal.  This
//   changes the world position and cone direction and this information is instanced for 
//   the affected lights.
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
	// Add light (instanced)
	void AddInstancedLight( ITr2InteriorLight* lightSource, const Vector3& viewPosition,
							const Matrix& objectToWorldMatrix );
	// Remove all instances of this light source
	void RemoveLight( ITr2InteriorLight* lightSource );
	// Clear all light sources
	void Clear( void );

	// How many light instances are in this set?
	unsigned int GetNumOfActiveLights( void ) const { return (unsigned int)m_lightInstances.size(); }

	// Populate constant buffer
	void PopulateLightData( Tr2InteriorPerObjectPSData* perObjectPSData );

	// Retrieve light positions (used for debug)
	void GetLightPositions( std::vector<Vector3>& positions ) const;

private:
	// Internal helper structure for managing light instances
	struct InternalLightInstance
	{
		// Mirror to world matrix
		Matrix mirrorToWorldMatrix;
		// Pointer to the underlying light source
		ITr2InteriorLight* lightSource;
		// Light importance, based on view position
		float importance;
		// Is lightData valid
		mutable bool lightDataValid;
		// Cached light data
		mutable Tr2InteriorPerObjectLightData lightData;

		// Sort descending by importance
		bool operator<( const InternalLightInstance& other )
		{
			return importance > other.importance;
		}

		// Comparison based on pointer equality
		bool operator==( const InternalLightInstance& other )
		{
			return lightSource == other.lightSource;
		}
	};

	// Comparison functor for light instance removal (would be more awesome as a lambda)
	class LightPointerComp
	{
	public:
		// Constructor
		LightPointerComp( ITr2InteriorLight* lightSource ) : m_lightSource( lightSource ) {}

		// Function call operator
		bool operator()( const InternalLightInstance& other )
		{
			return m_lightSource == other.lightSource;
		}

	private:
		// Pointer to the light source
		ITr2InteriorLight* m_lightSource;
	};

	// Helper function to compute light importance, based on camera position
	void ComputeLightImportance( InternalLightInstance& instance, const Vector3& viewPosition );

private:
	// List of light instances
	std::list<InternalLightInstance> m_lightInstances;
	// Are the instances sorted by importance?
	bool m_isSorted;
};

#endif // Tr2InteriorLightSet_H