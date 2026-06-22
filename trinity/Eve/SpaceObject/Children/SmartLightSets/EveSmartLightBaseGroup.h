// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "IEveSmartLightGroup.h"

BLUE_DECLARE_INTERFACE( IEveSmartLightGroupAttributeModifier );
BLUE_DECLARE_IVECTOR( IEveSmartLightGroupAttributeModifier );

BLUE_CLASS( EveSmartLightBaseGroup ) :
	public IEveSmartLightGroup,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightBaseGroup( IRoot* lockobj = nullptr );
	Color GetGroupColor() const;

	void SetInheritProperties( const Color* colorSet ) override;
	void SetColor( const Color& color ) override;
	void SetControllerVariable( const char* name, float value ) override;

	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList );

protected:
	PIEveSmartLightGroupAttributeModifierVector m_attributeModifiers;
	Color m_color;
	int32_t m_selectedColor;
	bool m_useFactionColor;
	const Color* m_parentColorSet;
};

TYPEDEF_BLUECLASS( EveSmartLightBaseGroup );
