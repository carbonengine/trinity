// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2ValueBinding_H
#define ITr2ValueBinding_H

struct TriCurveSetTime;

BLUE_INTERFACE( ITr2ValueBinding ) :
	public IRoot
{
	virtual void CopyValue() = 0;

	// Copies the value for a curve set applying its bindings at the given time.
	// Bindings that don't depend on that time copy their source as it stands.
	virtual void CopyValueAt( const TriCurveSetTime& )
	{
		CopyValue();
	}
};

#endif