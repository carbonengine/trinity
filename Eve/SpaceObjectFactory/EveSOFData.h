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


struct EveSofDataMeshInstance
{
	Quaternion rotation;
	Vector3 scaling;
	Vector3 translation;
	int32_t boneIndex;
};

BLUE_DECLARE_STRUCTURE_LIST( EveSofDataMeshInstance );


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
	PEveSofDataMeshInstanceStructureList m_instances;
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

BLUE_CLASS( EveSOFDataFactionColorSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionColorSet( IRoot* lockobj = NULL );
	~EveSOFDataFactionColorSet() {}

	// color type
	enum ColorType
	{
		TYPE_PRIMARY = 0,
		TYPE_SECONDARY,
		TYPE_TERTIARY,
		TYPE_BLACK,
		TYPE_WHITE,
		TYPE_YELLOW,
		TYPE_ORANGE,
		TYPE_RED,
		TYPE_BLUE,
		TYPE_GREEN,
		TYPE_CYAN,
		TYPE_FIRE,
		TYPE_HULL,
		TYPE_GLASS,
		TYPE_REACTOR,
		TYPE_DARKHULL,
		TYPE_BOOSTER,
		TYPE_KILLMARK,
		TYPE_PRIMARY_LIGHT,
		TYPE_SECONDARY_LIGHT,
		TYPE_TERTIARY_LIGHT,
		TYPE_WHITE_LIGHT,

		TYPE_MAX,
	};

	// color data
	Color m_colors[TYPE_MAX];
};
TYPEDEF_BLUECLASS( EveSOFDataFactionColorSet );

BLUE_CLASS( EveSOFDataLogo ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataLogo( IRoot* lockobj = NULL );
	~EveSOFDataLogo() {}

	PEveSOFDataTextureVector m_textures;
};
TYPEDEF_BLUECLASS( EveSOFDataLogo );


BLUE_CLASS( EveSOFDataLogoSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataLogoSet( IRoot* lockobj = NULL );
	~EveSOFDataLogoSet() {}

	// color type
	enum LogoType
	{
		TYPE_PRIMARY = 0,
		TYPE_SECONDARY,
		TYPE_TERTIARY,
		TYPE_MARKING_01,
		TYPE_MARKING_02,
		TYPE_MAX
	};

	// logo data
	EveSOFDataLogoPtr m_logos[TYPE_MAX];
};
TYPEDEF_BLUECLASS( EveSOFDataLogoSet );

BLUE_CLASS( EveSOFDataAreaMaterial ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataAreaMaterial( IRoot* lockobj = NULL );
	~EveSOFDataAreaMaterial() {}

	// materials
	enum MaterialType
	{
		MATERIAL1 = 0,
		MATERIAL2,
		MATERIAL3,
		MATERIAL4,
		MATERIAL_MAX,
	};

	// data
	std::string m_material[MATERIAL_MAX];
	EveSOFDataFactionColorSet::ColorType m_glowColorType;
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
		TYPE_ROCK,
		TYPE_MONUMENT,

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
		USAGE_SPACE_VIDEO = 2,
		USAGE_HANGAR_VIDEO = 3,
		USAGE_HAZE = 5,
	};

	// data
	std::string m_name;
	bool m_skinned;
	std::string m_layer1MapResPath;
	std::string m_layer2MapResPath;
	std::string m_maskMapResPath;
	Usage m_usage;
	uint32_t m_atlasSize;
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
	float m_blinkRate, m_blinkPhase, m_minScale, m_maxScale, m_falloff, m_intensity;
	int m_boneIndex;
	EveSOFDataFactionColorSet::ColorType m_colorType;
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

	std::string m_name;
	// visibility group name
	BlueSharedString m_visibilityGroup;
	// animated?
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
	float m_spacing, m_blinkRate, m_blinkPhase, m_blinkPhaseShift, m_minScale, m_maxScale, m_falloff, m_intensity;
	int m_boneIndex;
	bool m_isCircle;
	EveSOFDataFactionColorSet::ColorType m_colorType;
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
	// visibility group name
	BlueSharedString m_visibilityGroup;
	// animated?
	bool m_skinned;
	// items
	PEveSOFDataHullSpriteLineSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteLineSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteLineSet );


