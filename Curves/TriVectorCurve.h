/* 
	*************************************************************************************

	TriVectorCurve.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIVECTORCURVE_Description



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRIVECTORCURVE_H_
#define _TRIVECTORCURVE_H_

#define TRIVECTORCURVE_Description \
"TriVectorCurve is a keframed mathimatical function of time. Where each \r\n\
key defines the interpolation to the next key. TriVectorCurve also \r\n\
defines the extrapolation to happen when time is greater than \r\n\
mStart + mLength or less than mStart"

#include "include/ITriVectorCurve.h"
#include "include/ITriDuration.h"

#include "TriScalarKey.h"

BLUE_DECLARE_VECTOR( TriVectorKey );

class TriVectorCurve:
	public ITriVectorCurve,
	public ITriDuration,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	TriVectorCurve(IRoot* lockobj = NULL);
	~TriVectorCurve();

	using ITriVectorCurve::Unlock;
	
	std::wstring  mName;
	Be::Time mStart; 
	float    mLength;
	TRIEXTRAPOLATION mExtrapolation;

	Vector3 mValue;
	PTriVectorKeyVector mKeys;

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

	void UpdateValue( double time );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////
		
	Vector3* Update(
		Vector3* in,
		Be::Time time
		);

	Vector3* Update(
		Vector3* in,
		double time
		);
	
	Vector3* GetValueAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		double time
		);
	Vector3d* InterpolatedPosition(Vector3d* out, Be::Time) {return out;};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorCurve
	/////////////////////////////////////////////////////////////////////////////////////
	void  AddKey(
		float t, 
		const Vector3& v, 
		const Vector3& l,
		const Vector3& r,
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
	void Reset(float val);
	int mCurrKey;

public:
#if BLUE_WITH_PYTHON
	PyObject* PyAddKey ( PyObject* args );
	PyObject* PyGetKey ( PyObject* args );
	PyObject* PySetKey ( PyObject* args );
	PyObject* PyRemoveKey ( PyObject* args );
	double GetCurrentPos( Be::Time time );
	PyObject* PyCheckProximity ( PyObject* args );
	PyObject* PyCheckProximityToPoint ( PyObject* args );
#endif
};
TYPEDEF_BLUECLASS(TriVectorCurve);

#endif