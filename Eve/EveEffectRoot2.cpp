#include "StdAfx.h"
#include "EveEffectRoot2.h"

#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Tr2PointLight.h"
#include "Tr2LightManager.h"
#include "EveUpdateContext.h"
extern float g_eveSpaceObjectResourceUnloadingTimeThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneLowDetailThreshold;

EveEffectRoot2::EveEffectRoot2( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_effectChildren ),
	m_boundingSphere( 0, 0, 0, 0 ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_estimatedSize( 0.0f ),
	m_display( true ),
	m_startTime( 0 ),
	m_effectDuration( -1 ),
	m_lodLevel( TR2_LOD_HIGH )
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

	D3DXMatrixTransformation( &m_localTransform, 0, 0, &m_scaling, 0, &m_rotation, &m_translation );
	D3DXMatrixMultiply( &m_lastUpdateMatrix, &m_localTransform, &m_worldTransform );

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
	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
	{
		(*ecIt)->UpdateAsyncronous( updateContext, this, nullptr );
	}
}

void EveEffectRoot2::RenderDebugInfo( Tr2RenderContext& renderContext ) 
{

}


void EveEffectRoot2::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform ) 
{
	if( !m_display )
	{
		return;
	}

	if( m_dynamicLODSelection )
	{
		Vector4 boundingSphere;
		GetBoundingSphere( boundingSphere );
		BoundingSphereTransform( m_worldTransform, boundingSphere );
		
		Tr2Lod oldLod = m_lodLevel;
		m_lodLevel = TR2_LOD_LOW;
		if( frustum.IsSphereVisible( &boundingSphere ) )
		{
			m_estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );
			if( m_estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
			{
				m_lodLevel = TR2_LOD_HIGH;
			}
			else if( m_estimatedSize >= g_eveSpaceSceneLowDetailThreshold )
			{
				m_lodLevel = TR2_LOD_MEDIUM;
			}
		}

		if ( oldLod != m_lodLevel )
		{
			for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
			{
				(*ecIt)->ChangeLOD( m_lodLevel );
			}
		}
	}


	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
	{
		(*ecIt)->GetRenderables( frustum, renderables, m_worldTransform, m_lodLevel );
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

	D3DXMatrixTransformation( &m_worldTransform, NULL, NULL, NULL, NULL, &rotation, &translation );
}


void EveEffectRoot2::GetModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// This version of the function should perform an update on the model / ball position
	Matrix currentTransform;
	UpdateWorldTransform( t );
	D3DXMatrixTransformation( &currentTransform, 0, 0, &m_scaling, 0, &m_rotation, &m_translation );
	D3DXMatrixMultiply( &currentTransform, &currentTransform, &m_worldTransform );

	D3DXVec3TransformCoord( &position, (Vector3*)&m_boundingSphere, &currentTransform );
}

void EveEffectRoot2::GetCurrentModelCenterWorldPosition( Vector3 &position )
{
	// This version of the function does not perform an update on the object
	D3DXVec3TransformCoord( &position, (Vector3*)&m_boundingSphere, &m_lastUpdateMatrix );
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

void EveEffectRoot2::GetLights( Tr2LightManager& lightManager ) const
{
	XMMATRIX worldTransform = m_worldTransform;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		lightManager.AddPointLight( 
			Vector3( XMVector3TransformCoord( (* it )->m_position, worldTransform ) ), 
			( *it )->m_radius, 
			( *it )->m_color );
	}
}
