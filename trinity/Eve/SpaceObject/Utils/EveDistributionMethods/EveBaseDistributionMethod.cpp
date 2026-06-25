// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveBaseDistributionMethod.h"
#include "include/TriMath.h"
#include "Utilities/MatrixUtils.h"

static BlueStructureDefinition PlacementDataWithIdentifierDef[] = {
	{ "position", Be::FLOAT32_3, 0 },
	{ "direction", Be::FLOAT32_4, 12 },
	{ "scale", Be::FLOAT32_3, 28 },
	{ "additionalTranslation", Be::FLOAT32_3, 40 },
	{ "additionalRotation", Be::FLOAT32_4, 52 },
	{ "additionalScale", Be::FLOAT32_3, 68 },
	{ "boneIndex", Be::INT32_1, 80 },
	{ "lifeTime", Be::FLOAT32_1, 84 },
	{ "uniqueID", Be::UINT32_1, 88 },
	{ "initialPlacementID", Be::UINT32_1, 92 },
	{ 0 }
};

const PlacementDataWithIdentifier s_PlacementDataWithIdentifierDefaultKey;

EveBaseDistributionMethod::EveBaseDistributionMethod( IRoot* lockobj ) :
	PARENTLOCK( m_placementData ),
	PARENTLOCK( m_placementGenerators ),
	PARENTLOCK( m_distributionSpawners ),
	PARENTLOCK( m_distributionModifiers ),
	PARENTLOCK( m_distributionSpawnModifiers ),
	m_uniqueIDCounter( 0 ),
	m_numFreePlacements( 0 ),
	m_locationsCanReTrigger( true ),
	m_resetTransformOnUpdate( false ),
	m_timeOutOnTriggering( 2.f ),
	m_playTime( 0.f ),
	m_playtimeMultiplier( 1.f ),
	m_isPlaying( true ),
	m_placementDataCenter( 0.f, 0.f, 0.f )
{
	m_placementData.SetStructureDefinition( PlacementDataWithIdentifierDef );
	m_placementData.SetDefaultValue( &s_PlacementDataWithIdentifierDefaultKey );
	m_placementGenerators.SetNotify( this );
	m_distributionSpawners.SetNotify( this );
	m_distributionModifiers.SetNotify( this );
}

void EveBaseDistributionMethod::RegeneratePlacementData()
{
	m_initialPlacements.clear();
	uint32_t counter = 0;
	for( auto placementGenerator : m_placementGenerators )
	{
		placementGenerator->GetInitialPlacements( m_initialPlacements, counter );
	}

	for( uint32_t i = 0; i < uint32_t( m_initialPlacements.size() ); i++ )
	{
		m_uniqueIDIndices[m_initialPlacements[i].placement.uniqueID] = i;
	}

	m_numFreePlacements = uint32_t( m_initialPlacements.size() );
}

size_t EveBaseDistributionMethod::GetNumberOfPlacements()
{
	return m_placementData.GetSize();
}

const PlacementDataWithIdentifierStructureList* EveBaseDistributionMethod::GetPlacementData() const
{
	return &m_placementData;
}

const Vector3 EveBaseDistributionMethod::GetPlacementDataCenter() const
{
	return m_placementDataCenter;
}

const bool EveBaseDistributionMethod::GetHasDynamicMovement() const
{
	return m_resetTransformOnUpdate;
}

bool EveBaseDistributionMethod::Initialize()
{
	this->RestartDistribution();
	return true;
}

void EveBaseDistributionMethod::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if( theList == &m_placementGenerators )
	{
		this->RestartDistribution();
	}

	if( theList == &m_distributionSpawners )
	{
		this->RestartDistribution();
	}

	if( theList == &m_distributionModifiers )
	{
		m_resetTransformOnUpdate = false;

		for( auto modifier : m_distributionModifiers )
		{
			m_resetTransformOnUpdate = m_resetTransformOnUpdate || modifier->AffectsTransform();
		}
	}
}

