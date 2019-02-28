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

// global spacescene thresholds for loding, options dependent
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneVisibilityThreshold;

EvePlanet::EvePlanet( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_effectChildren ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_externalParameters ),
	m_display( true ),
	m_update( true ),
	m_renderScale( SCALE ),
	m_scaling( 1.0f ),	
	m_estimatedPixelDiameter( 0.f ),
	m_estimatedMaxPixelDiameter( 0.f ),
	m_currentLod( TR2_LOD_UNSPECIFIED ),
	m_albedoColor( 0, 0, 0, 0 ),
	m_emissiveColor( 0, 0, 0, 0 ),
	m_radius( 1 )
{
	PrepareResources();
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

void EvePlanet::UpdateEffectChildren( EveUpdateContext& updateContext, Matrix &worldTransform )
{
	EveChildUpdateParams params;
	params.spaceObjectParent = nullptr;
	params.childParent = nullptr;
	params.boneCount = 0;
	params.bones = nullptr;
	params.isVisible = m_display && m_currentLod > TR2_LOD_LOW;
	params.localToWorldTransform = CalculatePlanetScaleTransform( worldTransform );
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
		m_zOnlyModel->UpdateAsyncronous( updateContext, params );
		m_zOnlyModel->UpdateSyncronous( updateContext, params );
	}
}

void EvePlanet::Update( EveUpdateContext& updateContext )
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

	auto scale = Vector3( m_scaling, m_scaling, m_scaling );

	m_worldTransform = TransformationMatrix( scale, rotation, translation );

	UpdateEffectChildren( updateContext, m_worldTransform);

	for (auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it)
	{
		(*it)->Update( time, time );
	}

	// Used for audio for suns.
	TriObserverLocalVector::iterator observersEnd = m_observers.end();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		(*it)->Update( m_worldTransform );
	}
}

Matrix EvePlanet::CalculatePlanetScaleTransform( const Matrix& worldTransform ) const
{
	const auto planetScaleTransform = ScalingMatrix( 1.f / m_renderScale, 1.f / m_renderScale, 1.f / m_renderScale );
	return worldTransform * planetScaleTransform;
}

void EvePlanet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	const auto scaledTransform = CalculatePlanetScaleTransform( m_worldTransform );
	
	// pixel diameters, also for the max possible
	m_estimatedPixelDiameter = EstimatePixelDiameterPos( reinterpret_cast<const Vector3*>( &scaledTransform._41 ), 1.f / Tr2Renderer::GetProjectionTransform()._11, m_renderScale );
	m_estimatedMaxPixelDiameter = EstimatePixelDiameterPos( reinterpret_cast<const Vector3*>( &scaledTransform._41 ), tanf( FOV_MIN / 2.f ), m_renderScale );

	for (auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it)
	{
		(*it)->UpdateVisibility( frustum, scaledTransform, m_currentLod );
	}
	UpdateZOnlyVisibility( frustum );
}

void EvePlanet::UpdateZOnlyVisibility( const TriFrustum& frustum )
{
	if( nullptr != m_zOnlyModel )
	{
		m_zOnlyModel->UpdateVisibility( frustum, m_worldTransform, m_currentLod );
	}
}


const Vector3* EvePlanet::GetWorldPosition()
{
	return reinterpret_cast<Vector3*>( &m_worldTransform._41 );
}


bool EvePlanet::OnPrepareResources()
{
	return true;
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

	// visible at all?
	if( m_estimatedPixelDiameter > GetVisibilityThreshold() )
	{
		if( m_currentLod != TR2_LOD_HIGH )
		{
			SetLod( TR2_LOD_HIGH );
		}

		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
		{
			if( ( *ecIt ) != m_zOnlyModel )
			{
				( *ecIt )->GetRenderables( renderables );
			}
			if( nullptr != m_zOnlyModel )
			{
				m_zOnlyModel->GetRenderables( renderables );
			}
		}
	}
	else if( m_currentLod != TR2_LOD_LOW )
	{
		SetLod( m_currentLod );
	}
}

void EvePlanet::SetLod( Tr2Lod lod )
{
	m_currentLod = lod;
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->ChangeLOD( lod );
	}
	if( nullptr != m_zOnlyModel )
	{
		m_zOnlyModel->ChangeLOD( lod );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Just return a global variable, slightly modified by a constant, since this
//   is a planet, not a ship.
// Return value:
//   Returns the visibilty threshold for planets
// --------------------------------------------------------------------------------
float EvePlanet::GetVisibilityThreshold() const
{
	return g_eveSpaceSceneVisibilityThreshold * 1.5f;
}

// --------------------------------------------------------------------------------
// Description:
//   Just return a global variable, slightly modified by a constant, since this
//   is a planet, not a ship.
// Return value:
//   Returns the medium lod threshold for planets
// --------------------------------------------------------------------------------
float EvePlanet::GetMediumDetailThreshold() const
{
	return g_eveSpaceSceneMediumDetailThreshold * 1.5f;
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