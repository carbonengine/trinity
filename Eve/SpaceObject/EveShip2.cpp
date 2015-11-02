#include "StdAfx.h"

#include "TriValueBinding.h"

#include "EveShip2.h"
#include "Attachments/EveBoosterSet2.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Curves/TriCurveSet.h"
#include "Tr2GrannyAnimation.h"
#include "Eve/EveUpdateContext.h"
using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( eveShipsRendered, "Trinity/EveShip2/ShipsRendered", true, CST_COUNTER_LOW, "Number of EveShip objects rendered per frame." );

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveShip2::EveShip2( IRoot* lockobj ) :
	m_maxSpeed( 0.f ),
	m_displayKillCounterValue( 0 )
{
	m_speed.CreateInstance();

	// 0
	m_audioParameterInfo.destinationValue = nullptr;
	m_audioParameterInfo.classInfo = nullptr;
	m_audioParameterInfo.entry = nullptr;
}

EveShip2::~EveShip2()
{
}

void EveShip2::UpdateSyncronous( EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}
	EveMobile::UpdateSyncronous( updateContext );
	Be::Time time = updateContext.GetTime();
	float deltaT = updateContext.GetDeltaT();

	// can get the speed from destiny's position ball
	if( m_ballPosition )
	{
		Vector3 velocity;
		m_ballPosition->GetValueDotAt( &velocity, time );
		m_speed->m_value = D3DXVec3Length( &velocity );
	}

	UpdateShipSpeedForAudio();

	// update the attached boosters
	if( m_boosters )
	{
		// call update with this ship's transform
		m_boosters->Update( deltaT, time, &m_worldTransform, m_ballPosition, m_ballRotation );
	}
}

void EveShip2::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}
	EveMobile::UpdateAsyncronous( updateContext );
	if( m_boosters )
	{
		Be::Time time = updateContext.GetTime();
		float deltaT = updateContext.GetDeltaT();
		m_boosters->UpdateTrails( deltaT, time );
	}
}

void EveShip2::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	EveMobile::GetRenderables( frustum, renderables, parentTransform );

	// collect renderables of the boosters
	if( m_boosters )
	{
		if( DisplayChildren() )
		{
			m_boosters->GetRenderables( frustum, renderables );
		}
	}
}

