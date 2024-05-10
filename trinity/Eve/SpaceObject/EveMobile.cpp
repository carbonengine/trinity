////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"

#include "include/TriMath.h"
#include "Tr2GrannyAnimation.h"

#include "EveMobile.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Eve/EveUpdateContext.h"
#include "Utilities/BoundingBox.h"

extern bool g_eveSpaceSceneRaytracedShadows;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveMobile::EveMobile( IRoot* lockobj ) :
	PARENTLOCK( m_turretSets )
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
	EveSpaceObject2::OnListModified( event, key, key2, value, theList );
	if( ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( theList == &m_turretSets && value )
			{
				EveTurretSetPtr turretSet( BlueCastPtr( value ) );
				if( turretSet )
				{
					RebuildTurretPositions();

					if( IsInRegistry() )
					{
						turretSet->Register( this->GetComponentRegistry() );
					}

					if( g_eveSpaceSceneRaytracedShadows )
					{
						turretSet->SetShaderOption( BlueSharedString( "SHADOW_ALGORITHM" ), BlueSharedString( "SHADOW_RT" ) );
					}
					else
					{
						turretSet->SetShaderOption( BlueSharedString( "SHADOW_ALGORITHM" ), BlueSharedString( "SHADOW_CASCADED" ) );
					}
				}
			}
			break;
		case BELIST_REMOVED:
			if( theList == &m_turretSets && value )
			{
				EveTurretSetPtr turretSet( BlueCastPtr( value ) );
				if( turretSet )
				{
					RebuildTurretPositions();
					turretSet->UnRegister( this->GetComponentRegistry() );
				}
			}
			break;
		case BELIST_UNLOADSTART:
			if( IsInRegistry() )
			{
				for( ssize_t i = 0; i < theList->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( theList->GetAt( i ) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveMobile::RegisterComponents()
{
	EveSpaceObject2::RegisterComponents();
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		for( auto& turretSet : m_turretSets )
		{
			turretSet->RegisterComponents();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Unregisters itself and its children with the scene registration container.
// --------------------------------------------------------------------------------
void EveMobile::UnRegisterComponents()
{
	EveSpaceObject2::UnRegisterComponents();

	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto& turretSet : m_turretSets )
		{
			turretSet->UnRegister( registry );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return a transform used for turret locators
// --------------------------------------------------------------------------------
const Matrix* EveMobile::GetTurretTransform( unsigned int turretSetIndex ) const
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
	unsigned int activeTurretCount = 0;
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		// If the turretSet is in aggressive mode then update active turret counter
		if( ( *it )->GetState() > EveTurretSet::STATE_TARGETING )
		{
			activeTurretCount++;
		}

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
						( *it )->SetLocalTransform( turretIdx, localMatrix );
						( *it )->UpdateTurretTransforms( GetTurretTransform( ( *it )->GetSwarmID() ) );
					}
				}
			}
		}
		// call standard update function
		( *it )->UpdateSyncronous( updateContext, GetTurretTransform( ( *it )->GetSwarmID() ) );

		// next!
		++locatorInfoIdx;
	}
	m_activeTurretCount = activeTurretCount;
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::PrepareShaderData() function
// --------------------------------------------------------------------------------
void EveMobile::PrepareShaderData( EveUpdateContext& updateContext )
{
	EveSpaceObject2::PrepareShaderData( updateContext );

	m_spaceObjectShipData.y *= m_activationStrength;
}

void EveMobile::UpdateTurretsAsyncronous( EveUpdateContext& updateContext )
{
	// now prep to get the renderables
	IEveSpaceObject2::ParentData pd;
	memset( &pd, 0, sizeof( IEveSpaceObject2::ParentData ) );

	pd.transform = *GetTurretTransform( 0 );
	pd.shipData = m_spaceObjectShipData;
	pd.clipData = m_psData.clipData;
	pd.clipData.x = m_clipSphereFactor;

	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->UpdateAsyncronous( updateContext, &pd );
	}
}

int EveMobile::GetActiveTurretCount() const
{
	return m_activeTurretCount;
}

void EveMobile::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	EveSpaceObject2::SetShaderOption( name, value );
	for( auto it = m_turretSets.begin(); m_turretSets.end() != it; ++it )
	{
		( *it )->SetShaderOption( name, value );
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
	UpdateTurretsAsyncronous( updateContext );
}

void EveMobile::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	// call base to get spaceobject's renderables
	EveSpaceObject2::UpdateVisibility( frustum, parentTransform );

	if( !m_display )
	{
		return;
	}

	// collect renderables of the turrets
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->UpdateVisibility( frustum );
	}
}
// --------------------------------------------------------------------------------
// Description:
//   Override base ::GetRenderables() function, so we can draw the turrets etc.
// --------------------------------------------------------------------------------
void EveMobile::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	// call base to get spaceobject's renderables
	EveSpaceObject2::GetRenderables( renderables, impostors );

	// collect renderables of the turrets
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->GetRenderables( renderables, m_psData.shLightingCoefficients );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::RenderDebugInfo() function, so we can draw debuginfo of
//   the turrets etc.
// --------------------------------------------------------------------------------
void EveMobile::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	EveSpaceObject2::RenderDebugInfo( renderer );

	// let the turrets render some debug info
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}
}

void EveMobile::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	EveSpaceObject2::GetDebugOptions( options );

	// Get the debug options from the turrets
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

