////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"

#include "include/TriMath.h"
#include "Tr2GrannyAnimation.h"

#include "EveMobile.h"
#include "Curves/TriScalarCurve.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Eve/EveUpdateContext.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveMobile::EveMobile( IRoot* lockobj ) :
	PARENTLOCK( m_turretSets ),
	m_activationDelta( 0.f ),
	m_activationStrenght( 1.f ),
	m_playActivationCurve( false ),
	m_playClipSphereFactorCurve( false ),
	m_clipSphereFactor( 0.f ),
	m_clipSphereFactorDelta( 0.f ),
	m_clipSphereCenter( 0.f, 0.f, 0.f )
{
	// ship class needs to know if turrets get added or removed
	m_turretSets.SetNotify( this );
}

// --------------------------------------------------------------------------------
// Description:
//   Destruction!
// --------------------------------------------------------------------------------
EveMobile::~EveMobile()
{
}

// --------------------------------------------------------------------------------
// Description:
//   REset things once the red file is fully loaded
// --------------------------------------------------------------------------------
bool EveMobile::Initialize()
{
	EveSpaceObject2::Initialize();
	
	// initialize the locator counts
	ResetTurretLocatorCounter( true );

	// place all turrets
	RebuildTurretPositions();

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Is called when a list is modified
// SeeAlso:
//   IListNotify
// --------------------------------------------------------------------------------
void EveMobile::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( (event & BELIST_LOADING) == 0  )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
		case BELIST_REMOVED:
			if( theList == &m_turretSets && value )
			{
				EveTurretSetPtr turretSet( BlueCastPtr( value ) );
				if( turretSet )
				{
					RebuildTurretPositions();
				}
			}
			break;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return a transform used for turret locators
// --------------------------------------------------------------------------------
const Matrix* EveMobile::GetTurretTransform() const
{
	return &m_worldTransform;
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::UpdateSyncronous() function, so we can update the turrets and 
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveMobile::UpdateSyncronous( EveUpdateContext& updateContext )
{
	EveSpaceObject2::UpdateSyncronous( updateContext );

	Be::Time time = updateContext.GetTime();
	float deltaT = updateContext.GetDeltaT();

	// turret update
	unsigned int locatorInfoIdx = 0;
	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		// if the turretset is of locatortype JOINT, it means it is attached to an
		// animated bone, sowe must update the positions of all turrets in the set
		if( locatorInfoIdx < m_turretSetsLocatorInfo.size() )
		{
			const TurretSetLocatorInfo* locInfo = &m_turretSetsLocatorInfo[locatorInfoIdx];
			// only animated if is of type JOINT!
			if( locInfo->type == ELT_JOINT )
			{
				for( unsigned int turretIdx = 0; turretIdx < locInfo->locatorIndices.size(); ++turretIdx )
				{
					// this ship knows position
					const Matrix* localMatrix = GetLocatorTransform( locInfo->type, locInfo->locatorIndices[turretIdx] );
					// set it
					if( localMatrix )
					{
						(*it)->SetLocalTransform( turretIdx, localMatrix );
					}
				}
			}
		}
		// next!
		++locatorInfoIdx;

		// call standard update function
		(*it)->UpdateSyncronous( deltaT, time, GetTurretTransform() );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::PrepareShaderData() function
// --------------------------------------------------------------------------------
void EveMobile::PrepareShaderData( EveUpdateContext& updateContext )
{
	EveSpaceObject2::PrepareShaderData( updateContext );

	// play activation curve
	if( m_activationStrengthCurve && m_playActivationCurve )
	{
		float deltaT = updateContext.GetDeltaT();
		m_activationDelta += deltaT;
		m_activationStrenght = m_activationStrengthCurve->Update( m_activationDelta );
	}

	m_spaceObjectShipData.y *= m_activationStrenght;

	if( m_clipSphereFactorCurve && m_playClipSphereFactorCurve )
	{
		float deltaT = updateContext.GetDeltaT();
		m_clipSphereFactorDelta += deltaT;
		m_clipSphereFactor = m_clipSphereFactorCurve->Update( m_clipSphereFactorDelta );

		// This is here so we always have a correct clipsphere center, even when the ANIM_CMD_PLAY_CLIPSPHERE animation state is started when the object is not loaded
		m_clipSphereCenter = -1.f * GetBoundingSphereCenter();
	}

	// the m_clipSphereFactor goes from 0.0 to 1.0 and is the "amount" of visibility of this whole
	// object: 0.0 = fully visible, 1.0 = invisible.
	// the following formula calculates a special number to pass to the shader to help determine this
	float nearDist = std::max( 0.f, D3DXVec3Length( &m_clipSphereCenter ) - GetBoundingSphereRadius() );
	float insideSpherePercentage = std::min( 1.f, D3DXVec3Length( &m_clipSphereCenter ) / GetBoundingSphereRadius() );
	float disolveRadius = nearDist + m_clipSphereFactor * GetBoundingSphereRadius() * ( 1.f + insideSpherePercentage );
	m_psData.clipData = m_vsData.clipData = Vector4( m_clipSphereCenter + GetBoundingSphereCenter(), TriFloatSign( disolveRadius ) * disolveRadius * disolveRadius );
	m_psData.miscData.x = TriFloatSign( disolveRadius );
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::UpdateAsyncronous() function, so we can update the turrets and 
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveMobile::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	EveSpaceObject2::UpdateAsyncronous( updateContext );

	// now prep to get the renderables
	EveTurretSet::ParentData pd;
	pd.transform = *GetTurretTransform();
	pd.shipData = m_spaceObjectShipData;
	pd.clipData = m_psData.clipData;
	pd.clipDataEx = m_psData.miscData;

	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		(*it)->UpdateAsyncronous( updateContext, &pd );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::GetRenderables() function, so we can draw the turrets etc.
// --------------------------------------------------------------------------------
void EveMobile::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	// call base to get spaceobject's renderables
	EveSpaceObject2::GetRenderables( frustum, renderables, parentTransform );

	// collect renderables of the turrets
	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		(*it)->GetRenderables( frustum, renderables, m_psData.shLightingCoefficients );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::GetRenderablesCastingShadow() function, so we can draw shadows!
//   Warning: this function can be called on different threads, so it needs to be
//   thread-safe.
// --------------------------------------------------------------------------------
bool EveMobile::GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables )
{
	if( EveSpaceObject2::GetRenderablesCastingShadow( isSelf, frustum, renderables ) )
	{
		for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
		{
			// use the turretset's ::GetRenderablesCastingShadow() function cause we need the special "ortho" culling
			(*it)->GetRenderablesCastingShadow( frustum, renderables );
		}
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::RenderDebugInfo() function, so we can draw debuginfo of
//   the turrets etc.
// --------------------------------------------------------------------------------
void EveMobile::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	EveSpaceObject2::RenderDebugInfo( renderContext );

	// let the turrets render some debug info
	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		(*it)->RenderDebugInfo( renderContext );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   TBD.
// --------------------------------------------------------------------------------
bool EveMobile::ValidateTurretLocatorName( const char* locatorName, unsigned int& locatorsFoundA, unsigned int& locatorsFoundB ) const
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

		int ix = atoi( index.c_str() );

		if( ( ix == 0 ) || ( ix > 32 ) )
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

// --------------------------------------------------------------------------------
// Description:
//   Override base ::RenderDebugInfo() function, so we can draw debuginfo of
//   the turrets etc.
// --------------------------------------------------------------------------------
unsigned int EveMobile::GetTurretLocatorCount()
{
	unsigned int locatorsFoundA = 0;
	unsigned int locatorsFoundB = 0;

	unsigned int n = (unsigned int)m_locators.size();
	for( unsigned int i = 0; i < n ; ++i )
	{
		const char* locatorName =  m_locators[i]->GetName();
		// EveMobile keeps multiple types of locator, so validate the name
		ValidateTurretLocatorName( locatorName, locatorsFoundA, locatorsFoundB);
	}

	if( m_animationUpdater && m_animationUpdater->m_skeleton )
	{
		for( int boneIx = 0; boneIx < m_animationUpdater->m_skeleton->BoneCount; ++boneIx )
		{
			const granny_bone& bone = m_animationUpdater->m_skeleton->Bones[boneIx];
			ValidateTurretLocatorName( bone.Name, locatorsFoundA, locatorsFoundB );
		}		
	}

	if( (locatorsFoundA != locatorsFoundB) && locatorsFoundB )
	{
		// Pairs don't match up. Note we do allow having only the 'a' locator (for drones)
		return 0;
	}

	unsigned int tlc = 0;
	while( locatorsFoundA & 0x01 )
	{
		++tlc;
		locatorsFoundA >>= 1;
	}

	return tlc;
}

// --------------------------------------------------------------------------------
// Description:
//   Checks all attached turret sets for the number of single turrets this set
//   has, then tries to find this locator and it's type: granny bone or jessica
//   locator. Finally we tell the turret about its position and store some of
//   this data in a vector for future use.
//   Special case locators are fitted sequencially while _turret_ locators use
//   their slotNumbers. The specially named locators will fall back to _turret_
//   and use the same logic as they use if there are no free slots.
//   Do not call this function every frame, it's only to be called during setup
//   or init phases.
// SeeAlso:
//   EveTurretSet, EveLocatorType, GetTurretCount, GetLocatorTransform
// --------------------------------------------------------------------------------
void EveMobile::RebuildTurretPositions()
{
	// all new
	m_turretSetsLocatorInfo.clear();
	ResetTurretLocatorCounter( false );

	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		// turret knows unique name
		std::string name = (*it)->GetLocatorName();

		// prepare a new locatorInfo entry
		TurretSetLocatorInfo locatorInfo;

		bool turretInName = false;
		
		unsigned int locatorNumber = 0;

		// Check whether this is a standard turret locator or a special case
		size_t found = name.find( "_turret_" );
		if( found!=std::string::npos )
		{
			turretInName = true;
		}

		if( !turretInName )
		{
			unsigned int totalNumber = 0;
			// Check whether we have a locator with the name
			if( !GetTurretLocatorCountingInfo( name.c_str(), locatorNumber, totalNumber ) )
			{
				// Replace the locatorName with "locator_turret_" as a fallback
				name = "locator_turret_";
				// Try again
				if( !GetTurretLocatorCountingInfo( name.c_str(), locatorNumber, totalNumber ) )
				{
					CCP_LOGWARN( "Unable to get valid locator count for '%s'", name.c_str() );
					continue;
				}
				turretInName = true;
			}
			// Check whether we have an empty slot
			if( locatorNumber > totalNumber )
			{
				if( !turretInName )
				{
					// Replace the locatorName with "locator_turret_" as a fallback
					name = "locator_turret_";
					// Try again
					if( !GetTurretLocatorCountingInfo( name.c_str(), locatorNumber, totalNumber ) )
					{
						CCP_LOGWARN( "Unable to get valid locator count for '%s'", name.c_str() );
						continue;
					}
				}
				else
				{
					CCP_LOGWARN( "Unable to get valid locator count for '%s'. Too many turrets used even with fallback", name.c_str() );
					continue;
				}
			}
		}

		if ( turretInName )
		{
			// For _turret_ locators we use the slot number to get a unique
			// locator for the turret. This also applies to the case where we
			// fall back to using _turret_ locators.
			locatorNumber = (*it)->GetSlotNumber();
		}


		char ln = '0' + locatorNumber;
		std::string locatorBase = name + ln;


		// check how many turrets this locator will have (=versions on the ship)
		unsigned int locatorCount = CountLocatorsByPrefix( locatorBase.c_str() );

		for( unsigned int i = 0; i < locatorCount; ++i )
		{
			// adjust name: put 'A' or 'B' or 'C' at the end
			char ch = 'a' + (char)i;
			std::string locatorName = locatorBase + ch;

			// try to find it, could be two different types: bone from granny or locator from jessica
			unsigned int locatorIndex;
			locatorInfo.type = DetermineLocatorType(locatorName.c_str(), locatorIndex);

			// found something?
			if( locatorInfo.type != ELT_COUNT )
			{
				// this ship knows position
				const Matrix* localMatrix = GetLocatorTransform( locatorInfo.type, locatorIndex );
				// set it
				if( localMatrix )
				{
					(*it)->SetLocalTransform( i, localMatrix );
				}
				// add it to our locator entry
				locatorInfo.locatorIndices.push_back( locatorIndex );
			}
		}
		// add the locatorInfo to this ship's list
		m_turretSetsLocatorInfo.push_back( locatorInfo );

		m_turretLocatorCountingInfo[name].currentCount++;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets the current and total arguments with contents of the m_locatorCount for 
//   the name given.
//   Returns false if name is not found.
// SeeAlso:
//   
// --------------------------------------------------------------------------------
bool EveMobile::GetTurretLocatorCountingInfo( const char* name, unsigned int& current, unsigned int& total ) const
{
	auto mapIt = m_turretLocatorCountingInfo.find( name );
	if( mapIt == m_turretLocatorCountingInfo.end() )
	{
		return false;
	}

	current = mapIt->second.currentCount + 1;
	total = mapIt->second.totalCount;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Iterates over the locators of the ship, setting the count for each to 0.
// Arguments:
//   updateTotal: When set to true the totalCount is also updated. The locators are counted.
// SeeAlso:
//   EveTurretSet, RebuildTurretPositions
// --------------------------------------------------------------------------------
void EveMobile::ResetTurretLocatorCounter( bool updateTotal )
{
	
	for( auto it = m_locators.begin(); it != m_locators.end(); ++it )
	{
		std::string name = (*it)->GetName();
		std::string prefix;

		size_t found = name.rfind("_");
		if( found!=std::string::npos )
		{
			prefix = name.substr( 0, found + 1 );

			std::map<std::string, TurretLocatorCountingInfo>::iterator mapIt;
			// Find the entry in the map
			mapIt = m_turretLocatorCountingInfo.find(prefix);
			if( mapIt == m_turretLocatorCountingInfo.end() )
			{
				// Does not exist. Add it.
				TurretLocatorCountingInfo info;
				info.currentCount = 0;
				info.totalCount = 0;

				m_turretLocatorCountingInfo[prefix] = info;
			}
			else
			{
				// set the current count to 0
				mapIt->second.currentCount = 0;
			}

			if( updateTotal )
			{
				std::string numberStr = name.substr( found + 1, 1 );
				unsigned int numberInt = atoi( numberStr.c_str() );
				if( numberInt > m_turretLocatorCountingInfo[prefix].totalCount )
				{
					m_turretLocatorCountingInfo[prefix].totalCount = numberInt;
				}
			}
		}
	}
}

//   Determines if we render this object's children or not.
// SeeAlso:
//   EveTransform
// --------------------------------------------------------------------------------
bool EveMobile::DisplayChildren() const
{
	// if it is more than .5 -> render the children!
	return ( m_activationStrenght > 0.5f );
}

// --------------------------------------------------------------------------------
// Description:
//   Play the objects activationStrengthCurve from the beginning.
// --------------------------------------------------------------------------------
void EveMobile::PlayActivationCurve()
{
	m_activationDelta = 0.0f;
	m_playActivationCurve = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Play the objects buildingCurve from the beginning.
// --------------------------------------------------------------------------------
void EveMobile::PlayClipSphereFactorCurve()
{
	m_playClipSphereFactorCurve = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Modifies the clip sphere curve by setting it's length and the time elapsed
// --------------------------------------------------------------------------------
void EveMobile::ModifyClipSphereCurve( const std::map<std::string, float>& parameters )
{
	m_clipSphereFactorDelta = 0.f;
	if( parameters.find( "elapsedTime" ) != parameters.end() )
	{
		m_clipSphereFactorDelta = parameters.at( "elapsedTime" );
	}

	float curveLength = 1.0;
	if( parameters.find( "curveLength" ) != parameters.end() )
	{
		curveLength = parameters.at( "curveLength" );
	}
	m_clipSphereFactorCurve->ScaleTime( curveLength );
}

// --------------------------------------------------------------------------------
// Description:
//   Gets called by the state machine of this object to execute some command.
// Return Value:
//   Returns true if this implementation has handled the command.
// --------------------------------------------------------------------------------
bool EveMobile::ExecuteAnimationStateCommand( const EveAnimationCommand& cmd, const std::map<std::string, float>& parameters )
{
	std::string commandData = cmd.m_data;
	float commandFloatValue = cmd.m_floatValue;
	EveAnimationCmd commandType = cmd.m_command;

	switch( commandType )
	{
	case ANIM_CMD_PLAYACTIVATION:
		if( !commandData.empty() )
		{
			IRootPtr p;
			p.Attach( BeResMan->LoadObject( commandData.c_str() ) );
			if( p == NULL )
			{
				CCP_LOGERR( "EveMobile: Couldn't find PlayActivationCurve data resource file: %s", commandData.c_str() );
				return true;
			}

			ITriScalarFunctionPtr ptr;
			if( !p->QueryInterface( BlueInterfaceIID<ITriScalarFunction>(), (void**)&ptr ) )
			{
				CCP_LOGERR( "EveMobile: PlayActivationCurve resource file %s is not of correct type!", commandData.c_str() );
				return true;
			}
			m_activationStrengthCurve = ptr;
			PlayActivationCurve();
		}
		return true;

	case ANIM_CMD_ACTIVATE_TURRETS:
		for( auto it = m_turretSets.begin(); it != m_turretSets.end(); it++ )
		{
			(*it)->EnterStateIdle();
		}
		return true;

	case ANIM_CMD_DEACTIVATE_TURRETS:
		for( auto it = m_turretSets.begin(); it != m_turretSets.end(); it++ )
		{
			(*it)->EnterStateDeactive();
		}
		return true;

	case ANIM_CMD_ACTIVATION_STRENGTH_ZERO:
		m_activationStrenght = 0.f;
		m_playActivationCurve = false;
		return true;

	case ANIM_CMD_ACTIVATION_STRENGTH_ONE:
		m_activationStrenght = 1.f;
		m_playActivationCurve = false;
		return true;

	case ANIM_CMD_PLAY_CLIPSPHERE:
		if( !commandData.empty() )
		{
			IRootPtr p;
			p.Attach( BeResMan->LoadObject( commandData.c_str() ) );
			if( p == NULL )
			{
				CCP_LOGERR( "EveMobile: Couldn't find curve data resource file: %s", commandData.c_str() );
				return true;
			}

			ITriScalarFunctionPtr ptr;
			if( !p->QueryInterface( BlueInterfaceIID<ITriScalarFunction>(), (void**)&ptr ) )
			{
				CCP_LOGERR( "EveMobile: Curve resource file %s is not of correct type!", commandData.c_str() );
				return true;
			}

			m_clipSphereFactorCurve = ptr;

			m_clipSphereCenter = -1.0f * GetBoundingSphereCenter();

			ModifyClipSphereCurve( parameters );			
			PlayClipSphereFactorCurve();
		}
		return true;
	case ANIM_CMD_STOP_CLIPSPHERE:
		m_clipSphereFactor = 0.0f;
		m_playClipSphereFactorCurve = false;
		m_clipSphereCenter = Vector3( 0.0, 0.0, 0.0 );
		return true;
	case ANIM_CMD_SET_CLIPSPHERE:
		m_clipSphereFactor = commandFloatValue;
		m_playClipSphereFactorCurve = false;
		m_clipSphereCenter = -1.0f * GetBoundingSphereCenter();
		return true;

	default:
		// not handled here, so pass it up the chain
		return EveSpaceObject2::ExecuteAnimationStateCommand( cmd,  parameters );
	}
	return false;
}
