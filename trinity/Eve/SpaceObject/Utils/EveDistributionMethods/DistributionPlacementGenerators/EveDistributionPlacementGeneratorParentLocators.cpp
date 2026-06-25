// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorParentLocators.h"

EveDistributionPlacementGeneratorParentLocators::EveDistributionPlacementGeneratorParentLocators( IRoot* lockobj ) :
	m_requestRegeneration( false ),
	m_regenerated( false ),
	m_locatorSetName( "damage" ),
	m_oldNotify( nullptr )
{
}

EveDistributionPlacementGeneratorParentLocators::~EveDistributionPlacementGeneratorParentLocators()
{
	if( m_oldNotify )
	{
		m_locators->SetNotify( m_oldNotify );
		m_oldNotify = nullptr;
	}
}

void EveDistributionPlacementGeneratorParentLocators::GetInitialPlacements( std::vector<InitialPlacement>& placements, uint32_t& trackingID )
{
	const LocatorStructureList* locators = m_locators;
	m_requestRegeneration = false;

	if( locators == nullptr )
	{
		return;
	}

	placements.reserve( placements.size() + locators->size() );

	for( auto locator = locators->begin(); locator != locators->end(); ++locator )
	{
		InitialPlacement placement;
		placement.timeOutDuration = 0.0;
		PlacementDataWithIdentifier pdata;
		pdata.initialTranslation = locator->position;
		pdata.initialRotation = locator->direction;
		pdata.initialScale = locator->scale;
		pdata.boneIndex = locator->boneIndex;
		pdata.uniqueID = trackingID++;
		pdata.lifeTime = 0.0;
		placement.placement = pdata;
		placements.push_back( placement );
	}
}

bool EveDistributionPlacementGeneratorParentLocators::IsRequestingRegeneration()
{
	return m_requestRegeneration;
}

void EveDistributionPlacementGeneratorParentLocators::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( !m_regenerated )
	{
		if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( params.spaceObjectParent ) )
		{
			m_locators = const_cast<LocatorStructureList*>( spaceObject->GetLocatorsForSet( m_locatorSetName ) );
			if( m_locators != nullptr )
			{
				m_oldNotify = m_locators->SetNotify( this );
				if( m_oldNotify == this )
				{
					m_oldNotify = nullptr;
				}
				m_regenerated = true;
				m_requestRegeneration = true;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// INotify
bool EveDistributionPlacementGeneratorParentLocators::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_locatorSetName ) )
	{
		m_regenerated = false;
	}
	return true;
}



void EveDistributionPlacementGeneratorParentLocators::OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list )
{
	m_regenerated = false;

	if( m_oldNotify )
	{
		m_oldNotify->OnStructureListModified( event, item, index, m_locators );
	}
}
