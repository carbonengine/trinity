#include "StdAfx.h"
#include "EveEffectRoot.h"

#include "Utilities/BoundingSphere.h"
#include "Utilities/ViewDistanceInfo.h"
#include "TriFrustum.h"
#include "EveUpdateContext.h"
extern float g_eveSpaceObjectResourceUnloadingTimeThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneLowDetailThreshold;

EveEffectRoot::EveEffectRoot( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	m_boundingSphere( 0, 0, 0, 0 ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_estimatedSize( 0.0f ),
	m_dynamicLODSelection( false ),
	m_display( true ),
	m_playEffect( false ),
	m_lodLevel( LOD_INVALID ),
	m_startTime( 0 ),
	m_effectDuration( -1 )
{

}


// --------------------------------------------------------------------------------
// Description:
//   Assign LOD
// --------------------------------------------------------------------------------
void EveEffectRoot::AssignLOD()
{
	EveTransform* effectObject = nullptr;

	if( m_lodLevel == LOD_HIGH )
	{
		effectObject = dynamic_cast<EveTransform*>( m_highDetail->GetObject() );
	}
	else if( m_lodLevel == LOD_MEDIUM )
	{
		effectObject = dynamic_cast<EveTransform*>( m_mediumDetail->GetObject() );
	}
	else if( m_lodLevel == LOD_LOW )
	{
		effectObject = dynamic_cast<EveTransform*>( m_lowDetail->GetObject() );
	}

	if( !m_effectObject && effectObject )
	{
		effectObject->PlayCurveSets();
		m_effectObject = effectObject;
		if( m_loadedEventListener )
		{
			m_loadedEventListener->HandleEvent( NULL );
		}
	}
}

IBlueObjectProxyPtr EveEffectRoot::CreateLOD( const EveTransform* tf )
{
	IBlueObjectProxyPtr proxy;
	
	Be::Clsid clsid;
	if( BeClasses->FindClsid( clsid, "blue", "BlueObjectProxy" ) )
	{
		BeClasses->CreateInstance( clsid, BlueInterfaceIID<IBlueObjectProxy>(), (void**)&proxy );
	}

	if( proxy )
	{
		proxy->SetObject( tf->GetRawRoot() );
	}

	return proxy;
}

bool EveEffectRoot::Initialize()
{
	if( !m_highDetail )
	{
		if( m_mediumDetail )
		{
			m_highDetail = m_mediumDetail;
		}
		else if( m_lowDetail )
		{
			m_highDetail = m_lowDetail;
		}
		else if( m_effectObject )
		{
			m_highDetail = CreateLOD( m_effectObject );
		}
	}
	if( !m_mediumDetail )
	{
		if( m_highDetail )
		{
			m_mediumDetail = m_highDetail;
		}
		else if( m_lowDetail )
		{
			m_mediumDetail = m_lowDetail;
		}
		else if( m_effectObject )
		{
			m_mediumDetail = CreateLOD( m_effectObject );
		}
	}
	if( !m_lowDetail )
	{
		if( m_mediumDetail )
		{
			m_lowDetail = m_mediumDetail;
		}
		else if( m_highDetail )
		{
			m_lowDetail = m_highDetail;
		}
		else if( m_effectObject )
		{
			m_lowDetail = CreateLOD( m_effectObject );
		}
	}

	return true;
}

void EveEffectRoot::UpdateLOD( Be::Time time )
{
	Be::Time timeout = TimeFromDouble( (double)g_eveSpaceObjectResourceUnloadingTimeThreshold );
	if( m_highDetail )
	{
		m_highDetail->Update( time, timeout );
	}
	if( m_mediumDetail )
	{
		m_mediumDetail->Update( time, timeout );
	}
	if( m_lowDetail )
	{
		m_lowDetail->Update( time, timeout );
	}
}

void EveEffectRoot::UpdateSyncronous( EveUpdateContext& updateContext ) 
{	
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	D3DXMatrixTransformation( &m_localTransform, 0, 0, &m_scaling, 0, &m_rotation, &m_translation );
	D3DXMatrixMultiply( &m_lastUpdateMatrix, &m_localTransform, &m_worldTransform );
	
	AssignLOD();
	UpdateLOD( updateContext.GetTime() );

	if( !m_effectObject )
	{
		return;
	}

	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		(*it)->Update( m_lastUpdateMatrix );
	}
}

void EveEffectRoot::UpdateAsyncronous( EveUpdateContext& updateContext ) 
{	
	if( m_effectObject )
	{
		m_effectObject->Update( updateContext );
	}
}

void EveEffectRoot::RenderDebugInfo( Tr2RenderContext& renderContext ) 
{
	if( m_effectObject )
	{
		m_effectObject->RenderDebugInfo( renderContext );
	}
}


void EveEffectRoot::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform ) 
{
	if( !m_display )
	{
		return;
	}
	
	if( !m_playEffect )
	{
		return;
	}
	
	if( !m_effectObject || m_dynamicLODSelection )
	{
		Vector4 boundingSphere;
		GetBoundingSphere( boundingSphere );
		BoundingSphereTransform( m_lastUpdateMatrix, boundingSphere );
		
		m_lodLevel = LOD_LOW;
		if( frustum.IsSphereVisible( &boundingSphere ) )
		{
			m_estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );
			if( m_estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
			{
				m_lodLevel = LOD_HIGH;
			}
			else if( m_estimatedSize >= g_eveSpaceSceneLowDetailThreshold )
			{
				m_lodLevel = LOD_MEDIUM;
			}
		}
	}

	if( m_effectObject )
	{
		m_effectObject->GetRenderables( frustum, renderables, m_lastUpdateMatrix );
	}
}


