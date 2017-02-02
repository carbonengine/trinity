////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOFData_H
#define EveSOFData_H

#include "Eve/SpaceObject/EveSwarm.h"

// --------------------------------------------------------------------------------
// All data storage classes for gerenal purposes
// --------------------------------------------------------------------------------


BLUE_CLASS( EveSOFDataParameter ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataParameter( IRoot* lockobj = NULL );
	~EveSOFDataParameter() {}

	// simple shader parameter
	BlueSharedString m_name;
	Vector4 m_value;
};
TYPEDEF_BLUECLASS( EveSOFDataParameter );
BLUE_DECLARE_VECTOR( EveSOFDataParameter );


BLUE_CLASS( EveSOFDataTexture ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataTexture( IRoot* lockobj = NULL );
	~EveSOFDataTexture() {}

	// data
	std::string m_resFilePath;
	BlueSharedString m_name;
};
TYPEDEF_BLUECLASS( EveSOFDataTexture );
BLUE_DECLARE_VECTOR( EveSOFDataTexture );

BLUE_CLASS( EveSOFDataInstancedMesh ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataInstancedMesh( IRoot* lockobj = NULL );
	~EveSOFDataInstancedMesh() {}

	// data
	BlueSharedString m_name;
	Tr2Lod m_lowestLodVisible;
	std::string m_geometryResPath;
	std::string m_instanceGeometryResPath;
	BlueSharedString m_shader;
	PEveSOFDataTextureVector m_textures;
};
TYPEDEF_BLUECLASS( EveSOFDataInstancedMesh );
BLUE_DECLARE_VECTOR( EveSOFDataInstancedMesh );

BLUE_CLASS( EveSOFDataTransform ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataTransform( IRoot* lockobj = NULL );
	~EveSOFDataTransform() {}

	// data
	Vector3 m_position;
	Quaternion m_rotation;
	int m_boneIndex;
};
TYPEDEF_BLUECLASS( EveSOFDataTransform );
BLUE_DECLARE_VECTOR( EveSOFDataTransform );

BLUE_CLASS( EveSOFDataAreaMaterial ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataAreaMaterial( IRoot* lockobj = NULL );
	~EveSOFDataAreaMaterial() {}

	// data
	std::string m_material1;
	std::string m_material2;
	std::string m_material3;
	std::string m_material4;
	Color m_generalGlowColor;
};
TYPEDEF_BLUECLASS( EveSOFDataAreaMaterial );

BLUE_CLASS( EveSOFDataArea ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataArea( IRoot* lockobj = NULL );
	~EveSOFDataArea() {}

	// area type
	enum AreaType
	{
		TYPE_PRIMARY = 0,
		TYPE_GLASS,
		TYPE_SAILS,
		TYPE_REACTOR,
		TYPE_DARKHULL,
		TYPE_WRECK,

		TYPE_MAX,
		TYPE_NO_OVERWRITE = TYPE_MAX,
	};

	// material data
	EveSOFDataAreaMaterialPtr m_materials[TYPE_MAX];
};
TYPEDEF_BLUECLASS( EveSOFDataArea );





// --------------------------------------------------------------------------------
// All data storage classes for per pattern data
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOFDataPatternTransform ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataPatternTransform( IRoot* lockobj = NULL );
	~EveSOFDataPatternTransform() {}

	// per-hull positional data
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;

	// mirrored at yz plane?
	bool m_isMirrored;
};
TYPEDEF_BLUECLASS( EveSOFDataPatternTransform );


BLUE_CLASS( EveSOFDataPatternPerHull ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataPatternPerHull( IRoot* lockobj = NULL );
	~EveSOFDataPatternPerHull() {}

	// exact hull name
	BlueSharedString m_name;

	// per-hull positional data
	EveSOFDataPatternTransformPtr m_transformLayer1;
	EveSOFDataPatternTransformPtr m_transformLayer2;
};
TYPEDEF_BLUECLASS( EveSOFDataPatternPerHull );
BLUE_DECLARE_VECTOR( EveSOFDataPatternPerHull );


