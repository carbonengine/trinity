// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveLineChildContainer.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"

EveLineChildContainer::EveLineChildContainer( IRoot* lockobj ) :
	PARENTLOCK( m_lines ),
	m_meshSize( 0 ),
	m_regenerate( false ),
	m_parentTransform( IdentityMatrix() ),
	m_isVisible( true ),
	m_display( true )
{
	m_lines.SetNotify( this );
}

EveLineChildContainer::~EveLineChildContainer()
{
}

bool EveLineChildContainer::OnModified( Be::Var* value )
{
	m_regenerate = true;
	return true;
}

void EveLineChildContainer::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	m_regenerate = true;
}

bool EveLineChildContainer::Update( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	bool updateBounds = false;

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		const bool updateLocalBoundingSphere = ( *it )->Update( updateContext, params );
		updateBounds = updateBounds || updateLocalBoundingSphere;
	}

	if( m_regenerate )
	{
		GeneratePoints();
		CalculateBoundingSphere();
		updateBounds = true;
	}
	else if( updateBounds )
	{
		CalculateBoundingSphere( false );
	}

	return updateBounds;
}

void EveLineChildContainer::UpdateBuffer( Tr2RenderContext& renderContext, uint8_t*& data, const Matrix& systemLocation, const unsigned stride )
{
	if( !m_isVisible || !m_display )
	{
		unsigned count;
		GetPointCount( count );
		Matrix zeroMatrix = ScalingMatrix( Vector3( 0.0, 0.0, 0.0 ) );

		for( unsigned i = 0; i < count; i++ )
		{
			memcpy( data, &zeroMatrix, stride );
			data += stride;
		}
		return;
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->UpdateBuffer( renderContext, data, systemLocation, stride );
	}
}

void EveLineChildContainer::GetPointCount( unsigned& count )
{
	unsigned points, total = 0;
	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->GetPointCount( points );
		total += points;
	}
	count = total;
}

void EveLineChildContainer::GeneratePoints( const Matrix& parentTransform )
{
	if( parentTransform != IdentityMatrix() )
	{
		UpdateTransform( parentTransform );
		m_parentTransform = parentTransform;
	}
	else
	{
		UpdateTransform( m_parentTransform );
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->GeneratePoints( m_worldTransform );
	}
	m_regenerate = false;
}

void EveLineChildContainer::CalculateBoundingSphere( float meshSize, bool reCalculateChildren )
{
	if( meshSize != 0 )
	{
		m_meshSize = meshSize;
	}
	else if( m_meshSize != 0 )
	{
		meshSize = m_meshSize;
	}

	Vector4 sphere;
	CalculateBoundingSphereForLineSetPaths( sphere, m_lines, reCalculateChildren, meshSize );

	m_boundingSphere = sphere;
}

void EveLineChildContainer::GetBoundingSphere( Vector4& sphere )
{
	sphere = m_boundingSphere;
	BoundingSphereTransform( m_localTransform, sphere );
}

void EveLineChildContainer::UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation )
{
	if( !m_display )
	{
		return;
	}

	Vector4 sphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, sphere );

	if( !frustum.IsSphereVisible( &( sphere ) ) )
	{
		m_isVisible = false;
		return;
	}

	m_isVisible = true;

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->UpdateVisibility( frustum, parentLod, systemLocation );
	}
}

void EveLineChildContainer::AddLinesToSet( EveCurveLineSet& lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed )
{
	if( !m_display || !m_isVisible )
	{
		return;
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->AddLinesToSet( lineSet, color, animColor, scrollSpeed );
	}
}

// Debug renderable
void EveLineChildContainer::GetDebugOptions( Tr2DebugRendererOptions& options )
{
}

void EveLineChildContainer::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentWorldLocation )
{
	if( !renderer.HasOption( this, "LineSets" ) )
	{
		return;
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		Vector4 sphere = m_boundingSphere;
		BoundingSphereTransform( m_worldTransform * parentWorldLocation, sphere );
		renderer.DrawSphere( this, sphere.GetXYZ(), sphere.w, 8, Tr2DebugRenderer::Wireframe, 0xaaaaffaa );
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->RenderDebugInfo( renderer, parentWorldLocation );
	}
}