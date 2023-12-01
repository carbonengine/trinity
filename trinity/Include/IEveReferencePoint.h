#ifndef IEveReferencePoint_H
#define IEveReferencePoint_H

// Forward declaration. 
// Can't include these in interfaces.
struct Vector3d;

BLUE_INTERFACE(IEveReferencePoint) : IRoot
{	
	virtual	Vector3d* GetReferencePoint(Vector3d* out, Be::Time time) = 0;
};

#endif
