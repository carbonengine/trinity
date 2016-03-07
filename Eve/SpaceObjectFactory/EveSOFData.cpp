////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFData.h"

// --------------------------------------------------------------------------------
// Description:
//   x
// --------------------------------------------------------------------------------

EveSOFData::EveSOFData( IRoot* lockobj ) :
	PARENTLOCK( m_hull ),
	PARENTLOCK( m_faction ),
	PARENTLOCK( m_race ),
	PARENTLOCK( m_material )
{}
EveSOFData::~EveSOFData()
{}


EveSOFDataParameter::EveSOFDataParameter( IRoot* lockobj ) :
	m_value( 0.f, 0.f, 0.f, 0.f )
{}


EveSOFDataFactionHullArea::EveSOFDataFactionHullArea( IRoot* lockobj ) :
	PARENTLOCK( m_parameters )
{}


EveSOFDataFactionDecal::EveSOFDataFactionDecal( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_textures ),
	m_groupIndex( -1 ),
	m_isVisible( false )
{}


EveSOFDataTexture::EveSOFDataTexture( IRoot* lockobj )
{}


EveSOFDataInstancedMesh::EveSOFDataInstancedMesh( IRoot* lockobj ) :
	m_lowestLodVisible( TR2_LOD_LOW ),
	PARENTLOCK( m_textures )
{}


EveSOFDataGenericString::EveSOFDataGenericString( IRoot* lockobj )
{}


EveSOFDataGenericShader::EveSOFDataGenericShader( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_defaultTextures )
{}


EveSOFDataGenericDamage::EveSOFDataGenericDamage( IRoot* lockobj ) :
	m_flickerPerlinSpeed( 1.f ),
	m_flickerPerlinAlpha( 1.1f ),
	m_flickerPerlinBeta( 2.f ),
	m_flickerPerlinOffset( 0.f ),
	m_flickerPerlinScale( 1.f ),
	m_flickerPerlinN( 3 ),
	m_armorParticleRate( 0.f ),
	m_armorParticleAngle( 0.f ),
	m_armorParticleMinMaxSpeed( 0.f, 0.f ),
	m_armorParticleMinMaxLifeTime( 0.f, 0.f ),
	m_armorParticleSizes( 0.f, 0.f, 0.f, 0.f ),
	m_armorParticleColor0( 0.f, 0.f, 0.f, 0.f ),
	m_armorParticleColor1( 0.f, 0.f, 0.f, 0.f ),
	m_armorParticleColor2( 0.f, 0.f, 0.f, 0.f ),
	m_armorParticleColor3( 0.f, 0.f, 0.f, 0.f ),
	m_armorParticleTextureIndex( 0 ),
	m_armorParticleVelocityStretchRotation( 0.f ),
	m_armorParticleDrag( 0.f ),
	m_armorParticleTurbulenceAmplitude( 0.f ),
	m_armorParticleTurbulenceFrequency( 1 )
{}


EveSOFDataGeneric::EveSOFDataGeneric( IRoot* lockobj ) :
	PARENTLOCK( m_materialPrefixes ),
	PARENTLOCK( m_areaShaders ),
	PARENTLOCK( m_decalShaders ),
	PARENTLOCK( m_hullAreas ),
	PARENTLOCK( m_variants )
{}


EveSOFDataFaction::EveSOFDataFaction( IRoot* lockobj ) :
	PARENTLOCK( m_areas ),
	PARENTLOCK( m_decals ),
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	m_materialUsageMtl1( 0 ),
	m_materialUsageMtl2( 1 ),
	m_materialUsageMtl3( 2 ),
	m_materialUsageMtl4( 3 )
{}


EveSOFDataBoosterShape::EveSOFDataBoosterShape( IRoot* lockobj ) :
	m_noiseFunction( 0.f ),
	m_noiseSpeed( 0.f ),
	m_noiseAmplitureStart( 0.f, 0.f, 0.f, 0.f ),
	m_noiseAmplitureEnd( 0.f, 0.f, 0.f, 0.f ),
	m_noiseFrequency( 0.f, 0.f, 0.f, 0.f ),
	m_color( 0.f, 0.f, 0.f, 0.f )
{}
	

