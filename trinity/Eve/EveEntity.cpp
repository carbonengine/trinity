// Copyright © 2021 CCP ehf.

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
	m_componentIndexLookup( {} ),
	m_indexInRegistry( -1 )
{
}

EveEntity::~EveEntity()
{
	if( m_registry )
	{
		CCP_LOGERR( "EveEntity being destroyed while still registered in a component registry." );
	}
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

	registry->Register( this );
	this->RegisterComponents();
}


/// Unregisters the entity, but only if it is registered with the registry, else we just ignore this call
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
	m_registry->UnRegister( this );
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

std::optional<uint32_t> EveEntity::GetComponentIndex( uint32_t componentBit ) const
{
	auto index = std::find_if( m_componentIndexLookup.begin(), m_componentIndexLookup.end(), [componentBit]( const auto& pair ) {
		return pair.first == componentBit;
	} );

	return index == m_componentIndexLookup.end() ? std::nullopt : std::make_optional( index->second );
}

void EveEntity::SetComponentState( uint32_t componentBit, uint32_t index )
{
	auto componentIndex = std::find_if( m_componentIndexLookup.begin(), m_componentIndexLookup.end(), [componentBit]( const auto& pair ) {
		return pair.first == componentBit;
	} );

	if( componentIndex == m_componentIndexLookup.end() )
	{
		m_componentIndexLookup.push_back( { componentBit, index } );
	}
	else
	{
		componentIndex->second = index;
	}
}

void EveEntity::RemoveComponentState( uint32_t componentBit )
{
	auto removed = std::remove_if( m_componentIndexLookup.begin(), m_componentIndexLookup.end(), [componentBit]( const auto& pair ) {
		return pair.first == componentBit;
	} );

	m_componentIndexLookup.erase( removed, m_componentIndexLookup.end() );
}
