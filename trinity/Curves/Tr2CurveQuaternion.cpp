// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveQuaternion.h"

extern Be::VarChooser Tr2CurveInterpolationChooser[];

namespace
{

BlueStructureDefinition Tr2CurveQuaternionKeyDef[] = {
	{ "time", Be::FLOAT32_1, 0 },
	{ "value", Be::FLOAT32_4, 4 },
	{ "id", Be::USHORT_1, 20 },
	{ "interpolation", Be::USHORT_1, 22, Tr2CurveInterpolationChooser },
	{ 0 }
};

const Tr2CurveQuaternionKey s_defaultKey = { 0.f, Quaternion( 0.f, 0.f, 0.f, 1.f ), 0, Tr2CurveInterpolation::LINEAR };
}


// --------------------------------------------------------------------------------
Tr2CurveQuaternion::Tr2CurveQuaternion( IRoot* lockobj ) :
	PARENTLOCK( m_keys ),
	m_currentValue( 0.f, 0.f, 0.f, 1.f ),
	m_lastSegment( 0 ),
	m_extrapolationBefore( Tr2CurveExtrapolation::CLAMP ),
	m_extrapolationAfter( Tr2CurveExtrapolation::CLAMP )
{
	m_keys.SetStructureDefinition( Tr2CurveQuaternionKeyDef );
	m_keys.SetDefaultValue( &s_defaultKey );
}

// --------------------------------------------------------------------------------
void Tr2CurveQuaternion::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::Update( Quaternion* in, Be::Time time )
{
	*in = m_currentValue = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::Update( Quaternion* in, double time )
{
	*in = m_currentValue = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueAt( Quaternion* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueAt( Quaternion* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueDotAt( Quaternion* in, Be::Time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueDotAt( Quaternion* in, double )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueDoubleDotAt( Quaternion* in, Be::Time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveQuaternion::GetValueDoubleDotAt( Quaternion* in, double )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
float Tr2CurveQuaternion::Length()
{
	if( m_keys.empty() )
	{
		return 0;
	}
	return m_keys[m_keys.size() - 1].m_time;
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveQuaternion::GetValue( double time ) const
{
	if( m_keys.empty() )
	{
		return Quaternion( 0, 0, 0, 1 );
	}

	auto count = m_keys.size();

	if( count == 1 )
	{
		return m_keys[0].m_value;
	}

	if( m_extrapolationBefore == Tr2CurveExtrapolation::CLAMP && float( time ) <= m_keys[0].m_time )
	{
		return m_keys[0].m_value;
	}

	if( m_extrapolationAfter == Tr2CurveExtrapolation::CLAMP && float( time ) >= m_keys[m_keys.size() - 1].m_time )
	{
		return m_keys[m_keys.size() - 1].m_value;
	}

	float t = GetLocalTime( time );

	if( m_lastSegment + 1 < count )
	{
		// try cached last segment first
		auto& k0 = m_keys[m_lastSegment];
		auto& k1 = m_keys[m_lastSegment + 1];

		if( t >= k0.m_time && t < k1.m_time )
		{
			return GetSegmentValue( t, k0, k1 );
		}

		if( m_lastSegment + 2 < count )
		{
			// try a segment to the right of the cached one
			auto& k0 = m_keys[m_lastSegment + 1];
			auto& k1 = m_keys[m_lastSegment + 2];

			if( t >= k0.m_time && t < k1.m_time )
			{
				++m_lastSegment;
				return GetSegmentValue( t, k0, k1 );
			}
		}

		if( m_lastSegment > 1 )
		{
			// try a segment to the left of the cached one
			auto& k0 = m_keys[m_lastSegment - 1];
			auto& k1 = m_keys[m_lastSegment];

			if( t >= k0.m_time && t < k1.m_time )
			{
				--m_lastSegment;
				return GetSegmentValue( t, k0, k1 );
			}
		}
	}

	// cache miss: try all segments
	for( size_t i = 0; i + 1 < count; ++i )
	{
		auto& k0 = m_keys[i];
		auto& k1 = m_keys[i + 1];

		if( t >= k0.m_time && t < k1.m_time )
		{
			m_lastSegment = int32_t( i );
			return GetSegmentValue( t, k0, k1 );
		}
	}
	m_lastSegment = count - 2;
	return GetSegmentValue( t, m_keys[count - 2], m_keys[count - 1] );
}

// --------------------------------------------------------------------------------
float Tr2CurveQuaternion::GetLocalTime( double time ) const
{
	if( m_keys.empty() )
	{
		return 0;
	}

	double first = double( m_keys[0].m_time );
	double last = double( m_keys[m_keys.size() - 1].m_time );
	double length = last - first;
	if( time < first )
	{
		double intPart;
		double fracPart = modf( -( time - first ) / length, &intPart );

		if( m_extrapolationBefore == Tr2CurveExtrapolation::CYCLE )
		{
			fracPart = 1.0 - fracPart;
		}
		else
		{
			if( int64_t( intPart ) % 2 != 0 )
			{
				fracPart = 1.0 - fracPart;
			}
		}
		return float( fracPart * length + first );
	}
	if( time < last )
	{
		return float( time );
	}

	double intPart;
	double fracPart = modf( ( time - first ) / length, &intPart );

	if( m_extrapolationAfter == Tr2CurveExtrapolation::MIRROR )
	{
		if( int64_t( intPart ) % 2 != 0 )
		{
			fracPart = 1.0 - fracPart;
		}
	}
	return float( fracPart * length + first );
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveQuaternion::GetSegmentValue( float time, const Tr2CurveQuaternionKey& k0, const Tr2CurveQuaternionKey& k1 ) const
{
	if( k0.m_interpolation == Tr2CurveInterpolation::CONSTANT )
	{
		return time == k1.m_time ? k1.m_value : k0.m_value;
	}

	return Quaternion( XMQuaternionSlerp( k0.m_value, k1.m_value, ( time - k0.m_time ) / ( k1.m_time - k0.m_time ) ) );
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveQuaternion::GetCurrentValue() const
{
	return m_currentValue;
}

// --------------------------------------------------------------------------------
void Tr2CurveQuaternion::OnKeysChanged()
{
	std::stable_sort(
		m_keys.begin(),
		m_keys.end(),
		[]( const Tr2CurveQuaternionKey& k0, const Tr2CurveQuaternionKey& k1 ) { return k0.m_time < k1.m_time; } );
}

// --------------------------------------------------------------------------------
void Tr2CurveQuaternion::AddKey(
	float time,
	Quaternion value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::LINEAR> interpolation )
{
	Tr2CurveQuaternionKey key;
	key.m_time = time;
	key.m_value = value;
	key.m_interpolation = interpolation;
	key.m_id = 0;
	m_keys.Append( &key );
	OnKeysChanged();
}

// --------------------------------------------------------------------------------
void Tr2CurveQuaternion::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_extrapolationAfter = m_extrapolationBefore = extrapolation;
}
