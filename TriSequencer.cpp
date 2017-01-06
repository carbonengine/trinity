#include "StdAfx.h"
#include "TriSequencer.h"
#include "include/TriVector.h"
#include "include/TriMath.h"
#include "include/ITriDuration.h"

static inline void FunctionLength( ITriFunctionPtr curve, float& maxDuration )
{
	float l = 0.0f;
	if( ITriDurationPtr f = BlueCastPtr( curve ) )
	{
		l = f->Length();
	}
	else if( ITriCurveLengthPtr f = BlueCastPtr( curve ) )
	{
		l = f->Length();
	}

	if( l > maxDuration )
	{
		maxDuration = l;
	}
}

TriScalarSequencer::TriScalarSequencer(IRoot* lockobj) :
	mStart  ( 0 ),
	mValue  ( 0.0f ),
	mOperator( TRIOP_MULTIPLY ),
	mClamping ( false),
	mInMinClamp (0.0),
	mInMaxClamp (1.0),
	mOutMinClamp (0.0),
	mOutMaxClamp (1.0),	
	PARENTLOCK(mFunctions, ITriScalarFunction)	
{		
}

TriScalarSequencer::~TriScalarSequencer()
{
}

float TriScalarSequencer::Length()
{
	float maxDuration = 0.0f;

	for( auto it = mFunctions.begin(); it != mFunctions.end(); ++it )
	{
		FunctionLength( *it, maxDuration );
	}

	return maxDuration;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriScalarSequencer::Update(
	Be::Time t
	)
{
	return mValue = GetValueAt(t);
}

float TriScalarSequencer::Update(
	double t
	)
{
	return mValue = GetValueAt(t);
}


float TriScalarSequencer::GetValueAt(
	Be::Time now
	)
{
	if (mOperator == TRIOP_MULTIPLY)
		return ClampOut(GetValueAtMult(now));
	else
		return ClampOut(GetValueAtAdd(now));
}

float TriScalarSequencer::ClampIn(float v)
{
	if (mClamping == true)
	{
	if (v < mInMinClamp)
		return mInMinClamp;
	if (v > mInMaxClamp)
		return mInMaxClamp;
	}
	return v;
}

float TriScalarSequencer::ClampOut(float v)
{
	if (mClamping == true)
	{
	if (v < mOutMinClamp)
		return mOutMinClamp;
	if (v > mOutMaxClamp)
		return mOutMaxClamp;
	}
	return v;
}



float TriScalarSequencer::GetValueAtMult(
	Be::Time now
	)
{
	float ret = 1.0f;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		ret *= ClampIn(mFunctions[i]->Update(now));
	}
	return ret;
}

float TriScalarSequencer::GetValueAtAdd(
	Be::Time now
	)
{
	float ret = 0.0f;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		ret += ClampIn(mFunctions[i]->Update(now));
	}
	return ret;
}


float TriScalarSequencer::GetValueAt(
	double pos
	)
{
	if (mOperator == TRIOP_MULTIPLY)
		return ClampOut(GetValueAtMult(pos));
	else
		return ClampOut(GetValueAtAdd(pos));
}

float TriScalarSequencer::GetValueAtMult(
	double pos
	)
{
	float ret = 1.0f;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		ret *= ClampIn(mFunctions[i]->Update(pos));
	}
	return ret;
}

float TriScalarSequencer::GetValueAtAdd(
	double pos
	)
{
	float ret = 0.0f;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		ret += ClampIn(mFunctions[i]->Update(pos));
	}
	return ret;
}

void TriScalarSequencer::ScaleTime( 
	float scale 
	)
{
	for( int i = 0; i < mFunctions.GetSize();  i++ )
	{
		mFunctions[i]->ScaleTime( scale );
	}
}


TriVectorSequencer::TriVectorSequencer(IRoot* lockobj) :
	mStart  ( 0 ),
	mOperator( TRIOP_MULTIPLY ),
	mValue( 0.0f, 0.0f, 0.0f ),
	PARENTLOCK(mFunctions, ITriVectorFunction)	
{		
}

TriVectorSequencer::~TriVectorSequencer()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
Vector3* TriVectorSequencer::Update(
	Vector3* in,
	Be::Time t
	)
{	
	GetValueAt(in, t);
	mValue = *in;
	return in;
}

