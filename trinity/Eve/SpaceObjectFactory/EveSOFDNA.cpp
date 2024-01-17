////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
#include "Utilities/BoundingSphere.h"
#include "EveSOFDNA.h"
#include "EveSOFUtils.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "ITr2Renderable.h"

bool FileExists( const std::string& path )
{
	std::wstring wstrCopy( path.begin(), path.end() );
	return BePaths->FileExists( wstrCopy );
}

// dna syntax
static char s_dnaSeperatorCmd = ':';
static char s_dnaSeperatorArg = '?';
static char s_dnaSeperatorList = ';';

// dna commands
static std::string s_dnaCommands[] = {
	"invalid",				// CMD_INVALID
	"material",				// CMD_MATERIAL
	"mesh",					// CMD_MESH
	"respathinsert",		// CMD_RESPATHINSERT
	"variant",				// CMD_VARIANT
	"class",				// CMD_CLASS
	"pattern",				// CMD_PATTERN
	"layout",				// CMD_LAYOUT
	"experimental",			// CMD_EXPERIMENTAL
};

// build classes
static std::string s_dnaClasses[] = {
	"ship",					// BUILDCLASS_SHIP
	"mobile",				// BUILDCLASS_MOBILE
	"stationary",			// BUILDCLASS_STATIONARY
	"swarm",				// BUILDCLASS_SWARM
	"extension",			// BUILDCLASS_EXTENSION
};

