// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ManipulationTool.h"
#include "TriViewport.h"
#include "Tr2PrimitiveSet.h"

const Vector4 v4Red( 1.0f, 0.01f, 0.01f, 1.0f );
const Vector4 v4Green( 0.01f, 1.0f, 0.01f, 1.0f );
const Vector4 v4Blue( 0.01f, 0.01f, 1.0f, 1.0f );
const Vector4 v4Cyan( 0.0f, 1.0f, 1.0f, 1.0f );
const Vector4 v4Yellow( 1.0f, 1.0f, 0.01f, 1.0f );
const Vector4 v4LightGray( 0.5f, 0.5f, 0.5f, 1.0f );
const Vector3 v3Zero( 0.0f, 0.0f, 0.0f );
const Vector3 v3X( 1.0f, 0.0f, 0.0f );
const Vector3 v3Y( 0.0f, 1.0f, 0.0f );
const Vector3 v3Z( 0.0f, 0.0f, 1.0f );

Tr2ManipulationTool::Tr2ManipulationTool( IRoot* lockobj ) :
	PARENTLOCK( m_primitives ),
#if BLUE_WITH_PYTHON
	m_pythonUserData( NULL ),
#endif
	m_captured( false ),
	m_moved( false ),
	m_localTransform( IdentityMatrix() ),
	m_worldTransform( IdentityMatrix() )
{
	m_pivot.x = 0.0f;
	m_pivot.y = 0.0f;
	m_pivot.z = 0.0f;
	m_movement.x = 0.0f;
	m_movement.y = 0.0f;
	m_movement.z = 0.0f;
}

Tr2ManipulationTool::~Tr2ManipulationTool()
{
}

bool Tr2ManipulationTool::OnMoveCallback( Matrix& currentTransform, Matrix& nextTransform )
{
	bool returnValue = true;

	if( m_callback )
	{
		// Pass in the current and next position of the tools so the
		// user can choose if that is a legal move
		m_callback.Call( returnValue, currentTransform, nextTransform );
	}
	return returnValue;
}

void Tr2ManipulationTool::GetBaseVectors( Vector3& x, Vector3& y, Vector3& z )
{
	x = Normalize( Vector3( m_localTransform._11, m_localTransform._12, m_localTransform._13 ) );
	y = Normalize( Vector3( m_localTransform._21, m_localTransform._22, m_localTransform._23 ) );
	z = Normalize( Vector3( m_localTransform._31, m_localTransform._32, m_localTransform._33 ) );
}

Vector3 Tr2ManipulationTool::GetDesiredPlaneNormal( Vector3& ray, Matrix& viewMatrix )
{
	// What is the best plane to work on when moving the tool
	// Since the eye could lie on one of the valid planes
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;
	GetBaseVectors( xAxis, yAxis, zAxis );
	if( m_selectedAxis == "x" )
	{
		// We dont care on wich side we are.. just the distance
		float ydot = fabs( Dot( ray, yAxis ) ); // XZ plane
		float zdot = fabs( Dot( ray, zAxis ) ); // XY plane

		if( zdot > ydot )
		{
			return zAxis;
		}
		else
		{
			return yAxis;
		}
	}
	else if( m_selectedAxis == "y" )
	{
		float zdot = fabs( Dot( ray, zAxis ) ); // YX plane
		float xdot = fabs( Dot( ray, xAxis ) ); // YZ plane

		if( zdot > xdot )
		{
			return zAxis;
		}
		else
		{
			return xAxis;
		}
	}
	else if( m_selectedAxis == "z" )
	{
		float xdot = fabs( Dot( ray, xAxis ) ); // ZY plane
		float ydot = fabs( Dot( ray, yAxis ) ); // ZX plane

		if( xdot > ydot )
		{
			return xAxis;
		}
		else
		{
			return yAxis;
		}
	}
	else
	{
		Vector3 view;
		view.x = viewMatrix._13;
		view.y = viewMatrix._23;
		view.z = viewMatrix._33;
		return view;
	}
}

