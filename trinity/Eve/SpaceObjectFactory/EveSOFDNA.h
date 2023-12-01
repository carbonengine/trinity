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
class EveSOFUtilsParameterName;

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
		CMD_MATERIAL,
		CMD_MESH,
		CMD_RESPATHINSERT,
		CMD_VARIANT,
		CMD_CLASS,
		CMD_PATTERN,
		CMD_LAYOUT,
		CMD_EXPERIMENTAL,
		CMD_MAX
	}; 

	// initialize this dna
	void Setup( const char* dnaString, EveSOFDataMgrPtr dataMgr );
	void Setup( BlueSharedString layoutName, const EveSOFDataMgr::DNADescriptorData& descriptor, EveSOFDNAPtr parent, const EveSOFDataMgrPtr dataMgr );

	// is it correctly initialized?
	bool IsValid() const;
	// does it have the correct content?
	bool ValidateContent();

	// get generic data
	const char* GetAreaShaderLocationResPath() const;
	const char* GetDecalShaderLocationResPath() const;
	float GetDecalMinScreenSize( EveSOFDataHullDecalSetItem::Usage usage ) const;

	uint32_t GetDecalShader() const;
	const char* GetShaderPrefix( bool isAnimated ) const;
	std::string GetCompleteShaderPath( const char* shaderPath ) const;
	const EveSOFDataMgr::GenericShaderData* GetGenericAreaShaderData( const BlueSharedString& shaderName ) const;
	const EveSOFDataMgr::GenericDecalShaderData* GetGenericDecalShaderData( const BlueSharedString& shaderName ) const;
	const EveSOFDataMgr::GenericDamageData* GetGenericDamageData() const;
	const EveSOFDataMgr::GenericHullDamageData* GetGenericHullDamageData() const;
	const EveSwarm::BehaviorProperties* GetGenericSwarmProperties() const;
	const EveSOFDataMgr::GenericBannerShaderData& GetGenericBannerShaderData() const;

	// get racial data
	const EveSOFDataMgr::RaceBoosterData* GetRaceBoosterData() const;
	const EveSOFDataMgr::RaceDamageData* GetRaceDamageData() const;

	// get hull data
	size_t GetMultiHullCount() const;
	EveSOFDataHull::BuildClass GetBuildClass() const;
	CcpMath::Sphere GetHullBoundingSphere() const;
	CcpMath::Sphere GetParentBoundingSphere() const;
	const CcpMath::AxisAlignedEllipsoid& GetHullShapeEllipsoid() const;
	CcpMath::AxisAlignedEllipsoid& GetParentHullShapeEllipsoid();

	bool IsHullAnimated() const;
	bool IsHullUsingDecalSets() const;
	bool DynamicBoundingSphereEnabled() const;
	bool CastShadow() const;
	const EveSOFDataMgr::HullBoosterData* GetHullBoosterData( size_t n ) const;
	size_t GetHullBoosterCount() const;
	const Vector3* GetHullAudioPosition( size_t n ) const;
	std::string GetHullGeometryResPath() const;
	const char* GetModelRotationCurvePath() const;
	const char* GetModelTranslationCurvePath() const;
	EveSOFDataHull::ImpactEffectType GetImpactEffectType() const;
	const std::vector<EveSOFDataMgr::HullAreas>* GetHullMeshAreas( TriBatchType type, size_t n ) const;
	const std::vector<EveSOFDataMgr::HullChild>& GetHullChildren() const;
	const std::vector<EveSOFDataMgr::HullChildSetData>& GetHullChildSets() const;
	const std::vector<EveSOFDataMgr::HullInstancedMesh>& GetHullInstancedMeshes() const;
	const std::vector<EveSOFDataMgr::HullAnimation>& GetHullAnimations() const;
	const std::vector<EveSOFDataMgr::HullSoundEmitter>& GetHullSoundEmitters() const;
	const std::vector<EveSOFDataMgr::HullController>& GetHullControllers() const;
	const std::vector<EveSOFDataMgr::HullDecalSetData>& GetHullDecalSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullLightSetData>& GetHullLightSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullPlaneSetData>& GetHullPlaneSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullSpotlightSetData>& GetHullSpotlightSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullSpriteSetData>& GetHullSpriteSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullSpriteLineSetData>& GetHullSpriteLineSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullHazeSetData>& GetHullHazeSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullBannerData>& GetHullBanners( size_t n ) const;
	const std::vector<EveSOFDataMgr::HullBannerSetData>& GetHullBannerSets( size_t n ) const;
	const std::vector<EveSOFDataMgr::LocatorData>& GetHullTurretLocators( size_t n ) const;
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* GetHullLocators( const char* setName, size_t n ) const;
	unsigned int GetLocatorCount( const char* setName ) const;
	const std::vector<BlueSharedString> GetHullLocatorSetNames( size_t n ) const;
	const Vector3* GetHullNextSubsystemOffset( size_t n ) const;
	bool GetHullTextureWithMeshIndex( std::string & resPath, const BlueSharedString& textureName, int32_t meshIndex, size_t n, std::unordered_map<std::string, bool>* existingFilesCache ) const;

	// get faction data
	void ModifyTextureResPath( std::string& resPath, std::unordered_map<std::string, bool>* existingFilesCache ) const;
	const Vector4* GetFactionTurretParameters( const BlueSharedString& parameterName ) const;
	const EveSOFDataMgr::FactionPlaneSetColorData* GetFactionPlaneSetData( int groupIndex ) const;
	const EveSOFDataMgr::FactionSpotlightSetColorData* GetFactionSpotlightSetData( int groupIndex ) const;
	const EveSOFDataMgr::FactionChildData* GetFactionChildData( int groupIndex ) const;

	// get color data
	const Color* GetColorSet() const;

	// get logo data
	const EveSOFDataMgr::LogoData* GetLogo( size_t index ) const;
	const bool HasLogoSet( size_t index ) const;

	// get visibility data
	bool IsInVisibilityData( uint32_t h ) const;

	// get pattern data
	size_t GetPatternLayerCount() const;
	const EveSOFDataMgr::PatternApplicationData* GetPatternApplicationData( bool& theCallerNeedsToDeleteTheResultBecauseIAmBroken ) const;
	const EveSOFDataMgr::PatternProjectionData* GetPatternProjectionData( const EveSOFDataMgr::PatternApplicationData* patternApplicationData, size_t layer ) const;
	const EveSOFDataMgr::PatternLayerData* GetPatternLayerData( const EveSOFDataMgr::PatternApplicationData* patternApplicationData, size_t layer ) const;
	const Vector4 GetMaterialTargets( const EveSOFDataMgr::PatternLayerData* ) const;
	bool IsPatternLayerApplicableToArea( const EveSOFDataMgr::PatternLayerData* layerData, EveSOFDataArea::AreaType areaType ) const;

	// get layout data
	size_t GetLayoutCount() const;
	const EveSOFDataMgr::LayoutData* GetLayoutData( size_t layoutIndex ) const;
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* GetPlacementLocators( size_t hullIndex, BlueSharedString& locatorSetName ) const;

	// get mixed data
	const char* GetDnaString() const;
	const Vector4* GetMeshAreaParameter( EveSOFDataArea::AreaType areaType, const BlueSharedString& parameterName, const std::map<BlueSharedString, Vector4>* hullParameters = nullptr, unsigned int blockededMaterials = 0 ) const;
	const char* GetImpactShieldShader() const;
	unsigned int GetHighestMeshAreaIndex( TriBatchType areaType, size_t n = 0 ) const;

	bool IsUsingExperimentalFeatures() const;
	bool IsHullUsingExperimentalFeatures() const;
	EntityComponents::ReflectionMode GetReflectionMode() const;

	BlueSharedString GetFactionName() const;
	BlueSharedString GetRaceName() const;

	// a few setters for layouts / nested layouts
	void SetParentBoundingSphere( const CcpMath::Sphere& boundingSphere );
	void SetParentShapeEllipsoidInfo( const CcpMath::AxisAlignedEllipsoid& ellipsoid );
	void DisableAnimation();