void EveBaseDistributionMethod::RestartDistribution()
{
	m_placementData.Clear();
	this->RegeneratePlacementData();

	for( auto spawner : m_distributionSpawners )
	{
		spawner->Reset( m_initialPlacements );
	}

	m_playTime = 0.f;
	m_isPlaying = true;
	m_uniqueIDCounter = 0;
	m_resetTransformOnUpdate = false;

	for( auto modifier : m_distributionModifiers )
	{
		m_resetTransformOnUpdate = m_resetTransformOnUpdate || modifier->AffectsTransform();
	}
}


void EveBaseDistributionMethod::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	for( auto placementGenerator : m_placementGenerators )
	{
		placementGenerator->UpdateSyncronous( updateContext, params, *this );

		if( placementGenerator->IsRequestingRegeneration() )
		{
			this->RestartDistribution();
			return;
		}
	}

	float dt = updateContext.GetDeltaT() * m_playtimeMultiplier;
	m_playTime += dt;

	if( m_locationsCanReTrigger )
	{
		if( !m_initialPlacements.empty() && m_numFreePlacements < m_initialPlacements.size() )
		{
			for( size_t indx = m_initialPlacements.size() - 1; indx >= m_numFreePlacements; )
			{
				if( m_initialPlacements[indx].timeOutDuration > 0.f )
				{
					m_initialPlacements[indx].timeOutDuration -= dt;
				}
				else
				{
					SwapInitialPlacements( uint32_t( indx ), m_numFreePlacements );
					m_numFreePlacements++;
					continue;
				}

				if( indx == 0 )
				{
					break;
				}
				else
				{
					indx--;
				}
			}
		}
	}

	m_placementDataCenter = Vector3( 0.f, 0.f, 0.f );

	for( size_t indx = 0; indx < m_placementData.size(); indx++ )
	{
		m_placementData[indx].lifeTime += dt;

		if( m_resetTransformOnUpdate )
		{
			m_placementData[indx].translationFrameDelta = m_placementData[indx].additionalTranslation;
			m_placementData[indx].additionalTranslation = Vector3( 0.f, 0.f, 0.f );
			m_placementData[indx].additionalRotation = Quaternion( 0.f, 0.f, 0.f, 1.f );
			m_placementData[indx].additionalScale = Vector3( 1.f, 1.f, 1.f );
		}

		if( m_placementData[indx].boneIndex >= 0 && m_placementData[indx].boneIndex < params.boneCount )
		{
			Matrix boneMatrix = IdentityMatrix();
			TriMatrixCopyFrom3x4( &boneMatrix, &params.bones[m_placementData[indx].boneIndex] );
			Matrix m = GetInitialPlacementMatrixForPlacement( m_placementData[indx] );
			m = m * boneMatrix;
			Decompose( m_placementData[indx].initialScale, m_placementData[indx].initialRotation, m_placementData[indx].initialTranslation, m );
		}

		bool entityKilled = false;

		for( auto distributionModifier : m_distributionModifiers )
		{
			auto lifeTimeEvent = distributionModifier->ProcessDistributionModifier( m_placementData[indx], dt, params );

			if( lifeTimeEvent != DistributionEntityLifeTimeEvent::DO_NOTHING )
			{
				this->HandleDistributionEntityLifetimeEvent( indx, lifeTimeEvent );
				indx--;
				entityKilled = true;
				break;
			}
		}

		if( !entityKilled )
		{
			if( m_resetTransformOnUpdate )
			{
				m_placementData[indx].translationFrameDelta -= m_placementData[indx].additionalTranslation;
			}

			m_placementDataCenter += m_placementData[indx].initialTranslation + m_placementData[indx].additionalTranslation;
		}
	}

	m_placementDataCenter /= float( m_placementData.size() );

	for( auto spawner : m_distributionSpawners )
	{
		spawner->UpdateSyncronous( updateContext, params, *this );
	}
}