// --------------------------------------------------------------------------------
void EveMobile::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}

	EveSpaceObject2::GetLights( lightManager );
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   TBD.
// --------------------------------------------------------------------------------
bool EveMobile::ValidateTurretLocatorName( const char* locatorName, unsigned int& locatorsFoundA, unsigned int& locatorsFoundB ) const
{
	static const char* kLocatorPrefix = "locator_turret_";
	const unsigned int kLocatorPrefixLength = static_cast<unsigned int>( strlen( kLocatorPrefix ) );

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
size_t EveMobile::GetTurretLocatorIndex( size_t turretSetIdx, size_t slotIdx ) const
{
	if( m_turretSetsLocatorInfo.size() > turretSetIdx )
	{
		if( m_turretSetsLocatorInfo[turretSetIdx].locatorIndices.size() > slotIdx )
		{
			return m_turretSetsLocatorInfo[turretSetIdx].locatorIndices[slotIdx];
		}
	}
	return 0;
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

	auto n = static_cast<unsigned int>( m_locators.size() );
	for( unsigned int i = 0; i < n; ++i )
	{
		const char* locatorName = m_locators[i]->GetName();
		// EveMobile keeps multiple types of locator, so validate the name
		ValidateTurretLocatorName( locatorName, locatorsFoundA, locatorsFoundB );
	}

	if( m_animationUpdater && m_animationUpdater->m_skeleton )
	{
		for( int boneIx = 0; boneIx < m_animationUpdater->m_skeleton->BoneCount; ++boneIx )
		{
			const granny_bone& bone = m_animationUpdater->m_skeleton->Bones[boneIx];
			ValidateTurretLocatorName( bone.Name, locatorsFoundA, locatorsFoundB );
		}
	}

	if( ( locatorsFoundA != locatorsFoundB ) && locatorsFoundB )
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
bool EveMobile::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	bool res = EveSpaceObject2::GetLocalBoundingBox( min, max );
	if( !res )
	{
		return false;
	}

	Vector3 minBounds, maxBounds;
	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		if( ( *it )->GetLocalBoundingBox( minBounds, maxBounds ) )
		{
			BoundingBoxUpdate( min, max, minBounds, maxBounds );
		}
	}
	return true;
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

	for( auto it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		// turret knows unique name
		std::string name = ( *it )->GetLocatorName();

		// prepare a new locatorInfo entry
		TurretSetLocatorInfo locatorInfo;

		bool turretInName = false;

		unsigned int locatorNumber = 0;

		// Check whether this is a standard turret locator or a special case
		size_t found = name.find( "_turret_" );
		if( found != std::string::npos )
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

		if( m_clipSphereFactor != 0 )
		{
			( *it )->SetShaderOption( BlueSharedString( "SPACE_OBJECT_CLIPPING" ), BlueSharedString( "SOC_ENABLED" ) );
		}

		if( turretInName )
		{
			// For _turret_ locators we use the slot number to get a unique
			// locator for the turret. This also applies to the case where we
			// fall back to using _turret_ locators.
			locatorNumber = ( *it )->GetSlotNumber();
		}

		char ln = '0' + locatorNumber;
		std::string locatorBase = name + ln;


		// check how many turrets this locator will have (=versions on the ship)
		unsigned int locatorCount = CountLocatorsByPrefix( locatorBase.c_str() );
		if( locatorCount == 0 )
		{
			CCP_LOGERR( "Unable to find turret locator %s on ship %s,%s", locatorBase.c_str(), m_name.c_str(), m_dna.c_str() );
		}

		for( unsigned int i = 0; i < locatorCount; ++i )
		{
			// adjust name: put 'A' or 'B' or 'C' at the end
			char ch = 'a' + static_cast<char>( i );
			std::string locatorName = locatorBase + ch;

			// try to find it, could be two different types: bone from granny or locator from jessica
			unsigned int locatorIndex;
			locatorInfo.type = DetermineLocatorType( locatorName.c_str(), locatorIndex );

			// found something?
			if( locatorInfo.type != ELT_COUNT )
			{
				// this ship knows position
				const Matrix* localMatrix = GetLocatorTransform( locatorInfo.type, locatorIndex );
				// set it
				if( localMatrix )
				{
					( *it )->SetLocalTransform( i, localMatrix );
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
		std::string name = ( *it )->GetName();
		std::string prefix;

		size_t found = name.rfind( "_" );
		if( found != std::string::npos )
		{
			prefix = name.substr( 0, found + 1 );

			// Find the entry in the map
			std::map<std::string, TurretLocatorCountingInfo>::iterator mapIt = m_turretLocatorCountingInfo.find( prefix );
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
	return ( m_activationStrength > 0.5f );
}

void EveMobile::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	EveSpaceObject2::RegisterWithQuadRenderer( quadRenderer );
	for( auto it = begin( m_turretSets ); it != end( m_turretSets ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveMobile::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	EveSpaceObject2::AddQuadsToQuadRenderer( frustum, quadRenderer );
	for( auto it = begin( m_turretSets ); it != end( m_turretSets ); ++it )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveMobile::SetControllerVariable( const char* name, float value )
{
	EveSpaceObject2::SetControllerVariable( name, value );
	for( auto it = begin( m_turretSets ); it != end( m_turretSets ); ++it )
	{
		( *it )->SetControllerVariable( name, value );
	}
}

void EveMobile::HandleControllerEvent( const char* name )
{
	EveSpaceObject2::HandleControllerEvent( name );
	for( auto it = begin( m_turretSets ); it != end( m_turretSets ); ++it )
	{
		( *it )->HandleControllerEvent( name );
	}
}

void EveMobile::StartControllers()
{
	EveSpaceObject2::StartControllers();
	for( auto it = begin( m_turretSets ); it != end( m_turretSets ); ++it )
	{
		( *it )->StartControllers();
	}
}