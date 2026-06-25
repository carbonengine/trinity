// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObject//Utils/EveDistributionMethods/DistributionAttributeModifiers/IEveDistributionModifier.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

BLUE_INTERFACE( IEveDistributionMethod ) :
	public IRoot
{
	virtual void RegeneratePlacementData() = 0;
	virtual size_t GetNumberOfPlacements() = 0;
	virtual const PlacementDataWithIdentifierStructureList* GetPlacementData() const = 0;
	virtual const Vector3 GetPlacementDataCenter() const = 0;
	virtual const bool GetHasDynamicMovement() const = 0;

	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) {};
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) {};
	virtual void SetControllerVariable( const char* name, float value ) {};
};
