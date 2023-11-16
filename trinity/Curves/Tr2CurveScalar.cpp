////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveScalar.h"

extern Be::VarChooser Tr2CurveInterpolationChooser[];
extern Be::VarChooser Tr2CurveTangentTypeChooser[];

namespace
{

BlueStructureDefinition Tr2CurveScalarKeyDef[] =
{
	{ "time", Be::FLOAT32_1, 0 },
	{ "value", Be::FLOAT32_1, 4 },
	{ "leftTangent", Be::FLOAT32_1, 8 },
	{ "rightTangent", Be::FLOAT32_1, 12 },
	{ "id", Be::USHORT_1, 16 },
	{ "interpolation", Be::UBYTE_1, 18, Tr2CurveInterpolationChooser },
	{ "tangentType", Be::UBYTE_1, 19, Tr2CurveTangentTypeChooser },
	{ 0 }
};

const Tr2CurveScalarKey s_defaultKey = { 0.f, 0.f, 0.f, 0.f, 0, Tr2CurveInterpolation::HERMITE, Tr2CurveTangentType::AUTO_CLAMP };

const float EPSILON = 1.e-6f;

// --------------------------------------------------------------------------------
float GetAutoTangent( float prevTime, float prevValue, float time, float value, float nextTime, float nextValue )
{
	float left = 0;
	if( time - prevTime > EPSILON )
	{
		left = ( value - prevValue ) / ( time - prevTime );
	}
	float right = 0;
	if( nextTime - time > EPSILON )
	{
		right = ( nextValue - value ) / ( nextTime - time );
	}

	float x = ( time - prevTime ) / ( nextTime - prevTime );
	return left * ( 1 - x ) + right * x;
}

// --------------------------------------------------------------------------------
float GetAutoClamppedTangent( float prevTime, float prevValue, float time, float value, float nextTime, float nextValue )
{
	if( ( value < prevValue && value < nextValue ) || ( value > prevValue && value > nextValue ) )
	{
		return 0;
	}

	float valueDiff = std::abs( prevValue - nextValue );

	if( valueDiff == 0 )
	{
		return 0;
	}

	float keyDistance = std::abs( value - prevValue ) / valueDiff;
	keyDistance = std::min( 1.f, std::min( keyDistance, 1 - keyDistance ) * 6 );

	float autoTangent = ( nextValue - prevValue ) / ( nextTime - prevTime );

	return autoTangent * keyDistance;
}

}