BLUE_CLASS( EveSOFDataPatternLayer ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataPatternLayer( IRoot* lockobj = NULL );
	~EveSOFDataPatternLayer() {}

	// texture projection type
	enum ProjectionType
	{
		PROJECTION_REPEAT = 0,
		PROJECTION_CLAMP,
		PROJECTION_BORDER,
	};

	// material sources
	enum MaterialSource
	{
		SOURCE_MATERIAL1 = 0,
		SOURCE_MATERIAL2,
		SOURCE_MATERIAL3,
		SOURCE_MATERIAL4,
		SOURCE_PATTERN1,
		SOURCE_PATTERN2,
	};

	// name of the texture
	BlueSharedString m_textureName;
	// res path
	std::string m_textureResFilePath;
	// how is the texture projected?
	ProjectionType m_projectionTypeU, m_projectionTypeV;
	// what is the pattern's material source?
	MaterialSource m_materialSource;
	// what is the pattern's material target?
	bool m_isTargetMtl1, m_isTargetMtl2, m_isTargetMtl3, m_isTargetMtl4;
};
TYPEDEF_BLUECLASS( EveSOFDataPatternLayer );


BLUE_CLASS( EveSOFDataPattern ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataPattern( IRoot* lockobj = NULL );
	~EveSOFDataPattern() {}

	// pattern name
	std::string m_name;
	// pattern layer data
	EveSOFDataPatternLayerPtr m_layer1;
	EveSOFDataPatternLayerPtr m_layer2;
	// pattern placement per hull
	PEveSOFDataPatternPerHullVector m_projections;
};
TYPEDEF_BLUECLASS( EveSOFDataPattern );
BLUE_DECLARE_VECTOR( EveSOFDataPattern );




// --------------------------------------------------------------------------------
// All data storage classes for per-hull data
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOFDataHullSpotlightSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpotlightSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullSpotlightSetItem() {}

	// per-hull data of a spotlightset
	Matrix m_transform;
	int m_boneIndex, m_groupIndex;
	bool m_boosterGainInfluence;
	Vector3 m_spriteScale;
	float m_flareIntensity, m_spriteIntensity, m_coneIntensity;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpotlightSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpotlightSetItem );


BLUE_CLASS( EveSOFDataHullSpotlightSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpotlightSet( IRoot* lockobj = NULL );
	~EveSOFDataHullSpotlightSet() {}

	// data
	std::string m_name;
	bool m_skinned;
	float m_zOffset;
	std::string m_glowTextureResPath;
	std::string m_coneTextureResPath;
	// items
	PEveSOFDataHullSpotlightSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpotlightSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpotlightSet );


BLUE_CLASS( EveSOFDataHullPlaneSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullPlaneSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullPlaneSetItem() {}

	// per-hull data of a planeset
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;
	Color m_color;
	Vector4 m_layer1Transform, m_layer2Transform, m_layer1Scroll, m_layer2Scroll;
	int m_boneIndex, m_groupIndex, m_maskMapAtlasIndex;
};
TYPEDEF_BLUECLASS( EveSOFDataHullPlaneSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullPlaneSetItem );


BLUE_CLASS( EveSOFDataHullPlaneSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullPlaneSet( IRoot* lockobj = NULL );
	~EveSOFDataHullPlaneSet() {}

	// decal type
	enum Usage
	{
		USAGE_STANDARD = 0,
		USAGE_HOLOGRAM,
		USAGE_VIDEO,
	};

	// data
	std::string m_name;
	bool m_skinned;
	std::string m_layer1MapResPath;
	std::string m_layer2MapResPath;
	std::string m_maskMapResPath;
	Usage m_usage;
	Vector4 m_planeData;
	// items
	PEveSOFDataHullPlaneSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullPlaneSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullPlaneSet );


BLUE_CLASS( EveSOFDataHullSpriteSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpriteSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullSpriteSetItem() {}

	// per-hull data of a spriteset
	Vector3 m_position;
	float m_blinkRate, m_blinkPhase, m_minScale, m_maxScale, m_falloff;
	int m_boneIndex, m_groupIndex;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteSetItem );


BLUE_CLASS( EveSOFDataHullSpriteSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpriteSet( IRoot* lockobj = NULL );
	~EveSOFDataHullSpriteSet() {}

	// animated?
	std::string m_name;
	bool m_skinned;
	// items
	PEveSOFDataHullSpriteSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteSet );


