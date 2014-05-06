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
		std::string name;
		Matrix transform;
	};

	// hull data structs
	struct HullBoosterItemData
	{
		Matrix transform;
		Vector4 functionality;
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
		float coneIntensity;
		float flareIntensity;
		float spriteIntensity;
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
		int boneIndex;
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
		std::string designation;
		std::string shaderPath;
		std::map<std::string, TextureData> textures;
		std::map<std::string, Vector4> parameters;
	};

	struct HullDecalData
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scaling;
		std::string shaderPath;
		std::map<std::string, TextureData> textures;
		std::map<std::string, Vector4> parameters;
	};

	struct HullChild
	{
		std::string redFilePath;
		Vector3 translation;
		Quaternion rotation;
		Vector3 scaling;
	};

	struct HullData
	{
		std::string geometryResFilePath;
		Vector4 boundingSphere;
		bool isSkinned;
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
		std::vector<LocatorData> locatorAudio;
		std::vector<HullChild> children;
	};

	// faction data structs
	struct FactionSpriteSetColorData
	{
		Color color;
	};

	struct FactionSpotlightSetColorData
	{
		Color coneColor;
		Color flareColor;
		Color spriteColor;
	};

	struct FactionAreaData
	{
		std::map<std::string, Vector4> parameters;
	};

	struct FactionData
	{
		// texture insert
		std::string resPathInsert;

		// hull area paramaters
		std::map<std::string, FactionAreaData> opaqueAreaParameters;
		std::map<std::string, FactionAreaData> transparentAreaParameters;
		// spritesets
		std::map<int, FactionSpriteSetColorData> spriteSetsColor;
		// spotlight sets
		std::map<int, FactionSpotlightSetColorData> spotlightSetColors;
	};

	// race data structs
	struct BoosterData
	{
		float glowScale, symHaloScale, haloScaleX, haloScaleY;
		Color color, glowColor, haloColor, trailColor;
		Vector4 scale, trailSize;
		std::string textureResPath;
	};

	struct RaceData
	{
		// boosters
		BoosterData boosterData;
	};


	EveSOFDataMgr( IRoot* lockobj = NULL );
	~EveSOFDataMgr();

	// loading all the data
	bool LoadData( const char* filePath );
	bool SetData( IRootPtr dbData );

	// access to hull data
	bool HasHullData( const char* hullName ) const;
	const HullData* GetHullData( const char* hullName ) const;
	// access to faction data
	bool HasFactionData( const char* factionName ) const;
	const FactionData* GetFactionData( const char* factionName ) const;
	// access to race data
	bool HasRaceData( const char* raceName ) const;
	const RaceData* GetRaceData( const char* raceName ) const;

private:
	// load indiviual parts of data
	bool LoadHullData( EveSOFDataPtr srcData );
	bool LoadFactionData( EveSOFDataPtr srcData );
	bool LoadRaceData( EveSOFDataPtr srcData );
	HullAreas LoadHullAreaData( const EveSOFDataHullAreaPtr hullArea );

	// keep all hull data in a map
	std::map<std::string, HullData> m_hullData;
	// keep all faction data in a map
	std::map<std::string, FactionData> m_factionData;
	// keep all race data in a map
	std::map<std::string, RaceData> m_raceData;
};

TYPEDEF_BLUECLASS( EveSOFDataMgr );

#endif // EveSOFDataMgr_H