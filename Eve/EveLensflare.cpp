////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveLensflare.h"

#include "include/TriMath.h"
#include "Include/ITriFunction.h"
#include "TriFrustum.h"
#include "Tr2VariableStore.h"

#include "EveOccluder.h"
#include "EveTransform.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Mesh.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members and register some variable store handles
// --------------------------------------------------------------------------------
EveLensflare::EveLensflare( IRoot* lockobj ) :
	PARENTLOCK( m_flares ),
	PARENTLOCK( m_occluders ),
	PARENTLOCK( m_backgroundOccluders ),
	PARENTLOCK( m_distanceToEdgeCurves ),
	PARENTLOCK( m_distanceToCenterCurves ),
	PARENTLOCK( m_radialAngleCurves ),
	PARENTLOCK( m_xDistanceToCenter ),
	PARENTLOCK( m_yDistanceToCenter ),
	PARENTLOCK( m_bindings ),
	PARENTLOCK( m_curveSets ),
	m_display( true ),
	m_update( true ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_doOcclusionQueries( true ),
	m_cameraFactor( 20.f ),	
	m_occlusionIntensity( 1.f ),
	m_backgroundOcclusionIntensity( 1.f ),
	m_sunSize( 0.f ),
	m_directionVar( "LensflareFxDirectionScale", Vector4( 0.f, 0.f, 0.f, 1.f ) ),
	m_occScaleVar( "LensflareFxOccScale", Vector4( 1.f, 0.f, 0.f, 0.f ) )
{
	D3DXMatrixIdentity( &m_transform );
}

// --------------------------------------------------------------------------------
// Description:
//   Empty
// --------------------------------------------------------------------------------
EveLensflare::~EveLensflare()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Updates the lensflare's position by updating the translation curve. Also
//   calulates the overall size of the lensflare based on a magic formula and
//   updates the variable store values
// Arguments:
//   time - time delta since last frame
// --------------------------------------------------------------------------------
void EveLensflare::Update( Be::Time time )
{
	Vector3 dirVec;

	if( !m_update )
	{
		return;
	}

	// update the position of this lensflare
	if( m_translationCurve )
	{
		m_translationCurve->Update( &m_position, time );
	}

	// calc sun size depending on it's position in the client and some very strange, very old magic numbers
	m_sunSize = 1.f;
	if( m_translationCurve )
	{
		float distanceToCenter = D3DXVec3Length( &m_position ) / 0.1495978707e12f;
		m_sunSize = 1.5f / logf( distanceToCenter + 2.71f );
	}

	// pass important data to shader
	m_occScaleVar = Vector4( m_occlusionIntensity, m_backgroundOcclusionIntensity, 0.f, 0.f );

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( time );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Is a lot like ::Update but needs the current frame's frustum! Basiclly
//   calculates the transform for the billboard geomtry to become a 2d sprite.
//   Calculation itself is a little bit strange, but totally EVE legacy!
// Arguments:
//   frustum - the current view frustum of the current frame
// --------------------------------------------------------------------------------
void EveLensflare::PrepareRender( const TriFrustum& frustum )
{
	// restore occlusion intensity here to full, subsequent calls to ::RunOcclusionQueries()
	// will update this value
	m_occlusionIntensity = 1.f;
	m_backgroundOcclusionIntensity = 1.f;

	// debug
	if( !m_display )
	{
		return;
	}

	// calc direction to the sun (usually normalized m_position)
	if( D3DXVec3LengthSq( &m_position ) == 0.f )
	{
		Vector3 curPos( 0.f, 0.f, 0.f );
		curPos -= frustum.m_viewPos;
		D3DXVec3Normalize( &m_direction, &curPos );
	}
	else
	{
		Vector3 invPos = -m_position;
		D3DXVec3Normalize( &m_direction, &invPos );
	}

	Vector3 cameraSpacePos;
	D3DXVec3Scale( &cameraSpacePos, &frustum.m_viewDir, -1.f * m_cameraFactor );
	cameraSpacePos += frustum.m_viewPos;

	// build the matrix that will rotate the flares into position
	// by using the position of the camera and the direction to the world pos
	Vector3 negDirVec = -m_direction;
	TriMatrixArcFromForward(&m_transform, &negDirVec);
	m_transform._41 = cameraSpacePos.x;
	m_transform._42 = cameraSpacePos.y;
	m_transform._43 = cameraSpacePos.z;
	m_transform._44 = 1.0f;	

	// apply another scaling matrix to scale down for occlusion
	Matrix scaleMat;
	D3DXMatrixScaling( &scaleMat, m_occlusionIntensity, m_occlusionIntensity, 1.f );
	D3DXMatrixMultiply( &m_transform, &scaleMat, &m_transform );
    
	// pass important data to shader
	m_directionVar = Vector4( m_direction, m_sunSize );

	Vector4 direction( m_direction.x, m_direction.y, m_direction.z, 0 );
	D3DXVec4Transform( &direction, &direction, &Tr2Renderer::GetViewTransform() );
	D3DXVec4Transform( &direction, &direction, &frustum.m_projectionMatrix );
	direction.x /= direction.w;
	direction.y /= direction.w;
	float distanceToEdge = 1 - std::min( 1 - abs( direction.x ), 1 - abs( direction.y ) );
	float distanceToCenter = D3DXVec2Length( reinterpret_cast<Vector2*>( &direction ) );
	float radialAngle = atan2( direction.y, direction.x ) + TRI_PI;

	for( auto it = m_distanceToEdgeCurves.begin(); it != m_distanceToEdgeCurves.end(); ++it )
	{
		( *it )->UpdateValue( distanceToEdge );
	}

	for( auto it = m_distanceToCenterCurves.begin(); it != m_distanceToCenterCurves.end(); ++it )
	{
		( *it )->UpdateValue( distanceToCenter );
	}

	for( auto it = m_radialAngleCurves.begin(); it != m_radialAngleCurves.end(); ++it )
	{
		( *it )->UpdateValue( radialAngle );
	}

	for( auto it = m_xDistanceToCenter.begin(); it != m_xDistanceToCenter.end(); ++it )
	{
		( *it )->UpdateValue( direction.x + 10.0f );
	}

	for( auto it = m_yDistanceToCenter.begin(); it != m_yDistanceToCenter.end(); ++it )
	{
		( *it )->UpdateValue( direction.y + 10.0f );
	}

	for( auto it = m_bindings.begin(); it != m_bindings.end(); ++it )
	{
		( *it )->CopyValue();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Pass this call down to all the flares,
//   which are EveTransforms.
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveTransform
// --------------------------------------------------------------------------------
void EveLensflare::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// visibility?
	float viewDotDir = D3DXVec3Dot( &frustum.m_viewDir, &m_direction );	
	if( viewDotDir < 0.f )
	{
		return;
	}

	// add all the single flares, which are renderbales
	for( EveTransformVector::const_iterator it = m_flares.begin(); it != m_flares.end(); ++it )
	{
		(*it)->GetRenderables( frustum, renderables, m_transform );
	}

	if( m_mesh )
	{
		renderables.push_back( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Run the 1st visibility determination by rendering small sprites and use
//   EveOccluder
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
// SeeAlso:
//   EveOccluder
// --------------------------------------------------------------------------------
void EveLensflare::RunOcclusionQueries( Tr2RenderContext& renderContext, const TriFrustum& frustum )
{
	// debug
	if( !m_display )
	{
		return;
	}
	if( !m_doOcclusionQueries )
	{
		return;
	}

	// pass to occlusion modules
	for( EveOccluderVector::const_iterator it = m_occluders.begin(); it != m_occluders.end(); ++it )
	{
		// render query sprites
		(*it)->RunQuery( renderContext, frustum, m_transform );
		// "collect" results, which mgith not be from this frame...
		m_occlusionIntensity *= (*it)->GetValue();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Run the 2nd visibility determination by rendering small sprites and use
//   EveOccluder
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
// SeeAlso:
//   EveOccluder
// --------------------------------------------------------------------------------
void EveLensflare::RunBackgroundOcclusionQueries( Tr2RenderContext& renderContext, const TriFrustum& frustum )
{
	// debug
	if( !m_display )
	{
		return;
	}
	if( !m_doOcclusionQueries )
	{
		return;
	}

	// pass to occlusion modules
	for( EveOccluderVector::const_iterator it = m_backgroundOccluders.begin(); it != m_backgroundOccluders.end(); ++it )
	{
		// render query sprites
		(*it)->RunQuery( renderContext, frustum, m_transform );
		// "collect" results, which mgith not be from this frame...
		m_occlusionIntensity *= (*it)->GetValue();
		m_backgroundOcclusionIntensity *= (*it)->GetValue();
	}
}

void EveLensflare::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

bool EveLensflare::HasTransparentBatches()
{
	return false;
}

float EveLensflare::GetSortValue()
{
	return 1.f;
}

Tr2PerObjectData* EveLensflare::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return nullptr;
}