// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveLineContainer.h"
#include "EveCurveLineSet.h"
#include "EveConnector.h"
#include "Eve/EveUpdateContext.h"
#include "TriFrustum.h"
#include "ITr2Renderable.h"

EveLineContainer::EveLineContainer( IRoot* lockobj ) :
	PARENTLOCK( m_connectors ),
	m_display( true )
{
}

EveLineContainer::~EveLineContainer()
{
}

void EveLineContainer::Update( const EveUpdateContext& context )
{
	if( !m_lineSet )
	{
		return;
	}

	m_lineSet->ClearLines();
	for( auto it = m_connectors.begin(); it != m_connectors.end(); it++ )
	{
		( *it )->Update( context );
		( *it )->AddLine( m_lineSet );
	}
	m_lineSet->SubmitChanges();
}

void EveLineContainer::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	if( m_lineSet )
	{
		m_lineSet->UpdateVisibility( updateContext, parentTransform );
	}
}

void EveLineContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	if( m_lineSet )
	{
		m_lineSet->GetRenderables( renderables, impostors );
	}
}

bool EveLineContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_lineSet )
	{
		return m_lineSet->GetBoundingSphere( sphere, query );
	}
	return false;
}

// This version of the function should perform an update on the model / ball position
void EveLineContainer::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
	if( m_lineSet )
	{
		m_lineSet->UpdateModelCenterWorldPosition( position, t );
	}
}

// This version of the function should not update the object
void EveLineContainer::GetModelCenterWorldPosition( Vector3& position ) const
{
	if( m_lineSet )
	{
		m_lineSet->GetModelCenterWorldPosition( position );
	}
}

// If possible, return an AABB in local coordinates
bool EveLineContainer::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	if( m_lineSet )
	{
		return m_lineSet->GetLocalBoundingBox( min, max );
	}
	return false;
}

// Get the local to world transform
void EveLineContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	if( m_lineSet )
	{
		m_lineSet->GetLocalToWorldTransform( transform );
	}
}