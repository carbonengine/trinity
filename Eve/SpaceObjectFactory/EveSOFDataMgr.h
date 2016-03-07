////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOFDataMgr_H
#define EveSOFDataMgr_H

#include "EveSOFData.h"

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
		int boneIndex;
	};

	// hull data structs
	struct HullBoosterItemData
	{
		Matrix transform;
		Vector4 functionality;
		uint32_t atlasIndex0;
		uint32_t atlasIndex1;
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
		int boneIndex, groupIndex, maskMapAtlasIndex;
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
		std::vector<HullSpriteSetItemData> items;
	};

	struct HullSpriteLineSetItemData
	{
		Vector3 position, scaling;
		Quaternion rotation;
		float spacing, blinkRate, blinkPhase, blinkPhaseShift, minScale, maxScale, falloff;
		int boneIndex, groupIndex;
	};

	struct HullSpriteLineSetData
	{
		bool skinned;
		std::vector<HullSpriteLineSetItemData> items;
	};

	struct HullAreas
	{
		unsigned int index;
		unsigned int count;
		BlueSharedString designation;
		BlueSharedString shader;
		unsigned int blockedMaterials;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
	};

	struct HullDecalData
	{
		EveSOFDataHullDecal::Usage usage;
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
		Tr2Lod lowestLodVisible;
		Vector3 translation;
		Quaternion rotation;
		Vector3 scaling;
		int id;
	};

	struct HullInstancedMesh
	{
		BlueSharedString name;
		Tr2Lod lowestLodVisible;
		std::string geometryResPath;
		std::string instanceGeometryResPath;
		BlueSharedString shader;
		std::map<BlueSharedString, TextureData> textures;
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
		EveSOFDataHull::BuildClass buildClass;
		std::string geometryResFilePath;
		Vector4 boundingSphere;
		Vector3 shapeEllipsoidCenter;
		Vector3 shapeEllipsoidRadius;
		bool isSkinned;
		Vector3 audioPosition;
		std::vector<HullSpriteSetData> spriteSets;
		std::vector<HullSpotlightSetData> spotlightSets;
		std::vector<HullPlaneSetData> planeSets;
		std::vector<HullSpriteLineSetData> spriteLineSets;
		bool hasImpactEffect;
		std::vector<HullAreas> opaqueAreas;
		std::vector<HullAreas> decalAreas;
		std::vector<HullAreas> transparentAreas;
		std::vector<HullAreas> additiveAreas;
		std::vector<HullAreas> distortionAreas;
		std::vector<HullAreas> depthAreas;
		std::vector<HullDecalData> hullDecals;
		HullBoosterData boosters;
		std::vector<LocatorData> locatorTurrets;
		std::map<BlueSharedString, std::vector<LocatorDirectionData>> locatorSets;
		std::vector<HullChild> children;
		std::vector<HullInstancedMesh> instancedMeshes;
		std::vector<HullAnimation> animations;
		std::string modelRotationCurvePath;
		std::string modelTranslationCurvePath;
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

		// material usage ids (for turrets, etc.)
		int materialUsageList[4];

		// hull area parameter overloads
		std::map<BlueSharedString, FactionAreaData> areaParameters;
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
	struct RaceDamageData
	{
		std::map<BlueSharedString, Vector4> armorDamageParameters;
		std::map<BlueSharedString, TextureData> armorDamageTextures;
		std::map<BlueSharedString, Vector4> shieldDamageParameters;
		std::map<BlueSharedString, TextureData> shieldDamageTextures;
	};

	struct RaceBoosterDataShape
	{
		float noiseFunction;
		float noiseSpeed;
		Vector4 noiseAmplitureStart;
		Vector4 noiseAmplitureEnd;
		Vector4 noiseFrequency;
		Color color;
	};

	struct RaceBoosterData
	{
		float glowScale, symHaloScale, haloScaleX, haloScaleY;
		Color glowColor, haloColor, trailColor, warpGlowColor, warpHaloColor;
		Vector4 scale, trailSize;

		RaceBoosterDataShape shape0, shape1;
		RaceBoosterDataShape warpShape0, warpShape1;

		std::string shapeAtlasResPath;
		std::string gradient0ResPath;
		std::string gradient1ResPath;

		uint32_t shapeAtlasHeight;
		uint32_t shapeAtlasCount;

		float lightOffset;
		float lightRadius;
		float lightWarpRadius;
		float lightFlickerAmplitude;
		float lightFlickerFrequency;
		Color lightColor;
		Color lightWarpColor;

		bool volumetric;
	};

	struct RaceData
	{
		// boosters
		RaceBoosterData boosters;
		// hull area parameter overloads
		std::map<BlueSharedString, FactionAreaData> hullAreaParameters;
		// impact damage data
		RaceDamageData damage;
	};

	// material data structs
	struct MaterialData
	{
		// shader params
		std::map<BlueSharedString, Vector4> parameters;
	};

	// generic data structs
	struct GenericDamageData
	{
		// hull damage perlin noise
		float flickerPerlinSpeed;
		float flickerPerlinAlpha;
		float flickerPerlinBeta;
		float flickerPerlinOffset;
		float flickerPerlinScale;
		int flickerPerlinN;

		// armor damage particlesystem data
		float armorParticleRate;
		float armorParticleAngle;
		Vector2 armorParticleMinMaxSpeed;
		Vector2 armorParticleMinMaxLifeTime;
		Vector4 armorParticleSizes;
		Color armorParticleColors[4];
		uint32_t armorParticleTextureIndex;
		float armorParticleVelocityStretchRotation;
		float armorParticleDrag;
		float armorParticleTurbulenceAmplitude;
		uint32_t armorParticleTurbulenceFrequency;

		// shaders & resources
		std::string armorShader;
		std::string shieldShader;
		std::string shieldGeometryResFilePath;
	};

	struct GenericShaderData
	{
		// complete list of parameters
		std::vector<BlueSharedString> parameters;
		// complete list of textures
		std::vector<BlueSharedString> textures;
		// global textures for this shader
		std::map<BlueSharedString, TextureData> defaultTextures;
	};

	struct GenericData
	{
		// shader locations
		std::string shaderPrefix, shaderPrefixAnimated;
		std::string areaShaderLocation;
		std::string decalShaderLocation;
		// material perfixes
		std::vector<std::string> materialPrefixes;
		// shader-specific data
		std::map<BlueSharedString, GenericShaderData> areaShaderData;
		std::map<BlueSharedString, GenericShaderData> decalShaderData;
		// damage
		GenericDamageData damage;
		// hull area parameter overloads
		std::map<BlueSharedString, FactionAreaData> hullAreaParameters;
		// variants
		std::map<BlueSharedString, HullAreas> variants;
		// swarm behavior
		EveSwarm::BehaviorProperties swarmBehavior;
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
	bool UpdateGeneric( EveSOFDataGeneric* genericData );
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
	void GenerateGenericData( GenericData& gd, EveSOFDataGenericPtr srcData ) const;
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