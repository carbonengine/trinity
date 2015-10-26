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
			// number of arguments must be number of materials
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

	// rember dna string
	m_dna = dnaString;

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

	// additional dna subparts
	for( size_t dnaSubpart = 3; dnaSubpart < dnaParts.size(); ++dnaSubpart )
	{
		// split into command and args
		std::vector<std::string> cmdAndArgs;
		StringSplit( cmdAndArgs, dnaParts[ dnaSubpart ].c_str(), s_dnaSeperatorArg );
		if( cmdAndArgs.size() != 2 )
		{
			CCP_LOGERR( "Invalid SOF DNA, incorrect command and args: %s", dnaString );
			return;
		}

		// get commands
		std::vector<std::string> commandList;
		StringSplit( commandList, cmdAndArgs[1].c_str(), s_dnaSeperatorList );

		// put into map, warning: this might overwrite a similar command!
		m_commands[cmdAndArgs[0]] = commandList;
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
	// generics
	m_genericData = m_dataMgr->GetGenericData();
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
//   Return an array to all the damage locators on this hull
// --------------------------------------------------------------------------------
const std::vector<EveSOFDataMgr::LocatorDirectionData>& EveSOFDNA::GetHullDamageLocators() const
{
	return m_hullData->locatorDamage;
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
		if( !strcmp( resName, "MaterialMap" ) || !strcmp( resName, "PmdgMap" ) )
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
//   Return the build class for this hull
// --------------------------------------------------------------------------------
EveSOFDataHull::BuildClass EveSOFDNA::GetBuildClass() const
{
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
//   Search and area collection to find the data of a specific parameter
// --------------------------------------------------------------------------------
const Vector4* EveSOFDNA::SearchForParameterData( const std::map<BlueSharedString, EveSOFDataMgr::FactionAreaData>& areas, const BlueSharedString& areaDesignation, const BlueSharedString& parameterName ) const
{
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
const Vector4* EveSOFDNA::GetMeshAreaParameter( const BlueSharedString& areaDesignation, const BlueSharedString& parameterName, const std::map<BlueSharedString, Vector4>* hullParameters, unsigned int blockededMaterials ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// do we have a dna mesh command for this?
	std::vector<std::string> meshCommandArgs;
	if( GetDnaCommandArgs( CMD_MESH, meshCommandArgs ) )
	{
		// indentify material paramater and material index
		EveSOFUtilsParameterName param( m_genericData, parameterName.c_str() );
		if( param.IsValid() && ( param.GetMaterialIdx() < (int32_t)meshCommandArgs.size() ) )
		{
			// some materials are not flagged as blocked for overrides
			if( !( blockededMaterials & ( 1 << param.GetMaterialIdx() ) ) )
			{
				// get the material from the lib
				const EveSOFDataMgr::MaterialData* materialData = m_dataMgr->GetMaterialData( meshCommandArgs[ param.GetMaterialIdx() ].c_str() );
				if( materialData ) 
				{
					BlueSharedString pn( param.GetShortName() );
					auto parameterIt = materialData->parameters.find( pn );
					if( parameterIt != materialData->parameters.end() )
					{
						return &parameterIt->second;
					}
				}
			}
		}
	}

	// do we have it in the generic data?
	const Vector4* res = SearchForParameterData( m_genericData->hullAreaParameters, areaDesignation, parameterName );
	if( res )
	{
		return res;
	}

	// do we have it in the race data?
	res = SearchForParameterData( m_raceData->hullAreaParameters, areaDesignation, parameterName );
	if( res )
	{
		return res;
	}

	// do we have it in the faction data?
	res = SearchForParameterData( m_factionData->areaParameters, areaDesignation, parameterName );
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
	EveSOFUtilsParameterName paramName( m_genericData, parameterName.c_str() );
	// valid?
	if( !paramName.IsValid() )
	{
		return nullptr;
	}
	// change the material index into the usage index, which in this case is the turret material index
	int turretMaterialIdx = m_factionData->materialUsageList[ paramName.GetMaterialIdx() ];
	if( ( turretMaterialIdx < 0 ) || ( turretMaterialIdx >= int(m_genericData->materialPrefixes.size()) ) )
	{
		return nullptr;
	}
	// generate new material name with this index
	std::string turretParamName = paramName.ChangeMaterialIdx( m_genericData, turretMaterialIdx );

	// now use this parameter name to get the actual value
	return GetMeshAreaParameter( BlueSharedString( "hull" ), BlueSharedString( turretParamName ) );
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
//   Return the res path to the impact effect for this race
// --------------------------------------------------------------------------------
const char* EveSOFDNA::GetImpactEffectResPath() const
{
	// enabled?
	if( !m_hullData->hasImpactEffect )
	{
		return nullptr;
	}

	// set?
	if( m_raceData->impactEffectResPath.empty() )
	{
		return nullptr;
	}

	return m_raceData->impactEffectResPath.c_str();
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







