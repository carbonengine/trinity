////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveFiringEffectElementContainer.h"
#include "Eve/IEveFiringEffectElement.h"


EveFiringEffectElementContainer::EveFiringEffectElementContainer( IRoot* lockObj )
	:m_source( XMMatrixIdentity() ),
	m_destination( 0, 0, 0 ),
	m_destinationScale( 1.f ),
	m_display( true ),
	m_useSourceTransform( false ),
	m_displaySource( true ),
	m_displayDestination( true ),
	m_isActive( false )
{
}

void EveFiringEffectElementContainer::UpdateSyncronous( EveUpdateContext& updateContext )
{
	if( m_element )
	{
		if( m_useSourceTransform )
		{
			m_element->SetFiringTransform( m_source, m_destination );
		}
		else
		{
			m_element->SetFiringTransform( m_source.GetTranslation(), m_destination );
		}
		m_element->SetDestObjectScale( m_destinationScale );
		m_element->DisplayEndPoints( m_displaySource, m_displayDestination );
		if( m_isActive )
		{
			m_element->Update( updateContext );
		}
	}
}

void EveFiringEffectElementContainer::UpdateAsyncronous( EveUpdateContext& updateContext )
{
}

void EveFiringEffectElementContainer::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	if( m_element )
	{
		m_element->UpdateVisibility( frustum, parentTransform );
	}
}
void EveFiringEffectElementContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( m_display && m_element )
	{
		m_element->GetRenderables( renderables );
	}
}

bool EveFiringEffectElementContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return false;
}

void EveFiringEffectElementContainer::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
}

void EveFiringEffectElementContainer::GetModelCenterWorldPosition( Vector3 &position ) const
{
}

bool EveFiringEffectElementContainer::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	return false;
}

void EveFiringEffectElementContainer::GetLocalToWorldTransform( Matrix &transform ) const
{
	transform = XMMatrixIdentity();
}

void EveFiringEffectElementContainer::StartFiring()
{
	if( m_element )
	{
		m_element->StartFiring( 0 );
		m_isActive = true;
	}
}

void EveFiringEffectElementContainer::StopFiring()
{
	if( m_element )
	{
		m_element->StopFiring();
		m_isActive = false;
	}
}

void EveFiringEffectElementContainer::SetActive( bool active )
{
	if( active != m_isActive )
	{
		if( active )
		{
			StartFiring();
		}
		else
		{
			StopFiring();
		}
	}
}

bool EveFiringEffectElementContainer::GetActive() const
{
	return m_isActive;
}

void EveFiringEffectElementContainer::GetLights( Tr2LightManager& lightManager ) const
{
	if( m_element )
	{
		m_element->GetLights( lightManager );
	}
}