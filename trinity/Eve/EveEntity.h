// Copyright © 2021 CCP ehf.

#pragma once
#include "EveComponentRegistry.h"
#include <optional>

namespace EntityComponents
{
enum ReflectionMode
{
	REFLECT_HIGH,
	REFLECT_MEDIUM_AND_HIGH,
	REFLECT_LOW_MEDIUM_HIGH,
	REFLECT_NEVER
};

enum ReflectionSetting
{
	REFLECTION_SETTING_OFF,
	REFLECTION_SETTING_LOW,
	REFLECTION_SETTING_MEDIUM,
	REFLECTION_SETTING_HIGH,
	REFLECTION_SETTING_ULTRA,
};

bool ShouldReflect( ReflectionMode mode );

extern const Be::VarChooser ReflectionModeChooser[];
}

BLUE_CLASS( EveEntity ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveEntity( IRoot* lockobj = NULL );
	~EveEntity();

	void Register( EveComponentRegistry * registry );
	void UnRegister( EveComponentRegistry * registry );

	bool IsInRegistry() const;

protected:
	void ReRegister();

	virtual void RegisterComponents() {};
	virtual void UnRegisterComponents() {};

	EveComponentRegistry* GetComponentRegistry() const;

private:
	std::optional<uint32_t> GetComponentIndex( uint32_t componentBit ) const;
	void SetComponentState( uint32_t state, uint32_t index );
	void RemoveComponentState( uint32_t state );

	EveComponentRegistry* m_registry;
	std::vector<std::pair<uint32_t, uint32_t>> m_componentIndexLookup;

	size_t m_indexInRegistry;
	friend class EveComponentRegistry;
};

TYPEDEF_BLUECLASS( EveEntity );