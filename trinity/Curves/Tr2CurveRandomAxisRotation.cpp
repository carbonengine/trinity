////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveRandomAxisRotation.h"
#include <random>

namespace
{
	float RandAngle( std::default_random_engine& engine )
	{
		return float( engine() - engine.min() ) / ( engine.max() - engine.min() ) * XM_PI * 2;
	}
}

// --------------------------------------------------------------------------------
Tr2CurveRandomAxisRotation::Tr2CurveRandomAxisRotation( IRoot* lockobj )
	:m_seed( 0 ),
	m_period( 1.f )
{
	SeedChanged();
	m_currentValue = GetValue( 0 );
}

// --------------------------------------------------------------------------------
void Tr2CurveRandomAxisRotation::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::Update( Quaternion* in, Be::Time time )
{
	auto period = TimeFromDouble( std::abs( m_period ) );
	m_currentValue = GetValue( TimeAsDouble( time % period ) );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::Update( Quaternion* in, double time )
{
	m_currentValue = GetValue( time );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueAt( Quaternion* in, Be::Time time )
{
	auto period = TimeFromDouble( std::abs( m_period ) );
	*in = GetValue( TimeAsDouble( time % period ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueAt( Quaternion* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDoubleDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveRandomAxisRotation::GetValueDoubleDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveRandomAxisRotation::GetValue( double time ) const
{
	Quaternion result = m_postRotation;
	if( m_period != 0 )
	{
		result *= RotationQuaternion( 0, float( time / double( std::abs( m_period ) ) ) * XM_2PI, 0 );
	}
	result *= m_preRotation;
	return result;
}

// --------------------------------------------------------------------------------
bool Tr2CurveRandomAxisRotation::Initialize()
{
	if( m_seed != 0 )
	{
		SeedChanged();
	}
	return true;
}

// --------------------------------------------------------------------------------
uint32_t Tr2CurveRandomAxisRotation::GetSeed() const
{
	return m_seed;
}

// --------------------------------------------------------------------------------
void Tr2CurveRandomAxisRotation::SetSeed( uint32_t seed )
{
	m_seed = seed;
	SeedChanged();
}

// --------------------------------------------------------------------------------
void Tr2CurveRandomAxisRotation::SeedChanged()
{
	auto engine = std::default_random_engine();
	if( m_seed != 0 )
	{
		engine.seed( std::default_random_engine::result_type( m_seed ) );
	}
	else
	{
		engine.seed( std::default_random_engine::result_type( BeOS->GetActualTime() ) );
	}
	m_preRotation = RotationQuaternion( RandAngle( engine ), RandAngle( engine ), RandAngle( engine ) );
	m_postRotation = RotationQuaternion( RandAngle( engine ), RandAngle( engine ), RandAngle( engine ) );
}