Matrix EveBaseDistributionMethod::GetInitialPlacementMatrixForPlacement( PlacementDataWithIdentifier& placement )
{
	int32_t originIndex = this->GetInitialPlacementIndexByID( placement.initialPlacementID );

	if( originIndex < 0 )
	{
		return IdentityMatrix();
	}

	PlacementDataWithIdentifier placementCopy = m_initialPlacements[originIndex].placement;

	for( auto spawnModifiers : m_distributionSpawnModifiers )
	{
		spawnModifiers->ProcessSpawnModifier( placementCopy, m_initialPlacements.size() );
	}

	Matrix m = TransformationMatrix( placementCopy.initialScale, placementCopy.initialRotation, placementCopy.initialTranslation );
	return m;
}


void EveBaseDistributionMethod::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
}

void EveBaseDistributionMethod::HandleDistributionEntityLifetimeEvent( size_t index, DistributionEntityLifeTimeEvent event )
{
	if( event == DistributionEntityLifeTimeEvent::KILL_ENTITY )
	{
		if( index != m_placementData.size() - 1 )
		{
			m_placementData[index] = m_placementData[m_placementData.size() - 1];
		}
		m_placementData.Resize( m_placementData.size() - 1 );
		return;
	}

	PlacementDataWithIdentifier newPlacement;

	switch( event )
	{
	case DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_DISTRIBUTION:
		this->GetRandomPlacement( newPlacement );
		break;
	case DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_INITIAL_POSITION:
		newPlacement = m_placementData[index];
		newPlacement.lifeTime = 0.f;
		break;
	case DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_CURRENT_POSITION:
		newPlacement = m_placementData[index];
		newPlacement.initialTranslation += newPlacement.additionalTranslation;
		newPlacement.initialRotation += newPlacement.additionalRotation;
		newPlacement.initialScale += newPlacement.additionalScale;
		newPlacement.lifeTime = 0.f;
		break;
	default:
		break;
	}

	newPlacement.uniqueID = m_uniqueIDCounter++;
	m_placementData[index] = newPlacement;
}

int32_t EveBaseDistributionMethod::GetInitialPlacementIndexByID( uint32_t entityID )
{
	auto it = m_uniqueIDIndices.find( entityID );

	if( it == m_uniqueIDIndices.end() )
	{
		return -1;
	}

	return int32_t( it->second );
}

void EveBaseDistributionMethod::SwapInitialPlacements( uint32_t indexA, uint32_t indexB )
{
	m_uniqueIDIndices[m_initialPlacements[indexA].placement.uniqueID] = indexB;
	m_uniqueIDIndices[m_initialPlacements[indexB].placement.uniqueID] = indexA;
	std::swap( m_initialPlacements[indexA], m_initialPlacements[indexB] );
}

void EveBaseDistributionMethod::GetPlacement( PlacementDataWithIdentifier& placement, uint32_t entityID )
{
	if( m_numFreePlacements < 1 || m_initialPlacements.empty() )
	{
		return;
	}

	int32_t selectedIndex = this->GetInitialPlacementIndexByID( entityID );

	if( selectedIndex < 0 )
	{
		return;
	}

	m_initialPlacements[selectedIndex].timeOutDuration = m_timeOutOnTriggering;
	placement = m_initialPlacements[selectedIndex].placement;
	placement.initialPlacementID = m_initialPlacements[selectedIndex].placement.uniqueID;
	m_numFreePlacements--;

	if( m_numFreePlacements != selectedIndex )
	{
		SwapInitialPlacements( selectedIndex, m_numFreePlacements );
	}

	for( auto spawnModifiers : m_distributionSpawnModifiers )
	{
		spawnModifiers->ProcessSpawnModifier( placement, m_initialPlacements.size() );
	}

	EveChildUpdateParams params;

	for( auto distributionModifier : m_distributionModifiers )
	{
		distributionModifier->ProcessDistributionModifier( placement, 0.f, params );
	}
}

