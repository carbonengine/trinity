////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOFDataMgr_H
#define EveSOFDataMgr_H

// forwards
BLUE_DECLARE( EveSOFData );
BLUE_DECLARE( EveSOFDataHull );
BLUE_DECLARE( EveSOFDataFaction );
BLUE_DECLARE( EveSOFDataRace );
BLUE_DECLARE( EveSOFDataGeneric );
BLUE_DECLARE( EveSOFDataMaterial );
BLUE_DECLARE( EveSOFDataHullArea );

// --------------------------------------------------------------------------------
// Description:
//   This class is the manager for all the SOF data
// SeeAlso:
//   EveSOF
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOFDataMgr ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	// general structs
	struct TextureData
	{
		std::string resFilePath;
	};

	struct LocatorData
	{
		BlueSharedString name;
		Matrix transform;
	};

	struct LocatorDirectionData
	{
		Vector3 position;
		Quaternion rotation;
	};

	// hull data structs
	struct HullBoosterItemData
	{
		Matrix transform;
		Vector4 functionality;
		bool hasTrail;
	};

	struct HullBoosterData
	{
		bool alwaysOn, hasTrails;
		std::vector<HullBoosterItemData> items;
	};

	struct HullSpotlightSetItemData
	{
		Matrix transform;
		int boneIndex, groupIndex;
		bool boosterGainInfluence;
		Vector3 spriteScale;
		float coneIntensity, flareIntensity, spriteIntensity;
	};

	struct HullSpotlightSetData
	{
		bool skinned;
		float zOffset;
		std::string glowTextureResPath;
		std::string coneTextureResPath;
		std::vector<HullSpotlightSetItemData> items;
	};

	struct HullPlaneSetItemData
	{
		Vector3 position;
		Vector3 scaling;
		Quaternion rotation;
		Color color;
		Vector4 layer1Transform, layer2Transform, layer1Scroll, layer2Scroll;
		int boneIndex, groupIndex;
	};

	struct HullPlaneSetData
	{
		bool skinned;
		std::string layer1MapResPath;
		std::string layer2MapResPath;
		std::string maskMapResPath;
		Vector4 planeData;
		std::vector<HullPlaneSetItemData> items;
	};

	struct HullSpriteSetItemData
	{
		Vector3 position;
		float blinkRate, blinkPhase, minScale, maxScale, falloff;
		int boneIndex, groupIndex;
	};

	struct HullSpriteSetData
	{
		bool skinned;
		std::vector<HullSpriteSetItemData> m_items;
	};

	struct HullAreas
	{
		unsigned int index;
		unsigned int count;
		BlueSharedString designation;
		std::string shader;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
	};

	struct HullDecalData
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scaling;
		int groupIndex;
		int boneIndex;
		std::string shader;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
		std::vector<uint32_t> indexBuffer;
	};

	struct HullChild
	{
		std::string redFilePath;
		Vector3 translation;
		Quaternion rotation;
		Vector3 scaling;
		int id;
	};

	struct HullAnimation
	{
		std::string name;

		float startRotationTime;
		float endRotationTime;
		Quaternion startRotationValue;
		Quaternion endRotationValue;

		float startTranslationTime;
		float endTranslationTime;
		Vector3 startTranslationValue;
		Vector3 endTranslationValue;

		float startRate;
		float endRate;
		int id;
	};

	struct HullData
	{
		std::string geometryResFilePath;
		Vector4 boundingSphere;
		bool isSkinned;
		Vector3 audioPosition;
		std::vector<HullSpriteSetData> spriteSets;
		std::vector<HullSpotlightSetData> spotlightSets;
		std::vector<HullPlaneSetData> planeSets;
		std::vector<HullAreas> opaqueAreas;
		std::vector<HullAreas> transparentAreas;
		std::vector<HullAreas> additiveAreas;
		std::vector<HullAreas> distortionAreas;
		std::vector<HullAreas> depthAreas;
		std::vector<HullDecalData> hullDecals;
		HullBoosterData boosters;
		std::vector<LocatorData> locatorTurrets;
		std::vector<LocatorDirectionData> locatorDamage;
		std::vector<HullChild> children;
		std::vector<HullAnimation> animations;
		std::string modelRotationCurvePath;
		std::string modelTranslationCurvePath;
	};

	// faction data structs
	struct FactionSpriteSetColorData
	{
		bool isVisible;
		Color color;
	};

	struct FactionSpotlightSetColorData
	{
		Color coneColor;
		Color flareColor;
		Color spriteColor;
	};

	struct FactionPlaneSetColorData
	{
		Color color;
	};

	struct FactionAreaData
	{
		std::map<BlueSharedString, Vector4> parameters;
	};

	struct FactionDecalData
	{
		int groupIndex;
		bool isVisible;
		std::string shader;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
	};

	struct FactionData
	{
		// texture insert
		std::string resPathInsert;

		// material usage ids
		int materialUsageMain;
		int materialUsageMask;

		// hull area paramaters
		std::map<BlueSharedString, FactionAreaData> opaqueAreaParameters;
		std::map<BlueSharedString, FactionAreaData> transparentAreaParameters;
		std::map<BlueSharedString, FactionAreaData> additiveAreaParameters;
		// spritesets
		std::map<int, FactionSpriteSetColorData> spriteSetsColor;
		// spotlight sets
		std::map<int, FactionSpotlightSetColorData> spotlightSetsColors;
		// plane sets
		std::map<int, FactionPlaneSetColorData> planeSetsColors;
		// decals
		std::map<int, FactionDecalData> decalData;
	};

	// race data structs
	struct RaceBoosterData
	{
		float glowScale, symHaloScale, haloScaleX, haloScaleY;
		Color color, glowColor, haloColor, trailColor;
		Vector4 scale, trailSize;
		std::string textureResPath;
	};

	struct RaceData
	{
		// boosters
		RaceBoosterData boosters;
	};

	// material data structs
	struct MaterialData
	{
		// shader params
		std::map<BlueSharedString, Vector4> parameters;
	};

	// generic data structs
	struct GenericData
	{
		// shader locations
		std::string shaderPrefix, shaderPrefixAnimated;
		std::string areaShaderLocation;
		std::string decalShaderLocation;
	};


	EveSOFDataMgr( IRoot* lockobj = NULL );
	~EveSOFDataMgr();

	// loading all the data
	bool LoadData( const char* filePath );
	bool SetData( EveSOFData* dbData );

	// update individual parts
	bool UpdateHull( const char* hullName, EveSOFDataHull* hullData );
	bool UpdateFaction( const char* factionName, EveSOFDataFaction* factionData );
	bool UpdateRace( const char* raceName, EveSOFDataRace* raceData );
	bool UpdateMaterial( const char* materialName, EveSOFDataMaterial* materialData );

	// access to generic
	const GenericData* GetGenericData() const;
	// access to hull data
	bool HasHullData( const char* hullName ) const;
	const HullData* GetHullData( const char* hullName ) const;
	// access to faction data
	bool HasFactionData( const char* factionName ) const;
	const FactionData* GetFactionData( const char* factionName ) const;
	// access to race data
	bool HasRaceData( const char* raceName ) const;
	const RaceData* GetRaceData( const char* raceName ) const;
	// access to material data
	bool HasMaterialData( const char* materialName ) const;
	const MaterialData* GetMaterialData( const char* materialName ) const;