Vector3 Tr2ManipulationTool::MovePointOnPlane( int mx1, int my1, int mx2, int my2, Vector3& pointOnPlane, Vector3& planeNormal, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	// Tracking a point in 3d space based on the screen positions
	Vector3 ray, startpos;
	ScreenCoordinatesToRay( mx1, my1, ray, startpos, viewport, viewMatrix, projectionMatrix );
	Vector3 startPlanePos = RayToPlaneIntersection( startpos, ray, pointOnPlane, planeNormal );

	ScreenCoordinatesToRay( mx2, my2, ray, startpos, viewport, viewMatrix, projectionMatrix );
	Vector3 endPlanePos = RayToPlaneIntersection( startpos, ray, pointOnPlane, planeNormal );

	Vector3 result = endPlanePos - startPlanePos;
	return result;
}

int Tr2ManipulationTool::RayToSphereIntersection( Vector3& spCenter, float radius, Vector3& startPoint, Vector3& ray, Vector3& outValues )
{
	Vector3 pMinusC = startPoint - spCenter;
	float dotA = Dot( ray, ray );
	float dotB = 2.0f * Dot( ray, pMinusC );
	float dotC = Dot( pMinusC, pMinusC ) - ( radius * radius );
	float discrim = dotB * dotB - 4.0f * dotA * dotC;
	if( discrim > 0.0f )
	{
		outValues.x = ( -dotB + sqrt( discrim ) ) / ( 2.0f * dotA );
		outValues.y = ( -dotB - sqrt( discrim ) ) / ( 2.0f * dotA );
		return 2;
	}
	else if( discrim == 0.0f )
	{
		outValues.x = -dotB / ( 2.0f * dotA );
		outValues.y = 0.0f;
		return 1;
	}
	else
	{
		return 0;
	}
}

Vector3 Tr2ManipulationTool::RayToPlaneIntersection( Vector3& P, Vector3& d, Vector3& Q, Vector3& n )
{
	float denom = Dot( n, d );
	if( fabs( denom ) < FLT_EPSILON )
	{
		return P;
	}
	else
	{
		float distance = -Dot( Q, n );
		float t = -( Dot( n, P ) + distance ) / denom;
		Vector3 result = d * t + P;
		return result;
	}
}

void Tr2ManipulationTool::ScreenCoordinatesToRay( int x, int y, Vector3& outRay, Vector3& outStart, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	XMVECTOR det;
	XMMATRIX invP = XMMatrixInverse( &det, projectionMatrix );
	XMMATRIX invV = XMMatrixInverse( &det, viewMatrix );

	Vector3 start( 2.0f * float( x - viewport.m_x ) / viewport.m_width - 1.0f,
				   -2.0f * float( y - viewport.m_y ) / viewport.m_height + 1.0f,
				   0 );

	XMVECTOR vStart = XMVector3TransformCoord( start, invP );
	outStart = XMVector3TransformCoord( vStart, invV );


	Vector3 end = start;
	end.z = 0.5f;


	outRay = XMVector3TransformCoord( end, invP );
	outRay = XMVectorSubtract( outRay, vStart );
	outRay = XMVector3TransformNormal( outRay, invV );
	XMVector3Normalize( outRay );
}

void Tr2ManipulationTool::PointToScreenCoordinates( Vector3& inPoint, Vector2& outPoint, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	Vector4 out = Vector4( inPoint.x, inPoint.y, inPoint.z, 1 ) * viewMatrix;

	out = out * projectionMatrix;

	outPoint.x = viewport.m_x + viewport.m_width * ( 0.5f + 0.5f * out.x / out.w );
	outPoint.y = viewport.m_y + viewport.m_height * ( 0.5f - 0.5f * out.y / out.w );
}

