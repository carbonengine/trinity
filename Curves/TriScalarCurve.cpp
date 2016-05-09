#include "StdAfx.h"
#include "TriScalarCurve.h"


TriScalarCurve::TriScalarCurve(IRoot* lockobj) :
	mStart         ( 0           ), 
	mLength        ( 0.0f        ),
	mExtrapolation ( TRIEXT_NONE ),
	mCurrKey       ( 1           ),
	mValue		   ( 0.0f        ),
	mNotify		   ( 0			 ),
	m_timeOffset( 0.f ),
	m_timeScale( 1.f ),
	PARENTLOCK(mKeys, ITriScalarCurve)
{	
	mKeys.SetNotify(this);
}


TriScalarCurve::~TriScalarCurve()
{
	mNotify = 0;
}

INotify *TriScalarCurve::SetNotify(INotify *newPtr)
{
	INotify *oldPtr = mNotify;
	mNotify = newPtr;
	return oldPtr;
}


/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

void TriScalarCurve::UpdateValue( double time )
{
	Update( time );
}

float TriScalarCurve::Update(
	Be::Time t
	)
{
	return mValue = GetValueAt(t);
}

float TriScalarCurve::Update(
	double t
	)
{
	return mValue = GetValueAt(t);
}


float TriScalarCurve::GetValueAt(
	Be::Time now
	)
{
	return mLength ? GetValueAt(TimeAsDouble(now - mStart)) : mValue;
}


float TriScalarCurve::GetValueAt(
	double pos
	)
{
	if (mLength == 0.0f)
	{// there is no curve
		return mValue;
	}

	pos = pos / (double)m_timeScale - (double)m_timeOffset;

	// NB: IMPORTANT
	// You cannot make the assumption that curves start at 0.0
	// or even that the times are positive values!
	const float startTime = mKeys.front()->mTime;
	const float endTime = mKeys.back()->mTime;

	// just recompute this anyway
	const float length = endTime - startTime;

	if ( pos >= endTime )
	{// the curve is over, pos is after the last key
		if (mExtrapolation == TRIEXT_NONE)
		{
			return mValue;
		}
		else if (mExtrapolation == TRIEXT_CONSTANT)
		{
			return mKeys.back()->mValue;
		}
		else if (mExtrapolation == TRIEXT_GRADIENT)
		{
			TriScalarKey& ck = *mKeys.back();
			return ck.mValue + (float)(pos - ck.mTime) + ck.mRight;
		}
		else
		{// (mExtrapolation == TRIEXT_CYCLE)
			double cycledTime = fmod( pos, (double)length );
			// re-adjust to be within the bounds of the curve
			pos = (double)startTime + cycledTime;
		}
	}
	else if( pos <= startTime )
	{// the curve hasn't begun or pos is before or equal to the first key
		if (mExtrapolation == TRIEXT_NONE)
		{
			return mValue;
		}
		else if (mExtrapolation == TRIEXT_CONSTANT)
		{
			return mKeys[0]->mValue;
		}
		else if (mExtrapolation == TRIEXT_GRADIENT)
		{
			TriScalarKey& ck = *mKeys[0];
			return ck.mValue + (float)(pos * mLength - ck.mTime) + ck.mLeft;
		}
		else 
		{// (mExtrapolation == TRIEXT_CYCLE)
			return mKeys[0]->mValue;
		}
	}
	TriScalarKey* ck = mKeys[mCurrKey];
	TriScalarKey* ck_1 = mKeys[mCurrKey-1];
	while((pos >= ck->mTime) || (pos < ck_1->mTime))
	{		
		if (pos < ck_1->mTime)
		{// "cache miss" need to search form the beginning
			mCurrKey = 0;
		}
		mCurrKey++;
		ck_1 = mKeys[mCurrKey - 1];
		ck = mKeys[mCurrKey];				
	}	
	return Interpolate(ck_1, ck, (float)(pos - ck_1->mTime));
}

/////////////////////////////////////////////////////////////////////////////////////////
// compare function
/////////////////////////////////////////////////////////////////////////////////////////
static bool CompareKeys(IRoot* context, TriScalarKey* a, TriScalarKey* b)
{
	return a->mTime < b->mTime;
}

void TriScalarCurve::AddKey(float t, float v, float l,  float r, TRIINTERPOLATION i)
{	
	TriScalarKeyPtr k;
	
	k.CreateInstance();
	
	k->mTime = t;
	k->mValue = v;
	k->mLeft = l;
	k->mRight = r;
	k->mInterpolation = i;

	mKeys.Insert( -1, k );

	Sort();
}