static_assert( sizeof( s_dnaClasses ) / sizeof( s_dnaClasses[0] ) == EveSOFDataHull::BUILDCLASS_COUNT, 
			  "number of items in s_dnaClasses array does not match the number of items in EveSOFDataHull::BuildClass" );

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOFDNA::EveSOFDNA( IRoot* lockobj ) :
	m_factionData( nullptr ),
	m_raceData( nullptr ),
	m_patternData( nullptr ),
	m_genericData( nullptr )
{
}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveSOFDNA::~EveSOFDNA()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Checks if this DNA is valid and ready to go
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsValid() const
{
	// need all three basic parts: multi-hull list, faction data pointer and race data pointer
	if( m_hullDatas.empty() || !m_factionData || !m_raceData )
	{
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Checks the content of the DNA. This is slow and should only be called
//   for offline validation.
// --------------------------------------------------------------------------------
bool EveSOFDNA::ValidateContent()
{
	// just to be sure
	if( !IsValid() )
	{
		return false;
	}

	// check every single command
	for( auto cit = m_commands.begin(); cit != m_commands.end(); ++cit )
	{
		// the command string itself must exist!
		unsigned int cmd = CMD_INVALID;
		for( unsigned int i = 0; i < CMD_MAX; ++i )
		{
			if( cit->first.compare( s_dnaCommands[i] ) == 0 )
			{
				cmd = i;
				break;
			}
		}
		if( cmd == CMD_INVALID )
		{
			CCP_LOGERR( "Invalid command found: %s", cit->first.c_str() );
			return false;
		}

		// now validate the args for every command
		switch( cmd )
		{
		case CMD_MATERIAL:
			// number of arguments must number of materials
			if( m_genericData->materialPrefixes.size() != cit->second.size() )
			{
				return false;
			}
			// each arg is a material or "none" and must exist
			for( auto ait = cit->second.begin(); ait != cit->second.end(); ++ait )
			{
				if( ait->compare( "none" ) == 0 )
				{
					continue;
				}
				else if( !m_dataMgr->HasMaterialData( ait->c_str() ) )
				{
					return false;
				}
			}
			break;
		case CMD_MESH:
			// has one argument
			if( cit->second.size() != 1 )
			{
				return false;
			}
			break;
		case CMD_RESPATHINSERT:
			// has one argument
			if( cit->second.size() != 1 )
			{
				return false;
			}
			break;
		case CMD_VARIANT:
			// has one argument
			if( cit->second.size() != 1 )
			{
				// variant must exists in generic data
				if( m_genericData->variants.find( BlueSharedString( cit->second[0] ) ) == m_genericData->variants.end() )
				{
					return false;
				}
			}
			break;
		case CMD_CLASS:
			// has one argument
			if( cit->second.size() != 1 )
			{
				return false;
			}
			if( std::find( std::begin( s_dnaClasses ), std::end( s_dnaClasses ), cit->second[0] ) == std::end( s_dnaClasses ) )
			{
				return false;
			}
			break;
		case CMD_PATTERN:
			// has three argument: the pattern name and two material names
			if( cit->second.size() != 3 )
			{
				return false;
			}
			if( !m_dataMgr->HasPatternData( cit->second[0].c_str() ) )
			{
				return false;
			}
			if( ( cit->second[1].compare( "none" ) == 0 ) || ( cit->second[2].compare( "none" ) == 0 ) )
			{
				continue;
			}
			else if( ( !m_dataMgr->HasMaterialData( cit->second[1].c_str() ) ) || ( !m_dataMgr->HasMaterialData( cit->second[2].c_str() ) ) )
			{
				return false;
			}
			break;
		case CMD_LAYOUT:
			// Has at least 1 layout
			if( cit->second.size() == 0)
			{
				return false;
			}
			for( size_t i = 0; i < cit->second.size(); i++ )
			{
				if( !m_dataMgr->HasLayoutData( cit->second[i].c_str() ) )
				{
					return false;
				}
			}
			return true;
		}
	}

	// if we make it this far, it's all ok
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Set a complete dna string to this dna
// --------------------------------------------------------------------------------
void EveSOFDNA::Setup( const char* dnaString, EveSOFDataMgrPtr dataMgr )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<std::string> commandArgs;

	// rember dna string
	m_dna = dnaString;

	// remember the pointer to the BIG lib as long as this DNA object lives
	m_dataMgr = dataMgr;

	// split up dna string in all subparts
	std::vector<std::string> dnaParts;
	StringSplit( dnaParts, dnaString, s_dnaSeperatorCmd );

	// need three at least
	if( dnaParts.size() < 3 )
	{
		CCP_LOGERR( "Invalid SOF DNA, not enough subparts: %s", dnaString );
		return;
	}

	// additional dna subparts
	for( size_t dnaSubpart = 3; dnaSubpart < dnaParts.size(); ++dnaSubpart )
	{
		// split into command and args
		StringSplit( commandArgs, dnaParts[ dnaSubpart ].c_str(), s_dnaSeperatorArg );
		if( commandArgs.size() != 2 )
		{
			CCP_LOGERR( "Invalid SOF DNA, incorrect command and args: %s", dnaString );
			return;
		}

		// get commands
		std::vector<std::string> commandList;
		StringSplit( commandList, commandArgs[1].c_str(), s_dnaSeperatorList );

		// put into map, warning: this might overwrite a similar command!
		m_commands[commandArgs[0]] = commandList;
		commandArgs.clear();
	}

	// the main dna hull can be a list (multi-hulls)
	StringSplit( m_hullNames, dnaParts[0].c_str(), s_dnaSeperatorList );
	if( m_hullNames.empty() )
	{
		CCP_LOGERR( "Couldn't find at least one hull name: %s", dnaString );
		return;
	}

	// part 2 and 3 is faction and race
	m_factionName = dnaParts[1];
	m_raceName = dnaParts[2];

	// make sure we find this hull(s)
	m_hullDatas.clear();
	for( auto it = m_hullNames.begin(); it != m_hullNames.end(); ++it )
	{
		const EveSOFDataMgr::HullData* h = m_dataMgr->GetHullData( it->c_str() );
		if( h == nullptr )
		{
			CCP_LOGERR( "Couldn't find the requested hull: %s", it->c_str() );
			return;
		}
		m_hullDatas.push_back( h );
	}

	// just to be absolutley sure...
	if( m_hullNames.size() != m_hullDatas.size() )
	{
		CCP_LOGERR( "Missmatching sizes of found data and hullnames: %s", dnaString );
		return;
	}

	// make sure we find this faction
	m_factionData = m_dataMgr->GetFactionData( m_factionName.c_str() );
	if( m_factionData == nullptr )
	{
		CCP_LOGERR( "Couldn't find the requested faction: %s", dnaString );
		return;
	}
	// make sure we find this race
	m_raceData = m_dataMgr->GetRaceData( m_raceName.c_str() );
	if( m_raceData == nullptr )
	{
		CCP_LOGERR( "Couldn't find the requested race: %s", dnaString );
		return;
	}

	// do we have a pattern DNA commmand?
	if( GetDnaCommandArgs( CMD_PATTERN, commandArgs ) )
	{
		m_patternData = m_dataMgr->GetPatternData( commandArgs[0].c_str() );
	}

	// do we have a layout DNA commmand?
	if( GetDnaCommandArgs( CMD_LAYOUT, commandArgs ) )
	{
		m_layoutData = m_dataMgr->GetLayoutData( commandArgs );
	}

	// generics
	m_genericData = m_dataMgr->GetGenericData();

	// some dna commands require a custom block data
	if( HasDnaCommand( CMD_VARIANT ) )
	{
		SetupCustomData();
	}

	// store the parent bounding sphere here as a copy of the hull bounding sphere...
	m_parentBoundingSphere = GetHullBoundingSphere(); 
	m_parentHullShapeEllipsoid = GetHullShapeEllipsoid();

	m_isSkinned = m_hullDatas[0]->isSkinned;
}


// --------------------------------------------------------------------------------
// Description:
//   Setup this dna object from a descriptor and a parent
// --------------------------------------------------------------------------------
void EveSOFDNA::Setup( const BlueSharedString layoutName, const EveSOFDataMgr::DNADescriptorData& descriptor, EveSOFDNAPtr parent, const EveSOFDataMgrPtr dataMgr )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// remember the pointer to the BIG lib as long as this DNA object lives
	m_dataMgr = dataMgr;

	// The descriptor needs to have a hull!!!
	m_hullDatas.clear();
	StringSplit( m_hullNames, descriptor.hull.c_str(), s_dnaSeperatorList );
	for( auto hullName: m_hullNames)
	{
		const EveSOFDataMgr::HullData* h = m_dataMgr->GetHullData( hullName.c_str() );
		if( h == nullptr )
		{
			CCP_LOGERR( "Couldn't find the requested hull: %s for layout: %s", hullName.c_str(), layoutName.c_str() );
			return;
		}
		m_hullDatas.push_back( h );		
	}
	
	if( m_hullDatas.empty() )
	{
		CCP_LOGERR( "Couldn't find at least one hull name for layout: %s", layoutName.c_str() );
		return;
	}
 
	m_factionName = descriptor.faction.empty() ? parent->m_factionName : descriptor.faction.c_str();
	m_raceName = descriptor.race.empty() ? parent->m_raceName : descriptor.race.c_str();

	// make sure we find this faction
	m_factionData = m_dataMgr->GetFactionData( m_factionName.c_str() );
	if( m_factionData == nullptr )
	{
		CCP_LOGERR( "Couldn't find the requested faction: %s for layout: %s", m_factionName.c_str(), layoutName.c_str() );
		return;
	}
	// make sure we find this race
	m_raceData = m_dataMgr->GetRaceData( m_raceName.c_str() );
	if( m_raceData == nullptr )
	{
		CCP_LOGERR( "Couldn't find the requested race: %s for layout: %s", m_raceName.c_str(), layoutName.c_str() );
		return;
	}

	// Process Materials
	std::vector<std::string> materialArgs(4, "None");
	bool gotMaterials = parent->GetDnaCommandArgs( CMD_MATERIAL, materialArgs );
	// Small question about how this should function, if the parent has 2 materials
	// and the descriptor has 1 material, what should we do then? Override? Merge? TBD!
	if( !descriptor.material1.empty() )
	{
		materialArgs[0] = descriptor.material1.c_str();
		gotMaterials = true;
	}
	if( !descriptor.material2.empty() )
	{
		materialArgs[1] = descriptor.material2.c_str();
		gotMaterials = true;
	}
	if( !descriptor.material3.empty() )
	{
		materialArgs[2] = descriptor.material3.c_str();
		gotMaterials = true;
	}
	if( !descriptor.material4.empty() )
	{
		materialArgs[3] = descriptor.material4.c_str();
		gotMaterials = true;
	}

	if( gotMaterials )
	{
		m_commands[s_dnaCommands[CMD_MATERIAL]] = materialArgs;
	}

	// Process respath insert
	std::vector<std::string> respathInsert;
	if( parent->GetDnaCommandArgs( CMD_RESPATHINSERT, respathInsert ) )
	{
		m_commands[s_dnaCommands[CMD_RESPATHINSERT]] = respathInsert;
	}
	
	// Process respath insert
	std::vector<std::string> variant;
	if( parent->GetDnaCommandArgs( CMD_VARIANT, variant ) )
	{
		m_commands[s_dnaCommands[CMD_VARIANT]] = variant;
	}

	// nested layouts!
	if( !descriptor.layout.empty() )
	{
		m_commands[s_dnaCommands[CMD_LAYOUT]] = { std::string( descriptor.layout.c_str() ) };
		m_layoutData = m_dataMgr->GetLayoutData( m_commands[s_dnaCommands[CMD_LAYOUT]] );
	}

	// patterns
	// TODO make this work properly (inheriting the parent pattern)
	std::vector<std::string> patterns;
	if( !descriptor.pattern.empty() )
	{
		m_commands[s_dnaCommands[CMD_PATTERN]] = patterns;
		m_patternData = m_dataMgr->GetPatternData( patterns[0].c_str() );
	}
	// generics
	m_genericData = m_dataMgr->GetGenericData();

	// some dna commands require a custom block data
	if( parent->HasDnaCommand( CMD_VARIANT ) )
	{
		SetupCustomData();
	}

	// contruct the dna string
	m_dna = std::string( descriptor.hull.c_str() ) + s_dnaSeperatorCmd + std::string( m_factionName ) + s_dnaSeperatorCmd + std::string( m_raceName );
	for( auto commands: m_commands )
	{
		auto commandType = commands.first;
		auto commandArgs = commands.second;

		m_dna += s_dnaSeperatorCmd + commandType + s_dnaSeperatorArg;

		for( auto arg: commandArgs )
		{
			m_dna += arg + s_dnaSeperatorList;
		}
		m_dna.pop_back();
	}

        // This allows us to get the top parent hulls information
	m_parentBoundingSphere = parent->GetParentBoundingSphere();
	m_parentHullShapeEllipsoid = parent->GetParentHullShapeEllipsoid();

	m_isSkinned = m_hullDatas[0]->isSkinned;
}

void EveSOFDNA::SetParentBoundingSphere( const CcpMath::Sphere& boundingSphere )
{
	m_parentBoundingSphere = boundingSphere;
}

void EveSOFDNA::SetParentShapeEllipsoidInfo( const CcpMath::AxisAlignedEllipsoid& ellipsoid )
{
	m_parentHullShapeEllipsoid = ellipsoid;
}


// --------------------------------------------------------------------------------
// Description:
//   Return the dna string as a whole
// --------------------------------------------------------------------------------
void EveSOFDNA::SetupCustomData()
{
	// ok we need custom hull data, but set up basic stuff here
	m_customHullData.resize( m_hullDatas.size() );
	for( size_t i = 0; i < m_hullDatas.size(); ++i )
	{
		m_customHullData[i].buildClass = m_hullDatas[i]->buildClass;
		m_customHullData[i].geometryResFilePath = m_hullDatas[i]->geometryResFilePath;
		m_customHullData[i].boundingSphere = m_hullDatas[i]->boundingSphere;
		m_customHullData[i].shapeEllipsoid = m_hullDatas[i]->shapeEllipsoid;
		m_customHullData[i].isSkinned = m_hullDatas[i]->isSkinned;
		m_customHullData[i].enableDynamicBoundingSphere = m_hullDatas[i]->enableDynamicBoundingSphere;
		m_customHullData[i].castShadow = m_hullDatas[i]->castShadow;
		m_customHullData[i].audioPosition = m_hullDatas[i]->audioPosition;
	}

	// do we have a dna variant command for this?
	std::vector<std::string> variantCommandArgs;
	if( GetDnaCommandArgs( CMD_VARIANT, variantCommandArgs ) )
	{
		// do we have this variant's data?
		auto finder = m_genericData->variants.find( BlueSharedString( variantCommandArgs[0] ) );
		if( finder != m_genericData->variants.end() )
		{
			// apply to all multi hulls
			for( size_t hullIdx = 0; hullIdx < m_customHullData.size(); ++hullIdx )
			{
				// what area?
				std::vector<EveSOFDataMgr::HullAreas>* targetArea = &m_customHullData[hullIdx].opaqueAreas;
				if( finder->second.isTransparent )
				{
					targetArea = &m_customHullData[hullIdx].transparentAreas;
				}
				// for now only use a single additive area
				for( auto it = m_hullDatas[hullIdx]->opaqueAreas.begin(); it != m_hullDatas[hullIdx]->opaqueAreas.end(); ++it )
				{
					targetArea->push_back( finder->second.hullAreaData );
					targetArea->back().index = it->index;
					targetArea->back().count = it->count;
				}
			}
		}
	}

	// adjust pointers
	for( size_t i = 0; i < m_hullDatas.size(); ++i )
	{
		m_hullDatas[i] = &m_customHullData[i];
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return the dna string as a whole
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetDnaString() const
{
	return m_dna.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return area shader res path folder
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetAreaShaderLocationResPath() const
{
	return m_genericData->areaShaderLocation.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return decal shader res path folder
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetDecalShaderLocationResPath() const
{
	return m_genericData->decalShaderLocation.c_str();
}

float EveSOFDNA::GetDecalMinScreenSize( EveSOFDataHullDecalSetItem::Usage usage ) const
{
	return m_genericData->decalMinScreenSize[usage];
}

// --------------------------------------------------------------------------------
// Description:
//   Return decal shader
// --------------------------------------------------------------------------------
uint32_t EveSOFDNA::GetDecalShader() const
{
	return 0;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the prefix string for every shader
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetShaderPrefix( bool isAnimated ) const
{
	if( isAnimated )
	{
		return m_genericData->shaderPrefixAnimated.c_str();
	}
	else
	{
		return m_genericData->shaderPrefix.c_str();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return complete shader path with appropriate prefixes
// --------------------------------------------------------------------------------
std::string EveSOFDNA::GetCompleteShaderPath( const char* path ) const
{
	std::string shaderPath = std::string( "/" ) + std::string( path );
	StringInsertStubAfter( shaderPath, "/", GetShaderPrefix( IsHullAnimated() ) );
	return GetAreaShaderLocationResPath() + shaderPath;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the generic textures etc. for a given area shader
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericShaderData* EveSOFDNA::GetGenericAreaShaderData( const BlueSharedString& shaderName ) const
{
	auto finder = m_genericData->areaShaderData.find( shaderName );
	if( finder == m_genericData->areaShaderData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the generic textures etc. for a given decal shader
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericDecalShaderData* EveSOFDNA::GetGenericDecalShaderData( const BlueSharedString& shaderName ) const
{
	auto finder = m_genericData->decalShaderData.find( shaderName );
	if( finder == m_genericData->decalShaderData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the generic damage data, impacts are mostly the same for everything
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericDamageData* EveSOFDNA::GetGenericDamageData() const
{
	return &m_genericData->damage;
}


// --------------------------------------------------------------------------------
// Description:
//   Return the generic hull damage data, impacts are mostly the same for everything
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericHullDamageData* EveSOFDNA::GetGenericHullDamageData() const
{
	return &m_genericData->hullDamage;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the generic damage data, impacts are mostly the same for everything
// --------------------------------------------------------------------------------
const EveSwarm::BehaviorProperties* EveSOFDNA::GetGenericSwarmProperties() const
{
	return &m_genericData->swarmBehavior;
}

// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericBannerShaderData& EveSOFDNA::GetGenericBannerShaderData() const
{
	return m_genericData->bannerShader;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the factional group-planeset-data by a given groupindex
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionPlaneSetColorData* EveSOFDNA::GetFactionPlaneSetData( int groupIndex ) const
{
	// -1 is null
	if( groupIndex != -1 )
	{
		auto finder = m_factionData->planeSetsColors.find( groupIndex );
		if( finder != m_factionData->planeSetsColors.end() )
		{
			return &finder->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the factional group-spotlightset-data by a given groupindex
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionSpotlightSetColorData* EveSOFDNA::GetFactionSpotlightSetData( int groupIndex ) const
{
	// -1 is null
	if( groupIndex != -1 )
	{
		auto finder = m_factionData->spotlightSetsColors.find( groupIndex );
		if( finder != m_factionData->spotlightSetsColors.end() )
		{
			return &finder->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the factional group-child-data by a given groupindex
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionChildData* EveSOFDNA::GetFactionChildData( int groupIndex ) const
{
	// -1 is null
	if( groupIndex != -1 )
	{
		auto finder = m_factionData->childData.find( groupIndex );
		if( finder != m_factionData->childData.end() )
		{
			return &finder->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the planesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullPlaneSetData>& EveSOFDNA::GetHullPlaneSets( size_t n ) const
{
	return m_hullDatas[n]->planeSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spotlightsets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpotlightSetData>& EveSOFDNA::GetHullSpotlightSets( size_t n ) const
{
	return m_hullDatas[n]->spotlightSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spritesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpriteSetData>& EveSOFDNA::GetHullSpriteSets( size_t n ) const
{
	return m_hullDatas[n]->spriteSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spritelinesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpriteLineSetData>& EveSOFDNA::GetHullSpriteLineSets( size_t n ) const
{
	return m_hullDatas[n]->spriteLineSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the hazesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullHazeSetData>& EveSOFDNA::GetHullHazeSets( size_t n ) const
{
	return m_hullDatas[n]->hazeSets;
}

// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullBannerData>& EveSOFDNA::GetHullBanners( size_t n ) const
{
	return m_hullDatas[n]->banners;
}

// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullBannerSetData>& EveSOFDNA::GetHullBannerSets( size_t n ) const
{
	return m_hullDatas[n]->bannerSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the turret locators on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorData>& EveSOFDNA::GetHullTurretLocators( size_t n ) const
{
	return m_hullDatas[n]->locatorTurrets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the damage locators on this hull
// --------------------------------------------------------------------------------
const std::vector<BlueSharedString> EveSOFDNA::GetHullLocatorSetNames( size_t n ) const
{
	std::vector<BlueSharedString> locatorNames;

	for( auto locatorSet = m_hullDatas[n]->locatorSets.begin(); locatorSet != m_hullDatas[n]->locatorSets.end(); ++locatorSet )
	{
		locatorNames.push_back( locatorSet->first );
	}
	return locatorNames;
}

// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullNextSubsystemOffset( size_t n ) const
{
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* nextSubsystemlocators = GetHullLocators( "next_subsystem", n );
	if( nextSubsystemlocators && !nextSubsystemlocators->empty() )
	{
		// always just take first locator
		return &nextSubsystemlocators->at( 0 ).position;
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
bool EveSOFDNA::GetHullTextureWithMeshIndex( std::string& resPath, const BlueSharedString& textureName, int32_t meshIndex, size_t n, std::unordered_map<std::string, bool>* existingFilesCache ) const
{
	// find the textures with the meshindex lookup map
	auto hullFinder = m_hullDatas[n]->meshIndexToOpaqueAreaLookup.find( meshIndex );
	if( hullFinder != m_hullDatas[n]->meshIndexToOpaqueAreaLookup.end() )
	{
		auto& textures = m_hullDatas[n]->opaqueAreas[hullFinder->second].textures;
		// find the right texture with it's name
		auto texFinder = textures.find( textureName );
		if( texFinder != textures.end() )
		{
			resPath = texFinder->second.resFilePath;
			ModifyTextureResPath( resPath, existingFilesCache );
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the damage locators on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorDirectionData>* EveSOFDNA::GetHullLocators( const char* setName, size_t n ) const
{
	auto locatorSet = m_hullDatas[n]->locatorSets.find( BlueSharedString( setName ) );
	if( locatorSet == m_hullDatas[n]->locatorSets.end() )
	{
		return nullptr;
	}
	return &locatorSet->second;
}


// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the damage locators on this hull
// --------------------------------------------------------------------------------
unsigned int EveSOFDNA::GetLocatorCount( const char* setName ) const
{
	size_t count = 0;
	for( const auto& hull : m_hullDatas ) {
		auto locatorSet = hull->locatorSets.find( BlueSharedString( setName ) );
		
		if( locatorSet != hull->locatorSets.end() )
		{
			count += locatorSet->second.size();
		}
	}
	return (unsigned int)count;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the children of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullChild>& EveSOFDNA::GetHullChildren() const
{
	return m_hullDatas[0]->children;
}


// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the childSets of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullChildSetData>& EveSOFDNA::GetHullChildSets() const
{
	return m_hullDatas[0]->childSets;
}


// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the instanced meshes of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullInstancedMesh>& EveSOFDNA::GetHullInstancedMeshes() const
{
	return m_hullDatas[0]->instancedMeshes;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array of all the animations for children
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAnimation>& EveSOFDNA::GetHullAnimations() const
{
	return m_hullDatas[0]->animations;
}

// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSoundEmitter>& EveSOFDNA::GetHullSoundEmitters() const
{
	return m_hullDatas[0]->soundEmitters;
}

// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullController>& EveSOFDNA::GetHullControllers() const
{
	return m_hullDatas[0]->controllers;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns an array of all the decal sets for this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullDecalSetData>& EveSOFDNA::GetHullDecalSets( size_t n ) const
{
	return m_hullDatas[n]->hullDecalSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns an array of all the light sets for this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullLightSetData>& EveSOFDNA::GetHullLightSets( size_t n ) const
{
	return m_hullDatas[n]->hullLightSets;
}


// --------------------------------------------------------------------------------
// Description:
//   Changes the provided texture resource path, maybe modified depending on dna
// --------------------------------------------------------------------------------
void EveSOFDNA::ModifyTextureResPath( std::string& resPath, std::unordered_map<std::string, bool>* existingFilesCache ) const
{
	// try finding the insert string...
	const char* pathInsert = nullptr;

	// ...from faction?
	if( !m_factionData->resPathInsert.empty())
	{
		pathInsert = m_factionData->resPathInsert.c_str();
	}

	// ...from dna?
	std::vector<std::string> commandArgs;
	if( GetDnaCommandArgs( CMD_RESPATHINSERT, commandArgs ) )
	{
		// has only one parameter: a string
		if( commandArgs.size() == 1 )
		{
			// check for "none", which will null-ify the respathinsert
			if(commandArgs[0] == "none")
			{
				pathInsert = nullptr;
			}
			else
			{
				pathInsert = commandArgs[0].c_str();
			}
		}
	}

	// found anyting?
	if( pathInsert )
	{
		std::string resPathCopy = resPath;

		// insert sub folder
		size_t index = resPath.rfind("/");
		if( index != std::string::npos )
		{
			resPathCopy.insert( index + 1, std::string( pathInsert ) + "/" );
		}

		// insert part into filename
		std::string insertStr = "_" + std::string( pathInsert );
		if( StringInsertStubBefore( resPathCopy, "_", insertStr.c_str() ) )
		{
			if( existingFilesCache )
			{
				auto found = existingFilesCache->find( resPathCopy );
				if( found != end( *existingFilesCache ) )
				{
					if( found->second )
					{
						resPath = resPathCopy;
					}
				}
				else
				{
					auto exists = FileExists( resPathCopy );
					(*existingFilesCache)[resPathCopy] = exists;
					if( exists )
					{
						resPath = resPathCopy;
					}
				}
			}
			else if( FileExists( resPathCopy ) )
			{
				resPath = resPathCopy;
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return the redfile path to the model's geometry (gr2)
// --------------------------------------------------------------------------------
std::string EveSOFDNA::GetHullGeometryResPath() const
{
	// multi-hull geometry is different and needs some string mangeling
	if( m_hullDatas.size() == 1)
	{
		return m_hullDatas[0]->geometryResFilePath;
	}

	std::string modifiedResPath = m_hullDatas[0]->geometryResFilePath;

	// collect gr2 string insert based on variant numbers
	std::string variantNumber;
	for( auto it = m_hullNames.cbegin(); it != m_hullNames.cend(); ++it )
	{
		variantNumber += it->back();
	}

	// build a path to the shared folder
	std::vector<std::string> nameParts;
	StringSplit( nameParts, m_hullNames[0].c_str(), '_' );
	if( !nameParts.empty() )
	{
		// .../asc1/asc1_T3_s4v1/asc1_T3_s4v1.gr2 -> /asc1/asc1_T3_s4v1/asc1_T3_1232.gr2
		std::string oldGeo = nameParts.back() + ".gr2";
		std::string newGeo = variantNumber + ".gr2";
		StringReplace( modifiedResPath, oldGeo.c_str(), newGeo.c_str() );
		// .../asc1/asc1_T3_s4v1/asc1_T3_1232.gr2 -> /asc1/asc1_T3_all/asc1_T3_1232.gr2
		std::string oldFolder = nameParts.back() + "/";
		std::string newFolder = "all/";
		StringReplace( modifiedResPath, oldFolder.c_str(), newFolder.c_str() );
	}

	return modifiedResPath;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the redfile path to the model's rotation curve
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetModelRotationCurvePath() const
{
	if( m_hullDatas[0]->modelRotationCurvePath.empty() )
	{
		return nullptr;
	}
	return m_hullDatas[0]->modelRotationCurvePath.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the redfile path to the model's translation curve
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetModelTranslationCurvePath() const
{
	if( m_hullDatas[0]->modelTranslationCurvePath.empty() )
	{
		return nullptr;
	}
	return m_hullDatas[0]->modelTranslationCurvePath.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   How many multi-hulls are in this DNS string
// --------------------------------------------------------------------------------
size_t EveSOFDNA::GetMultiHullCount() const
{
	return m_hullDatas.size();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the build class for this hull
// --------------------------------------------------------------------------------
EveSOFDataHull::BuildClass EveSOFDNA::GetBuildClass() const
{
	std::vector<std::string> commandArgs;
	if( GetDnaCommandArgs( CMD_CLASS, commandArgs ) && !commandArgs.empty() )
	{
		auto found = std::find( std::begin( s_dnaClasses ), std::end( s_dnaClasses ), commandArgs[0] );
		if( found != std::end( s_dnaClasses ) )
		{
			return EveSOFDataHull::BuildClass( found - std::begin( s_dnaClasses ) );
		}
	}
	return m_hullDatas[0]->buildClass;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the bounding sphere info of this hull
// --------------------------------------------------------------------------------
CcpMath::Sphere EveSOFDNA::GetHullBoundingSphere() const
{
	CcpMath::Sphere boundingSphere = CcpMath::Sphere();

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < GetMultiHullCount(); ++hullIdx )
	{
		CcpMath::Sphere s( m_hullDatas[hullIdx]->boundingSphere );
		s.center += hullOffset;
		boundingSphere.IncludeSphere( s );

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}

	return boundingSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the bounding sphere info of the parent
// --------------------------------------------------------------------------------
CcpMath::Sphere EveSOFDNA::GetParentBoundingSphere() const
{
	return m_parentBoundingSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the shape ellipsoid, needed for layouts to recalculate the 
//	 ellipsoid data
// --------------------------------------------------------------------------------
CcpMath::AxisAlignedEllipsoid& EveSOFDNA::GetParentHullShapeEllipsoid()
{
	return m_parentHullShapeEllipsoid;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the center of this hull's shape ellipsoid
// --------------------------------------------------------------------------------
const CcpMath::AxisAlignedEllipsoid& EveSOFDNA::GetHullShapeEllipsoid() const
{
	// if multi-hull or using layout the ellipsoid is dynamically generated later
	return m_hullDatas[0]->shapeEllipsoid;
}
// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the audio position of this hull
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullAudioPosition( size_t n ) const
{
	// to simplify things only take the audiolocation from the last hull, which should hold the engine
	if( n + 1 == GetMultiHullCount() )
	{
		return &m_hullDatas[n]->audioPosition;
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Is this hull animated/skinned?
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsHullAnimated() const
{
	return m_isSkinned;
}

void EveSOFDNA::DisableAnimation()
{
	m_isSkinned = false;
}

// --------------------------------------------------------------------------------
// Description:
//   Is this hull animated/skinned?
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsHullUsingDecalSets() const
{
	return m_hullDatas[0]->isUsingDecalSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Does this hull use dynamic bounding spheres?
// --------------------------------------------------------------------------------
bool EveSOFDNA::DynamicBoundingSphereEnabled() const
{
	return ( m_hullDatas.size() > 1 ) || ( m_hullDatas[0]->enableDynamicBoundingSphere );
}

// --------------------------------------------------------------------------------
// Description:
//   Does this hull cast a shadow?
// --------------------------------------------------------------------------------
bool EveSOFDNA::CastShadow() const
{
	return m_hullDatas[0]->castShadow;
}

// --------------------------------------------------------------------------------
// Description:
//   Get a list of all the hull's meshareas given the type
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAreas>* EveSOFDNA::GetHullMeshAreas( TriBatchType type, size_t n ) const
{
	switch( type )
	{
	case TRIBATCHTYPE_OPAQUE:
		return &m_hullDatas[n]->opaqueAreas;
	case TRIBATCHTYPE_DECAL:
		return &m_hullDatas[n]->decalAreas;
	case TRIBATCHTYPE_TRANSPARENT:
		return &m_hullDatas[n]->transparentAreas;
	case TRIBATCHTYPE_ADDITIVE:
		return &m_hullDatas[n]->additiveAreas;
	case TRIBATCHTYPE_DISTORTION:
		return &m_hullDatas[n]->distortionAreas;
	default:
		return nullptr;
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a shader parameter for a faction override
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetMeshAreaParameter( EveSOFDataArea::AreaType areaType, const BlueSharedString& parameterName, const std::map<BlueSharedString, Vector4>* hullParameters, unsigned int blockededMaterials ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// do we have a dna mesh command for this?
	std::vector<std::string> commandArgs;
	if( GetDnaCommandArgs( CMD_MATERIAL, commandArgs ) )
	{
		// indentify material paramater and material index
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		if( param.IsMaterialIdxValid() && ( param.GetMaterialIdx() < (int32_t)commandArgs.size() ) )
		{
			// some materials are not flagged as blocked for overrides
			if( !( blockededMaterials & ( 1 << param.GetMaterialIdx() ) ) )
			{
				// get the material from the lib
				const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, commandArgs[param.GetMaterialIdx()].c_str(), &param );
				if( res )
				{
					return res;
				}
			}
		}
	}

	// do we have a dna pattern command for this?
	if( GetDnaCommandArgs( CMD_PATTERN, commandArgs ) )
	{
		// indentify material paramater and material index
		EveSOFUtilsParameterName param( m_genericData->patternMaterialPrefixes, parameterName.c_str() );
		if( param.IsMaterialIdxValid() && ( 1 + param.GetMaterialIdx() < (int32_t)commandArgs.size() ) )
		{
			// get the material from the lib
			const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, commandArgs[1 + param.GetMaterialIdx()].c_str(), &param );
			if( res )
			{
				return res;
			}
		}
	}

	// is this a pattern parameter?
	{
		EveSOFUtilsParameterName param( m_genericData->patternMaterialPrefixes, parameterName.c_str() );
		if( param.IsMaterialIdxValid() )
		{
			if( param.GetMaterialIdx() == 0)
			{
				// get the material from the lib using the racial name
				const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, m_factionData->defaultPatternLayer1MaterialName.c_str(), &param );
				if( res )
				{
					return res;
				}
			}
			else if( UsingSof6() && param.GetMaterialIdx() == 1 )
			{
				const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, m_factionData->defaultPatternLayer2MaterialName.c_str(), &param );
				if( res )
				{
					return res;
				}
			}
		}
	}

	// do we have it in the generic data?
	if( areaType == EveSOFDataArea::TYPE_WRECK )
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, GetColorSet(), &m_genericData->genericWreckMaterialData, areaType, &param );
		if( res )
		{
			return res;
		}
	}

	// do we have it in the race data?
	if( areaType == EveSOFDataArea::TYPE_PRIMARY || areaType == EveSOFDataArea::TYPE_REACTOR )
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, GetColorSet(), &m_raceData->areaMaterials, areaType, &param );
		if( res )
		{
			return res;
		}
	}

	// do we have it in the faction data?
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, GetColorSet(), &m_factionData->areaMaterials, areaType, &param );
		if( res )
		{
			return res;
		}
	}

	// do we have it in the hull data
	if( hullParameters )
	{
		auto it = hullParameters->find( parameterName );
		if( it != hullParameters->end() )
		{
			return &it->second;
		}

	}

	// nope, nothing found
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a shader parameter for a faction, but this time for a turret
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetFactionTurretParameters( const BlueSharedString& parameterName ) const
{
	// must change the material number in the parameter name, is for turrets
	EveSOFUtilsParameterName paramName( m_genericData->materialPrefixes, parameterName.c_str() );
	// valid material index means we might have to change it?
	if( paramName.IsMaterialIdxValid() )
	{
		// change the material index into the usage index, which in this case is the turret material index
		paramName.ChangeMaterialIdx( m_genericData, m_factionData->materialUsageList[paramName.GetMaterialIdx()] );
	}
	// now use this parameter name to get the actual value
	return GetMeshAreaParameter( EveSOFDataArea::TYPE_PRIMARY, BlueSharedString( paramName.GetFullName() ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the racial part of booster data
// --------------------------------------------------------------------------------
const EveSOFDataMgr::RaceBoosterData* EveSOFDNA::GetRaceBoosterData() const
{
	return &m_raceData->boosters;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the racial part of visual damage effects
// --------------------------------------------------------------------------------
const EveSOFDataMgr::RaceDamageData* EveSOFDNA::GetRaceDamageData() const
{
	return &m_raceData->damage;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the impact effect type for this hull
// --------------------------------------------------------------------------------
EveSOFDataHull::ImpactEffectType EveSOFDNA::GetImpactEffectType() const
{
	return m_hullDatas[0]->impactEffectType;
}

// --------------------------------------------------------------------------------
// Description:
//   What shader to use for the shield?
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetImpactShieldShader() const
{
	const EveSOFDataMgr::GenericDamageData* data = GetGenericDamageData();
	if( data )
	{
		switch( m_hullDatas[0]->impactEffectType )
		{
		case EveSOFDataHull::IMPACTEFFECT_NONE:
			return nullptr;
		case EveSOFDataHull::IMPACTEFFECT_ELLIPSOID:
			return data->shieldShaderEllipsoid.c_str();
		case EveSOFDataHull::IMPACTEFFECT_HULL:
			return data->shieldShaderHull.c_str();
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the hull part of booster data
// --------------------------------------------------------------------------------
const EveSOFDataMgr::HullBoosterData* EveSOFDNA::GetHullBoosterData( size_t n ) const
{
	return &m_hullDatas[n]->boosters;
}

// --------------------------------------------------------------------------------
size_t EveSOFDNA::GetHullBoosterCount() const
{
	size_t cntr = 0;
	for( auto it = m_hullDatas.begin(); it != m_hullDatas.end(); ++it )
	{
		cntr += ( *it )->boosters.items.size();
	}
	return cntr;
}

// --------------------------------------------------------------------------------
const Color* EveSOFDNA::GetColorSet() const
{
	return m_factionData->colorData.colors;
}

// --------------------------------------------------------------------------------
const EveSOFDataMgr::LogoData* EveSOFDNA::GetLogo( size_t index ) const
{
	return &m_factionData->logoSetData.logos[index];
}

// --------------------------------------------------------------------------------
const bool EveSOFDNA::HasLogoSet( size_t index ) const
{
	const EveSOFDataMgr::LogoData* logo = GetLogo( index );
	return logo->textures.size() > 0;
}

// --------------------------------------------------------------------------------
bool EveSOFDNA::IsInVisibilityData( uint32_t h ) const
{
	return ( m_factionData->visibilityData.count( h ) != 0 );
}

// --------------------------------------------------------------------------------
// Description:
//   Return the number of layers of this pattern
// --------------------------------------------------------------------------------
size_t EveSOFDNA::GetPatternLayerCount() const
{
	// do we have a dna command for a pattern?
	if( m_patternData )
	{
		// TODO - PHASE-6
		auto applicationData = GetHullPatternApplicationData();
		if( nullptr == applicationData )
		{
			return 0;
		}
		return applicationData->layerAndProjection.size();
	}

	// ok lets check for default patterns
	if( UsingSof6() )
	{
		auto applicationData = GetFactionalPatternApplicationData();
		if( nullptr != applicationData)
		{
			return applicationData->layerAndProjection.size();	
		}
		// could not find the hull in the pattern or couldn't find the pattern, so just fall out of this function
	}
	else if( m_hullDatas[0]->defaultPattern.enabled )
	{
		// there should be a default pattern!
		return 1;
	}
	return 0;
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern application data for the provided faction! (if available)
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternApplicationData* EveSOFDNA::GetFactionalPatternApplicationData() const
{
	// get the pattern info directly from the faction
	auto patternData = m_dataMgr->GetPatternData( m_factionData->defaultPatternName.c_str() );
	if( patternData )
	{
		auto finder = patternData->applicationData.find( BlueSharedString( m_hullNames[0] ) );
		if( finder != patternData->applicationData.end() )
		{
			return &finder->second;
		}
	}
	return nullptr;
}


// --------------------------------------------------------------------------------
// Description:
//   Return pattern application data for the provided hull! (if available)
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternApplicationData* EveSOFDNA::GetHullPatternApplicationData() const
{
	// TODO PHASE-6
	if( UsingSof6() )
	{
		auto finder = m_patternData->applicationData.find( BlueSharedString( m_hullNames[0] ) );
		if( finder != m_patternData->applicationData.end() )
		{
			return &finder->second;
		}
	}
	else
	{
		auto finder = m_patternData->old_applicationData.find( BlueSharedString( m_hullNames[0] ) );
		if( finder != m_patternData->old_applicationData.end() )
		{
			return &finder->second;
		}
	}
	
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern application data for the provided hull or faction! (if available)
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternApplicationData* EveSOFDNA::GetPatternApplicationData( bool& theCallerNeedsToDeleteTheResultBecauseIAmBroken ) const
{
    theCallerNeedsToDeleteTheResultBecauseIAmBroken = false;
	if( !HasDnaCommand( CMD_PATTERN ) )
	{
		if( UsingSof6() )
		{
			return GetFactionalPatternApplicationData();
		}
		else {
			// ok no DNA command for a pattern, so we use the default from the hull
			auto patternTransform = &m_hullDatas[0]->defaultPattern;
			auto patternLayer = &m_factionData->defaultPatternInfo;

			EveSOFDataMgr::PatternApplicationData* application = new EveSOFDataMgr::PatternApplicationData();
			application->layerAndProjection.push_back( std::make_pair( *patternLayer, *patternTransform ) );
            theCallerNeedsToDeleteTheResultBecauseIAmBroken = true;
			return application;
		}
	}

	return GetHullPatternApplicationData();
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern data, but needs to exist for provided hull! (if available)
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternProjectionData* EveSOFDNA::GetPatternProjectionData( const EveSOFDataMgr::PatternApplicationData* patternApplicationData, size_t layer ) const
{
	if( nullptr == patternApplicationData )
	{
		return nullptr;
	}
	if( !HasDnaCommand( CMD_PATTERN ) && !UsingSof6() )
	{
		if( layer > 0 )
		{
			// only 1 layer for default patterns, might be increased to 2
			return nullptr;
		}
		// ok no DNA command for a pattern, so we use the default from the hull
		return &patternApplicationData->layerAndProjection[0].second;
	}

	if( patternApplicationData->layerAndProjection.size() < layer )
	{
		return nullptr;
	}
	return &patternApplicationData->layerAndProjection[layer].second;
}


// --------------------------------------------------------------------------------
// Description:
//   Return pattern layer data, but needs to exist for provided hull!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternLayerData* EveSOFDNA::GetPatternLayerData( const EveSOFDataMgr::PatternApplicationData* patternApplicationData, size_t layer ) const 
{
	if( nullptr == patternApplicationData )
	{
		return nullptr;
	}
	if( !HasDnaCommand( CMD_PATTERN ) && !UsingSof6() )
	{
		if( layer > 0 )
		{
			// only 1 layer for default patterns, might be increased to 2
			return nullptr;
		}
		// ok no DNA command for a pattern, so we use the default from the hull
		return &patternApplicationData->layerAndProjection[0].first;
	}

	if( patternApplicationData->layerAndProjection.size() < layer )
	{
		return nullptr;
	}
	return &patternApplicationData->layerAndProjection[layer].first;
}

// --------------------------------------------------------------------------------
// Description:
//   Return applicable material targets for the pattern
// --------------------------------------------------------------------------------
const Vector4 EveSOFDNA::GetMaterialTargets( const EveSOFDataMgr::PatternLayerData* layerData ) const
{
	return layerData->materialTargets;	
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern data
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsPatternLayerApplicableToArea( const EveSOFDataMgr::PatternLayerData* layerData, EveSOFDataArea::AreaType areaType ) const
{			
	if( nullptr == layerData )
	{
		return false;
	}
	auto finder = layerData->applicableAreas.find( areaType );
	return ( finder != layerData->applicableAreas.end() ) && finder->second;
}


// --------------------------------------------------------------------------------
// Description:
//   Return how many layouts are in specified in the dna
// --------------------------------------------------------------------------------
size_t EveSOFDNA::GetLayoutCount() const
{
	return m_layoutData.size();
}

// --------------------------------------------------------------------------------
// Description:
//   Return layout data
// --------------------------------------------------------------------------------
const EveSOFDataMgr::LayoutData* EveSOFDNA::GetLayoutData( size_t layoutIndex ) const
{
	return m_layoutData[layoutIndex];
}

// --------------------------------------------------------------------------------
// Description:
//   Return locator distribution for layout placement
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorDirectionData>* EveSOFDNA::GetPlacementLocators( size_t hullIndex, BlueSharedString& locatorSetName ) const
{
	auto hull = m_hullDatas[hullIndex];
	auto locators = hull->locatorSets.find( locatorSetName );
	if( locators == hull->locatorSets.end())
	{
		return nullptr;
	}
	return &locators->second;
}

// --------------------------------------------------------------------------------
// Description:
//   Sometimes it is good to know how high an ID of a mesharea can go!
// --------------------------------------------------------------------------------
unsigned int EveSOFDNA::GetHighestMeshAreaIndex( TriBatchType areaType, size_t n ) const
{
	unsigned int cntr = 0;
	if( areaType == TRIBATCHTYPE_OPAQUE )
	{
		for( auto it = m_hullDatas[n]->opaqueAreas.cbegin(); it != m_hullDatas[n]->opaqueAreas.cend(); ++it )
		{
			if( it->index > cntr )
			{
				cntr = it->index;
			}
		}
	}
	return cntr;
}

// --------------------------------------------------------------------------------
// Description:
//   Just try to find a command
// --------------------------------------------------------------------------------
bool EveSOFDNA::HasDnaCommand( DnaCommand cmd ) const
{
	// try to find it!
	return ( m_commands.find( s_dnaCommands[cmd] ) != m_commands.end() );
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find a command in our cmd list and return it's arguments
// --------------------------------------------------------------------------------
bool EveSOFDNA::GetDnaCommandArgs( DnaCommand cmd, std::vector<std::string>& args ) const
{
	// try to find it!
	auto commandIt = m_commands.find( s_dnaCommands[ cmd ] );
	if( commandIt == m_commands.end() )
	{
		return false;
	}

	// just copy the args
	args = commandIt->second;

	return true;
}


// --------------------------------------------------------------------------------
// Description:
//	Check if we are using experimental features
// --------------------------------------------------------------------------------
bool EveSOFDNA::UsingSof6() const
{
	return ( !m_hullDatas.empty() && m_hullDatas[0]->sof6 );
}

BlueSharedString EveSOFDNA::GetFactionName() const
{
	return BlueSharedString(m_factionName);
}

BlueSharedString EveSOFDNA::GetRaceName() const
{
	return BlueSharedString(m_raceName);
}

EntityComponents::ReflectionMode EveSOFDNA::GetReflectionMode() const
{
	std::string category = m_hullDatas[0]->category;
	
	// Warning non future proofed code ahead!
	// this should come from the category (as in the category being an object that has information about features)
	if( category == "hangar" || category == "hangar4k" )
	{
		return EntityComponents::REFLECT_LOW_MEDIUM_HIGH;
	}
	if( category.find( "station" ) != std::string::npos || category.find( "structure" ) != std::string::npos )
	{
		return EntityComponents::REFLECT_MEDIUM_AND_HIGH;
	}
	if( category == "jumpgate" )
	{
		return EntityComponents::REFLECT_MEDIUM_AND_HIGH;
	}
	return EntityComponents::REFLECT_NEVER;

}
