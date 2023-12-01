////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#pragma once
#include "EveComponentRegistry.h"

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

	extern const Be::VarChooser ReflectionModeChooser[] ;
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
	
	bool IsInRegistry() const
	{
		return m_registry != nullptr;
	};

protected:
	void ReRegister();

	virtual void RegisterComponents(){};
	virtual void UnRegisterComponents(){};

	EveComponentRegistry* GetComponentRegistry() const;

	RegistrationState m_state;

private:
	EveComponentRegistry* m_registry;
};

TYPEDEF_BLUECLASS( EveEntity );