BLUE_CLASS( EveSOFDataHullHazeSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullHazeSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullHazeSetItem() {}

	// per-hull data of a haze set
	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	EveSOFDataFactionColorSet::ColorType m_colorType;
	float m_hazeBrightness, m_hazeFalloff, m_sourceSize, m_sourceBrightness;
	bool m_boosterGainInfluence;
};
TYPEDEF_BLUECLASS( EveSOFDataHullHazeSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullHazeSetItem );


BLUE_CLASS( EveSOFDataHullHazeSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullHazeSet( IRoot* lockobj = NULL );
	~EveSOFDataHullHazeSet() {}

	// haze type
	enum HazeType
	{
		TYPE_SPHERICAL = 0,
		TYPE_HALFSPHERICAL,
	};

	// general
	std::string m_name;
	// type
	HazeType m_hazeType;
	// visibility group name
	BlueSharedString m_visibilityGroup;
	// items
	PEveSOFDataHullHazeSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullHazeSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullHazeSet );


BLUE_CLASS( EveSOFDataHullBannerLight ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullBannerLight( IRoot* lockobj = nullptr );

	float m_radiusMultiplier;
	float m_brightness;
	float m_innerRadiusMultiplier;
	float m_noiseAmplitude;
	float m_noiseFrequency;
	int   m_noiseOctaves;
	float m_saturation;
};
TYPEDEF_BLUECLASS( EveSOFDataHullBannerLight );
BLUE_DECLARE_VECTOR( EveSOFDataHullBannerLight );


BLUE_CLASS( EveSOFDataHullBanner ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	enum Usage
	{
		ALLIANCE_LOGO,
		CORP_LOGO,
		CEO_PORTRAIT,
		VERTICAL_BANNER,
		HORIZONTAL_BANNER,

		// gates
		TARGET_SYSTEM_ALLIANCE_LOGO,
		TARGET_SYSTEM_VERTICAL_BANNER,
		TARGET_SYSTEM_HORIZONTAL_BANNER,
		TARGET_SYSTEM_INFO_0,
		TARGET_SYSTEM_INFO_1,
		TARGET_SYSTEM_INFO_2,
		TARGET_SYSTEM_INFO_3,
		TARGET_SYSTEM_INFO_4,
		TARGET_SYSTEM_STATUS,
		CURRENT_SYSTEM_ALLIANCE_LOGO,
		CURRENT_SYSTEM_VERTICAL_BANNER,
		CURRENT_SYSTEM_HORIZONTAL_BANNER,

		// publicity structures
		PUBLICITY_POSTER,
		PUBLICITY_PORTRAIT,
		RECRUITMENT_INFORMATION_0,
		RECRUITMENT_INFORMATION_1,
		RECRUITMENT_INFORMATION_2,
		RECRUITMENT_INFORMATION_3,
		RECRUITMENT_INFORMATION_4,

		_USAGE_COUNT,
	};

	EveSOFDataHullBanner( IRoot* lockobj = nullptr );

	float GetAspectRatio() const;
	float GetTargetAspectRatio() const;

	float GetAngleX() const;
	void SetAngleX( float angle );
	float GetAngleY() const;
	void SetAngleY( float angle );
	Vector3 GetScaling() const;
	void SetScaling( const Vector3& scaling );

	std::string m_name;
	BlueSharedString m_visibilityGroup;

	Usage m_usage;

	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	EveSOFDataHullBannerLightPtr m_lightOverride;

	float m_angleX;
	float m_angleY;
	int32_t m_boneIndex;

	bool m_maintainAspectRatio;
};
TYPEDEF_BLUECLASS( EveSOFDataHullBanner );
BLUE_DECLARE_VECTOR( EveSOFDataHullBanner );


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
	float m_lightScale;
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

BLUE_CLASS( EveSOFDataHullDecalSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullDecalSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullDecalSetItem() {}

	// decal type
	enum Usage
	{
		USAGE_STANDARD = 0,
		USAGE_KILLCOUNTER,
		USAGE_HOLE,
		USAGE_CYLINDRICAL,
		USAGE_GLOWCYLINDRICAL,
		USAGE_GLOWSTANDARD,
		USAGE_LOGO,

		USAGE_MAX,
	};

	// per-hull data of a hull decal
	std::string m_name;
	Usage m_usage;
	EveSOFDataLogoSet::LogoType m_logoType;
	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	int m_boneIndex, m_meshIndex;
	EveSOFDataFactionColorSet::ColorType m_glowColorType;
	PEveSOFDataTextureVector m_textures;
	PEveSOFDataParameterVector m_parameters;
	PEveSOFDataDecalIndexStructureList m_indexBuffer;
};
TYPEDEF_BLUECLASS( EveSOFDataHullDecalSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullDecalSetItem );


