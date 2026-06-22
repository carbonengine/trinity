// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightPointLight.h"
#include "Lights/Tr2PointLight.h"

BLUE_CLASS( EveSmartLightSpotLight ) :
	public EveSmartLightPointLight
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightSpotLight( IRoot* lockobj = nullptr );

	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size ) override;
};

TYPEDEF_BLUECLASS( EveSmartLightSpotLight );
