// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EvePlanet.h"
#include "EveTransform.h"
#include "TriFrustum.h"
#include "TriDevice.h"
#include "EveUpdateContext.h"
#include "Curves/TriCurveSet.h"

const float EvePlanet::SCALE = 1000000.0f;

// Temporary variables, should be set by whatever module controls fov
const float FOV_MIN = 0.65f;

EvePlanet::EvePlanet( IRoot* lockobj ) :
	EveEffectRoot2( lockobj ),
	m_update( true ),
	m_renderScale( SCALE ),
	m_estimatedPixelDiameter( 0.f ),
	m_estimatedMaxPixelDiameter( 0.f ),
	m_albedoColor( 0, 0, 0, 0 ),
	m_emissiveColor( 0.0, 0.0, 0.0, 0.0 ),
	m_radius( 1 ),
	m_minScreenSize( 2.0 )
{
}

EvePlanet::~EvePlanet()
{
}

void EvePlanet::RegisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	manager.RegisterSecondaryLightSource( &m_worldTransform.GetTranslation(), &m_radius, &m_albedoColor, &m_emissiveColor );
}

void EvePlanet::UnregisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	manager.UnregisterSecondaryLightSource( &m_worldTransform.GetTranslation() );
}

void EvePlanet::UpdateEffectChildren( const EveUpdateContext& updateContext, Matrix& worldTransform, float renderScale )
{
	EveChildUpdateParams params;
	params.spaceObjectParent = nullptr;
	params.childParent = nullptr;
	params.boneCount = 0;
	params.bones = nullptr;
	params.isVisible = m_display && m_lodLevel > TR2_LOD_LOW;
	params.localToWorldTransform = CalculatePlanetScaleTransform( worldTransform, renderScale );
	if( !m_effectChildren.empty() )
	{
		for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
		{
			( *it )->UpdateAsyncronous( updateContext, params );
			( *it )->UpdateSyncronous( updateContext, params );
		}
	}
	if( nullptr != m_zOnlyModel )
	{
		params.localToWorldTransform = worldTransform;
		m_zOnlyModel->UpdateSyncronous( updateContext, params );
		m_zOnlyModel->UpdateAsyncronous( updateContext, params );
	}
}

void EvePlanet::UpdatePlanetSyncronous( const EveUpdateContext& updateContext, float renderScale )
{
	if( !m_update )
	{
		return;
	}

	// update position and rotation from ball
	Quaternion rotation( 0.f, 0.f, 0.f, 1.f );
	Vector3 translation( 0.f, 0.f, 0.f );
	const auto time = updateContext.GetTime();

	if( m_ballPosition != nullptr )
	{
		m_ballPosition->Update( &translation, time );
	}

	if( m_ballRotation != nullptr )
	{
		m_ballRotation->Update( &rotation, time );
	}

	// Add Ball & EffectRoot Translation & Rotation
	translation += m_translation;
	rotation = Normalize( rotation * m_rotation );

	m_worldTransform = TransformationMatrix( m_scaling, rotation, translation );
	UpdateEffectChildren( updateContext, m_worldTransform, renderScale );

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( time, time );
	}

	// Update the controllers
	EveEffectRoot2::UpdateControllers( 0.5f );

	// Used for audio for suns.
	TriObserverLocalVector::iterator observersEnd = m_observers.end();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		( *it )->Update( m_worldTransform );
	}
}

Matrix EvePlanet::CalculatePlanetScaleTransform( const Matrix& worldTransform, float renderScale ) const
{
	const auto planetScaleTransform = ScalingMatrix( 1.f / renderScale, 1.f / renderScale, 1.f / renderScale );
	return worldTransform * planetScaleTransform;
}

void EvePlanet::UpdatePlanetVisibility( const EveUpdateContext& updateContext, float renderScale )
{
	const auto scaledTransform = CalculatePlanetScaleTransform( m_worldTransform, renderScale );

	// pixel diameters, also for the max possible
	m_estimatedPixelDiameter = EstimatePixelDiameterPos( reinterpret_cast<const Vector3*>( &scaledTransform._41 ), 1.f / Tr2Renderer::GetProjectionTransform()._11, renderScale );
	m_estimatedMaxPixelDiameter = EstimatePixelDiameterPos( reinterpret_cast<const Vector3*>( &scaledTransform._41 ), tanf( FOV_MIN / 2.f ), renderScale );

	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->UpdateVisibility( updateContext, scaledTransform, m_lodLevel );
	}
}

void EvePlanet::UpdateZOnlyVisibility( const EveUpdateContext& updateContext )
{
	if( nullptr != m_zOnlyModel )
	{
		m_zOnlyModel->UpdateVisibility( updateContext, m_worldTransform, m_lodLevel );
	}
}

