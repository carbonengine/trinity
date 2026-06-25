// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionMethod.h"
#include "DistributionAttributeModifiers/IEveDistributionModifier.h"
#include "DistributionSpawners/IEveDistributionSpawner.h"
#include "DistributionSpawnModifiers/IEveDistributionSpawnModifier.h"
#include "DistributionPlacementGenerators/IEveDistributionPlacementGenerators.h"

BLUE_DECLARE_INTERFACE( IEveDistributionMethodOwner );

BLUE_DECLARE_INTERFACE( IEveDistributionPlacementGenerators );
BLUE_DECLARE_IVECTOR( IEveDistributionPlacementGenerators );

BLUE_DECLARE_INTERFACE( IEveDistributionModifier );
BLUE_DECLARE_IVECTOR( IEveDistributionModifier );

BLUE_DECLARE_INTERFACE( IEveDistributionSpawner );
BLUE_DECLARE_IVECTOR( IEveDistributionSpawner );

BLUE_DECLARE_INTERFACE( IEveDistributionSpawnModifier );
BLUE_DECLARE_IVECTOR( IEveDistributionSpawnModifier );

BLUE_CLASS( EveBaseDistributionMethod ) :
	public IEveDistributionMethod,
	public IEveDistributionRulesParent,
	public IInitialize,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveBaseDistributionMethod( IRoot* lockobj = nullptr );

	void RestartDistribution();

	// IEveDistributionMethod
	void RegeneratePlacementData() override;
	size_t GetNumberOfPlacements() override;
	const PlacementDataWithIdentifierStructureList* GetPlacementData() const override;
	const Vector3 GetPlacementDataCenter() const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void SetControllerVariable( const char* name, float value ) override;
	const bool GetHasDynamicMovement() const override;

	// IEveDistributionRulesParent
	void AddEntities( uint32_t howMany = 1 ) override;
	int32_t TriggerEntityByID( uint32_t entityID ) override;
	uint32_t GetFreePlacementCount() override;
	int32_t GetClosestFreePlacement( Vector3 & position ) override;
	PlacementDataWithIdentifier* GetInitialPlacementData( uint32_t index ) override;

	// IInitialize
	bool Initialize() override;

	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

protected:
	int32_t GetInitialPlacementIndexByID( uint32_t entityID );
	void SwapInitialPlacements( uint32_t indexA, uint32_t indexB );
	void GetPlacement( PlacementDataWithIdentifier & placement, uint32_t entityID = 0 );
	void GetRandomPlacement( PlacementDataWithIdentifier & placement );
	Matrix GetInitialPlacementMatrixForPlacement( PlacementDataWithIdentifier & placement );

	std::vector<InitialPlacement> m_initialPlacements;
	std::map<uint32_t, uint32_t> m_uniqueIDIndices;
	uint32_t m_uniqueIDCounter;
	PPlacementDataWithIdentifierStructureList m_placementData;
	Vector3 m_placementDataCenter;

	bool m_locationsCanReTrigger;
	float m_timeOutOnTriggering;
	uint32_t m_numFreePlacements;

	float m_playtimeMultiplier;
	float m_playTime;
	bool m_isPlaying;
	bool m_resetTransformOnUpdate;

	PIEveDistributionPlacementGeneratorsVector m_placementGenerators;
	PIEveDistributionSpawnerVector m_distributionSpawners;
	PIEveDistributionSpawnModifierVector m_distributionSpawnModifiers;
	PIEveDistributionModifierVector m_distributionModifiers;

private:
	void HandleDistributionEntityLifetimeEvent( size_t index, DistributionEntityLifeTimeEvent event );
};

TYPEDEF_BLUECLASS( EveBaseDistributionMethod );
