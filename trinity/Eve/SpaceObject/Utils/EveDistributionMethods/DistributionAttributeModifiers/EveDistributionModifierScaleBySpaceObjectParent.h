// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionModifier.h"

#include <ITriFunction.h>

BLUE_CLASS( EveDistributionModifierScaleBySpaceObjectParent ) :
	public IEveDistributionModifier
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionModifierScaleBySpaceObjectParent( IRoot* lockobj = nullptr );

	bool AffectsTransform() override;
	DistributionEntityLifeTimeEvent ProcessDistributionModifier( PlacementDataWithIdentifier & placement, float deltaTime, const EveChildUpdateParams& params );

private:
	float m_authoredForBoundingRadius;
	float m_scaleFactor;
	ITriVectorFunctionPtr m_scaleCurve;
};

TYPEDEF_BLUECLASS( EveDistributionModifierScaleBySpaceObjectParent );