BLUE_CLASS( EveSOFDataHullSpriteLineSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpriteLineSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullSpriteLineSetItem() {}

	// per-hull data of a sprite line set
	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	float m_spacing, m_blinkRate, m_blinkPhase, m_blinkPhaseShift, m_minScale, m_maxScale, m_falloff;
	int m_boneIndex, m_groupIndex;
	bool m_isCircle;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteLineSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteLineSetItem );


BLUE_CLASS( EveSOFDataHullSpriteLineSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSpriteLineSet( IRoot* lockobj = NULL );
	~EveSOFDataHullSpriteLineSet() {}

	// animated?
	std::string m_name;
	bool m_skinned;
	// items
	PEveSOFDataHullSpriteLineSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteLineSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteLineSet );


BLUE_CLASS( EveSOFDataHullBoosterItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullBoosterItem( IRoot* lockobj = NULL );
	~EveSOFDataHullBoosterItem() {}

	// per-hull data of a booster
	Matrix m_transform;
	Vector4 m_functionality;
	uint32_t m_atlasIndex0;
	uint32_t m_atlasIndex1;
	bool m_hasTrail;
};
TYPEDEF_BLUECLASS( EveSOFDataHullBoosterItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullBoosterItem );


BLUE_CLASS( EveSOFDataHullBooster ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullBooster( IRoot* lockobj = NULL );
	~EveSOFDataHullBooster() {}

	// per-hull data of a booster
	bool m_alwaysOn, m_hasTrails;
	PEveSOFDataHullBoosterItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullBooster );
BLUE_DECLARE_VECTOR( EveSOFDataHullBooster );


BLUE_CLASS( EveSOFDataHullArea ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullArea( IRoot* lockobj = NULL );
	~EveSOFDataHullArea() {}

	// data
	unsigned int m_index;
	unsigned int m_count;
	BlueSharedString m_name;
	BlueSharedString m_shader;
	unsigned int m_blockedMaterials;
	EveSOFDataArea::AreaType m_areaType;
	PEveSOFDataTextureVector m_textures;
	PEveSOFDataParameterVector m_parameters;
};
TYPEDEF_BLUECLASS( EveSOFDataHullArea );
BLUE_DECLARE_VECTOR( EveSOFDataHullArea );


BLUE_CLASS( EveSOFDataHullLocator ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLocator( IRoot* lockobj = NULL );
	~EveSOFDataHullLocator() {}

	// data
	BlueSharedString m_name;
	Matrix m_transform;
};
TYPEDEF_BLUECLASS( EveSOFDataHullLocator );
BLUE_DECLARE_VECTOR( EveSOFDataHullLocator );


BLUE_CLASS( EveSOFDataHullLocatorSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLocatorSet( IRoot* lockobj = NULL );
	~EveSOFDataHullLocatorSet() {}

	// data
	BlueSharedString m_name;
	PEveSOFDataTransformVector m_locators;
};
TYPEDEF_BLUECLASS( EveSOFDataHullLocatorSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullLocatorSet );


BLUE_CLASS( EveSOFDataHullChild ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullChild( IRoot* lockobj = NULL );
	~EveSOFDataHullChild() {}

	// data
	std::string m_name;
	std::string m_redFilePath;
	Tr2Lod m_lowestLodVisible;
	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scaling;
	int m_id;
	int m_groupIndex;
};
TYPEDEF_BLUECLASS( EveSOFDataHullChild );
BLUE_DECLARE_VECTOR( EveSOFDataHullChild );


BLUE_CLASS( EveSOFDataHullAnimation ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullAnimation( IRoot* lockobj = NULL );
	~EveSOFDataHullAnimation() {}

	std::string m_name;

	// modelRotationCurve control
	Vector4 m_startRotationValue;
	Vector4 m_endRotationValue;
	float m_startRotationTime;
	float m_endRotationTime;
	
	// modelTranslationCurve control
	Vector3 m_startTranslationValue;
	Vector3 m_endTranslationValue;
	float m_startTranslationTime;
	float m_endTranslationTime;
	
	// The id of the children whose partice systems are controlled by the animation
	int m_id;
	// The particle system spawn rates
	float m_startRate;
	float m_endRate;
};
TYPEDEF_BLUECLASS( EveSOFDataHullAnimation );
BLUE_DECLARE_VECTOR( EveSOFDataHullAnimation );

