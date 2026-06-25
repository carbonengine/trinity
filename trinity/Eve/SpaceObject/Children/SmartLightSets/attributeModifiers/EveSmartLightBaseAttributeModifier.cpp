// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "include/TriMath.h"

EveSmartLightBaseAttributeModifier::EveSmartLightBaseAttributeModifier( IRoot* lockobj ) :
	m_activationStrength( 1.f ),
	m_active( true ),
	m_startsActive( true ),
	m_isChangingActivation( false ),
	m_crossFadeDuration( 1.f ),
	m_crossFadeIntensity( 1.f ),
	m_activationValuePreMapped( 1.f ),
	m_activationValue( 1.f ),
	m_finalActivationStrength( 1.f ),
	m_perInstanceOffset( 0.f ),
	m_playTime( 0.f ),
	m_delayedActivation( 0.f ),
	m_lastActivationTimeStamp( 0.f ),
	m_lifeTimeFormula( PER_INSTANCE_LIFETIME ),
	m_restartPlayTimeWhenInactive( true )
{
	m_name = "";
}

bool EveSmartLightBaseAttributeModifier::Initialize()
{
	m_isChangingActivation = m_active && !m_startsActive;
	m_activationValuePreMapped = m_isChangingActivation ? 0.f : float( m_active );
	this->MapActivationValue();
	return true;
}

bool EveSmartLightBaseAttributeModifier::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_active ) )
	{
		m_isChangingActivation = true;
		if( m_crossFadeIntensity > 0.f )
		{
			m_activationValuePreMapped = pow( m_activationValuePreMapped, 1.f / m_crossFadeIntensity );
		}
		ResetPlayTime( m_active );
	}

	return true;
}

void EveSmartLightBaseAttributeModifier::ResetPlayTime( bool active )
{
	if( active != m_active )
	{
		m_isChangingActivation = true;
	}

	m_active = active;
	if( m_restartPlayTimeWhenInactive && !m_active )
	{
		m_playTime = 0.f;
	}
	m_lastActivationTimeStamp = m_playTime;
}

void EveSmartLightBaseAttributeModifier::MapActivationValue()
{
	float scaleValue = m_active ? m_activationValuePreMapped : 1.f - m_activationValuePreMapped;
	m_activationValue = pow( scaleValue, m_crossFadeIntensity );
	m_activationValue = m_active ? m_activationValue : 1.f - m_activationValue;
}

void EveSmartLightBaseAttributeModifier::UpdateActivationStrength( float parentActivationMultiplier, float deltaTime )
{
	if( m_isChangingActivation )
	{
		float activationTime = m_lastActivationTimeStamp + m_delayedActivation;
		if( m_playTime < activationTime && m_active )
		{
			if( parentActivationMultiplier > 0.f )
			{
				m_playTime += deltaTime;
			}
			return;
		}
		else
		{
			if( m_crossFadeDuration == 0.f )
			{
				m_activationValuePreMapped = m_active ? 1.f : 0.f;
			}
			else
			{
				float valueAdjustment = deltaTime / m_crossFadeDuration;
				valueAdjustment = m_active ? valueAdjustment : -valueAdjustment;
				m_activationValuePreMapped += valueAdjustment;
				m_activationValuePreMapped = TriClamp( m_activationValuePreMapped, 0.f, 1.f );
			}

			this->MapActivationValue();

			bool finishedActivating = m_active && m_activationValuePreMapped >= 1.f;
			bool finishedDeActivating = !m_active && m_activationValuePreMapped <= 0.f;

			if( finishedActivating || finishedDeActivating )
			{
				m_isChangingActivation = false;
				if( finishedDeActivating && m_restartPlayTimeWhenInactive )
				{
					ResetChildren( false );
				}
				if( finishedActivating )
				{
					ResetChildren( true );
				}
			}
		}
	}

	m_finalActivationStrength = parentActivationMultiplier * m_activationStrength * m_activationValue;

	if( m_finalActivationStrength > 0.f )
	{
		m_playTime += deltaTime;
	}
}

float EveSmartLightBaseAttributeModifier::GetActivationStrength( const PlacementDataWithIdentifier& placement )
{
	float activationMultiplier = 1.f;

	if( m_activationOverLifetime )
	{
		float idOffset = float( placement.initialPlacementID ) * m_perInstanceOffset;
		switch( m_lifeTimeFormula )
		{
		case PER_INSTANCE_LIFETIME:
			activationMultiplier = m_activationOverLifetime->GetValueAt( double( placement.lifeTime + idOffset ) );
			break;
		case PER_MODIFIER_PLAYTIME:
			activationMultiplier = m_activationOverLifetime->GetValueAt( double( m_playTime + idOffset ) );
			break;
		case STATIC:
			activationMultiplier = m_activationOverLifetime->GetValueAt( double( idOffset ) );
			break;
		default:
			break;
		}
	}

	return m_finalActivationStrength * activationMultiplier;
}
