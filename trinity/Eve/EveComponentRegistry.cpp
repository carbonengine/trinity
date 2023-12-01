////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#include "StdAfx.h"
#include "EveComponentRegistry.h"
#include "EveEntity.h"
#include "../ITr2VolumetricRenderable.h"

EveComponentRegistry::EveComponentRegistry(IRoot* lockobj)
{
	m_reflectionRenderables = std::vector<ITr2Renderable*>();
	m_lightOwners = std::vector<ITr2LightOwner*>();
	m_entityReregisterMutex = CCP_NEW( "EveComponentRegistry::m_entityReregisterMutex" ) CcpMutex( "EveComponentRegistry", "m_entityReregisterMutex", 1000 );
}

EveComponentRegistry::~EveComponentRegistry()
{
	CCP_DELETE m_entityReregisterMutex;
}

void EveComponentRegistry::Clear()
{
	m_entityReregisterMutex->Acquire();
	
	while(m_reflectionRenderables.size() > 0)
	{
		ITr2Renderable* reflectionObject = m_reflectionRenderables.back();
		if( reflectionObject != nullptr )
		{
			if( EveEntityPtr entity = BlueCastPtr( reflectionObject ) )
			{
				// this guy modifies m_reflectionRenderables
				entity->UnRegister( this );
			}
		}
		else
		{
			m_reflectionRenderables.pop_back();
		}
	}
	m_entityReregisterMutex->Release();

}

void EveComponentRegistry::RegisterComponent( ComponentType type, EveEntity* entity, RegistrationState& state )
{
	if(HasComponent(type, state))
	{
		CCP_LOGERR( "EveComponentRegistry: RegisterComponent(%d) when entity already has that component", type );
		return;
	}

	m_entityReregisterMutex->Acquire();
	switch( type )
	{
	case REFLECTION_RENDERABLE:
		if( ITr2RenderablePtr component = BlueCastPtr( entity ) )
		{
			state.reflectionRenderable = true;
			m_reflectionRenderables.push_back( component );
			break;
		}
		CCP_LOGERR( "EveComponentRegistry: RegisterComponent(%d) entity is not ITr2Renderable", type );

		break;
	case VOLUMETRIC_RENDERABLE:
		if( ITr2VolumetricRenderablePtr component = BlueCastPtr( entity ) )
		{
			state.volumetricRenderable = true;
			m_volumetricRenderables.push_back( component );
			break;
		}
		CCP_LOGERR( "EveComponentRegistry: RegisterComponent(%d) entity is not ITr2Renderable", type );

		break;
	default:
		break;
	}
	m_entityReregisterMutex->Release();
}

void EveComponentRegistry::UnRegisterComponent( ComponentType type, EveEntity* entity, RegistrationState& state, bool silent )
{
	if( !HasComponent( type, state ) )
	{
		if( !silent )
		{
			CCP_LOGERR( "EveComponentRegistry: UnRegisterComponent(%d) but component was not found in list", type );
		}
		return;
	}

	m_entityReregisterMutex->Acquire();
	switch( type )
	{
	case REFLECTION_RENDERABLE:
		if( ITr2RenderablePtr component = BlueCastPtr( entity ) )
		{
			state.reflectionRenderable = false;
			auto it = std::find( m_reflectionRenderables.begin(), m_reflectionRenderables.end(), component );
			if( it != m_reflectionRenderables.end() )
			{
				m_reflectionRenderables.erase( it );
			}
		}
		break;
	case VOLUMETRIC_RENDERABLE:
		if( ITr2VolumetricRenderablePtr component = BlueCastPtr( entity ) )
		{
			state.volumetricRenderable = false;
			auto it = std::find( m_volumetricRenderables.begin(), m_volumetricRenderables.end(), component );
			if( it != m_volumetricRenderables.end() )
			{
				m_volumetricRenderables.erase( it );
			}
		}
		break;
	default:
		break;
	}
	m_entityReregisterMutex->Release();
}

bool EveComponentRegistry::HasComponent( ComponentType type, RegistrationState& state )
{
	switch( type )
	{
	case REFLECTION_RENDERABLE:
		return state.reflectionRenderable;
	case VOLUMETRIC_RENDERABLE:
		return state.volumetricRenderable;
	default:
		return false;
	}
}

void EveComponentRegistry::ReRegister( EveEntity* entity )
{
	m_entityReregisterMutex->Acquire();

	entity->UnRegister( this );
	entity->Register( this );

	m_entityReregisterMutex->Release();
}


size_t EveComponentRegistry::GetLightOwnerCount() const
{
	return GetLightOwners().size();
}


size_t EveComponentRegistry::GetReflectionRenderableCount() const
{
	return GetReflectionRenderables().size();
}