void Tr2ManipulationTool::SelectAxis( std::string axisName )
{
	bool validName = false;
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		if( ( *it )->m_name == axisName )
		{
			validName = true;
		}
	}

	if( !validName )
		return;

	ResetPrimitiveColors();
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		if( ( *it )->m_name == axisName )
		{
			( *it )->SetCurrentColor( (Color&)v4Yellow );
		}
	}
	m_selectedAxis = axisName;
}

// Utility functions for creating geometry to either render or to act as a collision geometry
Vector3* Tr2ManipulationTool::GetTrianglesAroundLine( const Vector3& start, const Vector3& end, float radius, int* numVectors )
{
	Vector3* outTriangles = new Vector3[18];
	*numVectors = 18;
	float rad = XM_PI * 2.0f / 3.0f;
	Vector3 triangle1[3] = { Vector3( radius, 0.0f, 0.0f ),
							 Vector3( cos( rad ) * radius, sin( rad ) * radius, 0.0f ),
							 Vector3( cos( 2.0f * rad ) * radius, sin( 2.0f * rad ) * radius, 0.0f ) };

	Vector3 dirOfLine;
	Vector3 zDir( 0.0f, 0.0f, 1.0f );
	Quaternion rotation = IdentityQuaternion();
	Vector3 temp;
	dirOfLine = Normalize( start - end );
	float dot = Dot( zDir, dirOfLine );
	if( fabs( dot ) != 1.0f )
	{
		temp = Cross( zDir, dirOfLine );
		rotation = RotationQuaternion( temp, acos( dot ) );
	}

	Matrix rot, t1, t2, compA, compB;
	rot = RotationMatrix( rotation );
	t1 = TranslationMatrix( start );
	t2 = TranslationMatrix( end );

	compA = rot * t1;
	compB = rot * t2;

	Vector3 startTri[3];
	Vector3 endTri[3];

	TransformCoords( startTri, triangle1, triangle1 + 3, compA );
	TransformCoords( endTri, triangle1, triangle1 + 3, compB );

	outTriangles[0] = startTri[1];
	outTriangles[1] = endTri[0];
	outTriangles[2] = startTri[0];

	outTriangles[3] = endTri[1];
	outTriangles[4] = endTri[0];
	outTriangles[5] = startTri[1];

	outTriangles[6] = startTri[2];
	outTriangles[7] = endTri[1];
	outTriangles[8] = startTri[1];

	outTriangles[9] = endTri[2];
	outTriangles[10] = endTri[1];
	outTriangles[11] = startTri[2];

	outTriangles[12] = startTri[0];
	outTriangles[13] = endTri[2];
	outTriangles[14] = startTri[2];

	outTriangles[15] = endTri[0];
	outTriangles[16] = endTri[2];
	outTriangles[17] = startTri[0];

	return outTriangles;
}

Vector3* Tr2ManipulationTool::GetConeTriangles( float height, float width, int subd, int* numVectors )
{
	Vector3 top( 0.0f, height, 0.0f );
	float rad = XM_PI / ( subd * 0.5f );
	int numVerts = subd * 6;
	Vector3* triangles = new Vector3[numVerts];
	*numVectors = numVerts;
	for( int i = 0; i < subd; i++ )
	{
		int offset = 6 * i;
		triangles[offset] = top;
		triangles[offset + 1] = Vector3( cos( ( i + 1 ) * rad ) * width, 0.0f, sin( ( i + 1 ) * rad ) * width );
		triangles[offset + 2] = Vector3( cos( i * rad ) * width, 0.0f, sin( i * rad ) * width );
		triangles[offset + 3] = Vector3( cos( ( i + 1 ) * rad ) * width, 0.0f, sin( ( i + 1 ) * rad ) * width );
		triangles[offset + 4] = Vector3( 0.0f, 0.0f, 0.0f );
		triangles[offset + 5] = Vector3( cos( i * rad ) * width, 0.0f, sin( i * rad ) * width );
	}
	return triangles;
}

