#include "StdAfx.h"
#include "TriVectorCurve.h"
#include "include/TriVector.h"
#include "Vector3d.h"
#include "Include/TriMath.h"

TriVectorCurve::TriVectorCurve(IRoot* lockobj) :
	mStart         ( 0           ), 
	mLength        ( 0.0f        ),
	mExtrapolation ( TRIEXT_NONE ),
	mCurrKey       ( 1           ),
	mValue( 0.0f, 0.0f, 0.0f ),
	PARENTLOCK(mKeys, ITriVectorCurve)
{	
	mKeys.SetNotify(this);
}


TriVectorCurve::~TriVectorCurve()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

void TriVectorCurve::UpdateValue( double time )
{
	Vector3 v;
	Update( &v, time );
}

Vector3* TriVectorCurve::Update(
	Vector3* in,
	Be::Time t
	)
{	
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}

Vector3* TriVectorCurve::Update(
	Vector3* in,
	double t
	)
{
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}


Vector3* TriVectorCurve::GetValueAt(
	Vector3* in,
	Be::Time now
	)
{
	GetValueAt(in, TimeAsDouble(now - mStart));
	return in;
}

	

Vector3* TriVectorCurve::GetValueAt(
	Vector3* in,
	double pos
	)
{
	if (mLength == 0.0f)
	{// there is no curve
		*in = mValue;
		return in;
	}

	// No gradient for Quaternions
	if ( (pos >= mKeys.back()->mTime) )
	{// the curve is over, pos is after the last key
		if (mExtrapolation == TRIEXT_NONE)
		{
			*in = mValue;
			return in;
		}
		else if (mExtrapolation == TRIEXT_CONSTANT)
		{
			*in = mKeys.back()->mValue;
			return in;
		}
			else if (mExtrapolation == TRIEXT_GRADIENT)
			{
				TriVectorKey& ck = *mKeys.back();
				*in = ck.mValue + (float)(pos - ck.mTime) * ck.mRight;
				return in;
			}
		else
		{// (mExtrapolation == TRIEXT_CYCLE)
			pos = fmod(pos, (double)mKeys.back()->mTime);
		}
	}
	else if ( (pos < 0.0) || (pos <= (mKeys[0]->mTime)) )
	{// the curve hasn't begun or pos is before or equal to the first key
		if (mExtrapolation == TRIEXT_NONE)
		{
			*in = mValue;
			return in;
		}
		else if (mExtrapolation == TRIEXT_CONSTANT)
		{
			*in = mKeys[0]->mValue;
			return in;
		}
		else if (mExtrapolation == TRIEXT_GRADIENT)
		{
			TriVectorKey& ck = *mKeys[0];
			*in = ck.mValue + (float)(pos * mLength - ck.mTime) * ck.mLeft;
			return in;
		}
		else 
		{// (mExtrapolation == TRIEXT_CYCLE)
			*in = mKeys[0]->mValue;
			return in;
		}
	}
	TriVectorKey* ck = mKeys[mCurrKey];
	TriVectorKey* ck_1 = mKeys[mCurrKey - 1];	
	while((pos >= ck->mTime) || (pos < ck_1->mTime))
	{		
		if (pos < ck_1->mTime)
		{// "cache miss" need to search form the begining
			mCurrKey = 0;
		}		
		mCurrKey++;
		ck_1 = mKeys[mCurrKey - 1];
		ck = mKeys[mCurrKey];				
	}	

	// Find the normalized time
	float nt = (float)(pos - ck_1->mTime) / float(ck->mTime - ck_1->mTime);
	if (ck_1->mInterpolation == TRIINT_CONSTANT)
	{
		*in = ck_1->mValue;
	}
    else if( ck_1->mInterpolation == TRIINT_LINEAR)
	{
        D3DXVec3Lerp(in, &ck_1->mValue, &ck->mValue, nt);
	}
	else if( ck_1->mInterpolation == TRIINT_HERMITE)
	{
		D3DXVec3Hermite(in, &ck_1->mValue, &ck_1->mRight, &ck->mValue, &ck->mLeft, nt);
	}
	return in;
}
/*
Vector3* TriVectorCurve::GetValueDotAt(
	Vector3* in,
	Be::Time now
	)
{
	return in;
}


Vector3* TriVectorCurve::GetValueDotAt(
	Vector3* in,
	double pos
	)
{
	return in;
}
*/
Vector3* TriVectorCurve::GetValueDoubleDotAt(
	Vector3* in,
	Be::Time now
	)
{
	return in;
}


Vector3* TriVectorCurve::GetValueDoubleDotAt(
	Vector3* in,
	double pos
	)
{
	return in;
}


Vector3* TriVectorCurve::GetValueDotAt(
	Vector3* in,
	Be::Time now
	)
{
	if(mLength)
	{
		GetValueDotAt(in, TimeAsDouble(now - mStart));
	}
	else
	{
		in->x = 0.0f;
		in->y = 0.0f;
		in->z = 0.0f;
	}
	return in;
}

