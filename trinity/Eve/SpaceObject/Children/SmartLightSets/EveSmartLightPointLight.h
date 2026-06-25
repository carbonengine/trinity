// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "Lights/Tr2PointLight.h"
#include "EveSmartLightBaseGroup.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "Eve/SpaceObject/Children/EveChildTransform.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "Eve/EveUpdateContext.h"


BLUE_CLASS( EveSmartLightPointLight ) :
	public EveSmartLightBaseGroup,
	public IInitialize,
	public INotify,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightPointLight( IRoot* lockobj = nullptr );
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size ) override;

	void RegisterComponents() override;

	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	// INotify
	bool OnModified( Be::Var * value );

	// IInitialize
	bool Initialize();

protected:
	bool m_display;
	LightData m_lightGroupData;
	Tr2Light::LIGHT_TYPE m_lightType;
	Vector3 m_staticOffsetTranslation;
	Quaternion m_staticOffsetRotation;
	Matrix m_worldTransform;

private:
	std::string m_name;
	float m_activationStrength;
	std::wstring m_lightProfilePath;
	Tr2LightProfileResPtr m_lightProfile;
	IEveDistributionMethodPtr m_distribution;
};

TYPEDEF_BLUECLASS( EveSmartLightPointLight );
