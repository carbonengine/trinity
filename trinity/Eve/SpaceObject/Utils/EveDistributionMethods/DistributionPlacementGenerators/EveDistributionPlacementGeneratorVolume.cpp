// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorVolume.h"
#include "include/TriMath.h"

EveDistributionPlacementGeneratorVolume::EveDistributionPlacementGeneratorVolume( IRoot* lockobj ) :
	m_numGenerated( 10 ),
	m_isRequestingRegeneration( true ),
	m_volumeCallbackID( 0 ),
	m_hollowVolume( false ),
	m_falloffFactor( 1.5f )
{
}

EveDistributionPlacementGeneratorVolume::~EveDistributionPlacementGeneratorVolume()
{
	RemoveVolumeCallbacks();
}

IEveVolumePtr EveDistributionPlacementGeneratorVolume::GetVolume() const
{
	return m_volume;
}

void EveDistributionPlacementGeneratorVolume::SetVolume( IEveVolume* volume )
{
	RemoveVolumeCallbacks();
	m_volume = volume;
	AddVolumeCallbacks();
}


void EveDistributionPlacementGeneratorVolume::GetInitialPlacements( std::vector<InitialPlacement>& placements, uint32_t& trackingID )
{
	if( m_volume == nullptr )
	{
		return;
	}

	std::vector<Vector3> points;
	const Vector3 up( 0.f, 1.f, 0.f );
	m_volume->GeneratePointsInVolume( points, m_numGenerated, m_hollowVolume, m_falloffFactor );
	Vector3 offset = m_volume->GetBoundingSphere().center;
	placements.reserve( placements.size() + points.size() );

	for( auto& point : points )
	{
		InitialPlacement placement;
		placement.timeOutDuration = 0.0;
		PlacementDataWithIdentifier pdata;
		pdata.initialTranslation = offset + point;
		point = Normalize( point );
		TriQuaternionRotationArc( &pdata.initialRotation, &up, &point );
		pdata.initialScale = Vector3( 1.f, 1.f, 1.f );
		pdata.boneIndex = -1;
		pdata.uniqueID = trackingID++;
		pdata.lifeTime = 0.0;
		placement.placement = pdata;
		placements.push_back( placement );
	}

	m_isRequestingRegeneration = false;
}

void EveDistributionPlacementGeneratorVolume::RequestRegeneration()
{
	m_isRequestingRegeneration = true;
}

bool EveDistributionPlacementGeneratorVolume::IsRequestingRegeneration()
{
	return m_isRequestingRegeneration;
}

void EveDistributionPlacementGeneratorVolume::AddVolumeCallbacks()
{
	if( m_volume )
	{
		const std::function<void( void )> ref = std::bind( &EveDistributionPlacementGeneratorVolume::RequestRegeneration, this );
		m_volumeCallbackID = m_volume->RegisterForChanges( ref );
	}
}

void EveDistributionPlacementGeneratorVolume::RemoveVolumeCallbacks()
{
	if( m_volume )
	{
		if( m_volumeCallbackID != 0 )
		{
			m_volume->UnregisterForChanges( m_volumeCallbackID );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool EveDistributionPlacementGeneratorVolume::Initialize()
{
	AddVolumeCallbacks();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// INotify
bool EveDistributionPlacementGeneratorVolume::OnModified( Be::Var* value )
{
	RequestRegeneration();
	return true;
}