Vector3* TriVectorCurve::GetValueDotAt(
	Vector3* in,
	double pos
	)
{
	if (mLength == 0.0f)
	{// there is no curve
		in->x = 0.0f;
		in->y = 0.0f;
		in->z = 0.0f;
		return in;
	}

	if ( (pos >= mKeys.back()->mTime) || (pos < 0.0) || (pos <= (mKeys[0]->mTime)) )		
	{// the curve is over, pos is after the last key
	//the curve hasn't begun or pos is before or equal to the first key
		if ((mExtrapolation == TRIEXT_NONE) ||  (mExtrapolation == TRIEXT_CONSTANT))		
		{
			in->x = 0.0f;
			in->y = 0.0f;
			in->z = 0.0f;
			return in;
		}
		else if (mExtrapolation == TRIEXT_GRADIENT)
		{
			if (pos >= mKeys.back()->mTime)
			{
				TriVectorKey& ck = *mKeys.back();
				//need to scale?
				*in = ck.mRight;
				return in;
			}
			else
			{
				TriVectorKey& ck = *mKeys.front();
				//need to scale?
				*in = ck.mLeft;
				return in;
			}
		}
		else 
		{// (mExtrapolation == TRIEXT_CYCLE)
			pos = fmod(pos, (double)mKeys.back()->mTime);
		}
	}	

	// Seek and find the normalized time
	TriVectorKey* ck = mKeys[mCurrKey];
	TriVectorKey* ck_1 = mKeys[mCurrKey - 1];	
	while((pos >= ck->mTime) || (pos < ck_1->mTime))
	{		
		if (pos < ck_1->mTime)
		{// "cache miss" need to search form the begining
			mCurrKey = 0;
		}		
		mCurrKey++;
		ck_1 = mKeys[mCurrKey - 1];
		ck = mKeys[mCurrKey];				
	}	
	// Find the normalized time
	float nt = (float)(pos - ck_1->mTime) / float(ck->mTime - ck_1->mTime);
	if (ck_1->mInterpolation == TRIINT_CONSTANT)
	{
		in->x = 0.0f;
		in->y = 0.0f;
		in->z = 0.0f;
		return in;
	}
    else if( ck_1->mInterpolation == TRIINT_LINEAR)
	{
        TriVectorLerpDot(in, &ck_1->mValue, &ck->mValue, nt);
	}
	else if( ck_1->mInterpolation == TRIINT_HERMITE)
	{
		TriVectorHermiteDot(in, &ck_1->mValue, &ck_1->mRight, &ck->mValue, &ck->mLeft, nt);
	}
	float dt = ck->mTime - ck_1->mTime;
	if (dt < 0.0000001f)
		dt = 0.0000001f;
	D3DXVec3Scale(in, in , 1.0f/dt);
	return in;
}


/////////////////////////////////////////////////////////////////////////////////////////
// compare function
/////////////////////////////////////////////////////////////////////////////////////////
static bool CompareKeys(IRoot* context, TriQuaternionKey* a, TriQuaternionKey* b)
{
	return a->mTime < b->mTime;
}

void TriVectorCurve::AddKey(
	float t,
	const Vector3& v, 
	const Vector3& l,
	const Vector3& r,
	TRIINTERPOLATION i
	)
{	
	TriVectorKeyPtr k;
	k.CreateInstance();

	k->mTime = t;
	k->mValue = v;
	k->mLeft = l;
	k->mRight = r;
	k->mInterpolation = i;

	mKeys.Insert( -1, k );
	Sort();
}

void TriVectorCurve::Sort(
	)
{
	if( mKeys.empty() )
	{
		mLength = 0.0f;
	}
	else
	{
		mKeys.Sort((IList::CompareFn)CompareKeys, NULL);
	    mLength = (float) (mKeys.back()->mTime - mKeys[0]->mTime);
	}
}


void TriVectorCurve::ScaleTime(
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
}

void TriVectorCurve::Reverse(
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
}

void TriVectorCurve::ScaleValue(
	float s
	)
{
	for(int i=0; i< mKeys.GetSize();i++)
	{
		mKeys[i]->mValue *= s;
	}
}

float TriVectorCurve::Length(
	)
{
	return mLength;
}

