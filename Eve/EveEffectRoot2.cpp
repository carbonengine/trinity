#include "StdAfx.h"
#include "EveEffectRoot2.h"

#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Tr2PointLight.h"
#include "Tr2LightManager.h"
#include "Curves/TriCurveSet.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"

extern float g_eveSpaceObjectResourceUnloadingTimeThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneLowDetailThreshold;

EveEffectRoot2::EveEffectRoot2( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_effectChildren ),
	PARENTLOCK( m_curveSets ),
	m_boundingSphere( 0, 0, 0, 0 ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_estimatedSize( 0.0f ),
	m_display( true ),
	m_startTime( 0 ),
	m_effectDuration( -1 ),
	m_lodLevel( TR2_LOD_HIGH ),
	m_dynamicLODSelection( false ),
	m_changeLOD( false ),
	m_secondaryLightingSphereRadiusLocal( 0.5f ),
	m_secondaryLightingSphereRadiusWorld( 0.5f ),
	m_secondaryLightingEmissiveColor( 0.f, 0.f, 0.f, 0.f )
{
}

bool EveEffectRoot2::Initialize()
{
	return true;
}

void EveEffectRoot2::UpdateSyncronous( EveUpdateContext& updateContext ) 
{	
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	m_lastUpdateMatrix = m_localTransform * m_worldTransform;
	m_secondaryLightingSphereRadiusWorld = m_secondaryLightingSphereRadiusLocal * ( m_scaling.x + m_scaling.y + m_scaling.z ) / 3.f;

	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		(*it)->Update( m_lastUpdateMatrix );
	}

	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
	{
		(*ecIt)->UpdateSyncronous( updateContext, this, nullptr );
	}
}

void EveEffectRoot2::UpdateAsyncronous( EveUpdateContext& updateContext ) 
{	
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Update( time, time );
	}

	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
	{
		(*ecIt)->UpdateAsyncronous( updateContext, this, nullptr );
	}
}

void EveEffectRoot2::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) 
{
	if( !m_display )
	{
		return;
	}

	m_changeLOD = false;
	if( m_dynamicLODSelection )
	{
		Vector4 boundingSphere;
		GetBoundingSphere( boundingSphere );
		BoundingSphereTransform( m_worldTransform, boundingSphere );
		
		if( frustum.IsSphereVisible( &boundingSphere ) )
		{
			m_estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );
		}

		Tr2Lod oldLod = m_lodLevel;
		m_lodLevel = TR2_LOD_LOW;

		if( m_estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
		{
			m_lodLevel = TR2_LOD_HIGH;
		}
		else if( m_estimatedSize >= g_eveSpaceSceneLowDetailThreshold )
		{
			m_lodLevel = TR2_LOD_MEDIUM;
		}

		m_changeLOD = oldLod != m_lodLevel;
	}
	
	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
	{
		(*ecIt)->UpdateVisibility( frustum, parentTransform, m_lodLevel );
	}
}

void EveEffectRoot2::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ) 
{
	if( !m_display )
	{
		return;
	}

	if( m_changeLOD )
	{
		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
		{
			(*ecIt)->ChangeLOD( m_lodLevel );
		}
	}

	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
	{
		(*ecIt)->GetRenderables( renderables );
	}
}


bool EveEffectRoot2::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{ 
	sphere = m_boundingSphere;
	return true;
};


void EveEffectRoot2::UpdateWorldTransform( Be::Time time )
{	
	Quaternion rotation;
	Vector3 translation;

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = Vector3( 0.0f, 0.0f, 0.0f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	
	if( m_modelRotation )
	{
		Quaternion modelRotation;
		m_modelRotation->Update( &modelRotation, time );
		rotation = modelRotation * rotation;
	}

	m_worldTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
	
	if( m_modelTranslation )
	{
		Vector3 modelTranslation;
		m_modelTranslation->Update( &modelTranslation, time );
		modelTranslation = TransformCoord( modelTranslation, m_worldTransform );
		m_worldTransform.GetTranslation() = modelTranslation;
	}
}


void EveEffectRoot2::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// This version of the function should perform an update on the model / ball position
	Matrix currentTransform;
	UpdateWorldTransform( t );
	currentTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	currentTransform = currentTransform * m_worldTransform;

	position = TransformCoord( m_boundingSphere.GetXYZ(), currentTransform );
}

void EveEffectRoot2::GetModelCenterWorldPosition( Vector3 &position ) const
{
	// This version of the function does not perform an update on the object
	position = TransformCoord( m_boundingSphere.GetXYZ(), m_lastUpdateMatrix );
}


bool EveEffectRoot2::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	// If possible, return an AABB in local coordinates
	return false;
}

