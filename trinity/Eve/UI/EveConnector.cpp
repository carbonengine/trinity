////////////////////////////////////////////////////////////
//
//    Created:   2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "include/TriMath.h"
#include "EveConnector.h"
#include "EveCurveLineSet.h"

EveConnector::EveConnector( IRoot* lockobj ) :
	m_type( PointToPoint ),
	m_animationColor( 1.f, 0.f, 0.f, 1.f ),
	m_color( 0.5f, 0.5f, 0.5f, 1.f ),
	m_destPosition( 0.f, 0.f, 0.f ),
	m_sourcePosition( 0.f, 0.f, 0.f ),
	m_normal( 0.f, 1.f, 0.f ),
	m_length( 0.f ),
	m_animationScale( 1.f ),
	m_animationSpeed( 0.f ),
	m_width( 1.f ),
	m_lineLength( 1.f ),
	m_isAnimated( false ),
	m_autoScaleAnimation( false )
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
	float length, angle;
	Vector3 v, v2, v3;
	Vector3 n( 0, 1, 0 );
	bool fade = false;
	// Currently we assume we always project onto the x,0,z plane. This may change later on, in that case using sourcePosition
	// as a point in the plane and for the relative position
	switch( m_type )
	{
	case StraightAnchor:
		v = TriVectorProjectOnPlane( m_destPosition, m_sourcePosition, n );
		m_lineLength = Length( v - m_destPosition );
		AddStraightLine( lineSet, m_destPosition, v );
		break;
	case CurvedAnchor:
		v = TriVectorRotateToPlane( m_destPosition, m_sourcePosition, n );
		v2 = m_destPosition - m_sourcePosition;
		v3 = v - m_sourcePosition;
		length = Length( v2 );
		angle = acos( Dot( Normalize( v2 ), Normalize( v3 ) ) );
		m_lineLength = length * angle;
		AddSpheredSegment( lineSet, m_destPosition, v, m_sourcePosition );
		break;
	case XZ_Circle:
		v = m_destPosition - m_sourcePosition;
		length = Length( v );
		m_lineLength = TRI_PI * length * 0.5f;
		AddCircle( lineSet, m_sourcePosition, length );
		break;
	case XZ_CircleStraight:
		v = TriVectorProjectOnPlane( m_destPosition, m_sourcePosition, n );
		v = v - m_sourcePosition;
		length = Length( v );
		m_lineLength = TRI_PI * length * 0.5f;
		AddCircle( lineSet, m_sourcePosition, length );
		break;
	case PointToPoint:
		v = m_destPosition - m_sourcePosition;
		m_lineLength = Length( v );
		if( m_length && m_lineLength > m_length )
		{
			v = Normalize( v ) * m_length;
			v += m_sourcePosition;
			fade = true;
		}
		else
		{
			v = m_destPosition;
		}
		AddStraightLine( lineSet, m_sourcePosition, v, fade );
		break;
	case Orbit:
		AddOrbit( lineSet, m_destPosition, m_length, m_normal );
		break;
	default:
		break;
	};
}

inline void EveConnector::AnimateSegment( EveCurveLineSet* lineSet, int lineID )
{
	if( m_isAnimated )
	{
		if( m_autoScaleAnimation )
		{
			float speed = m_animationSpeed;
			if( m_lineLength != 0 )
			{
				speed = m_animationSpeed / m_lineLength;
			}
			lineSet->ChangeLineAnimation( lineID, (Vector4)m_animationColor, speed, m_lineLength * m_animationScale );
		}
		else
		{
			lineSet->ChangeLineAnimation( lineID, (Vector4)m_animationColor, m_animationSpeed, m_animationScale );
		}
	}
}

inline void EveConnector::AddCircle( EveCurveLineSet* lineSet, const Vector3& center, float radius )
{
	AddSpheredSegment( lineSet, center + Vector3( 0, 0, radius ), center + Vector3( radius, 0, 0 ), center );
	AddSpheredSegment( lineSet, center + Vector3( radius, 0, 0 ), center + Vector3( 0, 0, -radius ), center );
	AddSpheredSegment( lineSet, center + Vector3( 0, 0, -radius ), center + Vector3( -radius, 0, 0 ), center );
	AddSpheredSegment( lineSet, center + Vector3( -radius, 0, 0 ), center + Vector3( 0, 0, radius ), center );
}

inline void EveConnector::AddOrbit( EveCurveLineSet* lineSet, const Vector3& center, float radius, const Vector3& planeNormal )
{
	Vector3 side( 1, 0, 0 ), front( 0, 0, -1 ), up( 0, 1, 0 ), upDir;

	// Draw the orbit circle
	upDir = Normalize( planeNormal );
	if( std::abs( Dot( upDir, up ) ) < 0.999 )
	{
		side = Normalize( Cross( up, upDir ) );
		front = Normalize( Cross( side, upDir ) );
	}
	side *= radius;
	front *= radius;

	AddSpheredSegment( lineSet, center + front, center + side, center );
	AddSpheredSegment( lineSet, center + side, center - front, center );
	AddSpheredSegment( lineSet, center - front, center - side, center );
	AddSpheredSegment( lineSet, center - side, center + front, center );

	// And a line to the orbit
	Vector3 planeDir = center - m_sourcePosition;
	float d = Dot( upDir, planeDir );
	planeDir = m_sourcePosition + upDir * d;
	planeDir = Normalize( planeDir - center );
	planeDir = planeDir * radius + center;

	AddStraightLine( lineSet, m_sourcePosition, planeDir );
}

inline void EveConnector::AddStraightLine( EveCurveLineSet* lineSet, const Vector3& source, const Vector3& destination, bool fadeEnd )
{
	Vector4 endColor = (Vector4)m_color;
	if( fadeEnd )
	{
		endColor *= 0;
	}
	int id = lineSet->AddStraightLine( source, (Vector4)m_color, destination, endColor, m_width );
	AnimateSegment( lineSet, id );

}

inline void EveConnector::AddSpheredSegment( EveCurveLineSet* lineSet, const Vector3& p0, const Vector3& p1, const Vector3& center )
{
	int id = lineSet->AddSpheredLineCrt( p0, (Vector4)m_color, p1, (Vector4)m_color, center, m_width );
	AnimateSegment( lineSet, id );
}