// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionPlacementGenerators.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"

BLUE_CLASS( EveDistributionPlacementGeneratorParentLocators ) :
	public IEveDistributionPlacementGenerators,
	public INotify,
	public IBlueStructureListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionPlacementGeneratorParentLocators( IRoot* lockobj = nullptr );
	~EveDistributionPlacementGeneratorParentLocators();

	void GetInitialPlacements( std::vector<InitialPlacement> & placements, uint32_t& trackingID ) override;
	bool IsRequestingRegeneration() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

	// INotify
	bool OnModified( Be::Var * value ) override;

	// IBlueStructureListNotify
	void OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list ) override;

private:
	void BuildInitialLocatorsFromParent( IEveSpaceObject2 * parent );
	bool m_regenerated;
	BlueSharedString m_locatorSetName;
	bool m_requestRegeneration;

	LocatorStructureListPtr m_locators;

	IBlueStructureListNotify* m_oldNotify;
};

TYPEDEF_BLUECLASS( EveDistributionPlacementGeneratorParentLocators );
