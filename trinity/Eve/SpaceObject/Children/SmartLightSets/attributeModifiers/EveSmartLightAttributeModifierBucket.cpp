// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierBucket.h"

EveSmartLightAttributeModifierBucket::EveSmartLightAttributeModifierBucket( IRoot* lockobj ) :
	PARENTLOCK( m_attributeModifiers ),
	m_name( "bucket" )
{
	m_restartPlayTimeWhenInactive = true;
}

void EveSmartLightAttributeModifierBucket::SetActive( bool isActive )
{
	m_isChangingActivation = isActive != m_active;
	if( m_isChangingActivation )
	{
		this->ResetPlayTime( isActive );
	}
	m_active = isActive;
}

void EveSmartLightAttributeModifierBucket::ResetPlayTime( bool active )
{
	ResetChildren( active );
}

void EveSmartLightAttributeModifierBucket::SetInheritProperties( const Color* colorSet )
{
	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->SetInheritProperties( colorSet );
	}
}

void EveSmartLightAttributeModifierBucket::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier )
{
	this->UpdateActivationStrength( activationMultiplier, updateContext.GetDeltaT() );
	activationMultiplier *= m_finalActivationStrength;

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->UpdateSyncronous( updateContext, params, activationMultiplier );
	}
}

void EveSmartLightAttributeModifierBucket::ProcessAttributeModifier( Vector3& attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength )
{
	float activationStrength = this->GetActivationStrength( placement ) * modifierStrength;

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->ProcessAttributeModifier( attribute, placement, entityPosition, entityDirection, activationStrength );
	}
}

void EveSmartLightAttributeModifierBucket::ResetChildren( bool parentActive )
{
	for( auto attributeModifier : m_attributeModifiers )
	{
		if( EveSmartLightBaseAttributeModifierPtr attributeModPtr = BlueCastPtr( attributeModifier ) )
		{
			bool isActive = parentActive && m_active;
			attributeModPtr->ResetPlayTime( isActive );
		}
	}
}