Vector3* TriVectorSequencer::Update(
	Vector3* in,
	double t
	)
{
	GetValueAt(in, t);
	mValue = *in;
	return in;
}

Vector3* TriVectorSequencer::GetValueAt(
	Vector3* in,
	Be::Time now
	)
{	
	if (mOperator == TRIOP_MULTIPLY)
		return GetValueAtMult(in, now);
	else if(mOperator == TRIOP_ADD)
		return GetValueAtAdd(in, now);
	else
		return GetValueAtAverage(in, now);
}

Vector3* TriVectorSequencer::GetValueAtMult(
	Vector3* in,
	Be::Time now
	)
{
	*in = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, now);
		in->x *= temp.x;
		in->y *= temp.y;
		in->z *= temp.z;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAverage(
	Vector3* in,
	Be::Time now
	)
{
	*in = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 temp;
	float multiplier = 1.f / (float)mFunctions.GetSize();
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, now);
		*in += temp * multiplier;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAdd(
	Vector3* in,
	Be::Time now
	)
{
	*in = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, now);
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAt(
	Vector3* in,
	double pos
	)
{
	if (mOperator == TRIOP_MULTIPLY)
		return GetValueAtMult(in, pos);
	else if(mOperator == TRIOP_ADD)
		return GetValueAtAdd(in, pos);
	else
		return GetValueAtAverage(in, pos);
}

Vector3* TriVectorSequencer::GetValueAtMult(
	Vector3* in,
	double pos
	)
{
	*in = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, pos);
		in->x *= temp.x;
		in->y *= temp.y;
		in->z *= temp.z;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAdd(
	Vector3* in,
	double pos
	)
{
	*in = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, pos);
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAverage(
	Vector3* in,
	double pos
	)
{
	*in = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 temp;
	float multiplier = 1.f / (float)mFunctions.GetSize();
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueAt(&temp, pos);
		*in += temp * multiplier;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueDotAt(
	Vector3* in,
	Be::Time now
	)
{	
	in->x = 0.0f; in->y = 0.0f; in->z = 0.0f;
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueDotAt(&temp, now);
		*in += temp;
	}
	return in;
}


Vector3* TriVectorSequencer::GetValueDotAt(
	Vector3* in,
	double pos
	)
{
	in->x = 0.0f; in->y = 0.0f; in->z = 0.0f;
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueDotAt(&temp, pos);
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueDoubleDotAt(
	Vector3* in,
	Be::Time now
	)
{	
	in->x = 0.0f; in->y = 0.0f; in->z = 0.0f;
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueDoubleDotAt(&temp, now);
		*in += temp;
	}
	return in;
}


Vector3* TriVectorSequencer::GetValueDoubleDotAt(
	Vector3* in,
	double pos
	)
{
	in->x = 0.0f; in->y = 0.0f; in->z = 0.0f;
	Vector3 temp;
	for(int i = 0; i < mFunctions.GetSize();  i++)
	{
		mFunctions[i]->GetValueDoubleDotAt(&temp, pos);
		*in += temp;
	}
	return in;
}


#if BLUE_WITH_PYTHON
PyObject* TriVectorSequencer::PyGetValueDoubleDotAt(PyObject* args)
{	
	PyObject* t;
	if (!PyArg_ParseTuple(args, "O", &t))
		return NULL;

	ITriVectorPtr q;
	q.Attach(new OTriVector);	
	
	if (PyLong_Check(t))
	{
        Vector3 temp;
		q->SetVector( GetValueDoubleDotAt(&temp, (Be::Time)PyLong_AsLongLong(t)) );		
	}
	else if (PyFloat_Check(t))
	{
        Vector3 temp;
		q->SetVector( GetValueDoubleDotAt(&temp, PyFloat_AS_DOUBLE(t)) );
	}
	else
	{
		BeOS->SetError(BEDEF, Clsid(), 
			"arg must be of type LongLong (Be::Time) or float");
		return nullptr;
	}	
	return PyOS->WrapBlueObject(q);
}
#endif

//// Quaternion