void EveEffectRoot2::GetLocalToWorldTransform( Matrix &transform ) const
{
	// Get the local to world transform
	transform = m_lastUpdateMatrix;
}

void EveEffectRoot2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveEffectRoot2::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if (!m_display )
	{
		return;
	}
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveEffectRoot2::GetLights( Tr2LightManager& lightManager ) const
{
	XMMATRIX worldTransform = m_lastUpdateMatrix;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_lastUpdateMatrix.GetX() ), 
		XMVectorAdd( XMVector3LengthEst( m_lastUpdateMatrix.GetY() ), XMVector3LengthEst( m_lastUpdateMatrix.GetZ() ) ) ) ) / 3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Called by all children. It is similar to what spaceobjects send to vs/ps
// --------------------------------------------------------------------------------
void EveEffectRoot2::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const
{
	// vs
	memset( &vsData, 0, sizeof( EveSpaceObjectVSData ) );
	// activation
	vsData.shipData.y = 1.f;
	// boundingsphere
	vsData.shipData.w = 1.f;

	// ps
	memset( &psData, 0, sizeof( EveSpaceObjectPSData ) );
	// activation
	psData.shipData.y = 1.f;
	// boundingsphere
	psData.shipData.w = 1.f;
}

void EveEffectRoot2::RegisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	static const Color s_noAlbedoColor( 0.f, 0.f, 0.f, 0.f );
	manager.RegisterSecondaryLightSource( 
		&m_worldTransform.GetTranslation(), 
		&m_secondaryLightingSphereRadiusWorld, 
		&s_noAlbedoColor, 
		&m_secondaryLightingEmissiveColor );
}

void EveEffectRoot2::UnregisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	manager.UnregisterSecondaryLightSource( &m_worldTransform.GetTranslation() );
}

// --------------------------------------------------------------------------------
// Description:
//   Plays all "top level" curve sets.
// --------------------------------------------------------------------------------
void EveEffectRoot2::Start()
{
	// play curvesets owned by this effect root
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Play();
	}

	// play curvesets on containers owned by this effect root
	for( auto cit = m_effectChildren.begin(); cit != m_effectChildren.end(); cit++ )
	{
		EveChildContainerPtr cont;
		if( (*cit)->QueryInterface(BlueInterfaceIID<EveChildContainer>(), (void**)&cont, BEQI_SILENT ) )
		{
			cont->PlayAllCurveSets();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stops all "top level" curve sets.
// --------------------------------------------------------------------------------
void EveEffectRoot2::Stop()
{
	// stop curvesets owned by this effect root
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Stop();
	}

	// stop curvesets on containers owned by this effect root
	for( auto cit = m_effectChildren.begin(); cit != m_effectChildren.end(); cit++ )
	{
		EveChildContainerPtr cont;
		if( (*cit)->QueryInterface(BlueInterfaceIID<EveChildContainer>(), (void**)&cont, BEQI_SILENT ) )
		{
			cont->StopAllCurveSets();
		}
	}
}




// --------------------------------------------------------------------------------
// Description:
//   Is mostly used for effects, so no damage locators at all!
// --------------------------------------------------------------------------------
unsigned int EveEffectRoot2::GetDamageLocatorCount() const
{
	return 0;
}

bool EveEffectRoot2::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	*out = m_worldTransform.GetTranslation();
	return true;
}

bool EveEffectRoot2::GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace )
{
	*out = Vector3( 0.f, 1.f, 0.f );
	return true;
}

void EveEffectRoot2::GetImpactPosition( Vector3& out, int damageLocatorIndex, const Vector3& direction )
{
	GetDamageLocatorPosition( &out, damageLocatorIndex, true );
}

bool EveEffectRoot2::HasImpactConfigurationShield() const
{
	return false;
}

int EveEffectRoot2::GetClosestDamageLocatorIndex( const Vector3* position )
{
	return 0;
}

int EveEffectRoot2::GetGoodDamageLocatorIndex( const Vector3 &position )
{
	return 0;
}

float EveEffectRoot2::GetRadius() const
{
	return m_boundingSphere.w;
}

// -----------------------------------------------------------------------------
// Description:
//   Create an impact effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
int EveEffectRoot2::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	return -1;
}

// -----------------------------------------------------------------------------
// Description:
//   Update the effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
bool EveEffectRoot2::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	return false;
}

void EveEffectRoot2::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	GetDamageLocatorPosition(out, -1, true );
	
	if( hit && source ) 
	{
		Vector3 local( *hit - *out );
		Vector3 dir = Normalize( *hit - *source );
		
		local -= dir * Dot( dir, local );

		local = Normalize( local );
		const Vector3 off = local * m_boundingSphere.w * 1.125f;
		*out += off;
	}
}

