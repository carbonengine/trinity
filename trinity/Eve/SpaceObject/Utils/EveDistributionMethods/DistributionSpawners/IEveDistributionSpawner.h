// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "../DistributionAttributeModifiers/IEveDistributionModifier.h"
#include "Eve/EveUpdateContext.h"


BLUE_INTERFACE( IEveDistributionRulesParent ) :
	public IRoot
{
	virtual void AddEntities( uint32_t howMany ) = 0;
	virtual int32_t TriggerEntityByID( uint32_t entityID ) = 0;
	virtual PlacementDataWithIdentifier* GetInitialPlacementData( uint32_t index ) = 0;
	virtual uint32_t GetFreePlacementCount() = 0;
	virtual int32_t GetClosestFreePlacement( Vector3 & position ) = 0;
};

BLUE_INTERFACE( IEveDistributionSpawner ) :
	public IRoot
{
	virtual void Reset( const std::vector<InitialPlacement>& placements ) {};
	virtual void Restart() {}; // reset values without regenerating data
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) {};
	virtual void SetControllerVariable( const char* name, float value ) {};
};
