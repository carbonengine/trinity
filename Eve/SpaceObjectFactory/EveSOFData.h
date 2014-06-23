////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOFData_H
#define EveSOFData_H


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
	std::string m_name;
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
	std::string m_name;
};
TYPEDEF_BLUECLASS( EveSOFDataTexture );
BLUE_DECLARE_VECTOR( EveSOFDataTexture );


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
	int m_boneIndex, m_groupIndex;
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

	// data
	bool m_skinned;
	std::string m_layer1MapResPath;
	std::string m_layer2MapResPath;
	std::string m_maskMapResPath;
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
	bool m_skinned;
	// items
	PEveSOFDataHullSpriteSetItemVector m_items;
};
TYPEDEF_BLUECLASS( EveSOFDataHullSpriteSet );
BLUE_DECLARE_VECTOR( EveSOFDataHullSpriteSet );


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
	std::string m_name;
	std::string m_shaderPath;
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
	std::string m_name;
	Matrix m_transform;
};
TYPEDEF_BLUECLASS( EveSOFDataHullLocator );
BLUE_DECLARE_VECTOR( EveSOFDataHullLocator );


BLUE_CLASS( EveSOFDataHullChild ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullChild( IRoot* lockobj = NULL );
	~EveSOFDataHullChild() {}

	// data
	std::string m_redFilePath;
	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scaling;
	int m_id;
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


BLUE_CLASS( EveSOFDataHullDecal ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataHullDecal( IRoot* lockobj = NULL );
	~EveSOFDataHullDecal() {}

	// per-hull data of a hull decal
	Vector3 m_position, m_scaling;
	Quaternion m_rotation;
	std::string m_shaderPath;
	int m_usageID;
	PEveSOFDataTextureVector m_textures;
	PEveSOFDataParameterVector m_parameters;
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

	// hull name
	std::string m_name;

	// description
	std::string m_description;

	// geometry
	std::string m_geometryResFilePath;
	Vector4 m_boundingSphere;
	bool m_isSkinned;

	// materials
	PEveSOFDataHullAreaVector m_opaqueAreas;
	PEveSOFDataHullAreaVector m_transparentAreas;
	PEveSOFDataHullAreaVector m_additiveAreas;
	PEveSOFDataHullAreaVector m_depthAreas;
	PEveSOFDataHullAreaVector m_distortionAreas;

	// effects on ship
	PEveSOFDataHullSpriteSetVector m_spriteSets;
	PEveSOFDataHullSpotlightSetVector m_spotlightSets;
	PEveSOFDataHullPlaneSetVector m_planeSets;

	// decals
	PEveSOFDataHullDecalVector m_hullDecals;

	// boosters
	EveSOFDataHullBoosterPtr m_booster;

	// locators
	PEveSOFDataHullLocatorVector m_locatorTurrets;
	PEveSOFDataHullLocatorVector m_locatorAudio;

	// children
	PEveSOFDataHullChildVector m_children;

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

	// list of textures and params
	PEveSOFDataParameterVector m_parameters;
	PEveSOFDataTextureVector m_textures;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionDecal );
BLUE_DECLARE_VECTOR( EveSOFDataFactionDecal );


BLUE_CLASS( EveSOFDataFactionHullArea ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataFactionHullArea( IRoot* lockobj = NULL );
	~EveSOFDataFactionHullArea() {}

	// designation
	std::string m_name;
	// list of params
	PEveSOFDataParameterVector m_parameters;
};
TYPEDEF_BLUECLASS( EveSOFDataFactionHullArea );
BLUE_DECLARE_VECTOR( EveSOFDataFactionHullArea );


BLUE_CLASS( EveSOFDataBooster ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFDataBooster( IRoot* lockobj = NULL );
	~EveSOFDataBooster() {}

	// data
	float m_glowScale, m_symHaloScale, m_haloScaleX, m_haloScaleY;
	Color m_color, m_glowColor, m_haloColor, m_trailColor;
	std::string m_textureResPath;
	Vector4 m_trailSize, m_scale;
};
TYPEDEF_BLUECLASS( EveSOFDataBooster );


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
	PEveSOFDataFactionHullAreaVector m_opaqueAreas;
	PEveSOFDataFactionHullAreaVector m_transparentAreas;
	PEveSOFDataFactionHullAreaVector m_additiveAreas;
	PEveSOFDataFactionHullAreaVector m_depthAreas;
	PEveSOFDataFactionHullAreaVector m_distortionAreas;

	PEveSOFDataFactionSpriteSetVector m_spriteSets;
	PEveSOFDataFactionSpotlightSetVector m_spotlightSets;
	PEveSOFDataFactionPlaneSetVector m_planeSets;

	PEveSOFDataFactionDecalVector m_decalUsageData;
};
TYPEDEF_BLUECLASS( EveSOFDataFaction );
BLUE_DECLARE_VECTOR( EveSOFDataFaction );


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
};
TYPEDEF_BLUECLASS( EveSOFDataRace );
BLUE_DECLARE_VECTOR( EveSOFDataRace );


BLUE_CLASS( EveSOFData ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSOFData( IRoot* lockobj = NULL );
	~EveSOFData();

	// hull data
	PEveSOFDataHullVector m_hull;
	// faction data
	PEveSOFDataFactionVector m_faction;
	// race data
	PEveSOFDataRaceVector m_race;
};
TYPEDEF_BLUECLASS( EveSOFData );



#endif // EveSOFData_H