private:
	// load indiviual parts of data
	bool LoadHullData( EveSOFDataPtr srcData );
	bool LoadFactionData( EveSOFDataPtr srcData );
	bool LoadRaceData( EveSOFDataPtr srcData );
	bool LoadGenericData( EveSOFDataPtr srcData );
	bool LoadMaterialData( EveSOFDataPtr srcData );
	HullAreas LoadHullAreaData( const EveSOFDataHullAreaPtr hullArea ) const;

	// helper functions to pass data from trinity object to stl containers
	void GenerateHullData( HullData& hd, EveSOFDataHullPtr srcData ) const;
	void GenerateFactionData( FactionData& fd, EveSOFDataFactionPtr srcData ) const;
	void GenerateRaceData( RaceData& rd, EveSOFDataRacePtr srcData ) const;
	void GenerateMaterialData( MaterialData& rd, EveSOFDataMaterialPtr srcData ) const;

	// keep all hull data in a map
	std::map<std::string, HullData> m_hullData;
	// keep all faction data in a map
	std::map<std::string, FactionData> m_factionData;
	// keep all race data in a map
	std::map<std::string, RaceData> m_raceData;
	// keep all material data in a map
	std::map<std::string, MaterialData> m_materialData;

	// keep the generic data
	GenericData m_genericData;
};

TYPEDEF_BLUECLASS( EveSOFDataMgr );

#endif // EveSOFDataMgr_H