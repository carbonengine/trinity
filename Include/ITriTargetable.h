/* 
	*************************************************************************

	ITriAnimation.h

	Created:   October 2005
	OS:        Win32
	Project:   Trinity

	Description:   

		TriAnimation is basically a conceptual grouping of curves and sounds involved in a single animation in a single place. 
		The reasoning behind this is to eliminate the cost of searching content files for animations since the location and the curves involved are static.
		TriAnimation also automates the behaviour that was previously performed in python for restarting curves after a set period of time, as well as offseting their
		start time and shortening the first animation cycle. This allows the TriAnimation to control animations / effects autonomously without using a thread.

	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2005

	*************************************************************************
*/

#ifndef _ITRITARGETABLE_H_
#define _ITRITARGETABLE_H_

BLUE_INTERFACE(ITriTargetable): IRoot
{
	virtual int GetClosestDamageLocatorIndex( const Vector3* position ) = 0;
	virtual bool GetDamageLocatorPosition( Vector3* out, int index ) = 0;
	virtual void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out ) = 0;
	virtual int GetInterestingDamageLocatorIndex( const Vector3 &position ) const = 0;
	virtual int GetGoodDamageLocatorIndex( const Vector3& position ) = 0;
};


#endif