typedef uint32_t EveSOFDataDecalIndex;
BLUE_DECLARE_STRUCTURE_LIST( EveSOFDataDecalIndex );


BLUE_CLASS( EveSOFDataHullDecal ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullDecal( IRoot* lockobj = NULL );
	~EveSOFDataHullDecal() {}

	// decal type
	enum Usage
	{
		USAGE_STANDARD = 0,
		USAGE_KILLCOUNTER,
		USAGE_LOGO,
	};

	// per-hull data of a hull decal
	std::string m_name;
	Usage m_usage;
	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	std::string m_shader;
	int m_groupIndex, m_boneIndex;
	PEveSOFDataTextureVector m_textures;
	PEveSOFDataParameterVector m_parameters;
	PEveSOFDataDecalIndexStructureList m_indexBuffer;
};
TYPEDEF_BLUECLASS( EveSOFDataHullDecal );
BLUE_DECLARE_VECTOR( EveSOFDataHullDecal );


BLUE_CLASS( EveSOFDataHull ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHull( IRoot* lockobj = NULL );
	~EveSOFDataHull() {}

	// trinity output class
	enum BuildClass
	{
		BUILDCLASS_SHIP = 0,
		BUILDCLASS_MOBILE,
		BUILDCLASS_STATIONARY,
		BUILDCLASS_SWARM,

		BUILDCLASS_COUNT,
	};

	// impact effect type
	enum ImpactEffectType
	{
		IMPACTEFFECT_NONE = 0,
		IMPACTEFFECT_ELLIPSOID,
		IMPACTEFFECT_HULL,
	};

	// hull name
	std::string m_name;

	// description
	std::string m_description;

	// class
	BuildClass m_buildClass;

	// geometry
	std::string m_geometryResFilePath;
	Vector4 m_boundingSphere;
	Vector3 m_shapeEllipsoidCenter;
	Vector3 m_shapeEllipsoidRadius;
	bool m_isSkinned;
	bool m_enableDynamicBoundingSphere;

	// materials
	PEveSOFDataHullAreaVector m_opaqueAreas;
	PEveSOFDataHullAreaVector m_decalAreas;
	PEveSOFDataHullAreaVector m_transparentAreas;
	PEveSOFDataHullAreaVector m_additiveAreas;
	PEveSOFDataHullAreaVector m_depthAreas;
	PEveSOFDataHullAreaVector m_distortionAreas;

	// patterns
	EveSOFDataPatternPerHullPtr m_defaultPattern;

	// effects on ship
	PEveSOFDataHullSpriteSetVector m_spriteSets;
	PEveSOFDataHullSpotlightSetVector m_spotlightSets;
	PEveSOFDataHullPlaneSetVector m_planeSets;
	PEveSOFDataHullSpriteLineSetVector m_spriteLineSets;
	ImpactEffectType m_impactEffectType;

	// decals
	PEveSOFDataHullDecalVector m_hullDecals;

	// boosters
	EveSOFDataHullBoosterPtr m_booster;
	Vector3 m_audioPosition;

	// locators
	PEveSOFDataHullLocatorVector m_locatorTurrets;
	PEveSOFDataHullLocatorSetVector m_locatorSets;

	// children
	PEveSOFDataHullChildVector m_children;

	// instanced meshes
	PEveSOFDataInstancedMeshVector m_instancedMeshes;

	// animations
	PEveSOFDataHullAnimationVector m_animations;

	// model curves
	std::string m_modelRotationCurvePath;
	std::string m_modelTranslationCurvePath;
};
TYPEDEF_BLUECLASS( EveSOFDataHull );
BLUE_DECLARE_VECTOR( EveSOFDataHull );




// --------------------------------------------------------------------------------
// All data storage classes for per-faction data
// --------------------------------------------------------------------------------

BLUE_CLASS( EveSOFDataFactionSpriteSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionSpriteSet( IRoot* lockobj = NULL );
	~EveSOFDataFactionSpriteSet() {}

	// per-faction data of a spriteset
	int m_groupIndex;
	std::string m_name;
	Color m_color;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionSpriteSet );
BLUE_DECLARE_VECTOR( EveSOFDataFactionSpriteSet );


BLUE_CLASS( EveSOFDataFactionSpotlightSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionSpotlightSet( IRoot* lockobj = NULL );
	~EveSOFDataFactionSpotlightSet() {}

	// per-faction data of a spotlight
	int m_groupIndex;
	std::string m_name;
	Color m_coneColor, m_spriteColor, m_flareColor;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionSpotlightSet );
BLUE_DECLARE_VECTOR( EveSOFDataFactionSpotlightSet );


BLUE_CLASS( EveSOFDataFactionPlaneSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionPlaneSet( IRoot* lockobj = NULL );
	~EveSOFDataFactionPlaneSet() {}

	// per-faction data of a planeset
	int m_groupIndex;
	std::string m_name;
	Color m_color;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionPlaneSet );
BLUE_DECLARE_VECTOR( EveSOFDataFactionPlaneSet );


BLUE_CLASS( EveSOFDataFactionDecal ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionDecal( IRoot* lockobj = NULL );
	~EveSOFDataFactionDecal() {}

	// group
	int m_groupIndex;
	bool m_isVisible;
	std::string m_name;
	std::string m_shader;
	PEveSOFDataParameterVector m_parameters;
	PEveSOFDataTextureVector m_textures;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionDecal );
BLUE_DECLARE_VECTOR( EveSOFDataFactionDecal );


BLUE_CLASS( EveSOFDataFactionChild ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionChild( IRoot* lockobj = NULL );
	~EveSOFDataFactionChild() {}

	// group
	int m_groupIndex;
	bool m_isVisible;
	std::string m_name;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionChild );
BLUE_DECLARE_VECTOR( EveSOFDataFactionChild );


BLUE_CLASS( EveSOFDataFactionHullArea ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionHullArea( IRoot* lockobj = NULL );
	~EveSOFDataFactionHullArea() {}

	// designation
	BlueSharedString m_name;
	// list of params
	PEveSOFDataParameterVector m_parameters;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionHullArea );
BLUE_DECLARE_VECTOR( EveSOFDataFactionHullArea );


BLUE_CLASS( EveSOFDataFaction ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFaction( IRoot* lockobj = NULL );
	~EveSOFDataFaction() {}

	// description
	std::string m_description;

	// faction name
	std::string m_name;

	// res path insert for pgs maps
	std::string m_resPathInsert;

	// data
	PEveSOFDataFactionHullAreaVector m_areas;
	PEveSOFDataFactionSpriteSetVector m_spriteSets;
	PEveSOFDataFactionSpotlightSetVector m_spotlightSets;
	PEveSOFDataFactionPlaneSetVector m_planeSets;
	PEveSOFDataFactionDecalVector m_decals;
	PEveSOFDataFactionChildVector m_children;

	// material usage
	int m_materialUsageMtl1;
	int m_materialUsageMtl2;
	int m_materialUsageMtl3;
	int m_materialUsageMtl4;

	// material lib names
	EveSOFDataAreaPtr m_areaTypes;

	// default pattern
	EveSOFDataPatternLayerPtr m_defaultPattern;
	std::string m_defaultPatternLayer1MaterialName;
};
TYPEDEF_BLUECLASS( EveSOFDataFaction );
BLUE_DECLARE_VECTOR( EveSOFDataFaction );




// --------------------------------------------------------------------------------
// All data storage classes for per-race data
// --------------------------------------------------------------------------------

BLUE_CLASS( EveSOFDataBoosterShape ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataBoosterShape( IRoot* lockobj = NULL );

	float m_noiseFunction;
	float m_noiseSpeed;
	Vector4 m_noiseAmplitureStart;
	Vector4 m_noiseAmplitureEnd;
	Vector4 m_noiseFrequency;
	Color m_color;
};
TYPEDEF_BLUECLASS( EveSOFDataBoosterShape );