BLUE_CLASS( EveSOFDataHullDecalSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullDecalSet( IRoot* lockobj = NULL );
	~EveSOFDataHullDecalSet() {}

	// general
	std::string m_name;
	// visibility group name
	BlueSharedString m_visibilityGroup;
	// items
	PEveSOFDataHullDecalSetItemVector m_items;
};

TYPEDEF_BLUECLASS( EveSOFDataHullDecalSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullDecalSet );


BLUE_CLASS( EveSOFDataHullLightSetItem ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLightSetItem( IRoot* lockobj = NULL );
	~EveSOFDataHullLightSetItem() {}
	
	enum LIGHT_TYPE 
	{
		POINT_LIGHT,
		TEXTURED_POINT_LIGHT,
		SPOT_LIGHT
	};

	struct HullLightSetItemData
	{
		Vector3 position;
		Quaternion rotation;
		float radius;
		float innerRadius;
		float brightness;
		float noiseAmplitude;
		float noiseFrequency;
		int noiseOctaves;
		EveSOFDataFactionColorSet::ColorType lightColor;
		std::wstring texturePath;
		float innerAngle;
		float outerAngle;
		LIGHT_TYPE type;
	} m_data;

	std::string m_name;

};
TYPEDEF_BLUECLASS( EveSOFDataHullLightSetItem );
BLUE_DECLARE_VECTOR( EveSOFDataHullLightSetItem );


BLUE_CLASS( EveSOFDataHullLightSetTexturedPointLight ) :
	public EveSOFDataHullLightSetItem
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLightSetTexturedPointLight( IRoot* lockobj = NULL );
	~EveSOFDataHullLightSetTexturedPointLight() {};
};
TYPEDEF_BLUECLASS( EveSOFDataHullLightSetTexturedPointLight );
BLUE_DECLARE_VECTOR( EveSOFDataHullLightSetTexturedPointLight );


BLUE_CLASS( EveSOFDataHullLightSetSpotLight ) :
	public EveSOFDataHullLightSetItem
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLightSetSpotLight( IRoot* lockobj = NULL );
	~EveSOFDataHullLightSetSpotLight() {};

};
TYPEDEF_BLUECLASS( EveSOFDataHullLightSetSpotLight );
BLUE_DECLARE_VECTOR( EveSOFDataHullLightSetSpotLight );


BLUE_CLASS( EveSOFDataHullLightSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullLightSet( IRoot* lockobj = NULL );
	~EveSOFDataHullLightSet() {}

	// general
	std::string m_name;
	// visibility group name
	BlueSharedString m_visibilityGroup;
	// items
	PEveSOFDataHullLightSetItemVector m_items;
};

TYPEDEF_BLUECLASS( EveSOFDataHullLightSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullLightSet );


BLUE_CLASS( EveSOFDataHullController ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	std::string GetName() const;

	std::string m_path;
};
TYPEDEF_BLUECLASS( EveSOFDataHullController );
BLUE_DECLARE_VECTOR( EveSOFDataHullController );


BLUE_CLASS( EveSOFDataHullSoundEmitter ): public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullSoundEmitter( IRoot* lockobj = NULL );
	~EveSOFDataHullSoundEmitter() {}

	std::string m_name;
	std::wstring m_prefix;
	Vector3 m_position;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSoundEmitter );
