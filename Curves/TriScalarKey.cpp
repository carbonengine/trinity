#include "StdAfx.h"
#include "TriScalarKey.h"
#include "Include/TriMath.h"


TriScalarKey::TriScalarKey(IRoot* lockobj) :
	mTime(  0.0f ),
	mValue( 0.0f ),
	mLeft(  0.0f ),
	mRight( 0.0f ),
	mInterpolation( TRIINT_NONE )
{
};

TriVectorKey::TriVectorKey(IRoot* lockobj) :
	mTime( 0.0f ),
	mValue( 0.0f, 0.0f, 0.0f ),
	mLeft( 0.0f, 0.0f, 0.0f ),
	mRight( 0.0f, 0.0f, 0.0f ),
	mInterpolation( TRIINT_NONE )
{		
};

TriQuaternionKey::TriQuaternionKey(IRoot* lockobj) :
	mTime(0.0),
	mValue(0.0f, 0.0f, 0.0f, 1.0f),
	mLeft(0.0f, 0.0f, 0.0f, 1.0f),
	mRight(0.0f, 0.0f, 0.0f, 1.0f),
	mInterpolation( TRIINT_SLERP )
{		
};

TriColorKey::TriColorKey(IRoot* lockobj) :
	mTime( 0.0f ),
	mValue( 0.0f, 0.0f, 0.0f, 0.0f ),
	mLeft( 0.0f, 0.0f, 0.0f, 0.0f ),
	mRight( 0.0f, 0.0f, 0.0f, 0.0f ),
	mInterpolation( TRIINT_NONE )
{		
};

float Interpolate(
	const TriScalarKey* k1, 
	const TriScalarKey* k2, 
	float t
	)
{
	CCP_ASSERT(k1);
	CCP_ASSERT(k2);	

	if (k1->mInterpolation == TRIINT_CONSTANT)
	{
        return k1->mValue;
	}
    else if( k1->mInterpolation == TRIINT_LINEAR)
	{
        return k1->mValue + (float)( ( ( k2->mValue - k1->mValue ) / (float)( k2->mTime - k1->mTime ) ) * t );
	}
    else if( k1->mInterpolation == TRIINT_HERMITE)
	{// (k1->mInterpolation == TRIINT_HERMITE)
		return Hermite(k1->mValue, k1->mRight, k2->mValue, k2->mLeft, t, k2->mTime - k1->mTime);
	}
	else		
	{// (k1->mInterpolation == TRIINT_SIGMOID)
		float sq = sqrtf( k1->mValue / k2->mValue );
		float exponent = expf( -t / k1->mRight );
		float ret = (1.0f + (sq-1.0f)*exponent);
		return ret*ret*k2->mValue;
	}
}


Vector3* Interpolate(
	Vector3* out, 
	const TriVectorKey* k1, 
	const TriVectorKey* k2, 
	float t
	)
{
	if (k1->mInterpolation == TRIINT_CONSTANT)
	{
		*out = k1->mValue;
        return out;
	}
    else if( k1->mInterpolation == TRIINT_LINEAR)
	{
        D3DXVec3Lerp(out, &k1->mValue, &k2->mValue, t);
	}
	else if( k1->mInterpolation == TRIINT_HERMITE)
	{
		D3DXVec3Hermite(out, &k1->mValue, &k1->mRight, &k2->mValue, &k2->mLeft, t);
	}
	else
	{//( k1->mInterpolation == TRIINT_CATMULLROM)
		D3DXVec3CatmullRom(out, &k1->mValue, &k1->mRight, &k2->mValue, &k2->mLeft, t);
	}

	return out;
}

Quaternion* Interpolate(
	Quaternion* out, 
	const TriQuaternionKey* k1, 
	const TriQuaternionKey* k2, 
	const TriQuaternionKey* k3, 
	const TriQuaternionKey* k4, 
	float t
	)
{
	if (k1->mInterpolation == TRIINT_CONSTANT)
	{
		*out = k1->mValue;
        return out;
	}
    else if( k1->mInterpolation == TRIINT_LINEAR)
	{
        return TriQuaternionLerp(out, &k1->mValue, &k2->mValue, t);
	}
	else if( k1->mInterpolation == TRIINT_HERMITE)
	{
		return TriQuaternionHermite(out, &k1->mValue, &k2->mValue, &k3->mValue, &k4->mValue, t);
	}
	else if( k1->mInterpolation == TRIINT_SLERP)
	{
		return static_cast<Quaternion*>( D3DXQuaternionSlerp(out, &k1->mValue, &k2->mValue, t) );
	}
	else 
	{//( k1->mInterpolation == TRIINT_SQUAD)
		return static_cast<Quaternion*>( D3DXQuaternionSquad(out, &k1->mValue, &k2->mValue, &k3->mValue, &k4->mValue, t) );
	}
}

Color* Interpolate(
	Color* out, 
	const TriColorKey* k1, 
	const TriColorKey* k2, 
	float t
	)
{
	if (k1->mInterpolation == TRIINT_CONSTANT)
	{
		*out = k1->mValue;
        return out;
	}
    else
	{//( k1->mInterpolation == TRIINT_LINEAR)
		return reinterpret_cast<Color*>( D3DXColorLerp(out, &k1->mValue, &k2->mValue, t) );        
	}
}