void TriScalarCurve::Sort(
	)
{
	if (mKeys.GetSize()>1) {
		mKeys.Sort((IList::CompareFn)CompareKeys, NULL);
		mLength = (float) (mKeys.back()->mTime - mKeys[0]->mTime);
	} else
		mLength = 0.0f;
	mCurrKey = 1;
	DoNotify();
}


void TriScalarCurve::ScaleTime(
	float s
	)
{
	if(mKeys.GetSize() >= 2)
	{
		for(int i=0; i< mKeys.GetSize();i++)
		{
			mKeys[i]->mTime *= s;
		}
		mLength = (float) (mKeys.back()->mTime - mKeys[0]->mTime);
	}
	DoNotify();
}

void TriScalarCurve::Reverse(
	)
{
	if(mKeys.GetSize() >= 2)
	{
		for(int i=0; i< mKeys.GetSize();i++)
		{
			mKeys[i]->mTime = mLength - mKeys[i]->mTime;
		}
	}
	Sort();
	DoNotify();
}


void TriScalarCurve::ScaleValue(
	float s
	)
{
	for(int i=0; i< mKeys.GetSize();i++)
	{
		mKeys[i]->mValue *= s;
	}
	DoNotify();
}


float TriScalarCurve::Length(	
	)
{
	return mLength;
}

Be::Time TriScalarCurve::Start(
	)
{
	return mStart;
}

void TriScalarCurve::SetStartTime(
	Be::Time startTime
	)
{
	mStart = startTime;
}

TRIEXTRAPOLATION TriScalarCurve::Extrapolation()
{
	return mExtrapolation;
}

void TriScalarCurve::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* currvalue,
	const IList* theList
	)
{
	//TriScalarKey* skey = (TriScalarKey*)currvalue;

	if (event & BELIST_LOADING)
		return;

	switch(event & BELIST_EVENTMASK)
	{
	case BELIST_REMOVED: {
		// called after removal.  are we removing start or end?
		ssize_t a = 0; ssize_t b = mKeys.GetSize()-1;
		if (key == a)
			a++;
		else if (key == b)
			b--;
		else
			break;
		if (a < b && b < mKeys.GetSize())
			mLength = (float) (mKeys[b]->mTime - mKeys[a]->mTime);
		else
			mLength = 0;
		mCurrKey = 1;
		break; }

	case BELIST_LOADFINISHED:
		Sort();
		break;

	case BELIST_UNLOADSTART:
		mLength = 0.0;
		mCurrKey = 1;
		break;
	}
}


#if BLUE_WITH_PYTHON
PyObject* TriScalarCurve::PyGetKey(PyObject* args)
{	
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;

	// Check the length of the keys list
	if ( index < 0 || index >= (int)mKeys.size())
	{
		PyErr_SetString( PyExc_TypeError, "Index out of range");
		return NULL;
	}

	// Wrap the key and return		
	TriScalarKey* key = new OTriScalarKey;
	TriScalarKey* src = mKeys[index];
	*key = *src;

	PyObject* pyKey = PyOS->WrapBlueObject(key);
	key->Unlock();

	return pyKey;
}

PyObject* TriScalarCurve::PySetKey(PyObject* args)
{	
	int index;
	PyObject* pyKey = NULL;
	if (!PyArg_ParseTuple(args, "iO", &index, &pyKey))
		return NULL;

	// Check the length of the keys list
	if ( index < 0 || index >= (int)mKeys.size())
	{
		PyErr_SetString( PyExc_TypeError, "Index out of range");
		return NULL;
	}

	TriScalarKey* src = BluePythonCast<TriScalarKey*>( pyKey );
	TriScalarKey* t = mKeys[index];
	*t = *src;

	Sort();	

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* TriScalarCurve::PyRemoveKey(PyObject* args)
{	
	int index;
	if (!PyArg_ParseTuple( args, "i", &index ))
		return NULL;

	// Check the length of the keys list
	if ( index < 0 || index >= (int)mKeys.size())
	{
		PyErr_SetString( PyExc_TypeError, "Index out of range");
		return NULL;
	}

	mKeys.Remove(index);	

	// If we removed the start key
	if ( mKeys.size() > 0 && index == 0 )
	{
		mKeys[0]->mTime = 0.0f;
	}
	mCurrKey = 1;
	Sort();


	Py_INCREF(Py_None);
	return Py_None;
}
#endif

double TriScalarCurve::GetCurrentPos( Be::Time time )
{	
	double t = TimeAsDouble( time - mStart );
	t = fmod( t, (double)mLength );
	return t;
}