BLUE_DECLARE_VECTOR( EveSOFDataHullSoundEmitter );


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
	
	// hull category used for validation
	BlueSharedString m_category;

	// class
	BuildClass m_buildClass;

	// geometry
	std::string m_geometryResFilePath;
	Vector4 m_boundingSphere;
	Vector3 m_shapeEllipsoidCenter;
	Vector3 m_shapeEllipsoidRadius;
	bool m_isSkinned;
	bool m_enableDynamicBoundingSphere;
	bool m_castShadow;
	
	// materials
	PEveSOFDataHullAreaVector m_opaqueAreas;
	PEveSOFDataHullAreaVector m_decalAreas;
	PEveSOFDataHullAreaVector m_transparentAreas;
	PEveSOFDataHullAreaVector m_additiveAreas;
	PEveSOFDataHullAreaVector m_distortionAreas;

	// patterns
	EveSOFDataPatternPerHullPtr m_defaultPattern;

	// effects on ship
	PEveSOFDataHullSpriteSetVector m_spriteSets;
	PEveSOFDataHullSpotlightSetVector m_spotlightSets;
	PEveSOFDataHullPlaneSetVector m_planeSets;
	PEveSOFDataHullSpriteLineSetVector m_spriteLineSets;
	PEveSOFDataHullHazeSetVector m_hazeSets;
	PEveSOFDataHullBannerVector m_banners;
	PEveSOFDataHullDecalSetVector m_decalSets;
	PEveSOFDataHullLightSetVector m_lightSets;
	ImpactEffectType m_impactEffectType;

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

	PEveSOFDataHullSoundEmitterVector m_soundEmitters;

	PEveSOFDataHullControllerVector m_controllers;

	// model curves
	std::string m_modelRotationCurvePath;
	std::string m_modelTranslationCurvePath;
};
TYPEDEF_BLUECLASS( EveSOFDataHull );
BLUE_DECLARE_VECTOR( EveSOFDataHull );




// --------------------------------------------------------------------------------
// All data storage classes for per-faction data
// --------------------------------------------------------------------------------

BLUE_CLASS( EveSOFDataFactionVisibilityGroupSet ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionVisibilityGroupSet( IRoot* lockobj = NULL );
	~EveSOFDataFactionVisibilityGroupSet() {}

	// visibility groups
	PEveSOFDataGenericStringVector m_visibilityGroups;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionVisibilityGroupSet );
BLUE_DECLARE_VECTOR( EveSOFDataFactionVisibilityGroupSet );


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
	PEveSOFDataFactionSpotlightSetVector m_spotlightSets;
	PEveSOFDataFactionPlaneSetVector m_planeSets;
	PEveSOFDataFactionChildVector m_children;
	EveSOFDataFactionVisibilityGroupSetPtr m_visibilityGroupSet;
	EveSOFDataFactionColorSetPtr m_colorSet;
	EveSOFDataLogoSetPtr m_logoSet;

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
	// colors
	EveSOFDataFactionColorSet::ColorType m_hullPrimaryHeatColorType;
	EveSOFDataFactionColorSet::ColorType m_hullReactorHeatColorType;
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

	// decal/alpha/transparency texture
	BlueSharedString m_transparencyTextureName;
	bool m_doGenerateDepthArea;

	// complete list of parameters
	PEveSOFDataGenericStringVector m_parameters;

	// default textures & parameters
	PEveSOFDataTextureVector m_defaultTextures;
	PEveSOFDataParameterVector m_defaultParameters;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericShader );
BLUE_DECLARE_VECTOR( EveSOFDataGenericShader );

BLUE_CLASS( EveSOFDataGenericDecalShader ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataGenericDecalShader( IRoot* lockobj = NULL );
	~EveSOFDataGenericDecalShader() {}

	BlueSharedString m_shader;

	// complete list of parameters
	PEveSOFDataGenericStringVector m_parameters;

	// default textures
	PEveSOFDataTextureVector m_defaultTextures;

	// parent texture
	PEveSOFDataGenericStringVector m_parentTextures;
};
TYPEDEF_BLUECLASS( EveSOFDataGenericDecalShader );
BLUE_DECLARE_VECTOR( EveSOFDataGenericDecalShader );


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

	// default textures
	std::string m_resPathDefaultAlliance;
	std::string m_resPathDefaultCorp;
	std::string m_resPathDefaultCeo;

	// shader locations
	std::string m_shaderPrefix;
	std::string m_shaderPrefixAnimated;
	std::string m_areaShaderLocation;
	std::string m_decalShaderLocation;

	float m_decalMinScreenSizes[EveSOFDataHullDecalSetItem::USAGE_MAX];

	// shader material pre-fixes
	PEveSOFDataGenericStringVector m_materialPrefixes;
	PEveSOFDataGenericStringVector m_patternMaterialPrefixes;

	// shader-specific data
	PEveSOFDataGenericShaderVector m_areaShaders;
	PEveSOFDataGenericDecalShaderVector m_decalShaders;

	// damage data
	EveSOFDataGenericDamagePtr m_damage;
	EveSOFDataGenericHullDamagePtr m_hullDamage;
	
	// swarm data
	EveSOFDataGenericSwarmPtr m_swarm;

	// generic wreck data
	EveSOFDataAreaMaterialPtr m_genericWreckMaterial;

	PEveSOFDataGenericShader m_bannerShader;

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