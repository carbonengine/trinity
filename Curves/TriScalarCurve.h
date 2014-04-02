/* 
	*************************************************************************************

	TriScalarCurve.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRISCALARCURVE_Description



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRISCALARCURVE_H_
#define _TRISCALARCURVE_H_

#define TRISCALARCURVE_Description \
"TriScalarCurve is a keframed mathimatical function of time. Where each \r\n\
key defines the interpolation to the next key. TriScalarCurve also \r\n\
defines the extrapolation to happen when time is greater than  \r\n\
mStart + mLength or less than mStart"

#include "include/ITriScalarCurve.h"
#include "include/ITriDuration.h"

#include "TriScalarKey.h"

BLUE_DECLARE_VECTOR( TriScalarKey );

class TriScalarCurve:
	public ITriScalarCurve,
	public ITriDuration,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	TriScalarCurve(IRoot* lockobj = NULL);
	~TriScalarCurve();

	using ITriScalarCurve::Unlock;
	
	std::wstring  mName;
	Be::Time mStart; 
	float    mLength;
	TRIEXTRAPOLATION mExtrapolation;

	float  mValue;
	PTriScalarKeyVector mKeys;
	int mCurrKey;

	float m_timeOffset;
	float m_timeScale;

	//Set a new notify target, return old.
	INotify *SetNotify(INotify *newPtr);

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
	// ITriScalarFunction
	/////////////////////////////////////////////////////////////////////////////////////
	
	// mStart is used
	float Update(
		Be::Time time
		);

	// mStart has no effect
	float Update(
		double time
		);

	// mStart is used
	float GetValueAt(
		Be::Time time
		);

	// mStart has no effect
	float GetValueAt(
		double time
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriScalarCurve
	/////////////////////////////////////////////////////////////////////////////////////
	void  AddKey(
		float t, 
		float v, 
		float l,  
		float r, 
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
	void SetStartTime(Be::Time startTime);
	void Reverse();

private:
	void Reset(float val);	

public:
#if BLUE_WITH_PYTHON
	PyObject* PyGetKey ( PyObject* args );
	PyObject* PySetKey ( PyObject* args );
	PyObject* PyRemoveKey ( PyObject* args );
#endif
	double GetCurrentPos( Be::Time time );

private:
	void DoNotify()
	{
		if (mNotify)
			mNotify->OnModified((Be::Var*)this);
	}
	INotify *mNotify; // notify owner upwards on change
};
TYPEDEF_BLUECLASS(TriScalarCurve);

#endif