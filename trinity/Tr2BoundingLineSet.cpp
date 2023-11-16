#include "StdAfx.h"
#include "Tr2BoundingLineSet.h"

Tr2BoundingLineSet::Tr2BoundingLineSet( IRoot* lockobj ):
	Tr2LineSet( lockobj ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f )
{
}

/////////////////////////////////////////////////////////////////////////////
// INotify
bool Tr2BoundingLineSet::OnModified( Be::Var * value )
{
	if( IsMatch( value, m_minBounds ) || IsMatch( value, m_maxBounds ) )
	{
		ClearLines();
		ClearPickingTriangles();
		AddBox( m_minBounds, m_maxBounds, (Vector4)m_color );
		AddPickingBox( m_minBounds, m_maxBounds );
		SubmitChanges();
	}

    return Tr2PrimitiveSet::OnModified( value );
}

void Tr2BoundingLineSet::UpdateBounds( const Vector3& min, const Vector3& max )
{
	m_minBounds = min;
	m_maxBounds = max;
	// update the lines
	ClearLines();
	ClearPickingTriangles();
	AddBox( m_minBounds, m_maxBounds, (Vector4)m_color );
	AddPickingBox( m_minBounds, m_maxBounds );
	SubmitChanges();
}

void Tr2BoundingLineSet::AddBox( const Vector3& min, const Vector3& max, const Vector4& color )
{
	Vector3 minA(max.x, min.y, min.z);
	Vector3 minB(min.x, max.y, min.z); 
	Vector3	minC(max.x, max.y, min.z); 
	Vector3	maxA(max.x, min.y, max.z); 
	Vector3	maxB(min.x, max.y, max.z); 
	Vector3	maxC(min.x, min.y, max.z);

	AddLine( min, color, minA, color );
	AddLine( min, color, minB, color );
	AddLine( minC, color, minB, color );
	AddLine( minA, color, minC, color );

	AddLine( max, color, maxA, color );
	AddLine( max, color, maxB, color );
	AddLine( maxC, color, maxB, color );
	AddLine( maxA, color, maxC, color );

	AddLine( min, color, maxC, color );
	AddLine( max, color, minC, color );
	AddLine( minB, color, maxB, color );
	AddLine( minA, color, maxA, color );
}

void Tr2BoundingLineSet::AddPickingBox( const Vector3& min, const Vector3& max )
{
	Vector3 minA(max.x, min.y, min.z);
	Vector3 minB(min.x, max.y, min.z); 
	Vector3	minC(max.x, max.y, min.z); 
	Vector3	maxA(max.x, min.y, max.z); 
	Vector3	maxB(min.x, max.y, max.z); 
	Vector3	maxC(min.x, min.y, max.z);

	AddPickingTriangle( maxA, max, maxB );
	AddPickingTriangle( maxA, maxB, maxC);
	AddPickingTriangle( maxC, maxB, min );
	AddPickingTriangle( min, maxB, minB );
	AddPickingTriangle( min, minB, minA );
	AddPickingTriangle( minA, minB, minC);
	AddPickingTriangle( minA, minC, max );
	AddPickingTriangle( minA, max, maxA );
	AddPickingTriangle( maxA, min, minA );
	AddPickingTriangle( maxA, maxC, min );
	AddPickingTriangle( max, minC, minB );
	AddPickingTriangle( max, minB, maxB );
}