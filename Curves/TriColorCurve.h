/* 
	*************************************************************************************

	TriColorCurve.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRICOLORCURVE_Description



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRICOLORCURVE_H_
#define _TRICOLORCURVE_H_

#define TRICOLORCURVE_Description \
"TriColorCurve is a keframed mathimatical function of time. Where each \r\n\
key defines the interpolation to the next key. TriColorCurve also \r\n\
defines the extrapolation to happen when time is greater than \r\n\
mStart + mLength or less than mStart"

#include "include/ITriColorCurve.h"
#include "include/ITriDuration.h"

#include "TriScalarKey.h"

BLUE_DECLARE_VECTOR( TriColorKey );

class TriColorCurve:
	public ITriColorCurve,
	public ITriDuration,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	TriColorCurve(IRoot* lockobj = NULL);
	~TriColorCurve();

	using ITriColorCurve::Unlock;
	
	std::wstring  mName;
	Be::Time mStart; 
	float    mLength;
	TRIEXTRAPOLATION mExtrapolation;

	Color mValue;
	PTriColorKeyVector mKeys;
	bool mUseHSV;

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

	void UpdateValue( double time ) { Color c; Update( &c, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////
	
	// mStart is used
	Color* Update(
		Color* in,
		Be::Time time
		);

	// mStart has no effect
	Color* Update(
		Color* in,
		double time
		);

	// mStart is used
	Color* GetValueAt(
		Color* in,
		Be::Time time
		);

	// mStart has no effect
	Color* GetValueAt(
		Color* in,
		double time
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorCurve
	/////////////////////////////////////////////////////////////////////////////////////
	void  AddKey(
		float t, 
		const Color& v, 
		const Color& l, 
		const Color& r, 
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
#endif
	double GetCurrentPos( Be::Time time );
};
TYPEDEF_BLUECLASS(TriColorCurve);

#endif