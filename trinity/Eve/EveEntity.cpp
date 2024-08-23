////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#include "StdAfx.h"
#include "EveEntity.h"
#include "Tr2Renderer.h"

// setting from EveSpaceScene
extern int g_eveReflectionMode;

namespace EntityComponents
{

	bool ShouldReflect( ReflectionMode mode )
	{
		if( g_eveReflectionMode == ReflectionSetting::REFLECTION_SETTING_OFF )
		{
			return false;
		}

		switch( mode )
		{
		case REFLECT_NEVER:
			return false;
		case REFLECT_LOW_MEDIUM_HIGH:
			return true;
		case REFLECT_MEDIUM_AND_HIGH:
			return g_eveReflectionMode != ReflectionSetting::REFLECTION_SETTING_LOW; // we have either medium, high or highest settings
		case REFLECT_HIGH:
			return g_eveReflectionMode == ReflectionSetting::REFLECTION_SETTING_HIGH || g_eveReflectionMode == REFLECTION_SETTING_ULTRA;
		default:
			return false;
		}
	}
}

EveEntity::EveEntity( IRoot* root ) :
	m_registry( nullptr ),
	m_state( 0 )
{}

EveEntity::~EveEntity()
{
	m_registry = nullptr;
}

bool EveEntity::IsInRegistry() const
{
	return m_registry != nullptr;
}

/// Registers the entity to an component registry.
/// If the entity has been registered with another registry, then we first unregister the entity
void EveEntity::Register( EveComponentRegistry* registry )
{
	if( m_registry == registry )
	{
		return;
	}

	if( m_registry != nullptr )
	{
		this->UnRegister( m_registry );
	}

	if( registry == nullptr )
	{
		return;
	}

	m_registry = registry;
	this->RegisterComponents();
}


/// Unregisteres the entity, but only if it is registered with the registry, else we just ignore this call
void EveEntity::UnRegister( EveComponentRegistry* registry )
{
	if( m_registry != registry || registry == nullptr )
	{
		// can't unregister from a registry that is not the registry that we are registered to...
		return;
	}
	// unregister the components tied to this entity
	m_registry->UnRegisterAllComponents( this );

	// unregister children
	this->UnRegisterComponents();

	m_registry = nullptr;
}

void EveEntity::ReRegister()
{
	if( m_registry )
	{
		m_registry->ReRegister( this );
	}
}

EveComponentRegistry* EveEntity::GetComponentRegistry() const
{
	return m_registry;
}