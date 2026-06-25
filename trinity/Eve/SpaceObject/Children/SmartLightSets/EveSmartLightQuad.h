// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "Lights/Tr2PointLight.h"
#include "EveSmartLightBaseGroup.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "Eve/SpaceObject/Children/EveChildTransform.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "Eve/SpaceObject/Children/EveChildQuad.h"
#include "ITr2Renderable.h"


BLUE_CLASS( EveSmartLightQuad ) :
	public EveSmartLightBaseGroup,
	public EveChildTransform,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightQuad( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void AddQuadsToQuadRenderer( const PlacementDataWithIdentifierStructureList& placements, size_t size, const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;

	// INotify
	bool OnModified( Be::Var * value );

	// IInitialize
	bool Initialize();

private:
	struct SimplifiedQuad
	{
		Vector4 m_parentTransform0;
		Vector4 m_parentTransform1;
		Vector4 m_parentTransform2;
		Vector4 m_localTransform0;
		Vector4 m_localTransform1;
		Vector4 m_localTransform2;

		Float_16 m_color[4];
		Float_16 m_brightness[2];
	};

	std::string m_name;
	bool m_display;
	bool m_editMode;
	float m_activationStrength;

	Vector3 m_staticOffsetTranslation;
	Vector3 m_staticQuadScale;

	unsigned m_effectKey;
	Tr2EffectPtr m_effect;

	Color m_color;
	float m_brightness;

	bool m_softQuad;
};

TYPEDEF_BLUECLASS( EveSmartLightQuad );
