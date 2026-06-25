// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriSequencer.h"
#include "include/TriVector.h"
#include "include/TriMath.h"
#include "include/ITriDuration.h"

extern bool g_expressionCurveFakeRandom;

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


TriVectorSequencer::TriVectorSequencer( IRoot* lockobj ) :
	mStart( 0 ),
	mOperator( TRIOP_MULTIPLY ),
	mValue( 0.0f, 0.0f, 0.0f ),
	PARENTLOCK( mFunctions, ITriVectorFunction )
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
	Be::Time t )
{
	GetValueAt( in, t );
	mValue = *in;
	return in;
}

Vector3* TriVectorSequencer::Update(
	Vector3* in,
	double t )
{
	GetValueAt( in, t );
	mValue = *in;
	return in;
}

Vector3* TriVectorSequencer::GetValueAt(
	Vector3* in,
	Be::Time now )
{
	if( mOperator == TRIOP_MULTIPLY )
		return GetValueAtMult( in, now );
	else if( mOperator == TRIOP_ADD )
		return GetValueAtAdd( in, now );
	else
		return GetValueAtAverage( in, now );
}

Vector3* TriVectorSequencer::GetValueAtMult(
	Vector3* in,
	Be::Time now )
{
	*in = Vector3( 1.0f, 1.0f, 1.0f );
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, now );
		in->x *= temp.x;
		in->y *= temp.y;
		in->z *= temp.z;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAverage(
	Vector3* in,
	Be::Time now )
{
	*in = Vector3( 0.0f, 0.0f, 0.0f );
	Vector3 temp;
	float multiplier = 1.f / (float)mFunctions.GetSize();
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, now );
		*in += temp * multiplier;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAdd(
	Vector3* in,
	Be::Time now )
{
	*in = Vector3( 0.0f, 0.0f, 0.0f );
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, now );
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAt(
	Vector3* in,
	double pos )
{
	if( mOperator == TRIOP_MULTIPLY )
		return GetValueAtMult( in, pos );
	else if( mOperator == TRIOP_ADD )
		return GetValueAtAdd( in, pos );
	else
		return GetValueAtAverage( in, pos );
}

Vector3* TriVectorSequencer::GetValueAtMult(
	Vector3* in,
	double pos )
{
	*in = Vector3( 1.0f, 1.0f, 1.0f );
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, pos );
		in->x *= temp.x;
		in->y *= temp.y;
		in->z *= temp.z;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAdd(
	Vector3* in,
	double pos )
{
	*in = Vector3( 0.0f, 0.0f, 0.0f );
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, pos );
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueAtAverage(
	Vector3* in,
	double pos )
{
	*in = Vector3( 0.0f, 0.0f, 0.0f );
	Vector3 temp;
	float multiplier = 1.f / (float)mFunctions.GetSize();
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, pos );
		*in += temp * multiplier;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueDotAt(
	Vector3* in,
	Be::Time now )
{
	in->x = 0.0f;
	in->y = 0.0f;
	in->z = 0.0f;
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueDotAt( &temp, now );
		*in += temp;
	}
	return in;
}


Vector3* TriVectorSequencer::GetValueDotAt(
	Vector3* in,
	double pos )
{
	in->x = 0.0f;
	in->y = 0.0f;
	in->z = 0.0f;
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueDotAt( &temp, pos );
		*in += temp;
	}
	return in;
}

Vector3* TriVectorSequencer::GetValueDoubleDotAt(
	Vector3* in,
	Be::Time now )
{
	in->x = 0.0f;
	in->y = 0.0f;
	in->z = 0.0f;
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueDoubleDotAt( &temp, now );
		*in += temp;
	}
	return in;
}


Vector3* TriVectorSequencer::GetValueDoubleDotAt(
	Vector3* in,
	double pos )
{
	in->x = 0.0f;
	in->y = 0.0f;
	in->z = 0.0f;
	Vector3 temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueDoubleDotAt( &temp, pos );
		*in += temp;
	}
	return in;
}