bool EveEffectRoot::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{ 
	sphere = m_boundingSphere;
	return true; 
};


void EveEffectRoot::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{ 
	Vector4 v; 
	if( GetBoundingSphere( v ) )
	{
		viewDistance.UpdateClipPlanes( v, frustum ); 
	}
}


void EveEffectRoot::UpdateWorldTransform( Be::Time time )
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


void EveEffectRoot::GetModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// This version of the function should perform an update on the model / ball position
	Matrix currentTransform;
	UpdateWorldTransform( t );
	D3DXMatrixTransformation( &currentTransform, 0, 0, &m_scaling, 0, &m_rotation, &m_translation );
	D3DXMatrixMultiply( &currentTransform, &currentTransform, &m_worldTransform );

	D3DXVec3TransformCoord( &position, (Vector3*)&m_boundingSphere, &currentTransform );
}

void EveEffectRoot::GetCurrentModelCenterWorldPosition( Vector3 &position )
{
	// This version of the function does not perform an update on the object
	D3DXVec3TransformCoord( &position, (Vector3*)&m_boundingSphere, &m_lastUpdateMatrix );
}


bool EveEffectRoot::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	// If possible, return an AABB in local coordinates
	return false;
}

void EveEffectRoot::GetLocalToWorldTransform( Matrix &transform )
{
	// Get the local to world transform
	transform = m_lastUpdateMatrix;
}

// --------------------------------------------------------------------------------
// Description:
//   Starts the effect
// --------------------------------------------------------------------------------
void EveEffectRoot::Start()
{
	m_playEffect = true;
	m_effectObject = nullptr;
	m_lodLevel = LOD_INVALID;
}

// --------------------------------------------------------------------------------
// Description:
//   Stops the effect
// --------------------------------------------------------------------------------
void EveEffectRoot::Stop()
{
	m_playEffect = false;
	m_effectObject = nullptr;
	m_lodLevel = LOD_INVALID;
	m_startTime = 0;

	if( m_highDetail )
	{
		m_highDetail->ClearObject();
	}
	if( m_mediumDetail )
	{
		m_mediumDetail->ClearObject();
	}
	if( m_lowDetail )
	{
		m_lowDetail->ClearObject();
	}
}