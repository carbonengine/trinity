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
	m_displayKillCounterValue( 0 ),
	m_acceleration( 0.f, 0.f, 0.f )
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

void EveShip2::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}
	EveMobile::UpdateSyncronous( updateContext );
	Be::Time time = updateContext.GetTime();

	if( m_ballPosition )
	{
		// need the speed!
		Vector3 v;
		GetWorldVelocity( v );
		m_speed->m_value = Length( v );
	}

	UpdateShipSpeedForAudio();

	// update the attached boosters
	if( m_boosters )
	{
		if( m_ballPosition )
		{
			m_ballPosition->GetValueDoubleDotAt( &m_acceleration, time );
		}
		else
		{
			m_acceleration = Vector3( 0.f, 0.f, 0.f );
		}
	}
}

void EveShip2::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}
	EveMobile::UpdateAsyncronous( updateContext );
	UpdateBoosters( updateContext );
}

void EveShip2::UpdateBoosters( const EveUpdateContext& updateContext )
{
	if( m_boosters )
	{
		Be::Time time = updateContext.GetTime();
		float deltaT = updateContext.GetDeltaT();
		m_boosters->Update( deltaT, time, m_worldTransform, m_speed->m_value, m_acceleration, m_worldRotation );
		m_boosters->UpdateTrails( deltaT, time );
	}
}

void EveShip2::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	EveMobile::UpdateVisibility( updateContext, parentTransform );

	if( !m_display )
	{
		return;
	}

	// collect renderables of the boosters
	if( DisplayBoosters() )
	{
		m_boosters->UpdateVisibility( updateContext );
	}
}

void EveShip2::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	EveMobile::GetRenderables( renderables, impostors );

	// collect renderables of the boosters
	if( DisplayBoosters() )
	{
		m_boosters->GetRenderables( renderables );
	}
}

EveBoosterSet2* EveShip2::GetBoosters()
{
	return m_boosters;
}

void EveShip2::SetBoosters( EveBoosterSet2* boosters )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_boosters ) )
	{
		entity->UnRegister( registry );
	}
	m_boosters = boosters;
	if( EveEntityPtr entity = BlueCastPtr( m_boosters ) )
	{
		entity->Register( registry );
	}
}

void EveShip2::RegisterComponents()
{
	EveMobile::RegisterComponents();
	auto registry = this->GetComponentRegistry();
	if( registry && m_boosters )
	{
		m_boosters->Register( registry );
	}
}

void EveShip2::UnRegisterComponents()
{
	EveMobile::UnRegisterComponents();
	auto registry = this->GetComponentRegistry();
	if( registry && m_boosters )
	{
		m_boosters->UnRegister( registry );
	}
}

void EveShip2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	EveMobile::RegisterWithQuadRenderer( quadRenderer );
	if( m_boosters )
	{
		m_boosters->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveShip2::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_display )
	{
		return;
	}
	EveMobile::AddQuadsToQuadRenderer( frustum, quadRenderer );
	if( DisplayBoosters() )
	{
		m_boosters->AddToQuadRenderer( quadRenderer, m_worldTransform );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Determines if we render the boosters or not
// SeeAlso:
//   EveTransform
// --------------------------------------------------------------------------------
bool EveShip2::DisplayBoosters() const
{
	// if it is more than .5 -> render the children!
	return m_boosters && DisplayChildren();
}

float EveShip2::GetKillCounterValue() const
{
    return static_cast<float>( m_displayKillCounterValue );
}

void EveShip2::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	EveMobile::GetDebugOptions( options );
	options.insert( "Boosters" );
}

// --------------------------------------------------------------------------------
// Description:
//   Render some debug info of this space ship: turret sets
// --------------------------------------------------------------------------------
void EveShip2::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	// now the boosters & trails
	if( m_boosters && renderer.HasOption( this, "Boosters" ) )
	{
		m_boosters->RenderDebugInfo( renderer );
	}

	// up
	EveMobile::RenderDebugInfo( renderer );
}

void EveShip2::RebuildCachedData( BlueAsyncRes* p )
{
	EveMobile::RebuildCachedData( p );

	// loading of data is done, so check for locators and re-attach turrets
	RebuildTurretPositions();
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
//   This function fills a parent data struct with parent data
// Arguments:
//   pd - the data buffer
// --------------------------------------------------------------------------------
void EveShip2::GetParentData( EveSpaceObject2::ParentData* pd ) const
{
	// call base
	EveSpaceObject2::GetParentData( pd );

	// add in the kill counter
	pd->killCount = m_displayKillCounterValue;
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
	if(( m_maxSpeed == 0.0f ) || ( m_boosters == NULL ))
	{
		return;
	}

	if( m_audioParameterInfo.destinationValue )
	{
		// copy value
		m_audioParameterInfo.destinationValue->mFloat = m_boosters->GetBoosterIntensity();

		// Notify it of the change
		if( m_audioSpeedNotify )
		{
			m_audioSpeedNotify->OnModified( (Be::Var*)m_audioParameterInfo.destinationValue );
		}

	}
}

float EveShip2::GetMaxSpeed() const
{
	return m_maxSpeed;
}

float EveShip2::GetBoosterIntensity() const
{
	if ( m_boosters != nullptr )
	{
		return m_boosters->GetBoosterIntensity();
	}

	return 0.0f;
}