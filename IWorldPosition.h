#ifndef IWorldPosition_H
#define IWorldPosition_H

// Forward declaration. 
// Can't include these in interfaces.
struct Vector3;

BLUE_INTERFACE(IWorldPosition) : IRoot
{	
	virtual Vector3 GetWorldPosition() = 0;
	virtual Quaternion GetWorldRotation() = 0;
};

#endif
