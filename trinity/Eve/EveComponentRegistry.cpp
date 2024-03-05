////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#include "StdAfx.h"
#include "EveComponentRegistry.h"
#include "../ITr2VolumetricRenderable.h"

EveComponentRegistry::EveComponentRegistry( IRoot* lockobj )
{
	m_componentCollections = std::vector<std::pair<const char*, std::unique_ptr<IEveComponentCollection>>>();
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
}

void EveComponentRegistry::ReRegister( EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	entity->UnRegister( this );
	entity->Register( this );
}

// UnRegisters all components for a specific entity
void EveComponentRegistry::UnRegisterAllComponents( EveEntity* entity )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	for( auto& it : m_componentCollections )
	{
		if( it.second->HasState( entity->m_state ) )
		{
			it.second->Remove( entity );
		}
	}
	entity->m_state = 0;
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
	if( !collection->HasState( entity->m_state ) )
	{
		collection->Add( entity );
		collection->AddState( entity->m_state );
	}
}

void EveComponentRegistry::RemoveFromCollection( IEveComponentCollection* collection, EveEntity* entity )
{
	if( collection->HasState( entity->m_state ) )
	{
		collection->Remove( entity );
		collection->RemoveState( entity->m_state );
	}
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