Vector3* Tr2ManipulationTool::GetBoxTriangles( const Vector3& min, const Vector3& max, int* numVectors )
{
	Vector3 minA( max.x, min.y, min.z );
	Vector3 minB( min.x, max.y, min.z );
	Vector3 minC( max.x, max.y, min.z );
	Vector3 maxA( max.x, min.y, max.z );
	Vector3 maxB( min.x, max.y, max.z );
	Vector3 maxC( min.x, min.y, max.z );
	Vector3* triangles = new Vector3[36];
	*numVectors = 36;
	triangles[0] = maxA;
	triangles[1] = max;
	triangles[2] = maxB;
	triangles[3] = maxA;
	triangles[4] = maxB;
	triangles[5] = maxC;
	triangles[6] = maxC;
	triangles[7] = maxB;
	triangles[8] = min;
	triangles[9] = min;
	triangles[10] = maxB;
	triangles[11] = minB;
	triangles[12] = min;
	triangles[13] = minB;
	triangles[14] = minA;
	triangles[15] = minA;
	triangles[16] = minB;
	triangles[17] = minC;
	triangles[18] = minA;
	triangles[19] = minC;
	triangles[20] = max;
	triangles[21] = minA;
	triangles[22] = max;
	triangles[23] = maxA;
	triangles[24] = maxA;
	triangles[25] = min;
	triangles[26] = minA;
	triangles[27] = maxA;
	triangles[28] = maxC;
	triangles[29] = min;
	triangles[30] = max;
	triangles[31] = minC;
	triangles[32] = minB;
	triangles[33] = max;
	triangles[34] = minB;
	triangles[35] = maxB;

	return triangles;
}

Vector3* Tr2ManipulationTool::GetCircleTriangles( float radius, int subd, int* numVectors )
{
	Vector3 top( radius, 0.0f, 0.0f );
	float rad = XM_PI / ( subd * 0.5f );
	int numVerts = subd * 3;
	Vector3* verts = new Vector3[numVerts];
	*numVectors = numVerts;
	for( int i = 0; i < subd; i++ )
	{
		int offset = 3 * i;
		verts[offset] = top;
		verts[offset + 1] = Vector3( cos( i * rad ) * radius, sin( i * rad ) * radius, 0.0f );
		verts[offset + 2] = Vector3( cos( ( i + 1 ) * rad ) * radius, sin( ( i + 1 ) * rad ) * radius, 0.0f );
	}
	return verts;
}

Vector3* Tr2ManipulationTool::GetCirclePoints( float radius, int subd, int* numVectors )
{
	float rad = XM_PI / ( subd * 0.5f );
	int numVerts = subd * 2;
	Vector3* verts = new Vector3[numVerts];
	*numVectors = numVerts;
	for( int i = 0; i < subd; i++ )
	{
		int offset = 2 * i;
		verts[offset] = Vector3( cos( i * rad ) * radius, sin( i * rad ) * radius, 0.0f );
		verts[offset + 1] = Vector3( cos( ( i + 1 ) * rad ) * radius, sin( ( i + 1 ) * rad ) * radius, 0.0f );
	}
	return verts;
}

void Tr2ManipulationTool::PyMove(
	int x,
	int y,
	int dx,
	int dy,
	TriViewport* viewport,
	Matrix view,
	Matrix proj )
{
	if( dx == 0 && dy == 0 )
	{
		return;
	}

	Tr2Viewport d3dviewport;
	d3dviewport.m_x = (float)viewport->x;
	d3dviewport.m_y = (float)viewport->y;
	d3dviewport.m_width = (float)viewport->width;
	d3dviewport.m_height = (float)viewport->height;
	d3dviewport.m_minZ = viewport->minZ;
	d3dviewport.m_maxZ = viewport->maxZ;
	Move( x, y, dx, dy, d3dviewport, view, proj );
}

void Tr2ManipulationTool::SetMoveCallback( const BlueScriptCallback& callback )
{
	m_callback = callback;
}