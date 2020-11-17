////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOFDataMgr_H
#define EveSOFDataMgr_H

#include "EveSOFData.h"
#include "Eve/SpaceObject/Attachments/Sets/EveBannerSet.h"

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

	struct AreaMaterialData
	{
		std::map<std::pair<uint32_t, uint32_t>, BlueSharedString> materialNames;
		std::map<std::pair<uint32_t, std::string>, EveSOFDataFactionColorSet::ColorType> glowColor;
	};

	// pattern data structs
	struct PatternProjectionData
	{
		bool enabled;
		Vector3 position;
		Vector3 scaling;
		Quaternion rotation;
		bool isMirrored;
	};

	struct PatternLayerData
	{
		// texture name
		BlueSharedString textureName;
		// texture resfilepath
		std::string textureResFilePath;
		// material source
		uint8_t materialSourceID;
		// material targets
		Vector4 materialTargets;
		// projection type
		Tr2RenderContextEnum::TextureAddressMode projectionAddressModeU, projectionAddressModeV;
	};

	struct PatternData
	{
		// pattern data (per hull)
		std::map<BlueSharedString, std::vector<PatternProjectionData>> projectionData;
		// pattern data (per layer)
		std::vector<PatternLayerData> layerData;
	};

	// hull data structs
	struct HullBoosterItemData
	{
		Matrix transform;
		Vector4 functionality;
		uint32_t atlasIndex0;
		uint32_t atlasIndex1;
		float lightScale;
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
		// BlinkData - combined into a float4 in the vertex buffer;
		float rate, phase, dutyCycle;
		int blinkMode; // selector
	};

	struct HullPlaneSetData
	{
		bool skinned;
		std::string layer1MapResPath;
		std::string layer2MapResPath;
		std::string maskMapResPath;
		EveSOFDataHullPlaneSet::Usage usage;
		uint32_t atlasSize;
		std::vector<HullPlaneSetItemData> items;
	};

	struct HullSpriteSetItemData
	{
		Vector3 position;
		float blinkRate, blinkPhase, minScale, maxScale, falloff, intensity;
		int boneIndex;
		EveSOFDataFactionColorSet::ColorType colorType;
	};

	struct HullSpriteSetData
	{
		bool skinned;
		uint32_t visibilityGroup;
		std::vector<HullSpriteSetItemData> items;
	};

	struct HullSpriteLineSetItemData
	{
		Vector3 position, scaling;
		Quaternion rotation;
		float spacing, blinkRate, blinkPhase, blinkPhaseShift, minScale, maxScale, falloff, intensity;
		int boneIndex;
		bool isCircle;
		EveSOFDataFactionColorSet::ColorType colorType;
	};

	struct HullSpriteLineSetData
	{
		bool skinned;
		uint32_t visibilityGroup;
		std::vector<HullSpriteLineSetItemData> items;
	};

	struct HullHazeSetItemData
	{
		Vector3 position, scaling;
		Quaternion rotation;
		EveSOFDataFactionColorSet::ColorType colorType;
		float hazeBrightness, hazeFalloff, sourceSize, sourceBrightness;
		bool boosterGainInfluence;
	};

	struct HullHazeSetData
	{
		EveSOFDataHullHazeSet::HazeType hazeType;
		uint32_t visibilityGroup;
		std::vector<HullHazeSetItemData> items;
	};

	struct HullBannerSetItemLightData
	{
		float radiusMultiplier;
		float brightness;
		float innerRadiusMultiplier;
		float noiseAmplitude;
		float noiseFrequency;
		int noiseOctaves;
		float saturation;
	};

	struct HullBannerSetItemData
	{
		EveBannerItem item;
		HullBannerSetItemLightData bannerLight;
		uint32_t visibilityGroup;
	};

	struct HullBannerSetData
	{
		std::vector<HullBannerSetItemData> items;
		EveSOFDataHullBanner::Usage usage;
	};

	struct HullAreas
	{
		unsigned int index;
		unsigned int count;
		EveSOFDataArea::AreaType areaType;
		BlueSharedString shader;
		unsigned int blockedMaterials;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
	};

	struct HullDecalSetItemData
	{
		EveSOFDataHullDecalSetItem::Usage usage;
		Vector3 position;
		Quaternion rotation;
		Vector3 scaling;
		int boneIndex;
		int meshIndex;
		uint32_t glowColorType;
		EveSOFDataLogoSet::LogoType logoType;
		std::map<BlueSharedString, TextureData> textures;
		std::map<BlueSharedString, Vector4> parameters;
		std::vector<uint32_t> indexBuffer;
	};

	struct HullDecalSetData
	{
		uint32_t visibilityGroup;
		std::vector<HullDecalSetItemData> items;
	};

	struct HullLightSetData
	{
		uint32_t visibilityGroup;
		std::vector<EveSOFDataHullLightSetItem::HullLightSetItemData> items;
	};

	struct HullChild
	{
		std::string redFilePath;
		Tr2Lod lowestLodVisible;
		Vector3 translation;
		Quaternion rotation;
		Vector3 scaling;
		int id;
		int groupIndex;
	};

	struct HullChildSetItemData
	{
		std::string redFilePath;
		Tr2Lod lowestLodVisible;
		Vector3 translation;
		Quaternion rotation;
		Vector3 scaling;
	};

	struct HullChildSetData
	{
		uint32_t visibilityGroup;
		std::vector<HullChildSetItemData> items;		
	};

	struct HullMeshInstance
	{
		Vector4 transform0;
		Vector4 transform1;
		Vector4 transform2;
		int boneIndex;
	};

	struct HullInstancedMesh
	{
		BlueSharedString name;
		Tr2Lod lowestLodVisible;
		std::string geometryResPath;
		BlueSharedString shader;
		std::map<BlueSharedString, TextureData> textures;
		std::vector<HullMeshInstance> instances;
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

	struct HullSoundEmitter
	{
		std::string name;
		std::wstring prefix;
		Vector3 position;
	};

	struct HullData
	{
		EveSOFDataHull::BuildClass buildClass;
		std::string geometryResFilePath;
		Vector4 boundingSphere;
		Vector3 shapeEllipsoidCenter;
		Vector3 shapeEllipsoidRadius;
		bool isSkinned;
		bool isUsingDecalSets;
		bool enableDynamicBoundingSphere;
		bool castShadow;
		Vector3 audioPosition;
		std::vector<HullSpriteSetData> spriteSets;
		std::vector<HullSpotlightSetData> spotlightSets;
		std::vector<HullPlaneSetData> planeSets;
		std::vector<HullSpriteLineSetData> spriteLineSets;
		std::vector<HullHazeSetData> hazeSets;
		std::vector<HullBannerSetData> bannerSets;
		std::vector<HullDecalSetData> hullDecalSets;
		std::vector<HullLightSetData> hullLightSets;
		std::vector<HullChildSetData> childSets;
		PatternProjectionData defaultPattern;
		EveSOFDataHull::ImpactEffectType impactEffectType;
		std::vector<HullAreas> opaqueAreas;
		std::vector<HullAreas> decalAreas;
		std::vector<HullAreas> transparentAreas;
		std::vector<HullAreas> additiveAreas;
		std::vector<HullAreas> distortionAreas;
		HullBoosterData boosters;
		std::vector<LocatorData> locatorTurrets;
		std::map<BlueSharedString, std::vector<LocatorDirectionData>> locatorSets;
		std::vector<HullChild> children;
		std::vector<HullInstancedMesh> instancedMeshes;
		std::vector<HullAnimation> animations;
		std::vector<HullSoundEmitter> soundEmitters;
		std::vector<BlueSharedString> controllers;
		std::string modelRotationCurvePath;
		std::string modelTranslationCurvePath;
		std::map<int32_t, size_t> meshIndexToOpaqueAreaLookup;
	};

	// color data structs
	struct ColorData
	{
		// color data
		Color colors[EveSOFDataFactionColorSet::TYPE_MAX];
	};


	// Logo data structs
	struct LogoData
	{
		std::map<BlueSharedString, TextureData> textures;
	};

	struct LogoSetData
	{
		LogoData logos[EveSOFDataLogoSet::TYPE_MAX];
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

	struct FactionChildData
	{
		bool isVisible;
	};

	struct FactionData
	{
		// texture insert
		std::string resPathInsert;

		// material usage ids (for turrets, etc.)
		int materialUsageList[4];

		// default pattern
		PatternLayerData defaultPattern;
		std::string defaultPatternLayer1MaterialName;

		// hull area materials
		AreaMaterialData areaMaterials;
		// color data
		ColorData colorData;
		// logo data
		LogoSetData logoSetData;
		// visibility data
		std::set<uint32_t> visibilityData;
		// SOF UPDATE 6- spotlightSetsColors, planeSetColors and childData can be removed after the update
		// spotlight sets
		std::map<int, FactionSpotlightSetColorData> spotlightSetsColors;
		// plane sets
		std::map<int, FactionPlaneSetColorData> planeSetsColors;
		// children
		std::map<int, FactionChildData> childData;
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
	};

	struct RaceData
	{
		// boosters
		RaceBoosterData boosters;
		// hull area materials
		AreaMaterialData areaMaterials;
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
		float armorParticleColorMidPoint;

		// shaders & resources
		std::string armorShader;
		std::string shieldShaderEllipsoid;
		std::string shieldShaderHull;
		std::string shieldGeometryResFilePath;
	};

	struct GenericHullDamageData
	{
		// hull damage particlesystem data
		float hullParticleRate;
		float hullParticleAngle;
		float hullParticleInnerAngle;
		float hullParticleColorMidpoint;
		Vector2 hullParticleMinMaxSpeed;
		Vector2 hullParticleMinMaxLifeTime;
		Vector4 hullParticleSizes;
		Color hullParticleColors[4];
		uint32_t hullParticleTextureIndex;
		float hullParticleVelocityStretchRotation;
		float hullParticleDrag;
		float hullParticleTurbulenceAmplitude;
		uint32_t hullParticleTurbulenceFrequency;
	};

	struct GenericShaderData
	{
		// complete list of parameters
		std::vector<BlueSharedString> parameters;
		// one transparency map (if shader has it)
		BlueSharedString transparencyTextureName;
		// does SOF need to genrate a depth area for this shader?
		bool doGenerateDepthArea;
		// global textures for this shader
		std::map<BlueSharedString, TextureData> defaultTextures;
		// global parameters for this shader
		std::map<BlueSharedString, Vector4> defaultParameters;
	};

	struct GenericDecalShaderData
	{
		// complete list of parameters
		std::vector<BlueSharedString> parameters;
		// global textures for this shader
		std::map<BlueSharedString, TextureData> defaultTextures;
		// parent textures from the hull
		std::vector<BlueSharedString> parentTextures;
	};

	struct VariantData
	{
		// what area does it go into?
		bool isTransparent;
		// the area data
		HullAreas hullAreaData;
	};

	struct GenericBannerShaderData
	{
		std::string shader;
		std::map<BlueSharedString, TextureData> defaultTextures;
		std::map<BlueSharedString, Vector4> defaultParameters;
	};

	struct GenericData
	{
		// default textures
		std::string resPathDefaultAlliance;
		std::string resPathDefaultCorp;
		std::string resPathDefaultCeo;
		// shader locations
		std::string shaderPrefix, shaderPrefixAnimated;
		std::string areaShaderLocation;
		std::string decalShaderLocation;
		float decalMinScreenSize[EveSOFDataHullDecalSetItem::USAGE_MAX];

		// material perfixes
		std::vector<std::string> materialPrefixes;
		std::vector<std::string> patternMaterialPrefixes;
		// shader-specific data
		std::map<BlueSharedString, GenericShaderData> areaShaderData;
		std::map<BlueSharedString, GenericDecalShaderData> decalShaderData;
		// damage
		GenericDamageData damage;
		GenericHullDamageData hullDamage;
		// generic wreck material data
		AreaMaterialData genericWreckMaterialData;
		// variants
		std::map<BlueSharedString, VariantData> variants;
		// swarm behavior
		EveSwarm::BehaviorProperties swarmBehavior;
		GenericBannerShaderData bannerShader;
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
	bool UpdatePattern( const char* patternName, EveSOFDataPattern* patternData );

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
	// access to pattern data
	bool HasPatternData( const char* patternName ) const;
	const PatternData* GetPatternData( const char* patternName ) const;

private:
	// load indiviual parts of data
	bool LoadHullData( EveSOFDataPtr srcData );
	bool LoadFactionData( EveSOFDataPtr srcData );
	bool LoadRaceData( EveSOFDataPtr srcData );
	bool LoadGenericData( EveSOFDataPtr srcData );
	bool LoadMaterialData( EveSOFDataPtr srcData );
	bool LoadPatternData( EveSOFDataPtr srcData );
	HullAreas LoadHullAreaData( const EveSOFDataHullAreaPtr hullArea ) const;

	// helper functions to pass data from trinity object to stl containers
	void GenerateHullData( HullData& hd, EveSOFDataHullPtr srcData ) const;
	void GenerateFactionData( FactionData& fd, EveSOFDataFactionPtr srcData ) const;
	void GenerateRaceData( RaceData& rd, EveSOFDataRacePtr srcData ) const;
	void GenerateGenericData( GenericData& gd, EveSOFDataGenericPtr srcData ) const;
	void GenerateMaterialData( MaterialData& rd, EveSOFDataMaterialPtr srcData ) const;
	void GeneratePatternData( PatternData& rd, EveSOFDataPatternPtr srcData ) const;

	// helper function to get the hash
	uint32_t GetVisibilityGroupHash( const BlueSharedString visibilityGroup ) const;

	// keep all hull data in a map
	std::map<std::string, HullData> m_hullData;
	// keep all faction data in a map
	std::map<std::string, FactionData> m_factionData;
	// keep all race data in a map
	std::map<std::string, RaceData> m_raceData;
	// keep all material data in a map
	std::map<std::string, MaterialData> m_materialData;
	// keep all pattern data in a map
	std::map<std::string, PatternData> m_patternData;

	// keep the generic data
	GenericData m_genericData;
};

TYPEDEF_BLUECLASS( EveSOFDataMgr );

#endif // EveSOFDataMgr_H