EveSOFDataBooster::EveSOFDataBooster( IRoot* lockobj ) :
	m_scale( 1.f, 1.f, 1.f, 1.f ),
	m_glowScale( 1.f ),
	m_glowColor( 0.f, 0.f, 0.f, 0.f ),
	m_haloColor( 0.f, 0.f, 0.f, 0.f ),
	m_warpGlowColor( 0.f, 0.f, 0.f, 0.f ),
	m_warpHaloColor( 0.f, 0.f, 0.f, 0.f ),
	m_haloScaleX( 1.f ),
	m_haloScaleY( 1.f ),
	m_symHaloScale( 1.f ),
	m_trailColor( 0.f, 0.f, 0.f, 0.f ),
	m_trailSize( 0.f, 0.f, 0.f, 0.f ),
	m_shapeAtlasHeight( 0 ),
	m_shapeAtlasCount( 0 ),
	m_lightOffset( 0.f ),
	m_lightRadius( 0.f ),
	m_lightWarpRadius( 0.f ),
	m_lightFlickerAmplitude( 0.f ),
	m_lightFlickerFrequency( 0.f ),
	m_lightColor( 0.f, 0.f, 0.f, 0.f ),
	m_lightWarpColor( 0.f, 0.f, 0.f, 0.f ),
	m_volumetric( false )
{
	m_shape0.CreateInstance();
	m_shape1.CreateInstance();
	m_warpShape0.CreateInstance();
	m_warpShape1.CreateInstance();
}


EveSOFDataRaceDamage::EveSOFDataRaceDamage( IRoot* lockobj ) :
	PARENTLOCK( m_armorImpactParameters ),
	PARENTLOCK( m_armorImpactTextures),
	PARENTLOCK( m_shieldImpactParameters ),
	PARENTLOCK( m_shieldImpactTextures )
{
}


EveSOFDataHull::EveSOFDataHull( IRoot* lockobj ) :
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_spriteLineSets ),
	PARENTLOCK( m_hullDecals ),
	PARENTLOCK( m_opaqueAreas ),
	PARENTLOCK( m_decalAreas ),
	PARENTLOCK( m_transparentAreas ),
	PARENTLOCK( m_additiveAreas ),
	PARENTLOCK( m_depthAreas ),
	PARENTLOCK( m_distortionAreas ),
	PARENTLOCK( m_locatorTurrets ),
	PARENTLOCK( m_locatorSets ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_instancedMeshes ),
	PARENTLOCK( m_animations ),
	m_buildClass( BUILDCLASS_SHIP ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_shapeEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shapeEllipsoidRadius( -1.f, -1.f, -1.f ),
	m_isSkinned( false ),
	m_hasImpactEffect( false ),
	m_audioPosition( Vector3( 0.f, 0.f, 0.f ) )
{}


EveSOFDataRace::EveSOFDataRace( IRoot* lockobj ) :
	PARENTLOCK( m_hullAreas )
{}


EveSOFDataMaterial::EveSOFDataMaterial( IRoot* lockobj ) :
	PARENTLOCK( m_parameters )
{}


EveSOFDataHullArea::EveSOFDataHullArea( IRoot* lockobj ) :
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters ),
	m_index( 0 ),
	m_count( 1 ),
	m_blockedMaterials( 0 )
{}


static BlueStructureDefinition s_eveSOFDecalIndexDef[] =
{ 
	{ "index",	Be::UINT32_1,	0 }, 
	{0} 
};

EveSOFDataHullDecal::EveSOFDataHullDecal( IRoot* lockobj ) :
	m_usage( USAGE_STANDARD ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_groupIndex( -1 ),
	m_boneIndex( -1 ),
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_indexBuffer )
{
	m_indexBuffer.SetStructureDefinition( s_eveSOFDecalIndexDef );
}


EveSOFDataHullLocator::EveSOFDataHullLocator( IRoot* lockobj )
{
	D3DXMatrixIdentity( &m_transform );
}


EveSOFDataHullLocatorSet::EveSOFDataHullLocatorSet( IRoot* lockobj ) :
	PARENTLOCK( m_locators )
{
}