void EveBaseDistributionMethod::GetRandomPlacement( PlacementDataWithIdentifier& placement )
{
	uint32_t selectedIndex = TriRandInt( m_numFreePlacements );
	selectedIndex = m_initialPlacements[selectedIndex].placement.uniqueID;
	GetPlacement( placement, selectedIndex );
}

void EveBaseDistributionMethod::AddEntities( uint32_t howMany )
{
	if( m_numFreePlacements < 1 || m_initialPlacements.empty() )
	{
		return;
	}

	size_t originalSize = m_placementData.size();
	m_placementData.Resize( originalSize + min( howMany, m_numFreePlacements ) );

	for( size_t i = 0; i < howMany; ++i )
	{
		if( m_numFreePlacements < 1 )
		{
			break;
		}
		PlacementDataWithIdentifier placement;
		this->GetRandomPlacement( placement );
		placement.uniqueID = m_uniqueIDCounter++;
		m_placementData[originalSize + i] = placement;
	}
}

int32_t EveBaseDistributionMethod::TriggerEntityByID( uint32_t entityID )
{
	if( m_numFreePlacements < 1 || m_initialPlacements.empty() )
	{
		return -1;
	}

	int32_t entityIdx = this->GetInitialPlacementIndexByID( entityID );

	if( entityIdx < 0 )
	{
		return -1;
	}

	if( m_initialPlacements[entityIdx].timeOutDuration > 0.0 )
	{
		return -1;
	}

	m_initialPlacements[entityIdx].timeOutDuration = m_timeOutOnTriggering;

	if( uint32_t( entityIdx ) < m_numFreePlacements )
	{
		m_numFreePlacements--;

		SwapInitialPlacements( entityIdx, m_numFreePlacements );
		entityIdx = m_numFreePlacements;
	}

	size_t originalSize = m_placementData.size();
	m_placementData.Resize( originalSize + 1 );
	m_placementData[originalSize] = m_initialPlacements[entityIdx].placement;
	m_placementData[originalSize].initialPlacementID = m_initialPlacements[entityIdx].placement.uniqueID;

	for( auto spawnModifiers : m_distributionSpawnModifiers )
	{
		spawnModifiers->ProcessSpawnModifier( m_placementData[originalSize], m_initialPlacements.size() );
	}

	return entityIdx;
}

PlacementDataWithIdentifier* EveBaseDistributionMethod::GetInitialPlacementData( uint32_t uniqueID )
{
	int32_t entityIdx = this->GetInitialPlacementIndexByID( uniqueID );

	if( entityIdx >= int32_t( m_initialPlacements.size() ) )
	{
		return nullptr;
	}
	return &m_initialPlacements[entityIdx].placement;
}

uint32_t EveBaseDistributionMethod::GetFreePlacementCount()
{
	return m_numFreePlacements;
}

int32_t EveBaseDistributionMethod::GetClosestFreePlacement( Vector3& position )
{
	// use sparingly as we don't have a sorted tree yet

	if( m_initialPlacements.size() < 1 || m_numFreePlacements < 1 )
	{
		return -1;
	}

	float bestDist = LengthSq( position - m_initialPlacements[0].placement.initialTranslation );
	uint32_t bestIndex = 0;

	for( uint32_t indx = 1; indx < m_numFreePlacements; indx++ )
	{
		float distSQ = LengthSq( position - m_initialPlacements[indx].placement.initialTranslation );
		if( bestDist > distSQ )
		{
			bestDist = distSQ;
			bestIndex = indx;
		}
	}

	return int32_t( m_initialPlacements[bestIndex].placement.uniqueID );
}

void EveBaseDistributionMethod::SetControllerVariable( const char* name, float value )
{
	for( auto spawner : m_distributionSpawners )
	{
		spawner->SetControllerVariable( name, value );
	}
}
