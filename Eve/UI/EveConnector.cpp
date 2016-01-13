////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveConnector.h"
#include "Eve/EveCurveLineSet.h"

EveConnector::EveConnector( IRoot* lockobj ) :
	m_animationColor( 1.f, 0.f, 0.f, 1.f ),
	m_color( 0.5f, 0.5f, 0.5f, 1.f ),
	m_destPosition( 0.f, 0.f, 0.f ),
	m_sourcePosition( 0.f, 0.f, 0.f ),
	m_animationScale( 1.f ),
	m_animationSpeed( 0.f ),
	m_width( 1.f ),
	m_isAnimated( false )
{
}

EveConnector::~EveConnector()
{
}

void EveConnector::Update( EveUpdateContext& context )
{
	if( m_sourceObject )
	{
		m_sourceObject->GetValueAt( &m_sourcePosition, context.GetTime() );
	}
	
	if( m_destObject )
	{
		m_destObject->GetValueAt( &m_destPosition, context.GetTime() );
	}
}

void EveConnector::AddLine( EveCurveLineSet* lineSet )
{
	int id = lineSet->AddStraightLine( m_sourcePosition, (Vector4)m_color, m_destPosition, (Vector4)m_color, m_width );
	if( m_isAnimated )
	{
		// Scale the animation by length so we get a more consistent size/speed
		// For it to look good we will need to do some falloff scaling in the shader as well
		Vector3 d = m_sourcePosition - m_destPosition;
		float length = D3DXVec3Length( &d );
		lineSet->ChangeLineAnimation( id, (Vector4)m_animationColor, m_animationSpeed / length, length / m_animationScale );
	}
}