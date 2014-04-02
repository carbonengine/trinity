/* 
	*************************************************************************************

	TriRotationCurve.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIROTATIONCURVE_Description



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRIROTATIONCURVE_H_
#define _TRIROTATIONCURVE_H_

#define TRIROTATIONCURVE_Description \
"TriRotationCurve is a keframed mathimatical function of time. Where each \r\n\
key defines the interpolation to the next key. TriRotationCurve also \r\n\
defines the extrapolation to happen when time is greater than \r\n\
mStart + mLength or less than mStart"

#include "include/ITriRotationCurve.h"
#include "include/ITriDuration.h"

#include "TriScalarKey.h"

BLUE_DECLARE_VECTOR( TriQuaternionKey );

class TriRotationCurve:
	public ITriRotationCurve,
	public ITriDuration,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	TriRotationCurve(IRoot* lockobj = NULL);
	~TriRotationCurve();

	using ITriRotationCurve::Unlock;
	
	std::wstring  mName;
	Be::Time mStart; 
	float    mLength;
	TRIEXTRAPOLATION mExtrapolation;

	Quaternion mValue;
	PTriQuaternionKeyVector mKeys;

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	/////////////////////////////////////////////////////////////////////////////////////
	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriFunction
	/////////////////////////////////////////////////////////////////////////////////////

	void UpdateValue( double time ) { Quaternion q; Update( &q, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriQuaternionFunction
	/////////////////////////////////////////////////////////////////////////////////////
	
	// mStart is used
	Quaternion* Update(
		Quaternion* in,
		Be::Time time
		);

	// mStart is used
	Quaternion* Update(
		Quaternion* in,
		double time
		);

	// mStart is used
	Quaternion* GetValueAt(
		Quaternion* in,
		Be::Time time
		);

	// mStart has no effect
	Quaternion* GetValueAt(
		Quaternion* in,
		double time
		);

	// mStart is used
	Quaternion* GetValueDotAt(
		Quaternion* in,
		Be::Time time
		);

	// mStart has no effect
	Quaternion* GetValueDotAt(
		Quaternion* in,
		double time
		);

	// mStart is used
	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		Be::Time time
		);

	// mStart has no effect
	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		double time
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriRotationCurve & ITriDuration (except AddKey)
	/////////////////////////////////////////////////////////////////////////////////////
	void  AddKey(
		float t, 
		const Quaternion& v, 
		const Quaternion& l,
		const Quaternion& r,
		TRIINTERPOLATION i
		);

	void Sort(
		);

	void ScaleTime(
		float s
		);

	void ScaleValue(
		float s
		);

	float Length(
		);
	/////////////////////////////////////////////////////////////////////////////////////
	//  ITriDuration
	/////////////////////////////////////////////////////////////////////////////////////

	Be::Time Start();
	
	TRIEXTRAPOLATION Extrapolation();
	
	void SetStartTime(
		Be::Time startTime
		);
	
	void Reverse();


private:
	void Reset(
		float val
		);

	// places mCk_1 and mCk at the correct places
	// returns the noramized time
	float Seek(
		double pos
		);

	TriQuaternionKey* mCk_1;
	TriQuaternionKey* mCk;
	int mCurrKey;

public:
#if BLUE_WITH_PYTHON
	PyObject* PyAddKey ( PyObject* args );
	PyObject* PyGetKey ( PyObject* args );
	PyObject* PySetKey ( PyObject* args );
	PyObject* PyRemoveKey ( PyObject* args );
#endif
	double GetCurrentPos( Be::Time time );
};
TYPEDEF_BLUECLASS(TriRotationCurve);

#endif