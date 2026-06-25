// Copyright © 2016 CCP ehf.

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

void EveConnector::Update( const EveUpdateContext& context )
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
	float radiusX = m_destPosition.x;
	float radiusY = m_destPosition.y;
	float rotation = m_destPosition.z;
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
	case Circle:
		AddCircle( lineSet, m_sourcePosition, m_length, m_normal );
		break;
	case Ellipse:
		AddEllipse( lineSet, m_sourcePosition, radiusX, radiusY, rotation, m_normal );
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

inline void EveConnector::AddCircle( EveCurveLineSet* lineSet, const Vector3& center, float radius, const Vector3& planeNormal )
{
	Vector3 side, front;
	CalculateSideAndFront( planeNormal, side, front );
	side *= radius;
	front *= radius;

	AddSpheredSegment( lineSet, center + front, center + side, center );
	AddSpheredSegment( lineSet, center + side, center - front, center );
	AddSpheredSegment( lineSet, center - front, center - side, center );
	AddSpheredSegment( lineSet, center - side, center + front, center );
}

inline void EveConnector::AddOrbit( EveCurveLineSet* lineSet, const Vector3& center, float radius, const Vector3& planeNormal )
{
	Vector3 side, front;
	Vector3 upDir = Normalize( planeNormal );
	CalculateSideAndFront( upDir, side, front );
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

inline void EveConnector::AddEllipse( EveCurveLineSet* lineSet, const Vector3& center, float radiusX, float radiusY, float rotation, const Vector3& normal )
{
	Vector3 side, front;
	CalculateSideAndFront( normal, side, front );

	float rotationRad = rotation * TRI_PI / 180.0f;
	float cosRot = cos( rotationRad );
	float sinRot = sin( rotationRad );

	Vector3 rotatedSide = side * cosRot + front * sinRot;
	Vector3 rotatedFront = -side * sinRot + front * cosRot;

	const int numSegments = 32;
	const float angleStep = 2.0f * TRI_PI / numSegments;

	// Generate points along the ellipse
	for( int i = 0; i < numSegments; i++ )
	{
		float t1 = i * angleStep;
		float t2 = ( i + 1 ) * angleStep;
		float tMid = ( t1 + t2 ) * 0.5f;

		Vector3 p1 = center + ( rotatedSide * cos( t1 ) * radiusX ) + ( rotatedFront * sin( t1 ) * radiusY );
		Vector3 p2 = center + ( rotatedSide * cos( t2 ) * radiusX ) + ( rotatedFront * sin( t2 ) * radiusY );
		// magic number 1.01 is to make sure the line is not too jagged
		Vector3 middle = center + ( ( rotatedSide * cos( tMid ) * radiusX ) + ( rotatedFront * sin( tMid ) * radiusY ) ) * 1.01;

		// default of 20 was overkill for an ellipse with this many curved line segments
		int lineSegments = 5;
		AddCurvedLine( lineSet, p1, p2, middle, lineSegments );
	}
}

inline void EveConnector::AddCurvedLine( EveCurveLineSet* lineSet, const Vector3& point1, const Vector3& point2, const Vector3& middle, int segments )
{

	int lineId = lineSet->AddCurvedLineCrt( point1, (Vector4)m_color, point2, (Vector4)m_color, middle, m_width, segments );

	AnimateSegment( lineSet, lineId );
}

void EveConnector::CalculateSideAndFront( const Vector3& upDir, Vector3& outSide, Vector3& outFront )
{
	Vector3 up( 0, 1, 0 );
	Vector3 upDirection = Normalize( upDir );
	if( std::abs( Dot( upDirection, up ) ) < 0.999 )
	{
		outSide = Normalize( Cross( up, upDirection ) );
		outFront = Normalize( Cross( outSide, upDirection ) );
	}
	else
	{
		Vector3 altUp( 1, 0, 0 );
		outSide = Normalize( Cross( altUp, upDirection ) );
		outFront = Normalize( Cross( outSide, upDirection ) );
	}
}
