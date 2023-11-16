#include "StdAfx.h"
#include "TriRigidOrientation.h"
#include "include/TriQuaternion.h"
#include "include/TriVector.h"
#include "Include/TriMath.h"

TriTorque::TriTorque(IRoot* lockobj) :
	mTime(  0.0f ),	
	mRot0( 0.0f, 0.0f, 0.0f, 1.0f ),
	mOmega0( 0.0f, 0.0f, 0.0f ),
	mTorque( 0.0f, 0.0f, 0.0f )
{
}

TriRigidOrientation::TriRigidOrientation(IRoot* lockobj) :
	mStart ( 0    ), 
	mI  ( 1.0f ),
	mDrag  ( 1.0f ),
	mCurrKey ( 0 ),
	PARENTLOCK( mStates, IRoot ),		
	mValue( 0.0f, 0.0f, 0.0f, 1.0f )
{	
}


TriRigidOrientation::~TriRigidOrientation()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

Quaternion* TriRigidOrientation::Update(
	Quaternion* in,
	Be::Time t
	)
{	
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}

Quaternion* TriRigidOrientation::Update(
	Quaternion* in,
	double t
	)
{
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}


Quaternion* TriRigidOrientation::GetValueAt(
	Quaternion* in,
	Be::Time now
	)
{	
	return GetValueAt(in, TimeAsDouble(now - mStart));	
}


Quaternion* TriRigidOrientation::GetValueAt(
	Quaternion* in,
	double t
	)
{	
	if ((mStates.GetSize() == 0) ||(t < 0.0) || (t < (mStates[0]->mTime)))
	{// there are no keys, the curve hasn't begun or t is before the first key
		*in = mValue;
		return in;
	}	
	Seek(t);
	Vector3 ZeroVec(0.0f, 0.0f, 0.0f);
	TriVectorExponentialDecayInteger(
		&mTauVector,
		&ZeroVec, 
		&mStates[mCurrKey]->mOmega0, 
		&mStates[mCurrKey]->mTorque, 
		mI, mDrag, 
		(float)(t - mStates[mCurrKey]->mTime));
	mTauConverter.x = mTauVector.x;	
	mTauConverter.y = mTauVector.y;
	mTauConverter.z = mTauVector.z;
	mTauConverter.w = 0.0f;
	mTauConverter = Exp( mTauConverter );
	*in = mStates[mCurrKey]->mRot0 * mTauConverter;
	return in;
}

Vector3* TriRigidOrientation::GetValueDotAt(
	Vector3* in,
	Be::Time now
	)
{	
	return GetValueDotAt(in, TimeAsDouble(now - mStart));	
}

Vector3* TriRigidOrientation::GetValueDotAt(
	Vector3* in, 
	double t
	)
{	
	if ((mStates.GetSize() == 0) || (t < 0.0) || (t < (mStates[0]->mTime)))
	{// there are no keys, the curve hasn't begun or t is before the first key
		*in = Vector3(0.0f, 0.0f, 0.0f);
		return in;
	}	
	Seek(t);

	return TriVectorExponentialDecay(in, 
		&mStates[mCurrKey]->mOmega0, 
		&mStates[mCurrKey]->mTorque, 
		mI, mDrag, 
		float(t - mStates[mCurrKey]->mTime));
}


Quaternion* TriRigidOrientation::GetValueDotAt(
	Quaternion* in,
	Be::Time time
	)
{
	return in;
}


Quaternion* TriRigidOrientation::GetValueDotAt(
	Quaternion* in,
	double time
	)
{
	return in;
}


Quaternion* TriRigidOrientation::GetValueDoubleDotAt(
	Quaternion* in,
	Be::Time time
	)
{
	return in;
}


Quaternion* TriRigidOrientation::GetValueDoubleDotAt(
	Quaternion* in,
	double time
	)
{
	return in;
}


void TriRigidOrientation::Seek(
	double t
	)
{	
	int n = (int)mStates.GetSize();
	if(t >= mStates.back()->mTime)
	{
		mCurrKey = n-1;		
		return;
	}
	else if (mCurrKey == n-1)	
		mCurrKey = 0;
			
	if( (t < mStates[mCurrKey]->mTime) ||	
	    (t >= mStates[mCurrKey+1]->mTime))
	{
		for(mCurrKey=0; mCurrKey < n-1;mCurrKey++)
		{
			if( (t >= mStates[mCurrKey]->mTime) &&	
				(t < mStates[mCurrKey+1]->mTime))
				break;
		}
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////
// compare function
/////////////////////////////////////////////////////////////////////////////////////////
static bool CompareKeys(IRoot* context, TriTorque* a, TriTorque* b)
{
	return a->mTime < b->mTime;
}

void TriRigidOrientation::Sort(
	)
{
	mStates.Sort((IList::CompareFn)CompareKeys, NULL);    
	mCurrKey = 0;

	// go over all the keys and set the correct
	// initial states by extrapolating from the state before
	for(int i = 1; i < mStates.GetSize(); i++)
	{		
		TriTorque* cs = mStates[i];
		TriTorque* cs_1 = mStates[i-1];
		float time = cs->mTime - cs_1->mTime;
		TriVectorExponentialDecay(
			&cs->mOmega0, 
			&cs_1->mOmega0, 
			&cs_1->mTorque, 
			mI, mDrag, time);
		Vector3 ZeroVec(0.0f, 0.0f, 0.0f);
		TriVectorExponentialDecayInteger(
			&mTauVector,
			&ZeroVec, 
			&cs_1->mOmega0, 
			&cs_1->mTorque, 
			mI, mDrag, time);
		mTauConverter.x = mTauVector.x;	
		mTauConverter.y = mTauVector.y;
		mTauConverter.z = mTauVector.z;
		mTauConverter.w = 0.0f;
		mTauConverter = Exp( mTauConverter );
		cs->mRot0 = cs_1->mRot0 * mTauConverter;
	}
	if (mStates.GetSize() > 0)
	{
		mValue = mStates[0]->mRot0;
	}
}
