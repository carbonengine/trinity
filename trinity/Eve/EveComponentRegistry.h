// Copyright © 2021 CCP ehf.

#pragma once

BLUE_DECLARE( EveEntity );
BLUE_DECLARE( EveComponentRegistry );

#include "EveEntity.h"
#include <shared_mutex>
#include <numeric>

// Yes templated false, see comment in GetComponentName
template <typename T>
struct workAroundForCppStandard : std::false_type
{
};

template <typename T>
inline const char* GetComponentName()
{
	/*
	We cannot use normal false values here due to the fact that the compiler might use this method
	before it knows of an templated (or registered) function...
	So we use a templated false value forcing the compiler to evaluate this when all the templating has been done ( I think )
	*/
	static_assert( workAroundForCppStandard<T>::value, "Type being used as a component which hasn't been registered via REGISTER_COMPONENT_TYPE" );
}

#define REGISTER_COMPONENT_TYPE( name, componentType )   \
                                                         \
	template <>                                          \
	inline const char* GetComponentName<componentType>() \
	{                                                    \
		return name;                                     \
	};

class IEveComponentCollection
{
public:
	virtual ~IEveComponentCollection() = default;

	virtual bool Add( EveEntity* entity, uint32_t* index ) = 0;
	virtual EveEntity* SwapWithBack( uint32_t index ) = 0;
	virtual void Clear() = 0;
	virtual uint32_t GetBit() const = 0;

	virtual size_t Size() const = 0;
};

template <typename T>
class EveComponentCollection : public IEveComponentCollection
{
public:
	EveComponentCollection( uint32_t id, const char* name );
	~EveComponentCollection();

	bool Add( EveEntity* entity, uint32_t* index ) override;
	EveEntity* SwapWithBack( uint32_t index ) override;
	void Clear() override;
	size_t Size() const override;

	uint32_t GetBit() const override;

private:
	const char* m_name;
	uint32_t m_bit;
	std::vector<T*> m_collection;
	friend class EveComponentRegistry;
};


BLUE_CLASS( EveComponentRegistry ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveComponentRegistry( IRoot* lockobj = NULL );
	~EveComponentRegistry();

	// Registers a specific components for a specific entity, creates a component collection if it doesn´t exist
	template <typename T>
	void RegisterComponent( EveEntity * entity );

	// UnRegisters a single component for a specific entity
	template <typename T>
	void UnRegisterComponent( EveEntity * entity );

	// UnRegisters all components for a specific entity
	void UnRegisterAllComponents( EveEntity * entity );

	void Clear();

	template <typename T>
	void Clear();

	// Runs over all the components and calls the processor to process it, returns true/false if it ran the processor
	template <typename T, typename R>
	void ProcessComponents( R processor ) const;

	// Runs over the components until the function returns true
	template <typename T, typename R>
	void ProcessComponentsUntil( R processor ) const;

	template <typename T>
	const std::vector<T*>& GetComponents();

	template <typename T>
	size_t ComponentCount() const;

	void Register( EveEntity * entity );
	void UnRegister( EveEntity * entity );
	void ReRegister( EveEntity * entity );

private:
	IEveComponentCollection* GetComponentCollection( const char* componentName ) const;

	void AddToCollection( IEveComponentCollection * collection, EveEntity * entity );
	void RemoveFromCollection( IEveComponentCollection * collection, EveEntity * entity );
	void RemoveCollectionFromEntityState( IEveComponentCollection * collection, EveEntity * entity );

	template <typename T>
	IEveComponentCollection* AddCollection( const char* componentName );

	std::vector<std::pair<const char*, size_t>> GetComponentInfo() const;

	// this m_componentCollections is a map of componentType to different EveComponentCollection<T>
	std::vector<std::pair<const char*, std::unique_ptr<IEveComponentCollection>>> m_componentCollections;
	mutable std::shared_mutex m_componentCollectionLoopGuard;

	std::vector<EveEntity*> m_registeredEntities;
};
TYPEDEF_BLUECLASS( EveComponentRegistry );


template <typename T>
EveComponentCollection<T>::EveComponentCollection( uint32_t id, const char* name ) :
	m_bit( 1 << id ),
	m_collection( std::vector<T*>() ),
	m_name( name )
{
}

template <typename T>
EveComponentCollection<T>::~EveComponentCollection()
{
}

