////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"

#include "Tr2GrannyAnimation.h"

#include "EveMobile.h"
#include "Eve/EveTurretSet.h"
#include "Eve/EveLocator2.h"
#include "Eve/EveUpdateContext.h"
#include "Utilities/ViewDistanceInfo.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveMobile::EveMobile( IRoot* lockobj ) :
	PARENTLOCK( m_turretSets ),
	m_activationDelta( 0.f ),
	m_playActivationCurve( false )
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
//   Override base ::UpdateSyncronous() function, so we can update the turrets and 
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveMobile::UpdateSyncronous( EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	EveSpaceObject2::UpdateSyncronous( updateContext );
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		(*it)->UpdateModelLOD();
	}

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
		(*it)->Update( deltaT, time, &m_worldTransform );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::UpdateAsyncronous() function, so we can update the turrets and 
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveMobile::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	EveSpaceObject2::UpdateAsyncronous( updateContext );

	// prepare shader data:
	if( m_activationStrengthCurve && m_playActivationCurve )
	{
		float deltaT = updateContext.GetDeltaT();
		m_activationDelta += deltaT;
		m_spaceObjectData.y = m_activationStrengthCurve->Update( m_activationDelta );
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
		(*it)->GetRenderables( frustum, renderables );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info using this object's bounds.
// --------------------------------------------------------------------------------
void EveMobile::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{ 
	EveSpaceObject2::UpdateViewDistanceInfo( frustum, viewDistance );

	if( !m_display || !m_isVisible )
	{
		return;
	}

	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); it++ )
	{
		(*it)->UpdateViewDistanceInfo( frustum, viewDistance );
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
		size_t found = name.find( "turret" );
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
//   Activate all of the object's turret sets
// --------------------------------------------------------------------------------
void EveMobile::ActivateTurrets()
{
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); it++ )
	{
		(*it)->EnterStateIdle();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Deactivate all of the object's turret sets
// --------------------------------------------------------------------------------
void EveMobile::DeactivateTurrets()
{
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); it++ )
	{
		(*it)->EnterStateDeactive();
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

// --------------------------------------------------------------------------------
// Description:
//   Determines if we render this object's children or not.
// SeeAlso:
//   EveTransform
// --------------------------------------------------------------------------------
bool EveMobile::DisplayChildren() const
{
	// so in m_spaceObjectData.y we store the current activation strength.
	// if it is more than .5 -> render the children!
	return ( m_spaceObjectData.y > 0.5f );
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
//   Set activation strength to the specified value.
// --------------------------------------------------------------------------------
void EveMobile::SetActivationStrength( float strength )
{
	m_spaceObjectData.y = strength;
	m_playActivationCurve = false;
}
