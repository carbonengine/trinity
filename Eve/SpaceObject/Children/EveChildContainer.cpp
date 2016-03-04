////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"

#include "Utilities/BoundingSphere.h"
#include "Curves/TriCurveSet.h"
#include "Eve/EveUpdateContext.h"

EveChildContainer::EveChildContainer( IRoot* lockobj ):
	EveChildTransform(),
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_curveSets ),
	m_display( true )
{
}

EveChildContainer::~EveChildContainer()
{
}

void EveChildContainer::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->GetRenderables( frustum, renderables, parentTransform, parentLod );
	}
}

bool EveChildContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool success = false;
	Vector4 bSphere( 0, 0, 0, -1 );
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( (*it)->GetBoundingSphere( bSphere ) )
		{
			BoundingSphereSetOrUpdate( bSphere, sphere, success );
			success = true;
		}
	}
	return success;
}
	
void EveChildContainer::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateSyncronous( updateContext, nullptr, this );
	}
}

void EveChildContainer::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	Matrix localToWorldTransform;
	if( spaceObjectParent )
	{
		spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if ( childParent )
	{
		childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else
	{
		return;
	}

	UpdateTransform( localToWorldTransform );

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateAsyncronous( updateContext, nullptr, this );
	}
	
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Update( time, time );
	}
}


void EveChildContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildContainer::ChangeLOD( Tr2Lod lod )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->ChangeLOD( lod );
	}
}

void EveChildContainer::PlayCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->PlayCurveSet( name );
	}
}
void EveChildContainer::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Stop();
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->StopCurveSet( name );
	}
}
float EveChildContainer::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			maxDuration = max( maxDuration, (*it)->GetMaxCurveDuration() );
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		maxDuration = max( maxDuration, (*it)->GetCurveSetDuration( name ) );
	}

	return maxDuration;
}

void EveChildContainer::Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->Transform( scale, rotation, translation );
	}
}