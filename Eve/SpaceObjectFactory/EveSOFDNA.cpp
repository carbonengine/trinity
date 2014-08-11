////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
#include "EveSOFDNA.h"

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
};

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOFDNA::EveSOFDNA( IRoot* lockobj ) :
	m_hullData( nullptr ),
	m_factionData( nullptr ),
	m_raceData( nullptr )
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
bool EveSOFDNA::isValid() const
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
//   Set a complete dna string to this dna
// --------------------------------------------------------------------------------
void EveSOFDNA::Setup( const char* dnaString, EveSOFDataMgrPtr dataMgr )
{
	// remember the pointer to the BIG lib as long as this DNA object lives
	m_dataMgr = dataMgr;

	// split up dna string in all subparts
	std::vector<std::string> dnaParts;
	StringSplit( dnaParts, dnaString, s_dnaSeperatorCmd );

	// need three at least
	if(dnaParts.size() < 3)
	{
		CCP_LOGERR( "Invalid SOF DNA, not enough subparts: %s", dnaString );
		return;
	}

	// names
	m_hullName = dnaParts[0];
	m_factionName = dnaParts[1];
	m_raceName = dnaParts[2];
	// pointers
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

	// additional dna subparts
	for( size_t dnaSubpart = 3; dnaSubpart < dnaParts.size(); ++dnaSubpart )
	{
		// split into command and args
		std::vector<std::string> cmdAndArgs;
		StringSplit( cmdAndArgs, dnaParts[ dnaSubpart ].c_str(), s_dnaSeperatorArg );
		if( cmdAndArgs.size() != 2 )
		{
			CCP_LOGERR( "Invalid SOF DNA, incorrect command and args: %s", dnaString );
			continue;
		}

		// get commands
		std::vector<std::string> commandList;
		StringSplit( commandList, cmdAndArgs[1].c_str(), s_dnaSeperatorList );

		// put into map, warning: this might overwrite a similar command!
		m_commands[cmdAndArgs[0]] = commandList;
	}
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
//   Return an array to all the spotlightsets on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullSpriteSetData>& EveSOFDNA::GetHullSpriteSets() const
{
	return m_hullData->spriteSets;
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
//   Return an array to all the children of this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullChild>& EveSOFDNA::GetHullChildren() const
{
	return m_hullData->children;
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
//   Return the faction's resource path insert string
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetFactionResPathInsert() const
{
	if( m_factionData->resPathInsert.empty() )
	{
		return nullptr;
	}
	return m_factionData->resPathInsert.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Changes the provided texture resource path, maybe modified depending on dna
// --------------------------------------------------------------------------------
void EveSOFDNA::ModifyTextureResPath( std::string& resPath, const char* resName ) const
{
	if( !strcmp( resName, "PgsMap" ) && !m_factionData->resPathInsert.empty() )
	{
		std::string resPathCopy = resPath;

		// insert sub folder
		size_t index = resPath.rfind("/");
		if( index != std::string::npos )
		{
			resPathCopy.insert( index + 1, m_factionData->resPathInsert + "/" );
		}

		// insert part into filename
		std::string insertStr = "_" + m_factionData->resPathInsert;
		if( StringInsertStub( resPathCopy, "_pgs", insertStr.c_str() ) && FileExists( resPathCopy ) )
		{
			resPath = resPathCopy;
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
//   Return a pointer to the bounding sphere info of this hull
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetHullBoundingSphere() const
{
	return &m_hullData->boundingSphere;
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
//   Get a list of all the hull's meshareas given the type
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::HullAreas>* EveSOFDNA::GetHullMeshAreas( TriBatchType type ) const
{
	switch( type )
	{
	case TRIBATCHTYPE_OPAQUE:
		return &m_hullData->opaqueAreas;
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
//   Return a shader parameter for a faction override
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::GetFactionMeshAreaParameters( TriBatchType type, const char* areaDesignation, const char* parameterName ) const
{
	const std::map<std::string, EveSOFDataMgr::FactionAreaData>* areaParameters = nullptr;

	// overrides are organized per areatype
	switch( type )
	{
	case TRIBATCHTYPE_OPAQUE:
		areaParameters = &m_factionData->opaqueAreaParameters;
		break;
	case TRIBATCHTYPE_TRANSPARENT:
		areaParameters = &m_factionData->transparentAreaParameters;
		break;
	default:
		return nullptr;
	}

	// do we have a dna mesh command for this?
	std::vector<std::string> meshCommandArgs;
	if( GetDnaCommandArgs( CMD_MESH, meshCommandArgs ) )
	{
		int argIdx = -1;
		std::string materialDataParameterName = std::string( parameterName );
		// identify mask, submask
		if( StringStartsWithI( parameterName, "Mask" ) )
		{
			// arg 0 is mask material
			argIdx = 0;
			StringRemove( materialDataParameterName, "Mask" );
		}
		else if( StringStartsWithI( parameterName, "SubMask" ) )
		{
			// arg 1 is submask material
			argIdx = 1;
			StringRemove( materialDataParameterName, "SubMask" );
		}

		if( argIdx != -1 )
		{
			// get the material from the lib
			const EveSOFDataMgr::MaterialData* materialData = m_dataMgr->GetMaterialData( meshCommandArgs[ argIdx ].c_str() );
			if( materialData ) 
			{
				auto parameterIt = materialData->parameters.find( materialDataParameterName.c_str() );
				if( parameterIt != materialData->parameters.end() )
				{
					return &parameterIt->second;
				}
			}
		}
	}

	// find the area, by designation string
	auto parameterListIt = areaParameters->find( areaDesignation );
	if( parameterListIt == areaParameters->end() )
	{
		return nullptr;
	}

	// find the parameter by parameter name
	const std::map<std::string, Vector4>* parameters = &parameterListIt->second.parameters;
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
//   Return a pointer to the racial part of booster data
// --------------------------------------------------------------------------------
const EveSOFDataMgr::RaceBoosterData* EveSOFDNA::GetRaceBoosterData() const
{
	return &m_raceData->boosters;
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
//   Return this dna's hull name
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetHullName() const
{
	return m_hullName.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return this dna's faction name
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetFactionName() const
{
	return m_factionName.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return this dna's race name
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetRaceName() const
{
	return m_raceName.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find a command in our cmd list and return it's arguments
// --------------------------------------------------------------------------------
bool EveSOFDNA::GetDnaCommandArgs( DnaCommand cmd, std::vector<std::string>& args ) const
{
	// straight out in mode cases:
	if( m_commands.empty() )
	{
		return false;
	}

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







