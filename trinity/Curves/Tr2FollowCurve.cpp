// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2FollowCurve.h"


Tr2FollowCurve::Tr2FollowCurve( IRoot* lockobj ) :
	PARENTLOCK( m_keys )
{
	m_keys.SetNotify( this );
}

Tr2FollowCurve::~Tr2FollowCurve()
{
	m_keys.Clear();
}

void Tr2FollowCurve::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

void Tr2FollowCurve::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_keys )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_REMOVED:
		case BELIST_INSERTED:
			std::stable_sort(
				m_keys.begin(),
				m_keys.end(),
				[]( ITr2FollowCurveKey* k0, ITr2FollowCurveKey* k1 ) { return k0->GetTime() < k1->GetTime(); } );
			break;
		default:
			break;
		}
	}
}

Vector3 Tr2FollowCurve::GetValue( double time ) const
{
	ITr2FollowCurveKey* currentKey = nullptr;
	ITr2FollowCurveKey* nextKey = nullptr;
	Vector3 value = Vector3();
	for( auto key = begin( m_keys ); key != end( m_keys ); ++key )
	{
		auto k = *key;

		if( time < k->GetTime() )
		{
			nextKey = k;
			break;
		}
		currentKey = k;
	}

	if( nextKey && currentKey )
	{
		value = GetSegmentValue( float( time ), *currentKey, *nextKey );
	}
	else if( currentKey )
	{
		value = currentKey->GetValue();
	}

	currentKey = nullptr;
	nextKey = nullptr;
	return value;
}

Vector3 Tr2FollowCurve::GetSegmentValue( float time, ITr2FollowCurveKey& k0, ITr2FollowCurveKey& k1 ) const
{
	switch( k0.GetInterpolationType() )
	{
	case Tr2FollowCurveKeyInterpolation::CONSTANT:
		return time == k1.GetTime() ? k1.GetValue() : k0.GetValue();
	case Tr2FollowCurveKeyInterpolation::LINEAR:
		if( k1.GetTime() == k0.GetTime() )
		{
			return k1.GetValue();
		}
		return k0.GetValue() + ( k1.GetValue() - k0.GetValue() ) * ( time - k0.GetTime() ) / ( k1.GetTime() - k0.GetTime() );
	case Tr2FollowCurveKeyInterpolation::HERMITE: {
		float length = k1.GetTime() - k0.GetTime();
		if( length == 0 )
		{
			return k1.GetValue();
		}
		Vector3 inTangent = k0.GetRightTangent() * length;
		Vector3 outTangent = k1.GetLeftTangent() * length;

		float s = ( time - k0.GetTime() ) / length;
		float s2 = s * s;
		float s3 = s2 * s;

		float c2 = -2.0f * s3 + 3.0f * s2;
		float c1 = 1.0f - c2;
		float c4 = s3 - s2;
		float c3 = s + c4 - s2;

		return k0.GetValue() * c1 + k1.GetValue() * c2 + inTangent * c3 + outTangent * c4;
	}
	default:
		return Vector3( 0, 0, 0 );
	}
}

Vector3* Tr2FollowCurve::Update( Vector3* in, Be::Time time )
{
	m_currentValue = GetValue( TimeAsDouble( time ) );
	*in = m_currentValue;
	return in;
}

Vector3* Tr2FollowCurve::Update( Vector3* in, double time )
{
	m_currentValue = GetValue( time );
	*in = m_currentValue;
	return in;
}

Vector3* Tr2FollowCurve::GetValueAt( Vector3* in, Be::Time time )
{
	return GetValueAt( in, TimeAsDouble( time ) );
}

Vector3* Tr2FollowCurve::GetValueAt( Vector3* in, double time )
{
	*in = GetValue( time );
	return in;
}

Vector3* Tr2FollowCurve::GetValueDotAt( Vector3* in, Be::Time time )
{
	return in;
}

Vector3* Tr2FollowCurve::GetValueDotAt( Vector3* in, double time )
{
	return in;
}

Vector3* Tr2FollowCurve::GetValueDoubleDotAt( Vector3* in, Be::Time time )
{
	return in;
}

Vector3* Tr2FollowCurve::GetValueDoubleDotAt( Vector3* in, double time )
{
	return in;
}

Vector3d* Tr2FollowCurve::InterpolatedPosition( Vector3d* out, Be::Time time )
{
	return out;
}
