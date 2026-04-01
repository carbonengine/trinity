//////////////////////////////////////////////////////////////////////////
//
// Created: April 2025
// Copyright CCP 2025
//
#include "Tr2VectorFunctionModifier.h"
#include "Tr2Renderer.h"
#include "Utilities/Vector3d.h"


Tr2VectorFunctionModifier::Tr2VectorFunctionModifier( IRoot* lockobj ) :
	m_offsetPosition( 0.0f, 0.0f, 0.0f ),
	m_scaleModifier( 1.0f ),
	m_useViewSpace( false )
{
}

Vector3* Tr2VectorFunctionModifier::Update( Vector3* in, Be::Time time )
{
	if( m_clientBall )
	{
		if( m_useSystemCoordinates )
		{
			Vector3d systemPosition;
			m_clientBall->InterpolatedPosition( &systemPosition, time );
			// potential data loss moving from double to float
			*in = ToVector3( systemPosition );
		}
		else
		{
			m_clientBall->Update( in, time );
		}
	}
	return GetTransformedPosition( in );
}

Vector3* Tr2VectorFunctionModifier::Update( Vector3* in, double time )
{
	CCP_ASSERT_M( true, "double version of GetValueDoubleDotAt is depricated." );
	return in;
}

Vector3* Tr2VectorFunctionModifier::GetValueAt( Vector3* in, Be::Time time )
{
	if( m_clientBall )
	{
		if( m_useSystemCoordinates )
		{
			Vector3d systemPosition;
			m_clientBall->InterpolatedPosition( &systemPosition, time );
			// potential data loss moving from double to float
			*in = ToVector3( systemPosition );
		}
		else
		{
			m_clientBall->GetValueAt( in, time );
		}
	}
	return GetTransformedPosition( in );
}

Vector3* Tr2VectorFunctionModifier::GetValueAt( Vector3* in, double time )
{
	CCP_ASSERT_M( true, "double version of GetValueDoubleDotAt is depricated." );
	return in;
}

Vector3* Tr2VectorFunctionModifier::GetValueDotAt( Vector3* in, Be::Time time )
{
	if( m_clientBall )
	{
		m_clientBall->GetValueDotAt( in, time );
		*in *= m_scaleModifier;
	}
	return in;
}

Vector3* Tr2VectorFunctionModifier::GetValueDotAt( Vector3* in, double time )
{
	CCP_ASSERT_M( true, "double version of GetValueDoubleDotAt is depricated." );
	return in;
}

Vector3* Tr2VectorFunctionModifier::GetValueDoubleDotAt( Vector3* in, Be::Time time )
{
	if( m_clientBall )
	{
		m_clientBall->GetValueDoubleDotAt( in, time );
		*in *= m_scaleModifier;
	}
	return in;
}

Vector3* Tr2VectorFunctionModifier::GetValueDoubleDotAt( Vector3* in, double time )
{
	CCP_ASSERT_M( true, "double version of GetValueDoubleDotAt is depricated." );
	return in;
}

Vector3d* Tr2VectorFunctionModifier::InterpolatedPosition( Vector3d* out, Be::Time time )
{
	if( m_clientBall )
	{
		m_clientBall->InterpolatedPosition( out, time );
	}
	return out;
}

Vector3* Tr2VectorFunctionModifier::GetTransformedPosition( Vector3* in ) const
{
	Vector3 offset = GetOffsetPosition();
	*in = ( *in + offset ) * m_scaleModifier;
	return in;
}

Vector3 Tr2VectorFunctionModifier::GetOffsetPosition() const
{
	if( !m_useViewSpace )
	{
		return m_offsetPosition;
	}

	return Transform( Vector4( m_offsetPosition, 0 ), Tr2Renderer::GetInverseViewTransform() ).GetXYZ();
}

Vector3 Tr2VectorFunctionModifier::ToVector3( Vector3d in )
{
	return Vector3( 
		CheckedDoubleToFloat( in.x ), 
		CheckedDoubleToFloat( in.y ), 
		CheckedDoubleToFloat( in.z )
	);
}

float Tr2VectorFunctionModifier::CheckedDoubleToFloat( double value )
{
	const double maxFloat = static_cast<double>( std::numeric_limits<float>::max() );
	CCP_ASSERT_M( std::isfinite( value ) && value < maxFloat && value > -maxFloat, "System coordinate overflow converting double to float" );

	return static_cast<float>( value );
}
