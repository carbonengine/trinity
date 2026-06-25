// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"

BLUE_DECLARE_INTERFACE( IEveSmartLightGroupAttributeModifier );
BLUE_DECLARE_IVECTOR( IEveSmartLightGroupAttributeModifier );

BLUE_CLASS( EveSmartLightAttributeModifierBucket ) :
	public EveSmartLightBaseAttributeModifier,
	public IEveSmartLightGroupAttributeModifier
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierBucket( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier ) override;
	void ProcessAttributeModifier( Vector3 & attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength ) override;
	void ResetPlayTime( bool active ) override;
	void SetActive( bool isActive );
	void SetInheritProperties( const Color* colorSet );

	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList );

protected:
	std::string m_name;
	PIEveSmartLightGroupAttributeModifierVector m_attributeModifiers;
	void ResetChildren( bool parentActive = true ) override;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierBucket );
