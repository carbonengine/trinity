// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveCircle.h"
#include "include/TriMath.h"
#include "Eve/EveUpdateContext.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierTransformCommon.h"

EveCircle::EveCircle( IRoot* lockobj ) :
	m_parentTransform( IdentityMatrix() ),
	m_circleRadius( 100.f ),
	m_segments( 64.f ),
	m_objectScale( 1.f, 1.f, 1.f ),
	m_circleDistort( 1.f, 0.f, 1.f, 0.f ),
	m_startPoint( 0.f ),
	m_movementSpeed( 0.f ),
	m_animValue( 0.f ),
	m_completeness( 1.f ),
	m_lineWidth( 1.f ),
	m_meshSize( 0.f ),
	m_scaleSegmentsByCompleteness( false ),
	m_regeneratePoints( true ),
	m_billboardObjects( false ),
	m_scaleEndpoints( true ),
	m_isVisible( true ),
	m_display( true )
{
}

EveCircle::~EveCircle()
{
}

bool EveCircle::Initialize()
{
	m_regeneratePoints = true;
	return true;
}

bool EveCircle::OnModified( Be::Var* value )
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

	if( IsMatch( value, m_startPoint ) )
	{
		m_startPoint = fmod( m_startPoint, 1.f );
	}


	m_regeneratePoints = true;
	return true;
}

bool EveCircle::Update( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
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

void EveCircle::GetPointCount( unsigned& count )
{
	count = unsigned( m_points.size() );
}

void EveCircle::GeneratePoints( const Matrix& parentTransform )
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


	const float totalArc = ( 1.f - abs( m_completeness - 1.f ) ) * XM_2PI;
	const float startOffset = m_startPoint * XM_2PI + max( m_completeness - 1.f, 0.f ) * XM_2PI + totalArc / ( 2 * seg );

	m_points.clear();
	m_points.reserve( seg );

	for( int i = 0; i < seg; i++ )
	{
		const float locOnCircle = startOffset + totalArc * ( ( float( i ) / seg ) + m_animValue / seg );
		float X = cos( locOnCircle ) * m_circleRadius;
		float Y = 0.f;
		if( m_circleDistort.y != 0 || m_circleDistort.w != 0 )
		{
			float distort1 = sin( locOnCircle ) < 0 ? m_circleDistort.x : m_circleDistort.z;
			float distort2 = cos( locOnCircle ) < 0 ? m_circleDistort.w : m_circleDistort.y;
			Y = pow( sin( locOnCircle ), 2.f ) * m_circleRadius * distort1;
			Y += pow( cos( locOnCircle ), 2.f ) * m_circleRadius * distort2;
		}
		float Z = sin( locOnCircle ) * m_circleRadius;
		m_points.emplace_back( Vector3( X, Y, Z ) );
	}

	m_regeneratePoints = false;
}

void EveCircle::CalculateBoundingSphere( float meshSize, bool reCalculateChildren )
{
	if( meshSize != 0 )
	{
		m_meshSize = meshSize;
	}
	else if( m_meshSize != 0 )
	{
		meshSize = m_meshSize;
	}

	m_boundingSphere = Vector4( Vector3( 0.f, 0.f, 0.f ), m_circleRadius + m_lineWidth + meshSize );
}

void EveCircle::GetBoundingSphere( Vector4& sphere )
{
	sphere = m_boundingSphere;
	BoundingSphereTransform( m_localTransform, sphere );
}

void EveCircle::UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation )
{
	if( !m_display )
	{
		return;
	}

	m_isVisible = false;

	Vector4 sphere = m_boundingSphere;
	BoundingSphereTransform( m_localTransform * systemLocation, sphere );

	if( frustum.IsSphereVisible( &( sphere ) ) )
	{
		m_isVisible = true;
	}
}

void EveCircle::AddLinesToSet( EveCurveLineSet& lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed )
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

		if( m_completeness != 1.f && nextPoint == 0 )
		{
			continue;
		}


		id = lineSet.AddStraightLine( TransformCoord( m_points[i], m_localTransform ), color, TransformCoord( m_points[nextPoint], m_localTransform ), color, m_lineWidth );

		if( scrollSpeed != 0 )
		{
			lineSet.ChangeLineAnimation( id, animColor, scrollSpeed, 1.f );
		}
	}
}

void EveCircle::UpdateBuffer( Tr2RenderContext& renderContext, uint8_t*& data, const Matrix& systemLocation, const unsigned stride )
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

	unsigned count = 0;
	for( auto point = m_points.begin(); point != m_points.end(); ++point )
	{
		float sizeMod = 1.f;

		if( m_scaleEndpoints && m_completeness != 1.f )
		{
			sizeMod = ( count + 2 >= unsigned( m_points.size() ) ) ? 1.0f - m_animValue : sizeMod;
			sizeMod = point == m_points.begin() ? sizeMod * m_animValue : sizeMod;
			sizeMod = max( 0.01f, sizeMod );
		}

		Vector3 dirToNextPoint( 0.f, 1.f, 0.f );
		const unsigned nextPoint = ( count + 1 >= unsigned( m_points.size() ) ) ? 0 : count + 1;
		translation = Lerp( m_points[count], m_points[nextPoint], m_animValue );

		if( m_billboardObjects )
		{
			Vector3 tmpScale, tmpTranslation;
			Quaternion tmpRotation;
			Decompose( tmpScale, tmpRotation, tmpTranslation, m_localTransform * systemLocation );
			Matrix rotMat = RotationMatrix( tmpRotation );

			const Vector3 angleToCamera = Tr2Renderer::GetViewPosition() - TransformCoord( translation, m_localTransform * systemLocation );
			dirToNextPoint = TransformCoord( angleToCamera, Inverse( rotMat ) );
		}
		else
		{
			const unsigned farPoint = ( nextPoint + 1 >= unsigned( m_points.size() ) ) ? 0 : nextPoint + 1;
			dirToNextPoint = Lerp( m_points[nextPoint], m_points[farPoint], m_animValue ) - translation;
		}

		TriQuaternionArcFromForward( &objRot, &dirToNextPoint );

		Matrix matrix = TransformationMatrix( sizeMod * m_objectScale, objRot, translation ) * m_localTransform;

		Matrix m = Transpose( matrix );
		memcpy( data, &m, stride );
		data += stride;
		count++;
	}
}

// Debug renderable
void EveCircle::GetDebugOptions( Tr2DebugRendererOptions& options )
{
}

void EveCircle::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& systemLocation )
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

	for( auto point = m_points.begin(); point != m_points.end(); ++point )
	{
		renderer.DrawSphere( this, TranslationMatrix( *point ) * m_localTransform * systemLocation, m_boundingSphere.w / 100.f, 4, Tr2DebugRenderer::Wireframe, 0xffffffff );
	}
}