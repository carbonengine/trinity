#include "StdAfx.h"
#include "TriRotationCurve.h"
#include "Include/TriMath.h"

TriRotationCurve::TriRotationCurve(IRoot* lockobj) :
	mStart         ( 0           ), 
	mLength        ( 0.0f        ),
	mExtrapolation ( TRIEXT_NONE ),
	mCurrKey       ( 1           ),
	mValue(0.0f, 0.0f, 0.0f, 1.0f),
	PARENTLOCK(mKeys, ITriRotationCurve)
{	
	mKeys.SetNotify(this);
}


TriRotationCurve::~TriRotationCurve()
{
}


/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

Quaternion* TriRotationCurve::Update(
	Quaternion* in,
	Be::Time t
	)
{	
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}

Quaternion* TriRotationCurve::Update(
	Quaternion* in,
	double t
	)
{
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}


Quaternion* TriRotationCurve::GetValueAt(
	Quaternion* in,
	Be::Time now
	)
{
	if(mLength)
	{
		GetValueAt(in, TimeAsDouble(now - mStart));
	}
	else
	{
		*in = mValue;
	}
	return in;
}


Quaternion* TriRotationCurve::GetValueAt(
	Quaternion* in,
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
			CCP_ASSERT(mKeys.GetSize());
			*in = mKeys.back()->mValue;
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
		else
		{
			*in = mKeys[0]->mValue;
			return in;
		}
	}	
	// Seek and find the normalized time
	float nt = Seek(pos);	

	if (mCk_1->mInterpolation == TRIINT_CONSTANT)
	{
		*in = mCk_1->mValue;
	}
    else if( mCk_1->mInterpolation == TRIINT_LINEAR)
	{
        TriQuaternionLerp(in, &mCk_1->mValue, &mCk->mValue, nt);
	}
	else if( mCk_1->mInterpolation == TRIINT_HERMITE)
	{
		TriQuaternionHermite(in, &mCk_1->mValue, &mCk_1->mRight, &mCk->mLeft, &mCk->mValue, nt);
	}
	else if( mCk_1->mInterpolation == TRIINT_SLERP)
	{
		D3DXQuaternionSlerp(in, &mCk_1->mValue, &mCk->mValue, nt);
	}
	else 
	{//( k1->mInterpolation == TRIINT_SQUAD)
		D3DXQuaternionSquad(in, &mCk_1->mValue, &mCk_1->mRight, &mCk->mLeft, &mCk->mValue, nt);
	}
	return in;
/*
	if (mCk_1->mInterpolation == TRIINT_SLERP)
	{
		return Interpolate(in, mCk_1, mCk, NULL, NULL, (float)(pos - mCk_1->mTime)/(mCk->mTime - mCk_1->mTime));
	}
	else
	{ // here we need to find what 4 quads we want
		float dt = mCk->mTime - mCk_1->mTime;
	    float kt = (float)(pos - mCk_1->mTime);
		int s = mKeys.GetSize();		
		// are we closer to mCk_1 or mCk?
		if (kt/dt >= 0.5f)
		{// mCk_1, mCk_1, mCk, ck1			
			if(mExtrapolation == TRIEXT_CYCLE)			
				return Interpolate(in, &mCk[(mCurrKey>1)?(mCurrKey-2):(s-1)], mCk_1, mCk, &mCk[(mCurrKey+1)%s], kt);
			else
				return Interpolate(in, &mCk[max(mCurrKey-1,0)], mCk_1, mCk, &mCk[min(mCurrKey+1,s-1)], kt);
		}
		else
		{// mCk_1, mCk, ck1, ck2
			if(mExtrapolation == TRIEXT_CYCLE)			
				return Interpolate(in, mCk_1, mCk, &mCk[(mCurrKey+1)%s], &mCk[(mCurrKey+2)%s], kt);
			else
				return Interpolate(in, mCk_1, mCk, &mCk[min(mCurrKey+1,s-1)], &mCk[min(mCurrKey+2,s-1)], kt);
		}
	}
*/
}


Quaternion* TriRotationCurve::GetValueDotAt(
	Quaternion* in,
	Be::Time now
	)
{
	if(mLength)
	{
		GetValueDotAt(in, TimeAsDouble(now - mStart));
	}
	else
	{
		D3DXQuaternionIdentity(in);
	}
	return in;
}


