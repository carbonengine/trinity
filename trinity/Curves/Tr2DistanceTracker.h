#pragma once
#ifndef Tr2DistanceTracker_h
#define Tr2DistanceTracker_h

#include "include/ITriFunction.h"

// Forward declarations
BLUE_DECLARE( Tr2DistanceTracker );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DistanceTracker is a 
// --------------------------------------------------------------------------------------
class Tr2DistanceTracker :
     public ITriFunction,
	 public INotify
{
public:
	// Constructor
	Tr2DistanceTracker( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	void UpdateValue( double time );
	bool OnModified( Be::Var* val );
private:
	std::wstring m_name;

	// Determine weather distance is signed based on direction vector
    bool m_signedDistance;

    // Track distance to closest point between source and target
    bool m_distanceToClosest;

    float m_value;

    // Normalized direction vector
    Vector3 m_direction;

    // Source and destination objects
    ITriVectorFunctionPtr m_target;
    ITriVectorFunctionPtr m_source;

    // Source and destination positions
    Vector3 m_targetPosition;
    Vector3 m_sourcePosition;
};

TYPEDEF_BLUECLASS( Tr2DistanceTracker );

#endif