// --------------------------------------------------------------------------------
Tr2CurveScalar::Tr2CurveScalar( IRoot* lockobj )
	:PARENTLOCK( m_keys ),
	m_timeOffset( 0.f ),
	m_timeScale( 1.f ),
	m_currentValue( 0.f ),
	m_lastSegment( 0 ),
	m_extrapolationBefore( Tr2CurveExtrapolation::CLAMP ),
	m_extrapolationAfter( Tr2CurveExtrapolation::CLAMP )
{
	m_keys.SetStructureDefinition( Tr2CurveScalarKeyDef );
	m_keys.SetDefaultValue( &s_defaultKey );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveScalar::GetName() const
{
	return m_name;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::SetName( const char* name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::Update( Be::Time time )
{
	return m_currentValue = GetValue( TimeAsDouble( time ) );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::Update( double time )
{
	return m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetValueAt( Be::Time time )
{
	return GetValue( TimeAsDouble( time ) );
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::ScaleTime( float s )
{
	m_timeScale = s;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetValueAt( double time )
{
	return GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetValue( double time ) const
{
	if( m_keys.empty() )
	{
		return 0;
	}

	auto count = m_keys.size();

	if( m_extrapolationBefore == Tr2CurveExtrapolation::LINEAR || m_extrapolationAfter == Tr2CurveExtrapolation::LINEAR )
	{
		float t = float( time / (double)m_timeScale - (double)m_timeOffset );

		if( m_extrapolationBefore == Tr2CurveExtrapolation::LINEAR && t < m_keys[0].m_time )
		{
			return m_keys[0].m_value - ( m_keys[0].m_time - t ) * m_keys[0].m_leftTangent;
		}
		else if( m_extrapolationAfter == Tr2CurveExtrapolation::LINEAR && t > m_keys[count - 1].m_time )
		{
			return m_keys[count - 1].m_value + ( t - m_keys[count - 1].m_time ) * m_keys[count - 1].m_rightTangent;
		}
	}

	if( count == 1 )
	{
		return m_keys[0].m_value;
	}

	if( m_extrapolationBefore == Tr2CurveExtrapolation::CLAMP && float( time / (double)m_timeScale - (double)m_timeOffset ) <= m_keys[0].m_time )
	{
		return m_keys[0].m_value;
	}

	if( m_extrapolationAfter == Tr2CurveExtrapolation::CLAMP && float( time / (double)m_timeScale - (double)m_timeOffset ) >= m_keys[m_keys.size() - 1].m_time )
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
float Tr2CurveScalar::GetTangent( double time ) const
{
	if ( m_keys.empty() )
	{
		return 0;
	}

	auto count = m_keys.size();

	if ( m_extrapolationBefore == Tr2CurveExtrapolation::LINEAR || m_extrapolationAfter == Tr2CurveExtrapolation::LINEAR )
	{
		float t = float( time / (double)m_timeScale - (double)m_timeOffset );

		if ( m_extrapolationBefore == Tr2CurveExtrapolation::LINEAR && t < m_keys[0].m_time )
		{
			return m_keys[0].m_leftTangent;
		}
		else if ( m_extrapolationAfter == Tr2CurveExtrapolation::LINEAR && t > m_keys[count - 1].m_time )
		{
			return m_keys[count - 1].m_rightTangent;
		}
	}

	if ( count == 1 )
	{
		return m_keys[0].m_rightTangent;
	}

	if ( m_extrapolationBefore == Tr2CurveExtrapolation::CLAMP && float( time / (double)m_timeScale - (double)m_timeOffset ) <= m_keys[0].m_time )
	{
		return 0;
	}

	if ( m_extrapolationAfter == Tr2CurveExtrapolation::CLAMP && float( time / (double)m_timeScale - (double)m_timeOffset ) >= m_keys[m_keys.size() - 1].m_time )
	{
		return 0;
	}

	float t = GetLocalTime( time );

	for ( size_t i = 0; i + 1 < count; ++i )
	{
		auto& k0 = m_keys[i];
		auto& k1 = m_keys[i + 1];

		if ( t >= k0.m_time && t < k1.m_time )
		{
			return GetSegmentTangent( t, k0, k1 );
		}
	}
	return GetSegmentTangent( t, m_keys[count - 2], m_keys[count - 1] );
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::OnKeysChanged()
{
	std::stable_sort( 
		m_keys.begin(), 
		m_keys.end(), 
		[]( const Tr2CurveScalarKey& k0, const Tr2CurveScalarKey& k1 ) { return k0.m_time < k1.m_time; } );

	for( size_t i = 0; i < m_keys.size(); ++i )
	{
		auto& k = m_keys[i];
		switch( k.m_tangentType )
		{
		case Tr2CurveTangentType::AUTO_CLAMP:
			if( i == 0 )
			{
				k.m_leftTangent = k.m_rightTangent = 0;
			}
			else if( i + 1 == m_keys.size() )
			{
				k.m_leftTangent = k.m_rightTangent = 0;
			}
			else
			{
				float tangent = GetAutoClamppedTangent( 
					m_keys[i - 1].m_time, 
					m_keys[i - 1].m_value, 
					k.m_time, 
					k.m_value, m_keys
					[i + 1].m_time, 
					m_keys[i + 1].m_value );

				k.m_leftTangent = tangent;
				k.m_rightTangent = tangent;
			}
			break;
		case Tr2CurveTangentType::AUTO:
			if( i == 0 )
			{
				k.m_leftTangent = k.m_rightTangent = 0;
			}
			else if( i + 1 == m_keys.size() )
			{
				k.m_leftTangent = k.m_rightTangent = 0;
			}
			else
			{
				float tangent = GetAutoTangent( 
					m_keys[i - 1].m_time, 
					m_keys[i - 1].m_value, 
					k.m_time, k.m_value, 
					m_keys[i + 1].m_time, 
					m_keys[i + 1].m_value );

				k.m_leftTangent = tangent;
				k.m_rightTangent = tangent;
			}
			break;
		case Tr2CurveTangentType::FREE_JOINED:
			k.m_rightTangent = k.m_leftTangent;
			break;
		}
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::Length()
{
	if( m_keys.empty() )
	{
		return 0;
	}
	return m_keys[m_keys.size() - 1].m_time;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetLocalTime( double time ) const
{
	if( m_keys.empty() )
	{
		return 0;
	}

	time = time / (double)m_timeScale - (double)m_timeOffset;

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
	if( time <= last )
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
float Tr2CurveScalar::GetSegmentValue( float time, const Tr2CurveScalarKey& k0, const Tr2CurveScalarKey& k1 ) const
{
	switch( k0.m_interpolation )
	{
	case Tr2CurveInterpolation::CONSTANT:
		return time == k1.m_time ? k1.m_value : k0.m_value;
	case Tr2CurveInterpolation::LINEAR:
		if( k1.m_time == k0.m_time )
		{
			return k1.m_value;
		}
		return k0.m_value + ( k1.m_value - k0.m_value ) * ( time - k0.m_time ) / ( k1.m_time - k0.m_time );
	case Tr2CurveInterpolation::HERMITE:
	{
		float length = k1.m_time - k0.m_time;
		if( length == 0 )
		{
			return k1.m_value;
		}
		float inTangent = k0.m_rightTangent * length;
		float outTangent = k1.m_leftTangent * length;

		float s = ( time - k0.m_time ) / length;
		float s2 = s * s;
		float s3 = s2 * s;

		float c2 = -2.0f * s3 + 3.0f * s2;
		float c1 = 1.0f - c2;
		float c4 = s3 - s2;
		float c3 = s + c4 - s2;

		return k0.m_value * c1 + k1.m_value * c2 + inTangent * c3 + outTangent * c4;
	}
	default:
		return 0;
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetSegmentTangent( float time, const Tr2CurveScalarKey& k0, const Tr2CurveScalarKey& k1 ) const
{
	switch ( k0.m_interpolation )
	{
	case Tr2CurveInterpolation::CONSTANT:
		return 0;
	case Tr2CurveInterpolation::LINEAR:
		return ( k1.m_value - k0.m_value ) / ( k1.m_time - k0.m_time );
	case Tr2CurveInterpolation::HERMITE:
	{
		float length = k1.m_time - k0.m_time;
		if ( length == 0 )
		{
			return k1.m_rightTangent;
		}
		float inTangent = k0.m_rightTangent * length;
		float outTangent = k1.m_leftTangent * length;

		float s = ( time - k0.m_time ) / length;
		float s2 = s * s;

		float m2s = 2.0f * s;

		float c1 = 6.0f * s2 - 6.0f * s;
		float c2 = -c1;
		float c4 = 3.0f * s2 - m2s;
		float c3 = c4 - m2s + 1;

		return ( k0.m_value * c1 + k1.m_value * c2 + inTangent * c3 + outTangent * c4 ) / length;
	}
	default:
		return 0;
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetTimeOffset() const
{
	return m_timeOffset;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::SetTimeOffset( float timeOffset )
{
	m_timeOffset = timeOffset;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetTimeScale() const
{
	return m_timeScale;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::SetTimeScale( float timeScale )
{
	m_timeScale = timeScale;
}

// --------------------------------------------------------------------------------
bool Tr2CurveScalar::IsEmpty() const
{
	return m_keys.empty();
}

// --------------------------------------------------------------------------------
float Tr2CurveScalar::GetCurrentValue() const
{
	return m_currentValue;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::AddKey(
	float time,
	float value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
	float leftTangent,
	float rightTangent,
	Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType )
{
	Tr2CurveScalarKey key;
	key.m_time = time;
	key.m_value = value;
	key.m_leftTangent = leftTangent;
	key.m_rightTangent = rightTangent;
	key.m_interpolation = interpolation;
	key.m_tangentType = tangentType;
	key.m_id = 0;

	m_keys.Append( &key );

	OnKeysChanged();
}

// --------------------------------------------------------------------------------
void Tr2CurveScalar::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_extrapolationAfter = m_extrapolationBefore = extrapolation;
}

// --------------------------------------------------------------------------------
Tr2CurveScalarKeyStructureList& Tr2CurveScalar::GetKeys()
{
	return m_keys;
}