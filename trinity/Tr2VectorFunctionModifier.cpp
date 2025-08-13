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
		m_clientBall->Update( in, time );
	}
	return GetTransformedPosition( in );
}

Vector3* Tr2VectorFunctionModifier::Update( Vector3* in, double time )
{
	if( m_clientBall )
	{
		m_clientBall->Update( in, time );
	}
	return GetTransformedPosition( in );
}

Vector3* Tr2VectorFunctionModifier::GetValueAt( Vector3* in, Be::Time time )
{
	if( m_clientBall )
	{
		m_clientBall->GetValueAt( in, time );
	}
	return GetTransformedPosition( in );
}

Vector3* Tr2VectorFunctionModifier::GetValueAt( Vector3* in, double time )
{
	if( m_clientBall )
	{
		m_clientBall->GetValueAt( in, time );
	}
	return GetTransformedPosition( in );
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
	if( m_clientBall )
	{
		m_clientBall->GetValueDotAt( in, time );
		*in *= m_scaleModifier;
	}
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
	if( m_clientBall )
	{
		m_clientBall->GetValueDoubleDotAt( in, time );
		*in *= m_scaleModifier;
	}
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
