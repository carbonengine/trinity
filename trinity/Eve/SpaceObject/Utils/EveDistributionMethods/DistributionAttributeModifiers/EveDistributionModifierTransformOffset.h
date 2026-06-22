// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionModifier.h"

#include <ITriFunction.h>

BLUE_CLASS( EveDistributionModifierTransformOffset ) :
	public IEveDistributionModifier
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionModifierTransformOffset( IRoot* lockobj = nullptr );

	bool AffectsTransform() override;
	DistributionEntityLifeTimeEvent ProcessDistributionModifier( PlacementDataWithIdentifier & placement, float deltaTime, const EveChildUpdateParams& params );

private:
	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scale;

	ITriVectorFunctionPtr m_translationCurve;
	ITriQuaternionFunctionPtr m_rotationCurve;
	ITriVectorFunctionPtr m_scaleCurve;
};

TYPEDEF_BLUECLASS( EveDistributionModifierTransformOffset );
