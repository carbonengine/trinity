// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierColor.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

EveSmartLightAttributeModifierColor::EveSmartLightAttributeModifierColor( IRoot* lockobj ) :
	m_saturationMultiplier( 1.f ),
	m_brightnessMultiplier( 1.f ),
	m_blendValue( 1.f ),
	m_blendColor( 0, 0, 0, 1 ),
	m_useFactionColor( false ),
	m_selectedColor( -1 ),
	m_parentColorSet( nullptr )
{
}

void EveSmartLightAttributeModifierColor::SetInheritProperties( const Color* colorSet )
{
	if( colorSet )
	{
		m_parentColorSet = colorSet;
	}
}

Color EveSmartLightAttributeModifierColor::GetGroupColor() const
{
	if( m_useFactionColor && m_parentColorSet != nullptr )
	{
		if( m_selectedColor >= 0 && m_selectedColor < SOFDataFactionColorChooser::TYPE_MAX )
		{
			return m_parentColorSet[m_selectedColor];
		}
	}
	return m_blendColor;
}

void EveSmartLightAttributeModifierColor::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier )
{
	this->UpdateActivationStrength( activationMultiplier, updateContext.GetDeltaT() );
}

void EveSmartLightAttributeModifierColor::ProcessAttributeModifier( Vector3& attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength )
{
	float activationStrength = this->GetActivationStrength( placement ) * modifierStrength;

	if( activationStrength <= 0.f )
	{
		return;
	}

	float activationAdjustedBrightnessMultiplier = 1.f + activationStrength * ( m_brightnessMultiplier - 1.f );
	float activationAdjustedBlendValue = 0.f + activationStrength * m_blendValue;
	float activationAdjustedSaturationMultiplier = 1.f + activationStrength * ( m_saturationMultiplier - 1.f );

	Color activeColor = Lerp( Color( attribute.x, attribute.y, attribute.z, 1.f ), GetGroupColor(), activationAdjustedBlendValue );

	if( activationAdjustedSaturationMultiplier != 1.f )
	{
		// color intensity
		float i = ( activeColor.r * 0.299f ) + ( activeColor.g * 0.587f ) + ( activeColor.b * 0.114f );
		activeColor = Lerp( Color( i, i, i, i ), activeColor, max( 0.0f, activationAdjustedSaturationMultiplier ) );
	}

	activeColor *= activationAdjustedBrightnessMultiplier;
	attribute.x = min( 1.f, max( 0.f, activeColor.r ) );
	attribute.y = min( 1.f, max( 0.f, activeColor.g ) );
	attribute.z = min( 1.f, max( 0.f, activeColor.b ) );
}