EveSOFDataTransform::EveSOFDataTransform( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_boneIndex( -1 )
{
}


EveSOFDataHullChild::EveSOFDataHullChild( IRoot* lockobj ) :
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_id( -1 )
{}


EveSOFDataHullAnimation::EveSOFDataHullAnimation( IRoot* lockobj ) :
	m_startRotationTime( -1.0f ),
	m_endRotationTime( -1.0f ),
	m_startRotationValue( 0.f, 0.f, 0.f, 1.f ),
	m_endRotationValue( 0.f, 0.f, 0.f, 1.f ),

	m_startTranslationTime( -1.0f ),
	m_endTranslationTime( -1.0f ),
	m_startTranslationValue( 0.f, 0.f, 0.f ),
	m_endTranslationValue( 0.f, 0.f, 0.f ),

	m_id( -1 ),
	m_startRate( -1 ),
	m_endRate( -1 )
{}


EveSOFDataHullSpotlightSet::EveSOFDataHullSpotlightSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false ),
	m_zOffset( 0.f )
{}


EveSOFDataHullSpotlightSetItem::EveSOFDataHullSpotlightSetItem( IRoot* lockobj ) :
	m_boneIndex( 0 ), m_groupIndex( -1 ),
	m_boosterGainInfluence( false ),
	m_spriteScale( 1.f, 1.f, 1.f ),
	m_coneIntensity( 0.f ),
	m_flareIntensity( 0.f ),
	m_spriteIntensity( 0.f )
{
	D3DXMatrixIdentity( &m_transform );
}


EveSOFDataHullPlaneSet::EveSOFDataHullPlaneSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false ),
	m_planeData( 1.f, 0.f, 0.f, 0.f )
{}


EveSOFDataHullPlaneSetItem::EveSOFDataHullPlaneSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_layer1Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer1Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_boneIndex( -1 ),
	m_groupIndex( -1 ),
	m_maskMapAtlasIndex( 0 )
{
}


EveSOFDataHullSpriteSet::EveSOFDataHullSpriteSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false )
{}


EveSOFDataHullSpriteSetItem::EveSOFDataHullSpriteSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_blinkRate( 0.1f ), m_blinkPhase( 0.0f ), m_minScale( 1.f ), m_maxScale( 10.f ), m_falloff( 0.f ),
	m_boneIndex( 0 ), m_groupIndex( -1 )
{}


EveSOFDataHullSpriteLineSet::EveSOFDataHullSpriteLineSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false )
{}


EveSOFDataHullSpriteLineSetItem::EveSOFDataHullSpriteLineSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ), m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_spacing( 1.f ), m_blinkRate( 0.1f ), m_blinkPhase( 0.f ), m_blinkPhaseShift( 0.f ), m_minScale( 1.f ), m_maxScale( 10.f ), m_falloff( 0.f ),
	m_boneIndex( 0 ), m_groupIndex( -1 )
{}


EveSOFDataFactionSpriteSet::EveSOFDataFactionSpriteSet( IRoot* lockobj ) :
	m_groupIndex( -1 ),
	m_color( 0.f, 0.f, 0.f, 0.f )
{}

EveSOFDataFactionSpotlightSet::EveSOFDataFactionSpotlightSet( IRoot* lockobj ) :
	m_groupIndex( -1 ),
	m_coneColor( 0.f, 0.f, 0.f, 0.f ),
	m_spriteColor( 0.f, 0.f, 0.f, 0.f ),
	m_flareColor( 0.f, 0.f, 0.f, 0.f )
{}

EveSOFDataFactionPlaneSet::EveSOFDataFactionPlaneSet( IRoot* lockobj ) :
	m_groupIndex( -1 ),
	m_color( 0.f, 0.f, 0.f, 0.f )
{}

EveSOFDataHullBooster::EveSOFDataHullBooster( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_alwaysOn( false ),
	m_hasTrails( true )
{}


EveSOFDataHullBoosterItem::EveSOFDataHullBoosterItem( IRoot* lockobj ) :
	m_functionality( 0.f, 1.f, 1.f, 1.f ),
	m_hasTrail( true ),
	m_atlasIndex0( 0 ),
	m_atlasIndex1( 0 )
{
	D3DXMatrixIdentity( &m_transform );
}


