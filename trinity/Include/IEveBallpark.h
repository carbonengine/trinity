// Copyright © 2023 CCP ehf.

#ifndef IEveBallpark_H
#define IEveBallpark_H

#include "IEveReferencePoint.h"

// Forward declarations.
// Can't include these in interfaces.
struct Vector3;
struct Vector3d;

BLUE_INTERFACE( IEveBallpark ) :
	IEveReferencePoint
{
	virtual void Delta(
		Vector3 * ref,
		Vector3 * refSmooth,
		Be::Time time ) = 0;

	virtual void DeltaVel(
		Vector3 * vel,
		Be::Time time ) = 0;

	virtual float GetUnitBase() = 0;

	virtual void SetUnitBase(
		float unit ) = 0;
};

#endif