Quaternion* TriRotationCurve::GetValueDotAt(
	Quaternion* in,
	double pos
	)
{
	if (mLength == 0.0f)
	{// there is no curve
		return static_cast<Quaternion*>( D3DXQuaternionIdentity(in) );
	}

	// No gradient for Quaternions	
	if ( (pos >= mKeys.back()->mTime) || (pos < 0.0) || (pos <= (mKeys[0]->mTime)) )		
	{// the curve is over, pos is after the last key
	//the curve hasn't begun or pos is before or equal to the first key
		if ((mExtrapolation == TRIEXT_NONE) ||  (mExtrapolation == TRIEXT_CONSTANT))		
		{
			return static_cast<Quaternion*>( D3DXQuaternionIdentity(in) );
		}
		else 
		{// (mExtrapolation == TRIEXT_CYCLE)
			pos = fmod(pos, (double)mKeys.back()->mTime);
		}
	}	
	// Seek and find the normalized time
	float nt = Seek(pos);	

	if (mCk_1->mInterpolation == TRIINT_CONSTANT)
	{
		*in = mCk_1->mValue;
	}
    else if( mCk_1->mInterpolation == TRIINT_LINEAR) 
	{
		//not quite correct.. but who cares?
        TriQuaternionSlerpDot(in, &mCk_1->mValue, &mCk->mValue, nt);
	}
	else if( mCk_1->mInterpolation == TRIINT_HERMITE)
	{
		TriQuaternionHermiteDot(in, &mCk_1->mValue, &mCk_1->mRight, &mCk->mLeft, &mCk->mValue, nt);
	}
	else if( mCk_1->mInterpolation == TRIINT_SLERP)
	{
		TriQuaternionSlerpDot(in, &mCk_1->mValue, &mCk->mValue, nt);
	}
	else 
	{//( k1->mInterpolation == TRIINT_SQUAD)
		//help!!!
		//D3DXQuaternionSquad(in, &mCk_1->mValue, &mCk_1->mRight, &mCk->mLeft, &mCk->mValue, nt);
		//this will just have to make sense until I can be botherd to work this out -Nonni
		TriQuaternionHermiteDot(in, &mCk_1->mValue, &mCk_1->mRight, &mCk->mLeft, &mCk->mValue, nt);
	}
	float dt = mCk->mTime - mCk_1->mTime;
	if (dt < 0.0000001f)
		dt = 0.0000001f;
	return TriQuaternionScale(in, in , 1.0f/dt);
}


Quaternion* TriRotationCurve::GetValueDoubleDotAt(
	Quaternion* in,
	Be::Time time
	)
{
	return in;
}


Quaternion* TriRotationCurve::GetValueDoubleDotAt(
	Quaternion* in,
	double time
	)
{
	return in;
}


float TriRotationCurve::Seek(
	double pos
	)
{
	CCP_ASSERT( mCurrKey < (int)mKeys.size() );
	CCP_ASSERT( mCurrKey > 0 );

	mCk = mKeys[mCurrKey];
	mCk_1 = mKeys[mCurrKey - 1];
	while((pos >= mCk->mTime) || (pos < mCk_1->mTime))
	{		
		if (pos < mCk_1->mTime)
		{// "cache miss" need to search from the beginning
			mCurrKey = 0;

			if( pos < mKeys[0]->mTime )
			{
				// Outside the range of the curve
				return 0.0f;
			}
		}		
		if( mCurrKey == (int)mKeys.size() - 1 )
		{
			// Outside the range of the curve
			return 1.0f;
		}

		mCurrKey++;

		mCk_1 = mKeys[mCurrKey - 1];
		mCk = mKeys[mCurrKey];				
	}	
	return (float)(pos - mCk_1->mTime) / float(mCk->mTime - mCk_1->mTime);
}

/////////////////////////////////////////////////////////////////////////////////////////
// compare function
/////////////////////////////////////////////////////////////////////////////////////////
static bool CompareKeys(IRoot* context, TriQuaternionKey* a, TriQuaternionKey* b)
{
	return a->mTime < b->mTime;
}

void TriRotationCurve::AddKey(
	float t,
	const Quaternion& v, 
	const Quaternion& l,
	const Quaternion& r,
	TRIINTERPOLATION i
	)
{	
	TriQuaternionKeyPtr k;
	k.CreateInstance();

	k->mTime = t;
	k->mValue = v;
	k->mLeft = l;
	k->mRight = r;
	k->mInterpolation = i;

	mKeys.Insert( -1, k );
	Sort();
}

