// Copyright © 2021 CCP ehf.

#include "StdAfx.h"
#include "EveComponentRegistry.h"
#include "../ITr2VolumetricRenderable.h"

EveComponentRegistry::EveComponentRegistry( IRoot* lockobj )
{
	m_componentCollections = std::vector<std::pair<const char*, std::unique_ptr<IEveComponentCollection>>>();
	m_registeredEntities = std::vector<EveEntity*>();
}

EveComponentRegistry::~EveComponentRegistry()
{
}

void EveComponentRegistry::Clear()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	for( auto& it : m_componentCollections )
	{
		it.second->Clear();
	}

	for( auto& entity : m_registeredEntities )
	{
		if( entity->m_registry == this )
		{
			// an entity may have been registered into another registry while we are clearing this one
			entity->m_indexInRegistry = -1;
			entity->m_registry = nullptr;
			entity->m_componentIndexLookup.clear();
		}
	}
	m_registeredEntities.clear();
}

void EveComponentRegistry::ReRegister( EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	entity->UnRegister( this );
	entity->Register( this );
}

void EveComponentRegistry::Register( EveEntity* entity )
{
	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	if( entity->m_indexInRegistry != -1 )
	{
		CCP_LOGERR( "EveComponentRegistry::Register: Entity is already registered." );
		return;
	}
	m_registeredEntities.push_back( entity );

	entity->m_registry = this;
	entity->m_indexInRegistry = m_registeredEntities.size() - 1;
}

void EveComponentRegistry::UnRegister( EveEntity* entity )
{
	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	if( entity->m_indexInRegistry == -1 )
	{
		CCP_LOGERR( "EveComponentRegistry::UnRegister: Entity is already unregistered." );
		return;
	}
	// single sized vector or last element don't need to be swapped
	if( m_registeredEntities.size() != 1 && entity->m_indexInRegistry != m_registeredEntities.size() - 1 )
	{
		auto backEntity = m_registeredEntities.back();
		backEntity->m_indexInRegistry = entity->m_indexInRegistry;
		m_registeredEntities[entity->m_indexInRegistry] = backEntity;
	}
	// pop the back element, which is now either the entity to be removed, or the one that was swapped
	m_registeredEntities.pop_back();

	entity->m_registry = nullptr;
	entity->m_indexInRegistry = -1;
}

// UnRegisters all components for a specific entity
void EveComponentRegistry::UnRegisterAllComponents( EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	for( auto& it : m_componentCollections )
	{
		RemoveFromCollection( it.second.get(), entity );
	}
}

IEveComponentCollection* EveComponentRegistry::GetComponentCollection( const char* componentName ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto& pair : m_componentCollections )
	{
		if( pair.first == componentName )
		{
			return pair.second.get();
		}
	}

	return nullptr;
}

void EveComponentRegistry::AddToCollection( IEveComponentCollection* collection, EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	auto collectionBit = collection->GetBit();

	if( !entity->GetComponentIndex( collectionBit ).has_value() )
	{
		uint32_t index;
		if( collection->Add( entity, &index ) )
		{
			entity->SetComponentState( collectionBit, index );
		}
	}
}

void EveComponentRegistry::RemoveFromCollection( IEveComponentCollection* collection, EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	auto collectionBit = collection->GetBit();

	auto entityIndex = entity->GetComponentIndex( collectionBit );

	if( entityIndex.has_value() )
	{
		EveEntity* swappedEntity = collection->SwapWithBack( entityIndex.value() );
		if( swappedEntity != nullptr )
		{
			swappedEntity->SetComponentState( collectionBit, entityIndex.value() );
		}
		entity->RemoveComponentState( collectionBit );
	}
}

void EveComponentRegistry::RemoveCollectionFromEntityState( IEveComponentCollection* collection, EveEntity* entity )
{
	entity->RemoveComponentState( collection->GetBit() );
}

std::vector<std::pair<const char*, size_t>> EveComponentRegistry::GetComponentInfo() const
{
	std::shared_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	auto info = std::vector<std::pair<const char*, size_t>>();
	for( auto& pair : m_componentCollections )
	{
		info.push_back( std::pair( pair.first, pair.second->Size() ) );
	}

	return info;
}
