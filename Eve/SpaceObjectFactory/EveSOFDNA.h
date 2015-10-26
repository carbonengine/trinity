////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveSOFDNA_H
#define EveSOFDNA_H

#include "EveSOFDataMgr.h"
#include "ITr2Renderable.h"

// forwards
BLUE_DECLARE( EveSOFDNA );

// --------------------------------------------------------------------------------
// Description:
//   This class is for encapsulating the DNA of an SOF ship
// SeeAlso:
//   EveSOF
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOFDNA ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSOFDNA( IRoot* lockobj = NULL );
	~EveSOFDNA();

	// commands from the dna
	enum DnaCommand
	{
		CMD_INVALID = 0,
		CMD_MESH,
		CMD_RESPATHINSERT,
		CMD_MAX
	}; 

	// initialize this dna
	void Setup( const char* dnaString, EveSOFDataMgrPtr dataMgr );

	// is it correctly initialized?
	bool IsValid() const;
	// does it have the correct content?
	bool ValidateContent();

	// get generic data
	const char* GetAreaShaderLocationResPath() const;
	const char* GetDecalShaderLocationResPath() const;
	const char* GetShaderPrefix( bool isAnimated ) const;
	std::string GetCompleteShaderPath( const char* shaderPath ) const;
	const EveSOFDataMgr::GenericShaderData* GetGenericAreaShaderData( const BlueSharedString& shaderName ) const;
	const EveSOFDataMgr::GenericShaderData* GetGenericDecalShaderData( const BlueSharedString& shaderName ) const;

	// get racial data
	const EveSOFDataMgr::RaceBoosterData* GetRaceBoosterData() const;
	const char* GetImpactEffectResPath() const;

	// get hull data
	EveSOFDataHull::BuildClass GetBuildClass() const;
	const Vector4* GetHullBoundingSphere() const;
	const Vector3* GetHullShapeEllipsoidCenter() const;
	const Vector3* GetHullShapeEllipsoidRadius() const;
	bool IsHullAnimated() const;
	const EveSOFDataMgr::HullBoosterData* GetHullBoosterData() const;
	const Vector3* GetHullAudioPosition() const;
	const char* GetHullGeometryResPath() const;
	const char* GetModelRotationCurvePath() const;
	const char* GetModelTranslationCurvePath() const;
	const std::vector<EveSOFDataMgr::HullAreas>* GetHullMeshAreas( TriBatchType type ) const;
	const std::vector<EveSOFDataMgr::HullChild>& GetHullChildren() const;
	const std::vector<EveSOFDataMgr::HullInstancedMesh>& GetHullInstancedMeshes() const;
	const std::vector<EveSOFDataMgr::HullAnimation>& GetHullAnimations() const;
	const std::vector<EveSOFDataMgr::HullDecalData>& GetHullDecals() const;
	const std::vector<EveSOFDataMgr::HullPlaneSetData>& GetHullPlaneSets() const;
	const std::vector<EveSOFDataMgr::HullSpotlightSetData>& GetHullSpotlightSets() const;
	const std::vector<EveSOFDataMgr::HullSpriteSetData>& GetHullSpriteSets() const;
	const std::vector<EveSOFDataMgr::LocatorData>& GetHullTurretLocators() const;
	const std::vector<EveSOFDataMgr::LocatorDirectionData>& GetHullDamageLocators() const;

	// get faction data
	void ModifyTextureResPath( std::string& resPath, const char* resName ) const;
	const Vector4* GetFactionTurretParameters( const BlueSharedString& parameterName ) const;
	const EveSOFDataMgr::FactionDecalData* GetFactionDecalData( int groupIndex ) const;
	const EveSOFDataMgr::FactionPlaneSetColorData* GetFactionPlaneSetData( int groupIndex ) const;
	const EveSOFDataMgr::FactionSpotlightSetColorData* GetFactionSpotlightSetData( int groupIndex ) const;
	const EveSOFDataMgr::FactionSpriteSetColorData* GetFactionSpriteSetData( int groupIndex ) const;

	// get dna data
	const char* GetDnaString() const;
	const Vector4* GetMeshAreaParameter( const BlueSharedString& areaDesignation, const BlueSharedString& parameterName, const std::map<BlueSharedString, Vector4>* hullParameters = nullptr, unsigned int blockededMaterials = 0 ) const;


private:
	// search for a dna command and get it's args
	bool GetDnaCommandArgs( DnaCommand cmd, std::vector<std::string>& args ) const;
	// search and area collection to find the data of a specific parameter
	const Vector4* SearchForParameterData( const std::map<BlueSharedString, EveSOFDataMgr::FactionAreaData>& areas, const BlueSharedString& areaDesignation, const BlueSharedString& parameterName ) const;

	// the dna as a string
	std::string m_dna;

	// a temporary pointer to the BIG data
	EveSOFDataMgrPtr m_dataMgr;
	// pointers to the specific data inside the BIG data
	const EveSOFDataMgr::HullData* m_hullData;
	const EveSOFDataMgr::FactionData* m_factionData;
	const EveSOFDataMgr::RaceData* m_raceData;
	const EveSOFDataMgr::GenericData* m_genericData;

	// decoded data
	std::string m_hullName;
	std::string m_factionName;
	std::string m_raceName;
	std::map<std::string, std::vector<std::string>> m_commands;
};

TYPEDEF_BLUECLASS( EveSOFDNA );

#endif // EveSOFDNA_H