Vector3 EvePlanet::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}


Quaternion EvePlanet::GetWorldRotation()
{
	return m_rotation;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate the pixeldiameter of this planet sphere at a given position. Is used
//   for loding decisions.
// Arguments:
//   scaledPlanetCenter - the position of the planet in the scaled system
//   tanFOV - tan( FOV / 2 )
// Return value:
//   Returns the pixel diameter
// --------------------------------------------------------------------------------
float EvePlanet::EstimatePixelDiameterPos( const Vector3* scaledPlanetCenter, float tanFOV, float scale ) const
{
	// calc distance
	const auto d( *scaledPlanetCenter - Tr2Renderer::GetViewPosition() );
	const auto depth = Length( d );

	return EstimatePixelDiameterDist( depth, tanFOV, scale );
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate the pixeldiameter of this planet sphere at a given distance to the
//   camera. Is used for loding decisions.
// Arguments:
//   sphere - return the bounding sphere data here
// Return value:
//   Returns the medium lod threshold for planets
// --------------------------------------------------------------------------------
float EvePlanet::EstimatePixelDiameterDist( float scaledDistance, float tanFOV, float scale ) const
{
	const auto halfWidthProjection = Tr2Renderer::GetViewport().width * 0.5f / tanFOV;

	// get radius od
	const auto radius = m_radius / scale;

	// clamp values close to zero and below
	const float epsilon = 1e-5f;
	if( scaledDistance < epsilon )
	{
		scaledDistance = epsilon;
	}

	if( radius < epsilon )
	{
		return 0.0f;
	}

	const auto ratio = radius / scaledDistance;
	return ratio * halfWidthProjection * 2.0f;
}

void EvePlanet::GetZOnlyRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	if( m_lodLevel != TR2_LOD_HIGH )
	{
		return;
	}

	if( nullptr != m_zOnlyModel )
	{
		m_zOnlyModel->GetRenderables( renderables );
	}
}

void EvePlanet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}
	if( m_lodLevel != TR2_LOD_HIGH )
	{
		return;
	}

	// visible at all?
	if( m_estimatedPixelDiameter > m_minScreenSize )
	{
		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
		{
			( *ecIt )->GetRenderables( renderables );
		}
	}
}

float EvePlanet::GetEstimatedPixelDiameter()
{
	return m_estimatedPixelDiameter;
}

ITriVectorFunctionPtr EvePlanet::GetTranslationCurve()
{
	return m_ballPosition;
}

void EvePlanet::UpdateLOD()
{
	if( !m_display )
	{
		return;
	}

	// visible at all?
	if( m_estimatedPixelDiameter > m_minScreenSize )
	{
		SetLod( TR2_LOD_HIGH );
	}
	else
	{
		SetLod( TR2_LOD_LOW );
	}
}

void EvePlanet::SetLod( Tr2Lod lod )
{

	m_lodLevel = lod;

	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->ChangeLOD( m_lodLevel );
	}
	if( nullptr != m_zOnlyModel )
	{
		m_zOnlyModel->ChangeLOD( m_lodLevel );
	}
}

void EvePlanet::SetRenderScale( float value )
{
	m_renderScale = value;
}

// --------------------------------------------------------------------------------
unsigned int EvePlanet::GetDamageLocatorCount() const
{
	return 1;
}

// --------------------------------------------------------------------------------
int EvePlanet::GetClosestDamageLocatorIndex( const Vector3* position )
{
	return 0;
}

// --------------------------------------------------------------------------------
bool EvePlanet::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	*out = inWorldSpace ? m_worldTransform.GetTranslation() : Vector3( 0.f, 0.f, 0.f );
	return true;
}

// --------------------------------------------------------------------------------
bool EvePlanet::GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace )
{
	*out = Vector3( 0.f, 1.f, 0.f );
	return true;
}

// --------------------------------------------------------------------------------
void EvePlanet::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	*out = m_worldTransform.GetTranslation();
}

// --------------------------------------------------------------------------------
int EvePlanet::GetGoodDamageLocatorIndex( const Vector3& position )
{
	return 0;
}

// --------------------------------------------------------------------------------
float EvePlanet::GetRadius() const
{
	return m_radius;
}

// --------------------------------------------------------------------------------
int EvePlanet::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	return 0;
}

// --------------------------------------------------------------------------------
bool EvePlanet::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	return false;
}

// --------------------------------------------------------------------------------
bool EvePlanet::GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon )
{
	return false;
}

// --------------------------------------------------------------------------------
bool EvePlanet::HasImpactConfigurationShield() const
{
	return false;
}


void EvePlanet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	for( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}

	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->GetDebugOptions( options );
		}
	}
}

void EvePlanet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	for( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}

	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}
}