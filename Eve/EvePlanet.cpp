#include "StdAfx.h"
#include "EvePlanet.h"
#include "EveTransform.h"
#include "TriFrustum.h"
#include "TriDevice.h"
#include "EveUpdateContext.h"

const float EvePlanet::SCALE = 1000000.0f;
static int s_evePlanetTextureQuality = 0;

// Temporary variables, should be set by whatever module controls fov
const float FOV_MIN = 0.65f;

// global spacescene thresholds for loding, options dependent
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneVisibilityThreshold;

EvePlanet::EvePlanet( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	m_display( true ),
	m_update( true ),
	m_needResources( false ),
	m_renderScale( SCALE ),
	m_scaling( 1.0f ),
	m_radius( 1.0f ),
	m_estimatedPixelDiameter( 0.f ),
	m_estimatedMaxPixelDiameter( 0.f ),
	m_currentTextureSize( 0 ),
	m_requiredTextureSize( 0 ),
	m_warpMode( false ),
	m_albedoColor( 0, 0, 0, 0 ),
	m_emissiveColor( 0, 0, 0, 0 )
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

// --------------------------------------------------------------------------------
// Description:
//   Main update method called every frame. Here we do all sorts of things: get
//   the position/orientation of the underlying ball and the target ball. Update
//   the meshes and call the python callback to bake new resources, if necessary!
// Arguments:
//   time - current game time
// --------------------------------------------------------------------------------
void EvePlanet::Update( EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}
	
	if ( m_pythonResourceCallback )
	{
		if(m_needResources)
		{
			m_currentTextureSize = m_requiredTextureSize;
			if( !m_pythonResourceCallback.CallVoid( 1, m_requiredTextureSize ) )
			{
#if BLUE_WITH_PYTHON
				PyOS->PyFlushError("EvePlanet resource preparation(allocate) callback failed");
#endif
			}
		}
		else if(!m_needResources/* && !m_warpMode*/)
		{
			m_currentTextureSize = 0;
			if( !m_pythonResourceCallback.CallVoid( 0 ) )
			{
#if BLUE_WITH_PYTHON
				PyOS->PyFlushError("EvePlanet resource preparation(free) callback failed");
#endif
			}
		}
	}

	// update position and rotation from ball
	Quaternion rotation( 0.f, 0.f, 0.f, 1.f );
	Vector3 translation( 0.f, 0.f, 0.f );
	Be::Time time = updateContext.GetTime();

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}

	// calculate worldmatrix
	Vector3 scale( m_scaling, m_scaling, m_scaling );
	m_worldTransform = TransformationMatrix( scale, rotation, translation );

	// update models	
	if( m_highDetail )
	{
		m_highDetail->Update( updateContext );
	}
	if( m_zOnlyModel )
	{
		m_zOnlyModel->Update( updateContext );
	}

	TriObserverLocalVector::iterator observersEnd = m_observers.end();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		(*it)->Update( m_worldTransform );
	}
}


void EvePlanet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	Matrix planetScaleTransform = ScalingMatrix( 1.f / m_renderScale, 1.f / m_renderScale, 1.f / m_renderScale );

	Matrix scaledTransform = m_worldTransform * planetScaleTransform;
	
	// pixel diameters, also for the max possible
	m_estimatedPixelDiameter = EstimatePixelDiameterPos( (const Vector3*)&scaledTransform._41, 1.f / Tr2Renderer::GetProjectionTransform()._11, m_renderScale );
	m_estimatedMaxPixelDiameter = EstimatePixelDiameterPos( (const Vector3*)&scaledTransform._41, tanf( FOV_MIN / 2.f ), m_renderScale );

	// update model visibility	
	if( m_highDetail )
	{
		m_highDetail->UpdateVisibility( frustum, scaledTransform );
	}
}


void EvePlanet::UpdateZOnlyVisibility( const TriFrustum& frustum )
{
	if( m_zOnlyModel )
	{
		m_zOnlyModel->UpdateVisibility( frustum, m_worldTransform );
	}
}

const Vector3* EvePlanet::GetWorldPosition()
{
	return (Vector3*)&m_worldTransform._41;
}

void EvePlanet::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_VIDEOMEMORY )
	{
		// Textures should be created in default memory, so we need to mark them as not ready at this point.
		m_needResources = false;
	}
}

bool EvePlanet::OnPrepareResources()
{
	s_evePlanetTextureQuality = gTriDev->GetMipLevelSkipCount();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the required texture size based on the pixel diameter.
// Arguments:
//   maxDiameter - maximum pixel diameter
// Return value:
//   Returns texture size
// --------------------------------------------------------------------------------
int EvePlanet::CalcRequiredTextureSize( float maxDiameter )
{
	int size = 32;

	if( maxDiameter > 1024.f )
	{
		size = 2048;
	}
	else if( maxDiameter > 512.f )
	{
		size = 1024;
	}
	else if( maxDiameter > 256.f )
	{
		size = 512;
	}
	else if( maxDiameter > 128.f )
	{
		size = 256;
	}
	else if( maxDiameter > 64.f )
	{
		size = 128;
	}
	else if( maxDiameter > 32.f )
	{
		size = 64;
	}
	return size >> s_evePlanetTextureQuality;
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
	Vector3 d( *scaledPlanetCenter - Tr2Renderer::GetViewPosition() );
	float depth = Length( d );

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
	float halfWidthProjection = Tr2Renderer::GetViewport().width * 0.5f / tanFOV;

	// get radius od 
	float radius = m_radius / scale;

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

	float ratio = radius / scaledDistance;
	return ratio * halfWidthProjection * 2.0f;
}

void EvePlanet::PrepareForWarp(float minDist, const Vector3& dest)
{
	m_warpMode = true;

	float distEstimatedMinSize = EstimatePixelDiameterDist( minDist / m_renderScale, 1.f / Tr2Renderer::GetProjectionTransform()._11, m_renderScale );

	if( distEstimatedMinSize > GetVisibilityThreshold() )
	{
		if ( distEstimatedMinSize > GetMediumDetailThreshold() )
		{
			m_requiredTextureSize = CalcRequiredTextureSize( distEstimatedMinSize );
		}
		else
		{
			m_requiredTextureSize = 64 >> s_evePlanetTextureQuality; // probably need better decision making here
		}
	}
}

void EvePlanet::WarpStopped()
{
	m_warpMode = false;
}

void EvePlanet::GetZOnlyRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	if( m_zOnlyModel )
	{
		m_zOnlyModel->GetRenderables( renderables, nullptr );
	}
}

void EvePlanet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}
	
	if(!m_warpMode)
	{
		m_requiredTextureSize = CalcRequiredTextureSize( m_estimatedMaxPixelDiameter );
		if ( m_estimatedMaxPixelDiameter < GetVisibilityThreshold() )
		{
			m_needResources = false;
		}
	}

	// visible at all?
	if ( m_estimatedPixelDiameter > GetVisibilityThreshold() )
	{
		// visible, so resources are needed after all
		m_needResources = true;

		// use EveTransform's/Tr2Mesh's inbuild low-detail mechanism
		if( m_highDetail )
		{
			m_highDetail->GetRenderables( renderables );
		}
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