void TriRotationCurve::Sort(
	)
{
	mKeys.Sort((IList::CompareFn)CompareKeys, NULL);
    mLength = (float) (mKeys.back()->mTime - mKeys[0]->mTime);

	// now we prepare the curve, by checking if any key
	// uses Hermite or squad

	ssize_t n = mKeys.GetSize();
	for(ssize_t i=0; i < n; i++)
	{
		if(mKeys[i]->mInterpolation == TRIINT_SQUAD)
		{
			TriQuaternionKey* q0 = mKeys[max(i-1, (ssize_t)0)];
			TriQuaternionKey* q1 = mKeys[i];
			TriQuaternionKey* q2 = mKeys[(i+1) % n];
			TriQuaternionKey* q3 = mKeys[(i+2) % n];

			D3DXQuaternionSquadSetup(
				&q1->mRight, 
				&q2->mLeft, 
				&q3->mLeft, 
				&q0->mValue,
				&q1->mValue,
				&q2->mValue,
				&q3->mValue);
		}
		if(mKeys[i]->mInterpolation == TRIINT_HERMITE)
		{
			TriQuaternionKey* q1 = mKeys[i];
			TriQuaternionKey* q2 = mKeys[(i+1) % n];
			float dt = q2->mTime - q1->mTime; 
			TriQuaternionHermiteSetup(
				&q1->mRight, 
				&q2->mLeft, 
				&q1->mValue, 
				&q1->mRight, 
				&q2->mLeft, 
				&q2->mValue, 
				dt
				);
		}
	}
}


void TriRotationCurve::ScaleTime(
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

void TriRotationCurve::Reverse(
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

void TriRotationCurve::ScaleValue(
	float s
	)
{
	for(int i=0; i< mKeys.GetSize();i++)
	{
		mKeys[i]->mValue *= s;
	}
}

float TriRotationCurve::Length(	
	)
{
	return mLength;
}

Be::Time TriRotationCurve::Start(
	)
{
	return mStart;
}

void TriRotationCurve::SetStartTime(
	Be::Time startTime
	)
{
	mStart = startTime;
}

TRIEXTRAPOLATION TriRotationCurve::Extrapolation()
{
	return mExtrapolation;
}

void TriRotationCurve::OnListModified(
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
				// Although one would think that the code here below would make sense
				// it fucks with the loading of the curve as the length is messed up
				// the reader/writer is redundantly calling clear key == -1 when loading a 
				// curve and not flagging with BELIST_LOADING :-(
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


#if BLUE_WITH_PYTHON
PyObject* TriRotationCurve::PyAddKey(PyObject* args)
{	

	float t; 
	PyObject* v; 
	PyObject* l; 
	PyObject* r; 
	TRIINTERPOLATION i;

	if (!PyArg_ParseTuple(args, "fOOOi", 
		&t, &v, &l, &r, &i))
		return NULL;

	Quaternion val;
	Quaternion left;
	Quaternion right;


	if( !BlueExtractVector( v, (float*)&val, 4 ) )
	{
		ITriQuaternionPtr iv( v );
		if( iv )
		{
			val = *iv->GetQuaternion();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Color value must be a 4-tuple or a TriQuaternion" );
			return NULL;
		}
	}

	if( !BlueExtractVector( l, (float*)&left, 4 ) )
	{
		ITriQuaternionPtr il( l );
		if( il )
		{
			left = *il->GetQuaternion();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Color left must be a 4-tuple or a TriQuaternion" );
			return NULL;
		}
	}

	if( !BlueExtractVector( r, (float*)&right, 4 ) )
	{
		ITriQuaternionPtr ir( r );
		if( ir )
		{
			right = *ir->GetQuaternion();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Color right must be a 4-tuple or a TriQuaternion" );
			return NULL;
		}
	}


    AddKey( t, val, left, right, i );
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* TriRotationCurve::PyGetKey(PyObject* args)
{	
	//ITriQuaternionKey
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
	TriQuaternionKey* key = new OTriQuaternionKey;
	TriQuaternionKey* src = mKeys[index];
	*key = *src;
	
	PyObject* pyKey = PyOS->WrapBlueObject(key);
	key->Unlock();

	return pyKey;
}

PyObject* TriRotationCurve::PySetKey(PyObject* args)
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
	
	TriQuaternionKey* src = BluePythonCast<TriQuaternionKey*>( pyKey );
	TriQuaternionKey* t = mKeys[index];
	*t = *src;

	Sort();	

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* TriRotationCurve::PyRemoveKey(PyObject* args)
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

double TriRotationCurve::GetCurrentPos( Be::Time time )
{	
	double t = TimeAsDouble( time - mStart );
	t = fmod( t, (double)mLength );
	return t;
}