void EveShip2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	EveSpaceObject2::RegisterWithQuadRenderer( quadRenderer );
	if( m_boosters )
	{
		m_boosters->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveShip2::AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( !m_isInFrustum || !m_display )
	{
		return;
	}
	EveSpaceObject2::AddQuadsToQuadRenderer( quadRenderer );
	if( m_boosters )
	{
		if( DisplayChildren() )
		{
			m_boosters->AddToQuadRenderer( quadRenderer, m_worldTransform );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Render some debug info of this space ship: turret sets
// --------------------------------------------------------------------------------
void EveShip2::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	// now the boosters & trails
	if( m_boosters )
	{
		m_boosters->RenderDebugInfo( renderContext );
	}

	// up
	EveMobile::RenderDebugInfo( renderContext );
}

void EveShip2::RebuildCachedData( BlueAsyncRes* p )
{
	EveMobile::RebuildCachedData( p );

	// loading of data is done, so check for locators and re-attach turrets
	RebuildTurretPositions();
}

// -----------------------------------------------------------------------------
// Description:
//   Set the boosterset of this ship from the outside
// -----------------------------------------------------------------------------
void EveShip2::SetBoosterSet( EveBoosterSet2Ptr set )
{
	m_boosters = set;
}

void EveShip2::RebuildBoosterSet()
{
	if( !m_boosters )
	{
		return;
	}

	m_boosters->Clear();

	static const char* kLocatorPrefix = "locator_booster";
	const unsigned int kLocatorPrefixLength = (unsigned int)strlen( kLocatorPrefix );

	unsigned int n = (unsigned int)m_locators.size();
	for( unsigned int i = 0; i < n ; ++i )
	{
		EveLocator2Ptr locator = m_locators[i];
		const char* locatorName = locator->GetName();
		if( strncmp( locatorName, kLocatorPrefix, kLocatorPrefixLength ) == 0 )
		{
			Vector4 functionality( 0.f, 1.f, 1.f, 1.f );
			m_boosters->Add( &locator->GetTransform(), &functionality, true, 0, 0 );
		}
	}

	m_boosters->PrepareResources();
}

bool EveShip2::Initialize()
{
	EveMobile::Initialize();

	return true;
}

// -----------------------------------------------------------------------------
// Description:
//   Overload the parent class's (EveSpaceObject) get function to put additional
//   data into the perobject data, that only the this class knows.
// SeeAlso:
//   EveSpaceObject2
// -----------------------------------------------------------------------------
Tr2PerObjectData* EveShip2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// booster intensity dictates some glow on this ship
	float boosterGlowIntensity = 0.f;
	if( m_boosters )
	{
		boosterGlowIntensity = m_boosters->GetBoosterIntensity();
	}

	// extra data for ps and vs: booster glow intensity
	m_spaceObjectShipData.x = boosterGlowIntensity;

	// parent
	return EveMobile::GetPerObjectData( accumulator );
}

bool ValidateTurretLocatorName2( const char* locatorName, unsigned int& locatorsFoundA, unsigned int& locatorsFoundB )
{
	static const char* kLocatorPrefix = "locator_turret_";
	const unsigned int kLocatorPrefixLength = (unsigned int)strlen( kLocatorPrefix );

	if( strncmp( locatorName, kLocatorPrefix, kLocatorPrefixLength ) == 0 )
	{
		std::string index;
		unsigned int i = kLocatorPrefixLength;
		while( isdigit( locatorName[i] ) )
		{
			index += locatorName[i];
			++i;
		}

		unsigned int ix = atoi( index.c_str() );

		if( (ix == 0) && (ix > 32) )
		{
			// Invalid turret locator index found
			return false;
		}

		--ix; // Indices start at 1

		if( locatorName[i] == 'a' )
		{
			locatorsFoundA |= 1 << ix;
		}
		else if( locatorName[i] == 'b' )
		{
			locatorsFoundB |= 1 << ix;
		}
		else
		{
			// Invalid turret name found
			return false;
		}

		return true;
	}

	return false;
}

void EveShip2::SetAudioParameter( IRoot* aud )
{
	// Reset everything
	m_audioSpeedParameter.Unlock();
	m_audioSpeedNotify.Unlock();
	m_audioParameterInfo.destinationValue = NULL;
	m_audioParameterInfo.classInfo = NULL;
	m_audioParameterInfo.entry = NULL;

	if( !aud )
	{
		return;
	}

	m_audioSpeedParameter = aud;
	INotifyPtr notify( BlueCastPtr( aud ) );

	m_audioParameterInfo.classInfo = m_audioSpeedParameter->ClassType();
	m_audioParameterInfo.offset = 0;
	m_audioParameterInfo.entry = TriValueBinding::FindEntry( "value", m_audioParameterInfo.classInfo, m_audioParameterInfo.offset );
	
	if( m_audioParameterInfo.entry )
	{
		if( m_audioParameterInfo.entry->mType == Be::FLOAT )
		{
			m_audioParameterInfo.destinationValue = BLUEMAPMEMBEROFFSET( 
															aud, 
															m_audioParameterInfo.entry, 
															m_audioParameterInfo.classInfo, 
															m_audioParameterInfo.offset );
		}
		if( m_audioParameterInfo.entry->mEditFlags & Be::NOTIFY )
		{
			m_audioSpeedNotify = notify;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This funstion fills all the relevant data to pass to each decal as "parent's
//   data. With it being virtual, sub classes can add more data.
// Arguments:
//   pd - the data buffer
// --------------------------------------------------------------------------------
void EveShip2::FillDecalParentData( EveSpaceObjectDecal::ParentData* pd ) const
{
	// call base
	EveSpaceObject2::FillDecalParentData( pd );

	// add in the kill counter
	pd->displayCounter = m_displayKillCounterValue;
}

// --------------------------------------------------------------------------------
// Description:
//   Return audio parameter
// --------------------------------------------------------------------------------
IRoot* EveShip2::GetAudioParameter() const
{
	return m_audioSpeedParameter;
}

// --------------------------------------------------------------------------------------
// Description:
//   Calculates the speed of a ship as an audio intensity factor and passes that on to
//   an AudParameter using blue reflection mechanisms, rather than by knowing about the
//   type (since we don't want to link audio and trinity).
//
//   Although this is super-hard coded and evil, the method of doing this through curvesets
//   was very inefficient, using up to 7% of the total CPU!
// --------------------------------------------------------------------------------------
void EveShip2::UpdateShipSpeedForAudio()
{
	if( m_maxSpeed == 0.0f )
	{
		return;
	}

	if( m_audioParameterInfo.destinationValue )
	{
		float speedFraction = (m_speed->m_value)/m_maxSpeed;
		float audioIntensityResult = 0.0f;

		// Scale audio intensity depending on the speed of the object
		// These values were taken from an authored curve. Ideally, they could be moved to WWise
		if( speedFraction < 0.1f )
		{
			audioIntensityResult = 0.0;
		}
		else if( speedFraction < 1.0f )
		{
			audioIntensityResult = (speedFraction - 0.1f) * (1.0f/0.9f);
		}
		else
		{
			audioIntensityResult = 1.0f + 9.0f*(min(speedFraction,1000000000.0f) - 1.0f)/(1000000000.0f-1.0f) ;
		}

		// copy value
		m_audioParameterInfo.destinationValue->mFloat = audioIntensityResult;

		// Notify it of the change
		if( m_audioSpeedNotify )
		{
			m_audioSpeedNotify->OnModified( (Be::Var*)m_audioParameterInfo.destinationValue );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Gets called by the state machine of this object to execute some command.
// Return Value:
//   Returns true if this implementation has handled the command.
// --------------------------------------------------------------------------------
bool EveShip2::ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters )
{
	switch( cmd )
	{
	case ANIM_CMD_TURNOFF_BOOSTERS:
		return true;

	case ANIM_CMD_TURNON_BOOSTERS:
		return true;

	default:
		// not handled here, so pass it up the chain
		return EveSpaceObject2::ExecuteAnimationStateCommand( cmd, data, parameters );
	}

	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Adds ship dynamic lights to the manager.
// Arguments:
//   lightManager - light manager
// --------------------------------------------------------------------------------
void EveShip2::GetLights( Tr2LightManager& lightManager ) const
{
	EveMobile::GetLights( lightManager );
	if( m_boosters )
	{
		m_boosters->GetLights( lightManager, m_worldTransform );
	}
}