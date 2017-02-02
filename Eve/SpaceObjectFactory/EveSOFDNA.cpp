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
	m_hullData( nullptr ),
	m_factionData( nullptr ),
	m_raceData( nullptr ),
	m_patternData( nullptr )
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
	// need all three basic parts
	if( !m_hullData || !m_factionData || !m_raceData )
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
		case CMD_MESH:
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

	// the 3 main dna names
	m_hullName = dnaParts[0];
	m_factionName = dnaParts[1];
	m_raceName = dnaParts[2];

	// make sure we find this hull
	m_hullData = m_dataMgr->GetHullData( m_hullName.c_str() );
	if( m_hullData == nullptr )
	{
		CCP_LOGERR( "Couldn't find the requested hull: %s", dnaString );
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
	m_customHullData.buildClass = m_hullData->buildClass;
	m_customHullData.geometryResFilePath = m_hullData->geometryResFilePath;
	m_customHullData.boundingSphere = m_hullData->boundingSphere;
	m_customHullData.shapeEllipsoidCenter = m_hullData->shapeEllipsoidCenter;
	m_customHullData.shapeEllipsoidRadius = m_hullData->shapeEllipsoidRadius;
	m_customHullData.isSkinned = m_hullData->isSkinned;
	m_customHullData.enableDynamicBoundingSphere = m_hullData->enableDynamicBoundingSphere;
	m_customHullData.audioPosition = m_hullData->audioPosition;

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
			for( auto it = m_hullData->opaqueAreas.begin(); it != m_hullData->opaqueAreas.end(); ++it )
			{
				targetArea->push_back( finder->second.hullAreaData );
				targetArea->back().index = it->index;
				targetArea->back().count = it->count;
			}
		}
	}

	// adjust pointer
	m_hullData = &m_customHullData;
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
	// -1 is null
	if( groupIndex != -1 )
	{
		auto finder = m_factionData->decalData.find( groupIndex );
		if( finder != m_factionData->decalData.end() )
		{
			return &finder->second;
		}
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
const std::vector<EveSOFDataMgr::HullPlaneSetData>& EveSOFDNA::GetHullPlaneSets() const
{
	return m_hullData->planeSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spotlightsets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpotlightSetData>& EveSOFDNA::GetHullSpotlightSets() const
{
	return m_hullData->spotlightSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spritesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpriteSetData>& EveSOFDNA::GetHullSpriteSets() const
{
	return m_hullData->spriteSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the spritelinesets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpriteLineSetData>& EveSOFDNA::GetHullSpriteLineSets() const
{
	return m_hullData->spriteLineSets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the turret locators on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorData>& EveSOFDNA::GetHullTurretLocators() const
{
	return m_hullData->locatorTurrets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the damage locators on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorDirectionData>* EveSOFDNA::GetHullLocators( const char* setName ) const
{
	auto locatorSet = m_hullData->locatorSets.find( BlueSharedString( setName ) );
	if( locatorSet == m_hullData->locatorSets.end() )
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
	return m_hullData->children;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array to all the instanced meshes of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullInstancedMesh>& EveSOFDNA::GetHullInstancedMeshes() const
{
	return m_hullData->instancedMeshes;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array of all the animations for children
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAnimation>& EveSOFDNA::GetHullAnimations() const
{
	return m_hullData->animations;
}

// --------------------------------------------------------------------------------
// Description:
//   Return an array of all the decals of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullDecalData>& EveSOFDNA::GetHullDecals() const
{
	return m_hullData->hullDecals;
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
const char* EveSOFDNA::GetHullGeometryResPath() const
{
	return m_hullData->geometryResFilePath.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the redfile path to the model's rotation curve
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetModelRotationCurvePath() const
{
	if( m_hullData->modelRotationCurvePath.empty() )
	{
		return nullptr;
	}
	return m_hullData->modelRotationCurvePath.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the redfile path to the model's translation curve
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetModelTranslationCurvePath() const
{
	if( m_hullData->modelTranslationCurvePath.empty() )
	{
		return nullptr;
	}
	return m_hullData->modelTranslationCurvePath.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the exact hull name from the object's dna
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetHullName() const
{
	return m_hullName.c_str();
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
	return m_hullData->buildClass;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the bounding sphere info of this hull
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetHullBoundingSphere() const
{
	return &m_hullData->boundingSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the center of this hull's shape ellipsoid
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullShapeEllipsoidCenter() const
{
	return &m_hullData->shapeEllipsoidCenter;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the radiuses of this hull's shape ellipsoid
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullShapeEllipsoidRadius() const
{
	return &m_hullData->shapeEllipsoidRadius;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the audio position of this hull
// --------------------------------------------------------------------------------
const Vector3* EveSOFDNA::GetHullAudioPosition() const
{
	return &m_hullData->audioPosition;
}

// --------------------------------------------------------------------------------
// Description:
//   Is this hull animated/skinned?
// --------------------------------------------------------------------------------
bool EveSOFDNA::IsHullAnimated() const
{
	return m_hullData->isSkinned;
}

// --------------------------------------------------------------------------------
// Description:
//   Does this hull use dynamic bounding spheres?
// --------------------------------------------------------------------------------
bool EveSOFDNA::DynamicBoundingSphereEnabled() const
{
	return m_hullData->enableDynamicBoundingSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Get a list of all the hull's meshareas given the type
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAreas>* EveSOFDNA::GetHullMeshAreas( TriBatchType type ) const
{
	switch( type )
	{
	case TRIBATCHTYPE_OPAQUE:
		return &m_hullData->opaqueAreas;
	case TRIBATCHTYPE_DECAL:
		return &m_hullData->decalAreas;
	case TRIBATCHTYPE_TRANSPARENT:
		return &m_hullData->transparentAreas;
	case TRIBATCHTYPE_ADDITIVE:
		return &m_hullData->additiveAreas;
	case TRIBATCHTYPE_DEPTH:
		return &m_hullData->depthAreas;
	case TRIBATCHTYPE_DISTORTION:
		return &m_hullData->distortionAreas;
	default:
		return nullptr;
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Search a material for parameter value
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::SearchForParameterData( const char* materialName, const EveSOFUtilsParameterName* parameterName ) const
{
	const EveSOFDataMgr::MaterialData* materialData = m_dataMgr->GetMaterialData( materialName );
	if( materialData )
	{
		BlueSharedString pn( parameterName->GetShortName() );
		auto parameterIt = materialData->parameters.find( pn );
		if( parameterIt != materialData->parameters.end() )
		{
			return &parameterIt->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::SearchForParameterData( const EveSOFDataMgr::AreaMaterialData* areaMaterialData, const EveSOFUtilsParameterName* parameterName ) const
{
	if( parameterName->IsMaterialIdxValid() )
	{
		return SearchForParameterData( areaMaterialData->material[parameterName->GetMaterialIdx()].c_str(), parameterName );
	}
	else
	{
		auto parameterIt = areaMaterialData->generalParameters.find( parameterName->GetFullName() );
		if( parameterIt != areaMaterialData->generalParameters.end() )
		{
			return &parameterIt->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Search an area collection to find the data of a specific parameter
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::SearchForParameterData( const std::map<BlueSharedString, EveSOFDataMgr::FactionAreaData>& areas, EveSOFDataArea::AreaType areaType, const BlueSharedString& parameterName ) const
{
	// temporary binding areaType enum to string, removed soon!
	BlueSharedString areaDesignation( "empty" );
	switch(areaType)
	{
	case EveSOFDataArea::TYPE_PRIMARY:
		areaDesignation = BlueSharedString( "hull" );
		break;
	case EveSOFDataArea::TYPE_GLASS:
		areaDesignation = BlueSharedString( "glass" );
		break;
	case EveSOFDataArea::TYPE_SAILS:
		areaDesignation = BlueSharedString( "sails" );
		break;
	case EveSOFDataArea::TYPE_REACTOR:
		areaDesignation = BlueSharedString( "reactor" );
		break;
	case EveSOFDataArea::TYPE_DARKHULL:
		areaDesignation = BlueSharedString( "darkhull" );
		break;
	case EveSOFDataArea::TYPE_WRECK:
		areaDesignation = BlueSharedString( "generic_wreck" );
		break;
	case EveSOFDataArea::TYPE_NO_OVERWRITE:
		return nullptr;
	}


	// try to find the specified hull
	auto parameterListIt = areas.find( areaDesignation );
	if( parameterListIt == areas.end() )
	{
		return nullptr;
	}

	// try to find the parameter
	const std::map<BlueSharedString, Vector4>* parameters = &parameterListIt->second.parameters;
	auto parameterIt = parameters->find( parameterName );
	if( parameterIt == parameters->end() )
	{
		return nullptr;
	}

	// found it!
	return &parameterIt->second;
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
	if( GetDnaCommandArgs( CMD_MESH, commandArgs ) )
	{
		// indentify material paramater and material index
		EveSOFUtilsParameterName param( m_genericData->materialPrefixes, parameterName.c_str() );
		if( param.IsMaterialIdxValid() && ( param.GetMaterialIdx() < (int32_t)commandArgs.size() ) )
		{
			// some materials are not flagged as blocked for overrides
			if( !( blockededMaterials & ( 1 << param.GetMaterialIdx() ) ) )
			{
				// get the material from the lib
				const Vector4* res = SearchForParameterData( commandArgs[param.GetMaterialIdx()].c_str(), &param );
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
			const Vector4* res = SearchForParameterData( commandArgs[1 + param.GetMaterialIdx()].c_str(), &param );
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
			const Vector4* res = SearchForParameterData( m_factionData->defaultPatternLayer1MaterialName.c_str(), &param );
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
		return SearchForParameterData( &m_genericData->genericWreckMaterialData, &param );
	}

	// do we have it in the race data?
	const Vector4* res = SearchForParameterData( m_raceData->hullAreaParameters, areaType, parameterName );
	if( res )
	{
		return res;
	}

	// do we have it in the faction data?
	res = SearchForParameterData( m_factionData->areaParameters, areaType, parameterName );
	if( res )
	{
		return res;
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
	return m_hullData->impactEffectType;
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
		switch( m_hullData->impactEffectType )
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
const EveSOFDataMgr::HullBoosterData* EveSOFDNA::GetHullBoosterData() const
{
	return &m_hullData->boosters;
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
	if( m_hullData->defaultPattern.enabled )
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
		return &m_hullData->defaultPattern;
	}
	auto finder = m_patternData->projectionData.find( BlueSharedString( m_hullName ) );
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
unsigned int EveSOFDNA::GetHighestMeshAreaIndex( TriBatchType areaType ) const
{
	unsigned int cntr = 0;
	if( areaType == TRIBATCHTYPE_OPAQUE )
	{
		for( auto it = m_hullData->opaqueAreas.cbegin(); it != m_hullData->opaqueAreas.cend(); ++it )
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







