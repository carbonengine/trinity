// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionPlacementGenerators.h"
#include "Eve/Volume/IEveVolume.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_CLASS( EveDistributionPlacementGeneratorVolume ) :
	public IEveDistributionPlacementGenerators,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionPlacementGeneratorVolume( IRoot* lockobj = nullptr );
	~EveDistributionPlacementGeneratorVolume();

	void GetInitialPlacements( std::vector<InitialPlacement> & placements, uint32_t& trackingID ) override;
	bool IsRequestingRegeneration() override;
	void RequestRegeneration();



	// IInitializ
	bool Initialize() override;

	// INotify
	bool OnModified( Be::Var * value ) override;

private:
	IEveVolumePtr GetVolume() const;
	void SetVolume( IEveVolume * volume );

	void AddVolumeCallbacks();
	void RemoveVolumeCallbacks();

	IEveVolumePtr m_volume;
	uint32_t m_numGenerated;
	bool m_isRequestingRegeneration;
	uint32_t m_volumeCallbackID;
	bool m_hollowVolume;
	float m_falloffFactor;
};

TYPEDEF_BLUECLASS( EveDistributionPlacementGeneratorVolume );
