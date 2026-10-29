// Copyright © 2023 CCP ehf.

#ifndef _ITRIDURATION_H_
#define _ITRIDURATION_H_

#include "ITriConstants.h"

// This interface ties together the properties that are shared by curves, but not all functions
BLUE_INTERFACE( ITriDuration ) :
	public IRoot
{
	virtual Be::Time Start() = 0;
	virtual void SetStartTime( Be::Time startTime ) = 0;
	// Required, because Scaling time will not resort
	virtual void Sort() = 0;

	virtual void ScaleTime(
		float s ) = 0;

	virtual void Reverse() = 0;

	virtual void ScaleValue(
		float s ) = 0;

	virtual float Length() = 0;

	virtual TRIEXTRAPOLATION Extrapolation() = 0;
};

#endif