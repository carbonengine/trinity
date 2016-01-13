////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveLineContainer.h"
#include "Eve/EveCurveLineSet.h"
#include "EveConnector.h"
#include "Eve/EveUpdateContext.h"
#include "TriFrustum.h"
#include "ITr2Renderable.h"

EveLineContainer::EveLineContainer( IRoot* lockobj ) :
	PARENTLOCK( m_connectors )
{
}

EveLineContainer::~EveLineContainer()
{
}

void EveLineContainer::Update( EveUpdateContext& context )
{
	if( !m_lineSet )
	{
		return;
	}

	m_lineSet->ClearLines();
	for( auto it = m_connectors.begin(); it != m_connectors.end(); it++ )
	{
		(*it)->Update( context );
		(*it)->AddLine( m_lineSet );
	}
	m_lineSet->SubmitChanges();
}

void EveLineContainer::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( m_lineSet )
	{
		m_lineSet->GetRenderables( frustum, renderables, parentTransform );
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
void EveLineContainer::GetModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	if( m_lineSet )
	{
		m_lineSet->GetModelCenterWorldPosition( position, t );
	}
}

// This version of the function should not update the object
void EveLineContainer::GetCurrentModelCenterWorldPosition( Vector3 &position )
{
	if( m_lineSet )
	{
		m_lineSet->GetCurrentModelCenterWorldPosition( position );
	}
}

// If possible, return an AABB in local coordinates
bool EveLineContainer::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	if( m_lineSet )
	{
		return m_lineSet->GetLocalBoundingBox( min, max );
	}
	return false;
}

// Get the local to world transform
void EveLineContainer::GetLocalToWorldTransform( Matrix &transform ) const
{
	if( m_lineSet )
	{
		m_lineSet->GetLocalToWorldTransform( transform );
	}
}