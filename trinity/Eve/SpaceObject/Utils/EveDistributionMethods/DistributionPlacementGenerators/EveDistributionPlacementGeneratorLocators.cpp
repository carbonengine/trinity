// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorLocators.h"

static BlueStructureDefinition LocatorStructureWithScaleDef[] = {
	{ "position", Be::FLOAT32_3, 0 },
	{ "direction", Be::FLOAT32_4, 12 },
	{ "scale", Be::FLOAT32_3, 28 },
	{ "boneIndex", Be::INT32_1, 40 },
	{ 0 }
};

const Locator s_LocatorWithScaleDefaultKey = Locator{ Vector3( 0.0, 0.0, 0.0 ), Quaternion( 0.0, 0.0, 0.0, 1.0 ), Vector3( 1.0, 1.0, 1.0 ), -1 };

EveDistributionPlacementGeneratorLocators::EveDistributionPlacementGeneratorLocators( IRoot* lockobj ) :
	PARENTLOCK( m_locators ),
	m_requestRegeneration( false )
{
	m_locators.SetStructureDefinition( LocatorStructureWithScaleDef );
	m_locators.SetDefaultValue( &s_LocatorWithScaleDefaultKey );
	m_locators.SetNotify( this );
}

void EveDistributionPlacementGeneratorLocators::OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list )
{
	m_requestRegeneration = true;
}

void EveDistributionPlacementGeneratorLocators::GetInitialPlacements( std::vector<InitialPlacement>& placements, uint32_t& trackingID )
{
	placements.reserve( placements.size() + m_locators.size() );
	for( size_t i = 0; i < m_locators.size(); ++i )
	{
		auto& locator = m_locators[i];
		InitialPlacement placement;
		placement.timeOutDuration = 0.f;
		PlacementDataWithIdentifier pdata;
		pdata.initialTranslation = locator.position;
		pdata.initialRotation = locator.direction;
		pdata.initialScale = locator.scale;
		pdata.boneIndex = locator.boneIndex;
		pdata.uniqueID = trackingID++;
		pdata.lifeTime = 0.f;
		placement.placement = pdata;
		placements.push_back( placement );
	}

	m_requestRegeneration = false;
}

bool EveDistributionPlacementGeneratorLocators::IsRequestingRegeneration()
{
	return m_requestRegeneration;
}

void EveDistributionPlacementGeneratorLocators::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
}
