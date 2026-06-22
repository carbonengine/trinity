// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightBaseGroup.h"

EveSmartLightBaseGroup::EveSmartLightBaseGroup( IRoot* lockobj ) :
	PARENTLOCK( m_attributeModifiers ),
	m_color( 0.f, 0.f, 0.f, 1.f ),
	m_useFactionColor( false ),
	m_selectedColor( -1 ),
	m_parentColorSet( nullptr )
{
	m_attributeModifiers.SetNotify( this );
}

void EveSmartLightBaseGroup::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if( theList == &m_attributeModifiers )
	{
		if( event == BELIST_INSERTED && m_parentColorSet != nullptr )
		{
			if( IEveSmartLightGroupAttributeModifierPtr attributeModifier = BlueCastPtr( value ) )
			{
				attributeModifier->SetInheritProperties( m_parentColorSet );
			}
		}
	}
}

void EveSmartLightBaseGroup::SetInheritProperties( const Color* colorSet )
{
	if( colorSet )
	{
		m_parentColorSet = colorSet;
	}

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->SetInheritProperties( colorSet );
	}
}

Color EveSmartLightBaseGroup::GetGroupColor() const
{
	if( m_useFactionColor && m_parentColorSet != nullptr )
	{
		if( m_selectedColor >= 0 && m_selectedColor < SOFDataFactionColorChooser::TYPE_MAX )
		{
			return m_parentColorSet[m_selectedColor];
		}
	}
	return m_color;
}

void EveSmartLightBaseGroup::SetColor( const Color& color )
{
	m_color = color;
}

void EveSmartLightBaseGroup::SetControllerVariable( const char* name, float value )
{
	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->SetControllerVariable( name, value );
	}
}
