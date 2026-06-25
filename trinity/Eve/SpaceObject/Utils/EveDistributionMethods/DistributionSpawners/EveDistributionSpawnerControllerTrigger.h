// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawner.h"

BLUE_DECLARE_INTERFACE( IEveDistributionSpawner );
BLUE_DECLARE_IVECTOR( IEveDistributionSpawner );

BLUE_CLASS( EveDistributionSpawnerControllerTrigger ) :
	public IEveDistributionSpawner,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnerControllerTrigger( IRoot* lockobj = nullptr );

	void Reset( const std::vector<InitialPlacement>& placements ) override;
	void Restart() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;
	void SetControllerVariable( const char* name, float value ) override;

	bool OnModified( Be::Var * value ) override;

private:
	std::string m_variableName;
	float m_value;
	bool m_invertReceivedValue;
	bool m_isActive;
	bool m_restartOnReceivingValue;

	PIEveDistributionSpawnerVector m_distributionSpawners;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnerControllerTrigger );
