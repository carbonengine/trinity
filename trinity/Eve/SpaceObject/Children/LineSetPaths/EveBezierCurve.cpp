// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveBezierCurve.h"
#include "include/TriMath.h"
#include "Eve/EveUpdateContext.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierTransformCommon.h"

EveBezierCurve::EveBezierCurve( IRoot* lockobj ) :
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_parentTransform( IdentityMatrix() ),
	m_point1( 0.f, 0.f, 0.f ),
	m_point2( 0.f, 0.f, 0.f ),
	m_bezierPoint( 0.f, 0.f, 0.f ),
	m_segments( 24.f ),
	m_objectScale( 1.f, 1.f, 1.f ),
	m_segmentOffset( 0.f ),
	m_movementSpeed( 0.f ),
	m_animValue( 0.f ),
	m_completeness( 1.f ),
	m_lineWidth( 1.f ),
	m_meshSize( 0.f ),
	m_scaleSegmentsByCompleteness( true ),
	m_regeneratePoints( true ),
	m_billboardObjects( true ),
	m_scaleEndpoints( true ),
	m_isVisible( true ),
	m_display( true )
{
}

EveBezierCurve::~EveBezierCurve()
{
}

bool EveBezierCurve::Initialize()
{
	m_regeneratePoints = true;
	return true;
}

bool EveBezierCurve::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_completeness ) )
	{
		m_completeness = min( 2.f, max( 0.f, m_completeness ) );
	}

	if( IsMatch( value, m_segments ) )
	{
		m_segments = min( max( 1.f, m_segments ), 128.f );
		// 128: enough for each arc to have up to 64 parts. smooth enough, even for very large arcs
	}

	if( IsMatch( value, m_segmentOffset ) )
	{
		m_segmentOffset = min( max( 0.f, m_segmentOffset ), 1.f );
	}


	m_regeneratePoints = true;
	return true;
}

bool EveBezierCurve::Update( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_movementSpeed != 0 )
	{
		float dt = updateContext.GetDeltaT();

		m_animValue += m_movementSpeed * dt;
		m_animValue = fmod( m_animValue, 1.f );
	}

	if( m_regeneratePoints )
	{
		GeneratePoints();
		CalculateBoundingSphere();
		return true;
	}

	return false;
}

void EveBezierCurve::GetPointCount( unsigned& count )
{
	count = unsigned( m_points.size() );
}

void EveBezierCurve::GeneratePoints( const Matrix& parentTransform )
{
	const int seg = m_scaleSegmentsByCompleteness ? int( ( m_segments + 0.5f ) * ( 1.f - abs( m_completeness - 1.f ) ) ) : int( m_segments + 0.5f );

	if( seg <= 1 )
	{
		return;
	}

	if( parentTransform != IdentityMatrix() )
	{
		UpdateTransform( parentTransform );
		m_parentTransform = parentTransform;
	}
	else
	{
		UpdateTransform( m_parentTransform );
	}

	m_points.clear();
	m_points.reserve( seg );

	for( int i = 0; i < seg; i++ )
	{
		float LoC = ( float( i ) / float( seg ) ) + m_segmentOffset / float( seg ); // LoC = Location on Curve
		LoC = ( ( LoC * ( min( m_completeness, 1.f ) - max( 0.f, m_completeness - 1.0f ) ) ) + max( 0.f, m_completeness - 1.0f ) );
		float X = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.x + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.x + LoC * LoC * m_point2.x;
		float Y = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.y + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.y + LoC * LoC * m_point2.y;
		float Z = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.z + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.z + LoC * LoC * m_point2.z;

		m_points.emplace_back( Vector3( X, Y, Z ) );
	}

	m_regeneratePoints = false;
}

void EveBezierCurve::CalculateBoundingSphere( float meshSize, bool reCalculateChildren )
{
	if( meshSize != 0 )
	{
		m_meshSize = meshSize;
	}
	else if( m_meshSize != 0 )
	{
		meshSize = m_meshSize;
	}

	const Vector3 center = ( m_point1 + m_point2 + m_bezierPoint ) / 3.f;
	const float rad = max( max( LengthSq( m_bezierPoint - center ), LengthSq( m_point2 - center ) ), LengthSq( m_point1 - center ) );
	m_boundingSphere = Vector4( center, sqrt( rad ) + meshSize );
}

void EveBezierCurve::GetBoundingSphere( Vector4& sphere )
{
	sphere = m_boundingSphere;
	BoundingSphereTransform( m_localTransform, sphere );
}

void EveBezierCurve::UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation )
{
	if( !m_display )
	{
		return;
	}

	m_isVisible = false;

	Vector4 sphere = m_boundingSphere;

	BoundingSphereTransform( m_localTransform * systemLocation, sphere );

	m_isVisible = frustum.IsSphereVisible( &( sphere ) );
}

void EveBezierCurve::AddLinesToSet( EveCurveLineSet& lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed )
{
	if( !m_display || !m_isVisible )
	{
		return;
	}

	if( m_regeneratePoints )
	{
		GeneratePoints();
		CalculateBoundingSphere();
	}

	int seg = m_scaleSegmentsByCompleteness ? int( ( m_segments + 0.5f ) * ( 1.f - abs( m_completeness - 1.f ) ) ) : int( m_segments + 0.5f );
	seg = min( seg, (int)m_points.size() );

	for( int i = 0; i < seg; i++ )
	{
		int nextPoint = ( i + 1 ) % seg;
		unsigned id;
		if( nextPoint != 0 )
		{
			id = lineSet.AddStraightLine( TransformCoord( m_points[i], m_localTransform ), color, TransformCoord( m_points[nextPoint], m_localTransform ), color, m_lineWidth );
		}
		else
		{
			if( m_completeness < 1 )
			{
				continue;
			}

			id = lineSet.AddStraightLine( TransformCoord( m_points[i], m_localTransform ), color, TransformCoord( m_point2, m_localTransform ), color, m_lineWidth );
		}

		if( scrollSpeed != 0 )
		{
			lineSet.ChangeLineAnimation( id, animColor, scrollSpeed, 1.f );
		}
	}
}