private:
	// special cusomt data setup
	void SetupCustomData();
	// search for a dna
	bool GetDnaCommandArgs( DnaCommand cmd, std::vector<std::string>& args ) const;
	bool HasDnaCommand( DnaCommand cmd ) const;

	// the factional pattern application data
	const EveSOFDataMgr::PatternApplicationData* GetFactionalPatternApplicationData() const;
	const EveSOFDataMgr::PatternApplicationData* GetHullPatternApplicationData() const;

	// the dna as a string
	std::string m_dna;

	// a temporary pointer to the BIG data
	EveSOFDataMgrPtr m_dataMgr;
	// pointers to the specific data inside the BIG data or a custom data block
	std::vector<const EveSOFDataMgr::HullData*> m_hullDatas;
	const EveSOFDataMgr::FactionData* m_factionData;
	const EveSOFDataMgr::RaceData* m_raceData;
	const EveSOFDataMgr::GenericData* m_genericData;
	const EveSOFDataMgr::PatternData* m_patternData;
	std::vector<const EveSOFDataMgr::LayoutData*> m_layoutData;

	// custom data blocks
	std::vector<EveSOFDataMgr::HullData> m_customHullData;

	// decoded data
	std::vector<std::string> m_hullNames;
	std::string m_factionName;
	std::string m_raceName;
	std::map<std::string, std::vector<std::string>> m_commands;

	// Parent data
	CcpMath::Sphere m_parentBoundingSphere;
	CcpMath::AxisAlignedEllipsoid m_parentHullShapeEllipsoid;

	// We may have a skinned hull, but used in non skinned situations, like layouts
	bool m_isSkinned;

	bool m_experimental;
};

TYPEDEF_BLUECLASS( EveSOFDNA );

#endif // EveSOFDNA_H