template <typename T>
bool EveComponentCollection<T>::Add( EveEntity* entity, uint32_t* index )
{
	if( T* component = dynamic_cast<T*>( entity ) )
	{
		*index = (uint32_t)m_collection.size();
		m_collection.push_back( component );
		return true;
	}
	CCP_ASSERT_M( false, "Entity is not of correct type" );
	return false;
}

template <typename T>
EveEntity* EveComponentCollection<T>::SwapWithBack( uint32_t index )
{
	EveEntity* swappedEntity = nullptr;
	// removing the only element or the last element needs no swapping
	if( m_collection.size() != 1 && index != m_collection.size() - 1 )
	{
		m_collection[index] = m_collection.back();
		swappedEntity = dynamic_cast<EveEntity*>( m_collection[index] );
	}
	m_collection.pop_back();
	return swappedEntity;
}

template <typename T>
void EveComponentCollection<T>::Clear()
{
	m_collection.clear();
}

template <typename T>
uint32_t EveComponentCollection<T>::GetBit() const
{
	return m_bit;
}

template <typename T>
size_t EveComponentCollection<T>::Size() const
{
	return m_collection.size();
}

// Registers a specific components for a specific entity, creates a component collection if it doesn´t exist
template <typename T>
void EveComponentRegistry::RegisterComponent( EveEntity* entity )
{
	const char* componentName = GetComponentName<T>();

	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	IEveComponentCollection* collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		collection = AddCollection<T>( componentName );
	}
	AddToCollection( collection, entity );
}

// UnRegisters a single components for a specific entity
template <typename T>
void EveComponentRegistry::UnRegisterComponent( EveEntity* entity )
{
	const char* componentName = GetComponentName<T>();

	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	auto collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		CCP_LOGERR( "EveComponentRegistry: RemoveFromCollection '%s' has not been registered as a component", GetComponentName<T>() );
		return;
	}

	RemoveFromCollection( collection, entity );
}


template <typename T>
void EveComponentRegistry::Clear()
{
	const char* componentName = GetComponentName<T>();
	std::unique_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );
	auto collection = GetComponentCollection( componentName );
	if( collection == nullptr )
	{
		return;
	}
	for( auto& component : static_cast<EveComponentCollection<T>*>( collection )->m_collection )
	{
		EveEntity* entity = dynamic_cast<EveEntity*>( component );
		RemoveCollectionFromEntityState( collection, entity );
	}
	collection->Clear();
}

template <typename T, typename R>
void EveComponentRegistry::ProcessComponents( R processor ) const
{
	const char* componentName = GetComponentName<T>();

	std::shared_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	auto collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		return;
	}

	for( auto& component : static_cast<EveComponentCollection<T>*>( collection )->m_collection )
	{
		processor( component );
	}
}

template <typename T, typename R>
void EveComponentRegistry::ProcessComponentsUntil( R processor ) const
{
	const char* componentName = GetComponentName<T>();

	std::shared_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	auto collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		return;
	}

	for( auto& component : static_cast<EveComponentCollection<T>*>( collection )->m_collection )
	{
		if( processor( component ) )
		{
			return;
		}
	}
}

template <typename T>
const std::vector<T*>& EveComponentRegistry::GetComponents()
{
	const char* componentName = GetComponentName<T>();

	std::shared_lock<std::shared_mutex> lock( m_componentCollectionLoopGuard );

	auto collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		static auto v = std::vector<T*>();
		return v;
	}
	return static_cast<EveComponentCollection<T>*>( collection )->m_collection;
}

template <typename T>
size_t EveComponentRegistry::ComponentCount() const
{
	const char* componentName = GetComponentName<T>();
	auto collection = GetComponentCollection( componentName );

	if( collection == nullptr )
	{
		return 0;
	}

	return collection->Size();
}

template <typename T>
IEveComponentCollection* EveComponentRegistry::AddCollection( const char* componentName )
{
	int32_t componentCollectionIndex = (int32_t)m_componentCollections.size();
	// no more than 32 EveComponentCollections can exist, since we use a bitwise comparison in the entity m_state
	CCP_ASSERT( componentCollectionIndex < 32 );

	m_componentCollections.push_back( { componentName, std::make_unique<EveComponentCollection<T>>( componentCollectionIndex, componentName ) } );
	return m_componentCollections.back().second.get();
}