TriQuaternionSequencer::TriQuaternionSequencer(IRoot* lockobj) :
	mStart  ( 0 ),
	mValue( 0.0f, 0.0f, 0.0f, 1.0f ),
	PARENTLOCK(mFunctions, ITriQuaternionFunction)	
{		
}

TriQuaternionSequencer::~TriQuaternionSequencer()
{
}


/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriQuaternionSequencer::Length()
{
	float maxDuration = 0.0f;
	
	for( auto it = mFunctions.begin(); it != mFunctions.end(); ++it )
	{
		FunctionLength( *it, maxDuration );
	}

	return maxDuration;
}

Quaternion* TriQuaternionSequencer::Update(
	Quaternion* in,
	Be::Time t
	)
{	
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}

Quaternion* TriQuaternionSequencer::Update(
	Quaternion* in,
	double t
	)
{
	GetValueAt(&mValue, t);
	*in = mValue;
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueAt(
	Quaternion* in,
	Be::Time now 
	)
{	
	D3DXQuaternionIdentity(in);	
	Quaternion temp;
	for(ssize_t i = mFunctions.GetSize() - 1; i >= 0 ;  i--)
	{
		mFunctions[i]->GetValueAt(&temp, now);
		*in *= temp; 
	}
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueAt(
	Quaternion* in,
	double pos
	)
{
	D3DXQuaternionIdentity(in);	
	Quaternion temp;
	for(ssize_t i = mFunctions.GetSize() - 1; i >= 0 ;  i--)
	{
		mFunctions[i]->GetValueAt(&temp, pos);
		*in *= temp; 
	}
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueDotAt(
	Quaternion* in,
	Be::Time time
	)
{
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueDotAt(
	Quaternion* in,
	double time
	)
{
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueDoubleDotAt(
	Quaternion* in,
	Be::Time time
	)
{
	return in;
}


Quaternion* TriQuaternionSequencer::GetValueDoubleDotAt(
	Quaternion* in,
	double time
	)
{
	return in;
}

////

//Color


TriColorSequencer::TriColorSequencer(IRoot* lockobj) :
    mStart  ( 0 ),
    mOperator( TRIOP_MULTIPLY ),
    mValue( 0.0f, 0.0f, 0.0f, 0.0f ),
    PARENTLOCK(mFunctions, ITriColorFunction)  
{       
}

TriColorSequencer::~TriColorSequencer()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriColorSequencer::Length()
{
	float maxDuration = 0.0f;

	for( auto it = mFunctions.begin(); it != mFunctions.end(); ++it )
	{
		FunctionLength( *it, maxDuration );
	}

	return maxDuration;
}

Color* TriColorSequencer::Update(
    Color* in,
    Be::Time t
    )
{   
    GetValueAt(in, t);
    mValue = *in;
    return in;
}

Color* TriColorSequencer::Update(
    Color* in,
    double t
    )
{
    GetValueAt(in, t);
	mValue = *in;
    return in;
}

Color* TriColorSequencer::GetValueAt(
    Color* in,
    Be::Time now
    )
{   
    if (mOperator == TRIOP_MULTIPLY)
        return GetValueAtMult(in, now);
    else
        return GetValueAtAdd(in, now);
}

Color* TriColorSequencer::GetValueAtMult(
    Color* in,
    Be::Time now
    )
{
    *in = Color(1.0f, 1.0f, 1.0f,1.0f);
    Color temp;
    for(int i = 0; i < mFunctions.GetSize();  i++)
    {
        mFunctions[i]->GetValueAt(&temp, now);
        in->r *= temp.r;
        in->g *= temp.g;
        in->b *= temp.b;
		in->a *= temp.a;

    }
    return in;
}

Color* TriColorSequencer::GetValueAtAdd(
    Color* in,
    Be::Time now
    )
{
    *in = Color(0.0f, 0.0f, 0.0f,0.0f);

    Color temp;
    for(int i = 0; i < mFunctions.GetSize();  i++)
    {
        mFunctions[i]->GetValueAt(&temp, now);
        *in += temp;
    }
    return in;
}

Color* TriColorSequencer::GetValueAt(
    Color* in,
    double pos
    )
{
    if (mOperator == TRIOP_MULTIPLY)
        return GetValueAtMult(in, pos);
    else
        return GetValueAtAdd(in, pos);
}

Color* TriColorSequencer::GetValueAtMult(
    Color* in,
    double pos
    )
{
    *in = Color(1.0f, 1.0f, 1.0f,1.0f);

    Color temp;
    for(int i = 0; i < mFunctions.GetSize();  i++)
    {
        mFunctions[i]->GetValueAt(&temp, pos);
        in->r *= temp.r;
        in->g *= temp.g;
        in->b *= temp.b;
		in->a *= temp.a;

    }
    return in;
}

Color* TriColorSequencer::GetValueAtAdd(
    Color* in,
    double pos
    )
{
    *in = Color(1.0f, 1.0f, 1.0f,1.0f);

    Color temp;
    for(int i = 0; i < mFunctions.GetSize();  i++)
    {
        mFunctions[i]->GetValueAt(&temp, pos);
        *in += temp;
    }
    return in;
}

TriXYZScalarSequencer::TriXYZScalarSequencer(IRoot* lockobj) :
    mValue( 0.0f, 0.0f, 0.0f )
{
}

TriXYZScalarSequencer::~TriXYZScalarSequencer()
{
}

float TriXYZScalarSequencer::Length()
{
	float maxDuration = 0.0f;

	FunctionLength( (ITriFunctionPtr)mXCurve, maxDuration );
	FunctionLength( (ITriFunctionPtr)mYCurve, maxDuration );
	FunctionLength( (ITriFunctionPtr)mZCurve, maxDuration );

	return maxDuration;
}
/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
Vector3* TriXYZScalarSequencer::Update(
    Vector3* in,
    Be::Time t
    )
{
    GetValueAt(in, t);
	mValue = *in;
    return in;
}

Vector3* TriXYZScalarSequencer::Update(
    Vector3* in,
    double t
    )
{
    GetValueAt(in, t);
	mValue = *in;
    return in;
}

Vector3* TriXYZScalarSequencer::GetValueAt(
    Vector3* in,
    Be::Time now
    )
{

	if(mXCurve)
	{
		in->x = mXCurve->GetValueAt(now);
	}
	else
	{
		in->x = 0.0f;
	}

	if(mYCurve)
	{

		in->y = mYCurve->GetValueAt(now);
	}
	else
	{
		in->y = 0.0f;
	}

	if(mZCurve)
	{
		in->z = mZCurve->GetValueAt(now);
	}
	else
	{
		in->z = 0.0f;
	}

	return in;
}

Vector3* TriXYZScalarSequencer::GetValueAt(
    Vector3* in,
    double pos
    )
{
	if(mXCurve)
	{
		in->x = mXCurve->GetValueAt(pos);
	}
	else
	{
		in->x = 0.0f;
	}

	if(mYCurve)
	{
		in->y = mYCurve->GetValueAt(pos);
	}
	else
	{
		in->y = 0.0f;
	}

	if(mZCurve)
	{
		in->z = mZCurve->GetValueAt(pos);
	}
	else
	{
		in->z = 0.0f;
	}

	return in;
}


/////


TriYPRSequencer::TriYPRSequencer(IRoot* lockobj) :
    mValue( 0.0f, 0.0f, 0.0f, 1.0f ),
	mYawPitchRoll( 0.0f, 0.0f, 0.0f )
{
}


TriYPRSequencer::~TriYPRSequencer()
{
}


/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

Quaternion* TriYPRSequencer::Update(
    Quaternion* in,
    Be::Time t
    )
{
    GetValueAt(&mValue, t);
    *in = mValue;
    return in;
}

Quaternion* TriYPRSequencer::Update(
    Quaternion* in,
    double t
    )
{
    GetValueAt(&mValue, t);
    *in = mValue;
    return in;
}

inline float ToRadian( float degrees )
{
    return degrees / 90.0f * TRI_PIBY2;
}

Quaternion* TriYPRSequencer::GetValueAt(
    Quaternion* in,
    Be::Time now
    )
{
	if(mYawCurve)
		mYawPitchRoll.x = mYawCurve->GetValueAt(now);
	if(mPitchCurve)
		mYawPitchRoll.y = mPitchCurve->GetValueAt(now);
	if(mRollCurve)
		mYawPitchRoll.z = mRollCurve->GetValueAt(now);

    return static_cast<Quaternion*>( D3DXQuaternionRotationYawPitchRoll(in, ToRadian(mYawPitchRoll.x), ToRadian(mYawPitchRoll.y ), ToRadian(mYawPitchRoll.z)) );
}


Quaternion* TriYPRSequencer::GetValueAt(
    Quaternion* in,
    double pos
    )
{
if(mYawCurve)
		mYawPitchRoll.x = mYawCurve->GetValueAt(pos);
	if(mPitchCurve)
		mYawPitchRoll.y = mPitchCurve->GetValueAt(pos);
	if(mRollCurve)
		mYawPitchRoll.z = mRollCurve->GetValueAt(pos);

    return static_cast<Quaternion*>( D3DXQuaternionRotationYawPitchRoll(in, ToRadian(mYawPitchRoll.x), ToRadian(mYawPitchRoll.y ), ToRadian(mYawPitchRoll.z)) );
}


/////////////



TriRGBAScalarSequencer::TriRGBAScalarSequencer(IRoot* lockobj) :
    mValue( 0.0f, 0.0f, 0.0f, 0.0f )
{
}

TriRGBAScalarSequencer::~TriRGBAScalarSequencer()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriRGBAScalarSequencer::Length()
{
	float maxDuration;

	FunctionLength( (ITriFunctionPtr)mAlphaCurve, maxDuration );
	FunctionLength( (ITriFunctionPtr)mRedCurve, maxDuration );
	FunctionLength( (ITriFunctionPtr)mGreenCurve, maxDuration );
	FunctionLength( (ITriFunctionPtr)mBlueCurve, maxDuration );

	return maxDuration;
}


Color* TriRGBAScalarSequencer::Update(
    Color* in,
    Be::Time t
    )
{
    GetValueAt(in, t);
	mValue = *in;
    return in;
}

Color* TriRGBAScalarSequencer::Update(
    Color* in,
    double t
    )
{
    GetValueAt(in, t);
	mValue = *in;
    return in;
}

Color* TriRGBAScalarSequencer::GetValueAt(
    Color* in,
    Be::Time now
    )
{

    if(mRedCurve)
    {
        in->r = mRedCurve->GetValueAt(now);
    }
	else
	{
		in->r = 0.0f;
	}

    if(mGreenCurve)
    {

        in->g = mGreenCurve->GetValueAt(now);
    }
	else
	{
		in->g = 0.0f;
	}

    if(mBlueCurve)
    {
        in->b = mBlueCurve->GetValueAt(now);
    }
	else
	{
		in->b = 0.0f;
	}

    if(mAlphaCurve)
    {
        in->a = mAlphaCurve->GetValueAt(now);
    }
	else
	{
		in->a = 0.0f;
	}
    
    return in;
}

Color* TriRGBAScalarSequencer::GetValueAt(
    Color* in,
    double pos
    )
{
    if(mRedCurve)
    {
        in->r = mRedCurve->GetValueAt(pos);
    }
	else
	{
		in->r = 0.0f;
	}

    if(mGreenCurve)
    {
        in->g = mGreenCurve->GetValueAt(pos);
    }
	else
	{
		in->g = 0.0f;
	}

    if(mBlueCurve)
    {
        in->b = mBlueCurve->GetValueAt(pos);
    }
	else
	{
		in->b = 0.0f;
	}

    if(mAlphaCurve)
    {
        in->a = mAlphaCurve->GetValueAt(pos);
    }
	else
	{
		in->a = 0.0f;
	}
    
    return in;
}



////////////////




TriPerlinCurve::TriPerlinCurve(IRoot* lockobj) :
	mValue  ( 0.0f ),
    mAlpha  ( 1.1f),
	mBeta   ( 2.0f),
	mSpeed  ( 1.0f),
	mScale  ( 1.0f),
	mOffset ( 0.0f),
	mN	    ( 3 ),
	mLastUpdated(-1.0),
	mStart  (0)
{
	// time is set automatically so that the algorithm doesn't try to calculate the 
	// curve many years forward (from time 0), resulting in floating point error jitteriness
	// Downside is that if a client runs without restart for several years, floating point errors occur.
	// I'll take the risk!

	//mStart = BeOS->GetActualTime();
	mStartOffset = TriRandInt((int)10000000000);
}

TriPerlinCurve::~TriPerlinCurve()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriPerlinCurve::Update(
	Be::Time t
	)
{
	if (mLastUpdated == TimeAsDouble(t)){
		return mValue;
	}
	else{
		mLastUpdated = TimeAsDouble(t);
		return mValue = GetValueAt(t);
	}
}

float TriPerlinCurve::Update(
	double t
	)
{
	if (mLastUpdated == t){
		return mValue;
	}
	else{
		mLastUpdated = t;
		return mValue = GetValueAt(t);
	}
}


float TriPerlinCurve::GetValueAt(
	Be::Time now
	)
{
	if (mStart == 0.0)
		mStart = TimeAsDouble(now);

	double pos = TimeAsDouble(now) - mStart;
	pos += mStartOffset;
	double ret = ((PerlinNoise1D(pos * mSpeed ,	 mAlpha, mBeta, mN) + 1.0) / 2.0 ) * mScale + mOffset;
	return (float)ret;
}


float TriPerlinCurve::GetValueAt(
	double pos
	)
{
// 	if (mStart == 0.0)
// 		mStart = pos;
// 
// 	pos -= mStart;

	pos += mStartOffset;
	return (((float)PerlinNoise1D( pos  * mSpeed , mAlpha, mBeta, mN) + 1.0f ) / 2.0f) * mScale + mOffset;
}


void TriPerlinCurve::ScaleTime(
	float s
	)
{
	mScale = s;
}

/////////////////////////////////////////////////////////////////////////////////////
TriSineCurve::TriSineCurve(IRoot* lockobj) :
    mValue  ( 0.0f ),
    mSpeed  ( 1.0f),
    mScale  ( 1.0f),
    mOffset ( 0.0f)
{
    // time is set automatically so that the algorithm doesn't try to calculate the 
    // curve many years forward (from time 0), resulting in floating point error jitteriness
    // Downside is that if a client runs without restart for several years, floating point errors occur.
    // I'll take the risk!

    mStart = BeOS->GetCurrentFrameTime();
}

TriSineCurve::~TriSineCurve()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriSineCurve::Update(
    Be::Time t
    )
{
    return mValue = GetValueAt(t);
}

float TriSineCurve::Update(
    double t
    )
{
    return mValue = GetValueAt(t);
}


float TriSineCurve::GetValueAt(
    Be::Time now
    )
{
    double pos = TimeAsDouble(now -mStart);
    double ret = sinf(  (float) (pos * ( 3.1415926535897932384626433832795f  * 2.0f )) * mSpeed) * mScale + mOffset;
    return (float)ret;
}


float TriSineCurve::GetValueAt(
    double pos
    )
{
    double ret = sinf(  (float) (pos * ( 3.1415926535897932384626433832795f  * 2.0f )) * mSpeed) * mScale + mOffset;
    return (float)ret;
}

void TriSineCurve::ScaleTime(
	float s
	)
{
	mScale = s;
}

/////////////////////////////


TriRandomConstantCurve::TriRandomConstantCurve(IRoot* lockobj) :
    mValue  ( 0.0f ),
    mMin      ( 0.0f ),
    mMax      ( 1.0f ),
	mHold	  ( true )
{
	Randomize();
}

TriRandomConstantCurve::~TriRandomConstantCurve()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriRandomConstantCurve::Update(
    Be::Time t
    )
{

    return mValue;
}

float TriRandomConstantCurve::Update(
    double t
    )
{
    return mValue;
}


float TriRandomConstantCurve::GetValueAt(
    Be::Time now
    )
{
	if (!mHold)
		Randomize();
    return mValue;
}


float TriRandomConstantCurve::GetValueAt(
    double pos
    )
{
	if (!mHold)
		Randomize();
    return mValue;
}

void TriRandomConstantCurve::ScaleTime(
	float s
	)
{
	// there is really no way to scale a random constant curve...
}

void TriRandomConstantCurve::Randomize()
{
	    mValue = mMin + (mMax - mMin) * TriRand();
}