BLUE_CLASS( EveSOFDataBooster ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataBooster( IRoot* lockobj = NULL );
	~EveSOFDataBooster() {}

	// data
	float m_glowScale, m_symHaloScale, m_haloScaleX, m_haloScaleY;
	Color m_glowColor, m_haloColor, m_trailColor;
	Color m_warpGlowColor;
	Color m_warpHaloColor;
	Vector4 m_trailSize, m_scale;

	EveSOFDataBoosterShapePtr m_shape0, m_shape1;
	EveSOFDataBoosterShapePtr m_warpShape0, m_warpShape1;
	std::string m_shapeAtlasResPath;
	std::string m_gradient0ResPath;
	std::string m_gradient1ResPath;
	uint32_t m_shapeAtlasHeight;
	uint32_t m_shapeAtlasCount;

	float m_lightOffset;
	float m_lightRadius;
	float m_lightWarpRadius;
	float m_lightFlickerAmplitude;
	float m_lightFlickerFrequency;
	Color m_lightColor;
	Color m_lightWarpColor;

	bool m_volumetric;
};
TYPEDEF_BLUECLASS( EveSOFDataBooster );


BLUE_CLASS( EveSOFDataRaceDamage ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataRaceDamage( IRoot* lockobj = NULL );
	~EveSOFDataRaceDamage() {}

	// armor damage
	PEveSOFDataParameterVector m_armorImpactParameters;
	PEveSOFDataTextureVector m_armorImpactTextures;

	// shield damage
	PEveSOFDataParameterVector m_shieldImpactParameters;
	PEveSOFDataTextureVector m_shieldImpactTextures;
};
TYPEDEF_BLUECLASS( EveSOFDataRaceDamage );
BLUE_DECLARE_VECTOR( EveSOFDataRaceDamage );


BLUE_CLASS( EveSOFDataRace ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataRace( IRoot* lockobj = NULL );
	~EveSOFDataRace() {}

	// race name
	std::string m_name;

	// data
	EveSOFDataBoosterPtr m_booster;

	// hull area data
	PEveSOFDataFactionHullAreaVector m_hullAreas;

	// impact effect
	EveSOFDataRaceDamagePtr m_damage;
};
TYPEDEF_BLUECLASS( EveSOFDataRace );
BLUE_DECLARE_VECTOR( EveSOFDataRace );






// --------------------------------------------------------------------------------
// All data storage classes for per material data
// --------------------------------------------------------------------------------

BLUE_CLASS( EveSOFDataMaterial ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataMaterial( IRoot* lockobj = NULL );
	~EveSOFDataMaterial() {}

	// material name
	std::string m_name;

	// list of params
	PEveSOFDataParameterVector m_parameters;
};
TYPEDEF_BLUECLASS( EveSOFDataMaterial );
BLUE_DECLARE_VECTOR( EveSOFDataMaterial );









// --------------------------------------------------------------------------------
// All data storage classes for generic data
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOFDataGenericString ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericString( IRoot* lockobj = NULL );
	~EveSOFDataGenericString() {}

	std::string m_str;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericString );
BLUE_DECLARE_VECTOR( EveSOFDataGenericString );

BLUE_CLASS( EveSOFDataGenericDamage ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericDamage( IRoot* lockobj = NULL );
	~EveSOFDataGenericDamage() {}

	// hull damage flicker
	float m_flickerPerlinSpeed;
	float m_flickerPerlinAlpha;
	float m_flickerPerlinBeta;
	int m_flickerPerlinN;

	// armor damage particles
	float m_armorParticleRate;
	float m_armorParticleAngle;
	Vector2 m_armorParticleMinMaxSpeed;
	Vector2 m_armorParticleMinMaxLifeTime;
	Vector4 m_armorParticleSizes;
	Color m_armorParticleColor0;
	Color m_armorParticleColor1;
	Color m_armorParticleColor2;
	Color m_armorParticleColor3;
	uint32_t m_armorParticleTextureIndex;
	float m_armorParticleVelocityStretchRotation;
	float m_armorParticleDrag;
	float m_armorParticleTurbulenceAmplitude;
	uint32_t m_armorParticleTurbulenceFrequency;
	float m_armorParticleColorMidPoint;

	// resources
	std::string m_armorShader;
	std::string m_shieldShaderEllipsoid;
	std::string m_shieldShaderHull;
	std::string m_shieldGeometryResFilePath;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericDamage );
