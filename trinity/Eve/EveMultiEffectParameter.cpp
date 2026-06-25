// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveMultiEffectParameter.h"

BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveEffectRoot2 );


EveMultiEffectParameter::EveMultiEffectParameter( IRoot* lockobj ) :
	m_object( nullptr ),
	m_owner( nullptr ),
	m_type( TYPE_UNDEFINED )
{
}

void EveMultiEffectParameter::SetOwner( EveMultiEffect* owner )
{
	this->m_owner = owner;
}

bool EveMultiEffectParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_object ) && this->m_owner != nullptr )
	{
		this->m_owner->Rebind();
	}
	return true;
}

void EveMultiEffectParameter::SetParameterObject( IRoot* object )
{
	m_object = object;
}

IRoot* EveMultiEffectParameter::GetParameterObject()
{
	return m_object;
}

bool EveMultiEffectParameter::IsValid()
{
	if( m_object == nullptr )
	{
		return false;
	}

	switch( m_type )
	{
	case TYPE_EVESPACEOBJECT:
		if( EveSpaceObject2Ptr ship = BlueCastPtr( m_object ) )
		{
			return true;
		}
		break;
	case TYPE_EVEEFFECTROOT:
		if( EveEffectRoot2Ptr effectRoot = BlueCastPtr( m_object ) )
		{
			return true;
		}
		break;
	case TYPE_ANYTHING:
		return true;
	default:
		break;
	}

	return false;
}

BlueSharedString EveMultiEffectParameter::GetName()
{
	return m_name;
}