////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
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
};

// build classes
static std::string s_dnaClasses[] = {
	"ship",					// BUILDCLASS_SHIP
	"mobile",				// BUILDCLASS_MOBILE
	"stationary",			// BUILDCLASS_STATIONARY
	"swarm",				// BUILDCLASS_SWARM
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
	m_genericData( nullptr ),
	m_customHullData()
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

	// generics
	m_genericData = m_dataMgr->GetGenericData();

	// some dna commands require a custom block data
	if( HasDnaCommand( CMD_VARIANT ) )
	{
		SetupCustomData();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return the dna string as a whole
// --------------------------------------------------------------------------------
void EveSOFDNA::SetupCustomData()
{
	// ok we need custom hull data, but set up basic stuff here
	m_customHullData.buildClass = m_hullDatas[0]->buildClass;
	m_customHullData.geometryResFilePath = m_hullDatas[0]->geometryResFilePath;
	m_customHullData.boundingSphere = m_hullDatas[0]->boundingSphere;
	m_customHullData.shapeEllipsoidCenter = m_hullDatas[0]->shapeEllipsoidCenter;
	m_customHullData.shapeEllipsoidRadius = m_hullDatas[0]->shapeEllipsoidRadius;
	m_customHullData.isSkinned = m_hullDatas[0]->isSkinned;
	m_customHullData.enableDynamicBoundingSphere = m_hullDatas[0]->enableDynamicBoundingSphere;
	m_customHullData.audioPosition = m_hullDatas[0]->audioPosition;

	// do we have a dna variant command for this?
	std::vector<std::string> variantCommandArgs;
	if( GetDnaCommandArgs( CMD_VARIANT, variantCommandArgs ) )
	{
		// do we have this variant's data?
		auto finder = m_genericData->variants.find( BlueSharedString( variantCommandArgs[0] ) );
		if( finder != m_genericData->variants.end() )
		{
			// what area?
			std::vector<EveSOFDataMgr::HullAreas>* targetArea = &m_customHullData.opaqueAreas;
			if( finder->second.isTransparent )
			{
				targetArea = &m_customHullData.transparentAreas;
			}
			// for now only use a single additive area
			for( auto it = m_hullDatas[0]->opaqueAreas.begin(); it != m_hullDatas[0]->opaqueAreas.end(); ++it )
			{
				targetArea->push_back( finder->second.hullAreaData );
				targetArea->back().index = it->index;
				targetArea->back().count = it->count;
			}
		}
	}

	// adjust pointer
	m_hullDatas[0] = &m_customHullData;
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
//   Return the generic textures for a given area shader
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
//   Return the generic textures for a given decal shader
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericShaderData* EveSOFDNA::GetGenericDecalShaderData( const BlueSharedString& shaderName ) const
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
// Description:
//   Return the factional group-decal-data by a given groupindex
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionDecalData* EveSOFDNA::GetFactionDecalData( int groupIndex ) const
{
	auto finder = m_factionData->decalData.find( groupIndex );
	if( finder != m_factionData->decalData.end() )
	{
		return &finder->second;
	}
	return nullptr;
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
//   Return the factional group-spriteset-data by a given groupindex
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionSpriteSetColorData* EveSOFDNA::GetFactionSpriteSetData( int groupIndex ) const
{
	// -1 is null
	if( groupIndex != -1 )
	{
		auto finder = m_factionData->spriteSetsColor.find( groupIndex );
		if( finder != m_factionData->spriteSetsColor.end() )
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
//   Return an array to all the children of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullChild>& EveSOFDNA::GetHullChildren() const
{
	return m_hullDatas[0]->children;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the instanced meshes of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullInstancedMesh>& EveSOFDNA::GetHullInstancedMeshes( size_t n ) const
{
	return m_hullDatas[n]->instancedMeshes;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array of all the animations for children
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAnimation>& EveSOFDNA::GetHullAnimations( size_t n ) const
{
	return m_hullDatas[n]->animations;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array of all the decals of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullDecalData>& EveSOFDNA::GetHullDecals( size_t n ) const
{
	return m_hullDatas[n]->hullDecals;
}

// --------------------------------------------------------------------------------
// Description:
//   Changes the provided texture resource path, maybe modified depending on dna
// --------------------------------------------------------------------------------
void EveSOFDNA::ModifyTextureResPath( std::string& resPath, const char* resName ) const
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
		// hardcoded texture param names which'll get an override
		if( !strcmp( resName, "MaterialMap" ) || !strcmp( resName, "PaintMaskMap" ) || !strcmp( resName, "PmdgMap" ) )
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
			if( StringInsertStubBefore( resPathCopy, "_", insertStr.c_str() ) && FileExists( resPathCopy ) )
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
	std::vector<std::string> commandArgs;
	if( GetDnaCommandArgs( CMD_MESH, commandArgs ) && !commandArgs.empty() )
	{
		// need to put the res: hard-coded
		return "res:" + commandArgs[0];
	}
	return m_hullDatas[0]->geometryResFilePath;
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
//   Return a pointer to the bounding sphere info of this hull
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetHullBoundingSphere( size_t n ) const
{
	return &m_hullDatas[n]->boundingSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the center of this hull's shape ellipsoid
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullShapeEllipsoidCenter( size_t n ) const
{
	return &m_hullDatas[n]->shapeEllipsoidCenter;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the radiuses of this hull's shape ellipsoid
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullShapeEllipsoidRadius( size_t n ) const
{
	return &m_hullDatas[n]->shapeEllipsoidRadius;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the audio position of this hull
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullAudioPosition( size_t n ) const
{
	return &m_hullDatas[n]->audioPosition;
}

// --------------------------------------------------------------------------------
// Description:
//   Is this hull animated/skinned?
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsHullAnimated() const
{
	return m_hullDatas[0]->isSkinned;
}

// --------------------------------------------------------------------------------
// Description:
//   Does this hull use dynamic bounding spheres?
// --------------------------------------------------------------------------------
bool EveSOFDNA::DynamicBoundingSphereEnabled() const
{
	return m_hullDatas[0]->enableDynamicBoundingSphere;
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
	case TRIBATCHTYPE_DEPTH:
		return &m_hullDatas[n]->depthAreas;
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
			// get the material from the lib using the racial name
			const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, m_factionData->defaultPatternLayer1MaterialName.c_str(), &param );
			if( res )
			{
				return res;
			}
		}
	}

	// do we have it in the generic data?
	if( areaType == EveSOFDataArea::TYPE_WRECK )
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, &m_genericData->genericWreckMaterialData, areaType, &param );
		if( res )
		{
			return res;
		}
	}

	// do we have it in the race data?
	if( areaType == EveSOFDataArea::TYPE_PRIMARY || areaType == EveSOFDataArea::TYPE_REACTOR )
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, &m_raceData->areaMaterials, areaType, &param );
		if( res )
		{
			return res;
		}
	}

	// do we have it in the faction data?
	{
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		const Vector4* res = EveSOFUtils::SearchForParameterData( m_dataMgr, &m_factionData->areaMaterials, areaType, &param );
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
// Description:
//   Return the number of layers of this pattern
// --------------------------------------------------------------------------------
size_t EveSOFDNA::GetPatternLayerCount() const
{
	// do we have a dna command for a pattern?
	if( m_patternData )
	{
		return m_patternData->layerData.size();
	}
	// there should be a default pattern!
	if( m_hullDatas[0]->defaultPattern.enabled )
	{
		return 1;
	}
	return 0;
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern data, but needs to exist for provided hull!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternProjectionData* EveSOFDNA::GetPatternProjectionData( size_t layer ) const
{
	if( !HasDnaCommand( CMD_PATTERN ) )
	{
		// ok no DNA command for a pattern, so we use the default from the hull
		return &m_hullDatas[0]->defaultPattern;
	}
	auto finder = m_patternData->projectionData.find( BlueSharedString( m_hullNames[0] ) );
	if( finder == m_patternData->projectionData.end() )
	{
		return nullptr;
	}
	if( finder->second.size() < layer )
	{
		return nullptr;
	}
	return &finder->second[ layer ];
}

// --------------------------------------------------------------------------------
// Description:
//   Return pattern data
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternLayerData* EveSOFDNA::GetPatternLayerData( size_t layer ) const
{
	if( !HasDnaCommand( CMD_PATTERN ) )
	{
		// ok no DNA command for a pattern, so we use the default from the race
		return &m_factionData->defaultPattern;
	}
	if( m_patternData->layerData.size() < layer )
	{
		return nullptr;
	}
	return &m_patternData->layerData[layer];
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