void TriVectorCurve::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* currvalue,
	const IList* theList
	)
{
	//TriScalarKey* skey = (TriScalarKey*)currvalue;

	switch(event & BELIST_EVENTMASK)
	{
	case BELIST_REMOVED:
		if(event & BELIST_LOADING)
		{
			return;
		}
		else
		{
			if( mKeys.GetSize() > 1 )
			{				
				if ( key == 0 )
				{
					mKeys[1]->mTime = 0.0f;
					Sort();
				}
				else
				{
					mLength = (float) (mKeys.back()->mTime - mKeys[0]->mTime);
				}
			}
			else
			{
				// Although one woule think that the code here below would make sense
				// it fucks with the loading of the curve as the length is messed upp
				// the reader/writer is redundantly calling clear key == -1 when loading a 
				// curve and not flaggin with BELIST_LOADING :-(
				//else
				//{
				//	mLength = 0.0f;
				//}
				if ( key == 0 )
				{
					mLength = 0.0f;
				}	
				mCurrKey = 1;
			}	
			// if a key is removed while the current key is set at the end will cause a crash
			if( mCurrKey >= mKeys.GetSize() )
			{
				mCurrKey = 1;
			}
		}
		break;

	case BELIST_LOADFINISHED:
		if( mKeys.empty() )
		{
			mLength = 0.f;
		}
		else if( fabs( mLength - ( mKeys.back()->mTime - mKeys[0]->mTime ) ) > FLT_EPSILON )
		{
			CCP_LOGWARN( 
				"The curve %S length (%f) does not match the key length (%f), this has been fixed",
				mName.c_str(), mLength, (mKeys.back()->mTime - mKeys[0]->mTime)
				);		
			Sort();
		}
		break;

	case BELIST_UNLOADSTART:
		mLength = 0.0;
		mCurrKey = 1;
		break;
	}
}

Be::Time TriVectorCurve::Start(
	)
{
	return mStart;
}

void TriVectorCurve::SetStartTime(
	Be::Time startTime
	)
{
	mStart = startTime;
}

TRIEXTRAPOLATION TriVectorCurve::Extrapolation()
{
	return mExtrapolation;
}

#if BLUE_WITH_PYTHON
PyObject* TriVectorCurve::PyAddKey(PyObject* args)
{	

	float t; 
	PyObject* v; 
	PyObject* l; 
	PyObject* r; 
	TRIINTERPOLATION i;

	if (!PyArg_ParseTuple(args, "fOOOi", 
		&t, &v, &l, &r, &i))
		return NULL;

	Vector3 val;
	Vector3 left;
	Vector3 right;


	if( !BlueExtractVector( v, (float*)&val, 3 ) )
	{
		ITriVectorPtr iv( v );
		if( iv )
		{
			val = *iv->GetVector();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Value must be a 3-tuple or a TriVector" );
			return NULL;
		}
	}

	if( !BlueExtractVector( l, (float*)&left, 3 ) )
	{
		ITriVectorPtr il( l );
		if( il )
		{
			left = *il->GetVector();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Left value must be a 3-tuple or a TriVector" );
			return NULL;
		}
	}

	if( !BlueExtractVector( r, (float*)&right, 3 ) )
	{
		ITriVectorPtr ir( r );
		if( ir )
		{
			right = *ir->GetVector();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Color right must be a 3-tuple or a TriVector" );
			return NULL;
		}
	}


    AddKey( t, val, left, right, i );
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* TriVectorCurve::PyGetKey(PyObject* args)
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
	TriVectorKey* key = new OTriVectorKey;
	TriVectorKey* src = mKeys[index];
	*key = *src;

	PyObject* pyKey = PyOS->WrapBlueObject(key);
	key->Unlock();

	return pyKey;
}

PyObject* TriVectorCurve::PySetKey(PyObject* args)
{	
	//ITriQuaternionKey
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

	TriVectorKey* src = BluePythonCast<TriVectorKey*>( pyKey );
	TriVectorKey* t = mKeys[index];
	*t = *src;

	Sort();	

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* TriVectorCurve::PyRemoveKey(PyObject* args)
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

double TriVectorCurve::GetCurrentPos( Be::Time time )
{	
	double t = TimeAsDouble( time - mStart );
	t = fmod( t, (double)mLength );
	return t;
}

PyObject* TriVectorCurve::PyCheckProximity(PyObject* args)
{
	PyObject* vC;
	float range;
	int stopAtFirst = 0;
	if (!PyArg_ParseTuple(args, "Of|i", &vC, &range, &stopAtFirst))
		return NULL;

	ITriVectorCurvePtr veevee(vC);

	size_t size;
	Be::Time* values;
	CheckProximityOfCurves(this, static_cast<TriVectorCurve*>( veevee.p ), range, &size, &values, (stopAtFirst != 0));

	PyObject* list = PyList_New(size);
	for (size_t i = 0; i < size; i++)
	{
		PyList_SET_ITEM((PyListObject*)(list), i, PyLong_FromLongLong(values[i]));
	}
	delete[] values;
	return list;	
}

PyObject* TriVectorCurve::PyCheckProximityToPoint(PyObject* args)
{
	PyObject* vP;
	float range;
	int stopAtFirst = 0;
	if (!PyArg_ParseTuple(args, "Of|i", &vP, &range, &stopAtFirst))
		return NULL;

	CCP_ASSERT(ITriVectorPtr(vP));

	size_t size;
	Be::Time* values; 
	CheckProximityOfCurveToPoint(this, ITriVectorPtr(vP)->GetVector(), range, &size, &values, (stopAtFirst != 0));

	PyObject* list = PyList_New(size);
	for (size_t i = 0; i < size; i++)
	{
		PyList_SET_ITEM((PyListObject*)(list), i, PyLong_FromLongLong(values[i]));
	}
	delete[] values;
	return list;	
}

#endif