BLUE_DECLARE_VECTOR( EveSOFDataGenericDamage );


BLUE_CLASS( EveSOFDataGenericHullDamage ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericHullDamage( IRoot* lockobj = NULL );
	~EveSOFDataGenericHullDamage() {}
	
	// hull debris particles
	float m_hullParticleRate;
	float m_hullParticleInnerAngle;
	float m_hullParticleAngle;
	float m_hullParticleColorMidpoint;
	Vector2 m_hullParticleMinMaxSpeed;
	Vector2 m_hullParticleMinMaxLifeTime;
	Vector4 m_hullParticleSizes;
	Color m_hullParticleColor0;
	Color m_hullParticleColor1;
	Color m_hullParticleColor2;
	Color m_hullParticleColor3;
	uint32_t m_hullParticleTextureIndex;
	float m_hullParticleVelocityStretchRotation;
	float m_hullParticleDrag;
	float m_hullParticleTurbulenceAmplitude;
	uint32_t m_hullParticleTurbulenceFrequency;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericHullDamage );
BLUE_DECLARE_VECTOR( EveSOFDataGenericHullDamage );

BLUE_CLASS( EveSOFDataGenericShader ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericShader( IRoot* lockobj = NULL );
	~EveSOFDataGenericShader() {}

	BlueSharedString m_shader;

	// complete list of parameters
	PEveSOFDataGenericStringVector m_parameters;

	// default textures & parameters
	PEveSOFDataTextureVector m_defaultTextures;
	PEveSOFDataParameterVector m_defaultParameters;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericShader );
BLUE_DECLARE_VECTOR( EveSOFDataGenericShader );


BLUE_CLASS( EveSOFDataGenericSwarm ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericSwarm( IRoot* lockobj = NULL ) {}
	~EveSOFDataGenericSwarm() {}

	EveSwarm::BehaviorProperties m_behavior;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericSwarm );
BLUE_DECLARE_VECTOR( EveSOFDataGenericSwarm );


BLUE_CLASS( EveSOFDataGenericVariant ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericVariant( IRoot* lockobj = NULL );
	~EveSOFDataGenericVariant() {}

	// name id
	BlueSharedString m_name;

	// tranparent
	bool m_isTransparent;

	// the area data
	EveSOFDataHullAreaPtr m_hullArea;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericVariant );
BLUE_DECLARE_VECTOR( EveSOFDataGenericVariant );


BLUE_CLASS( EveSOFDataGeneric ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGeneric( IRoot* lockobj = NULL );
	~EveSOFDataGeneric() {}

	// shader locations
	std::string m_shaderPrefix;
	std::string m_shaderPrefixAnimated;
	std::string m_areaShaderLocation;
	std::string m_decalShaderLocation;

	// shader material pre-fixes
	PEveSOFDataGenericStringVector m_materialPrefixes;
	PEveSOFDataGenericStringVector m_patternMaterialPrefixes;

	// shader-specific data
	PEveSOFDataGenericShaderVector m_areaShaders;
	PEveSOFDataGenericShaderVector m_decalShaders;

	// damage data
	EveSOFDataGenericDamagePtr m_damage;
	EveSOFDataGenericHullDamagePtr m_hullDamage;
	
	// swarm data
	EveSOFDataGenericSwarmPtr m_swarm;

	// generic wreck data
	PEveSOFDataFactionHullAreaVector m_hullAreas;
	EveSOFDataAreaMaterialPtr m_genericWreckMaterial;

	// effect data
	PEveSOFDataGenericVariantVector m_variants;
};
TYPEDEF_BLUECLASS( EveSOFDataGeneric );



BLUE_CLASS( EveSOFData ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFData( IRoot* lockobj = NULL );
	~EveSOFData();

	// global data
	EveSOFDataGenericPtr m_generic;
	// hull data
	PEveSOFDataHullVector m_hull;
	// faction data
	PEveSOFDataFactionVector m_faction;
	// race data
	PEveSOFDataRaceVector m_race;
	// material data
	PEveSOFDataMaterialVector m_material;
	// pattern data
	PEveSOFDataPatternVector m_pattern;
};
TYPEDEF_BLUECLASS( EveSOFData );



#endif // EveSOFData_H