#if BLUE_WITH_PYTHON
PyObject* TriVectorSequencer::PyGetValueDoubleDotAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriVectorPtr q;
	q.Attach( new OTriVector );

	if( PyLong_Check( t ) )
	{
		Vector3 temp;
		q->SetVector( GetValueDoubleDotAt( &temp, (Be::Time)PyLong_AsLongLong( t ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Vector3 temp;
		q->SetVector( GetValueDoubleDotAt( &temp, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}
#endif


//Color


TriColorSequencer::TriColorSequencer( IRoot* lockobj ) :
	mStart( 0 ),
	mOperator( TRIOP_MULTIPLY ),
	mValue( 0.0f, 0.0f, 0.0f, 0.0f ),
	PARENTLOCK( mFunctions, ITriColorFunction )
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
	Be::Time t )
{
	GetValueAt( in, t );
	mValue = *in;
	return in;
}

Color* TriColorSequencer::Update(
	Color* in,
	double t )
{
	GetValueAt( in, t );
	mValue = *in;
	return in;
}

Color* TriColorSequencer::GetValueAt(
	Color* in,
	Be::Time now )
{
	if( mOperator == TRIOP_MULTIPLY )
		return GetValueAtMult( in, now );
	else
		return GetValueAtAdd( in, now );
}

Color* TriColorSequencer::GetValueAtMult(
	Color* in,
	Be::Time now )
{
	*in = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	Color temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, now );
		in->r *= temp.r;
		in->g *= temp.g;
		in->b *= temp.b;
		in->a *= temp.a;
	}
	return in;
}

Color* TriColorSequencer::GetValueAtAdd(
	Color* in,
	Be::Time now )
{
	*in = Color( 0.0f, 0.0f, 0.0f, 0.0f );

	Color temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, now );
		*in += temp;
	}
	return in;
}

Color* TriColorSequencer::GetValueAt(
	Color* in,
	double pos )
{
	if( mOperator == TRIOP_MULTIPLY )
		return GetValueAtMult( in, pos );
	else
		return GetValueAtAdd( in, pos );
}

Color* TriColorSequencer::GetValueAtMult(
	Color* in,
	double pos )
{
	*in = Color( 1.0f, 1.0f, 1.0f, 1.0f );

	Color temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, pos );
		in->r *= temp.r;
		in->g *= temp.g;
		in->b *= temp.b;
		in->a *= temp.a;
	}
	return in;
}

Color* TriColorSequencer::GetValueAtAdd(
	Color* in,
	double pos )
{
	*in = Color( 1.0f, 1.0f, 1.0f, 1.0f );

	Color temp;
	for( int i = 0; i < mFunctions.GetSize(); i++ )
	{
		mFunctions[i]->GetValueAt( &temp, pos );
		*in += temp;
	}
	return in;
}


////////////////




TriPerlinCurve::TriPerlinCurve( IRoot* lockobj ) :
	mValue( 0.0f ),
	mAlpha( 1.1f ),
	mBeta( 2.0f ),
	mSpeed( 1.0f ),
	mScale( 1.0f ),
	mOffset( 0.0f ),
	mN( 3 ),
	mLastUpdated( -1.0 ),
	mStart( 0 )
{
	// time is set automatically so that the algorithm doesn't try to calculate the
	// curve many years forward (from time 0), resulting in floating point error jitteriness
	// Downside is that if a client runs without restart for several years, floating point errors occur.
	// I'll take the risk!

	//mStart = BeOS->GetActualTime();
	mStartOffset = TriRandInt( (int)10000000000 );
}

TriPerlinCurve::~TriPerlinCurve()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////
float TriPerlinCurve::Update(
	Be::Time t )
{
	if( mLastUpdated == TimeAsDouble( t ) )
	{
		return mValue;
	}
	else
	{
		mLastUpdated = TimeAsDouble( t );
		return mValue = GetValueAt( t );
	}
}

float TriPerlinCurve::Update(
	double t )
{
	if( mLastUpdated == t )
	{
		return mValue;
	}
	else
	{
		mLastUpdated = t;
		return mValue = GetValueAt( t );
	}
}


float TriPerlinCurve::GetValueAt(
	Be::Time now )
{
	if( mStart == 0.0 )
		mStart = TimeAsDouble( now );

	double pos = TimeAsDouble( now ) - mStart;
	if( g_expressionCurveFakeRandom )
	{
		pos = pos * mSpeed + 0.21f;
	}
	else
	{
		pos += mStartOffset;
		pos *= mSpeed;
	}
	double ret = ( ( PerlinNoise1D( pos, mAlpha, mBeta, mN ) + 1.0 ) / 2.0 ) * mScale + mOffset;
	return (float)ret;
}


float TriPerlinCurve::GetValueAt(
	double pos )
{
	// 	if (mStart == 0.0)
	// 		mStart = pos;
	//
	// 	pos -= mStart;

	if( g_expressionCurveFakeRandom )
	{
		pos = pos * mSpeed + 0.21f;
	}
	else
	{
		pos += mStartOffset;
		pos *= mSpeed;
	}
	return ( ( (float)PerlinNoise1D( pos, mAlpha, mBeta, mN ) + 1.0f ) / 2.0f ) * mScale + mOffset;
}


void TriPerlinCurve::ScaleTime(
	float s )
{
	mScale = s;
}