void EveBezierCurve::UpdateBuffer( Tr2RenderContext& renderContext, uint8_t*& data, const Matrix& systemLocation, const unsigned stride )
{
	if( !m_isVisible || !m_display )
	{
		for( auto point = m_points.begin(); point != m_points.end(); ++point )
		{
			Matrix m = ScalingMatrix( Vector3( 0.f, 0.f, 0.f ) );
			memcpy( data, &m, stride );
			data += stride;
		}
		return;
	}

	Vector3 scale, translation;
	Quaternion objRot, worldRot;

	Vector3 targetPoint = m_point2;
	Vector3 dirToNextPoint( 0.f, 1.f, 0.f );
	unsigned count = 0;
	for( auto point = m_points.begin(); point != m_points.end(); ++point )
	{
		float sizeMod = 1.f;

		if( m_scaleEndpoints )
		{
			unsigned completnessCountMod = m_completeness < 1.f ? count + 2 : count + 1;
			sizeMod = completnessCountMod >= unsigned( m_points.size() ) ? 1.0f - m_animValue : sizeMod;
			sizeMod = point == m_points.begin() ? sizeMod * m_animValue : sizeMod;
			sizeMod = max( 0.01f, sizeMod );
		}

		const unsigned nextPoint = ( count + 1 >= unsigned( m_points.size() ) ) ? 0 : count + 1;


		if( nextPoint == 0 )
		{
			if( m_completeness < 1.f )
			{
				Matrix m = ScalingMatrix( Vector3( 0.f, 0.f, 0.f ) );
				memcpy( data, &m, stride );
				data += stride;
				continue;
			}

			translation = Lerp( *point, TransformCoord( m_point2, m_worldTransform ), m_animValue );
			dirToNextPoint = TransformCoord( m_point2, m_worldTransform ) - translation;
		}
		else
		{
			if( nextPoint + 1 == unsigned( m_points.size() ) )
			{
				Vector3 lerpedTarget = Lerp( m_points[nextPoint], TransformCoord( m_point2, m_worldTransform ), min( 1.f, m_completeness ) );
				targetPoint = Lerp( m_points[nextPoint], lerpedTarget, pow( m_animValue, 2.f ) );
			}
			else
			{
				targetPoint = Lerp( m_points[nextPoint], m_points[nextPoint + 1], m_animValue );
			}

			translation = Lerp( m_points[count], m_points[nextPoint], m_animValue );
			dirToNextPoint = targetPoint - translation;
		}

		if( m_billboardObjects )
		{
			Vector3 tmpScale, tmpTranslation;
			Quaternion tmpRotation;
			Decompose( tmpScale, tmpRotation, tmpTranslation, m_localTransform * systemLocation );
			Matrix rotMat = RotationMatrix( tmpRotation );

			const Vector3 angleToCamera = Tr2Renderer::GetViewPosition() - TransformCoord( translation, m_localTransform * systemLocation );
			dirToNextPoint = TransformCoord( angleToCamera, Inverse( rotMat ) );
		}

		TriQuaternionArcFromForward( &objRot, &dirToNextPoint );

		Matrix matrix = TransformationMatrix( sizeMod * m_objectScale, objRot, translation ) * m_localTransform;

		Matrix m = Transpose( matrix );
		memcpy( data, &m, stride );
		data += stride;
		count++;
	}
}

void EveBezierCurve::GetDebugOptions( Tr2DebugRendererOptions& options )
{
} // No Bezier specific options

void EveBezierCurve::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& systemLocation )
{
	if( !renderer.HasOption( this, "LineSets" ) )
	{
		return;
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		Vector4 sphere = m_boundingSphere;
		BoundingSphereTransform( m_localTransform * systemLocation, sphere );
		renderer.DrawSphere( this, sphere.GetXYZ(), sphere.w, 4, Tr2DebugRenderer::Wireframe, 0xaaaaaaff );
	}

	renderer.DrawSphere( this, TransformCoord( m_bezierPoint, m_localTransform * systemLocation ), m_boundingSphere.w / 60.f, 8, Tr2DebugRenderer::Wireframe, 0xbbbbffff );
	renderer.DrawSphere( this, TransformCoord( m_point1, m_localTransform * systemLocation ), m_boundingSphere.w / 50.f, 8, Tr2DebugRenderer::Wireframe, 0xbbffbbff );
	renderer.DrawSphere( this, TransformCoord( m_point2, m_localTransform * systemLocation ), m_boundingSphere.w / 50.f, 8, Tr2DebugRenderer::Wireframe, 0xbbffbbff );

	renderer.DrawSphere( this, TransformCoord( m_translation, m_localTransform * systemLocation ), m_boundingSphere.w / 60.f, 8, Tr2DebugRenderer::Wireframe, 0xbbbbffff );


	for( auto point = m_points.begin(); point != m_points.end(); ++point )
	{
		renderer.DrawSphere( this, TranslationMatrix( *point ) * m_localTransform * systemLocation, m_boundingSphere.w / 100.f, 4, Tr2DebugRenderer::Wireframe, 0xffffffff );
	}
}