////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFData.h"
#include "Eve/SpaceObject/Attachments/Sets/EveBannerSet.h"

namespace
{
const BlueSharedString PRIMARY_VISIBILITY_GROUP( "primary" );
}

// --------------------------------------------------------------------------------
// Description:
//   x
// --------------------------------------------------------------------------------

EveSOFData::EveSOFData( IRoot* lockobj ) :
	PARENTLOCK( m_hull ),
	PARENTLOCK( m_faction ),
	PARENTLOCK( m_race ),
	PARENTLOCK( m_material ),
	PARENTLOCK( m_pattern ),
	PARENTLOCK( m_layout )
{}
EveSOFData::~EveSOFData()
{}


EveSOFDataAreaMaterial::EveSOFDataAreaMaterial( IRoot* lockobj ) :
	m_glowColorType( SOFDataFactionColorChooser::TYPE_HULL )
{}


EveSOFDataArea::EveSOFDataArea( IRoot* lockobj )
{}


EveSOFDataFactionColorSet::EveSOFDataFactionColorSet( IRoot* lockobj )
{
	std::fill( std::begin( m_colors ), std::end( m_colors ), Color( 0, 0, 0, 1 ) );

	// set default colors
	m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_BILLBOARD] = Color( 2.5f, 2.5f, 2.5f, 2.5f ); // default white with 2.5 in hdr
	m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_WARP_FX] = Color( uint32_t( 0xFFFF6333 ) ); // Artist authored default value
	m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_DOCKED_FX] = Color( uint32_t( 0xFF4C82E2 ) ); // Artist authored default value
	m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_ATTACK_FX] = Color( uint32_t( 0xFFFF180B ) ); // Artist authored default value
	m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_SIEGE_FX] = Color( uint32_t( 0xFFFF5E2D ) ); // Artist authored default value
}

EveSOFDataLogo::EveSOFDataLogo( IRoot* lockobj ):
	PARENTLOCK( m_textures )
{
}

EveSOFDataLogoSet::EveSOFDataLogoSet( IRoot* lockobj )
{
	for( uint32_t i = 0; i < TYPE_MAX; ++i )
	{
		m_logos[i] = EveSOFDataLogoPtr();
	}
}

EveSOFDataBlink::EveSOFDataBlink( IRoot* lockobj ) 
{
}

EveSOFDataBlinkType::EveSOFDataBlinkType( IRoot* lockobj )
{
	for ( uint32_t i = 0; i < TYPE_CYCLE; ++i )
	{
		m_blinkType[i] = EveSOFDataBlinkPtr(); 
	}
}

EveSOFDataParameter::EveSOFDataParameter( IRoot* lockobj ) :
	m_value( 0.f, 0.f, 0.f, 0.f )
{}


EveSOFDataFactionHullArea::EveSOFDataFactionHullArea( IRoot* lockobj ) :
	PARENTLOCK( m_parameters )
{}


EveSOFDataFactionChild::EveSOFDataFactionChild( IRoot* lockobj ) :
	m_groupIndex( -1 ),
	m_isVisible( false )
{}


EveSOFDataTexture::EveSOFDataTexture( IRoot* lockobj )
{}


static BlueStructureDefinition s_eveSOFMeshInstanceDef[] =
{
	{ "rotation",	 Be::FLOAT32_4,	0 },
	{ "scaling",	 Be::FLOAT32_3,	16 },
	{ "translation", Be::FLOAT32_3,	28 },
	{ "boneIndex",   Be::INT32_1,	40 },
	{ 0 }
};


EveSOFDataInstancedMesh::EveSOFDataInstancedMesh( IRoot* lockobj ) :
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_displayModifier( SHADER_ALL ),
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_instances )
{
	m_instances.SetStructureDefinition( s_eveSOFMeshInstanceDef );
}


EveSOFDataGenericString::EveSOFDataGenericString( IRoot* lockobj )
{}

EveSOFDataVisibilityGroup::EveSOFDataVisibilityGroup( IRoot* lockobj )
{}

EveSOFDataGenericShader::EveSOFDataGenericShader( IRoot* lockobj ) :
	m_doGenerateDepthArea( true ),
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_defaultTextures ),
	PARENTLOCK( m_defaultParameters )
{}


EveSOFDataGenericDecalShader::EveSOFDataGenericDecalShader( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_defaultTextures ),
	PARENTLOCK( m_parentTextures ),
	m_additive( false )
{}


EveSOFDataGenericDamage::EveSOFDataGenericDamage( IRoot* lockobj ) :
	m_flickerPerlinSpeed( 1.f ),
	m_flickerPerlinAlpha( 1.1f ),
	m_flickerPerlinBeta( 2.f ),
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
	m_armorParticleTurbulenceFrequency( 1 ),
	m_armorParticleColorMidPoint( 0.5f )
{}

EveSOFDataGenericHullDamage::EveSOFDataGenericHullDamage( IRoot* lockobj ) :
	m_hullParticleRate( 0.f ),
	m_hullParticleInnerAngle( 0.f ),
	m_hullParticleAngle( 0.f ),
	m_hullParticleColorMidpoint( 0.5f ),
	m_hullParticleMinMaxSpeed( 0.f, 0.f ),
	m_hullParticleMinMaxLifeTime( 0.f, 0.f ),
	m_hullParticleSizes( 0.f, 0.f, 0.f, 0.f ),
	m_hullParticleColor0( 0.f, 0.f, 0.f, 0.f ),
	m_hullParticleColor1( 0.f, 0.f, 0.f, 0.f ),
	m_hullParticleColor2( 0.f, 0.f, 0.f, 0.f ),
	m_hullParticleColor3( 0.f, 0.f, 0.f, 0.f ),
	m_hullParticleTextureIndex( 0 ),
	m_hullParticleVelocityStretchRotation( 0.f ),
	m_hullParticleDrag( 0.f ),
	m_hullParticleTurbulenceAmplitude( 0.f ),
	m_hullParticleTurbulenceFrequency( 1 )
{}

EveSOFDataGeneric::EveSOFDataGeneric( IRoot* lockobj ) :
	PARENTLOCK( m_materialPrefixes ),
	PARENTLOCK( m_patternMaterialPrefixes ),
	PARENTLOCK( m_areaShaders ),
	PARENTLOCK( m_decalShaders ),
	PARENTLOCK( m_variants ),
	PARENTLOCK( m_bannerShader ),
	PARENTLOCK( m_visibilityGroups ),
	PARENTLOCK( m_hullCategories )
{
	std::fill( std::begin( m_decalMinScreenSizes ), std::end( m_decalMinScreenSizes ), 0.f );
}


EveSOFDataGenericVariant::EveSOFDataGenericVariant( IRoot* lockobj ) :
	m_isTransparent( false )
	{}


EveSOFDataFaction::EveSOFDataFaction( IRoot* lockobj ) :
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_children ),
	m_materialUsageMtl1( 0 ),
	m_materialUsageMtl2( 1 ),
	m_materialUsageMtl3( 2 ),
	m_materialUsageMtl4( 3 )
{
}


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
	m_lightWarpColor( 0.f, 0.f, 0.f, 0.f )
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


std::string EveSOFDataHullController::GetName() const
{
	auto slash = m_path.rfind( '/' );
	if( slash == std::string::npos )
	{
		return "";
	}
	std::string result = m_path.substr( slash + 1 );
	auto dot = result.rfind( '.' );
	if( dot != std::string::npos )
	{
		result = result.substr( 0, dot );
	}
	return result;
}


EveSOFDataHull::EveSOFDataHull( IRoot* lockobj ) :
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_spriteLineSets ),
	PARENTLOCK( m_hazeSets ),
	PARENTLOCK( m_banners ),
	PARENTLOCK( m_bannerSets ),
	PARENTLOCK( m_decalSets ),
	PARENTLOCK( m_lightSets ),
	PARENTLOCK( m_opaqueAreas ),
	PARENTLOCK( m_decalAreas ),
	PARENTLOCK( m_transparentAreas ),
	PARENTLOCK( m_additiveAreas ),
	PARENTLOCK( m_distortionAreas ),
	PARENTLOCK( m_locatorTurrets ),
	PARENTLOCK( m_locatorSets ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_childSets ),
	PARENTLOCK( m_instancedMeshes ),
	PARENTLOCK( m_animations ),
	PARENTLOCK( m_soundEmitters ),
	PARENTLOCK( m_controllers ),
	m_buildClass( BUILDCLASS_SHIP ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_shapeEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shapeEllipsoidRadius( -1.f, -1.f, -1.f ),
	m_isSkinned( false ),
	m_enableDynamicBoundingSphere( false ),
	m_castShadow( true ),
	m_sof6( false ),
	m_impactEffectType( IMPACTEFFECT_NONE ),
	m_audioPosition( 0.f, 0.f, 0.f )
{}


EveSOFDataRace::EveSOFDataRace( IRoot* lockobj ) :
	m_hullPrimaryHeatColorType( SOFDataFactionColorChooser::TYPE_BOOSTER ),
	m_hullReactorHeatColorType( SOFDataFactionColorChooser::TYPE_REACTOR )
{}


EveSOFDataMaterial::EveSOFDataMaterial( IRoot* lockobj ) :
	PARENTLOCK( m_parameters )
{}

EveSOFDataPatternMaterialOverride::EveSOFDataPatternMaterialOverride( IRoot* lockobj ) :
	m_isTargetMtl1( true ),
	m_isTargetMtl2( true ),
	m_isTargetMtl3( true ),
	m_isTargetMtl4( true )
{
}

EveSOFDataPatternPerHull::EveSOFDataPatternPerHull( IRoot* lockobj )
{}


EveSOFDataPatternLayer::EveSOFDataPatternLayer(IRoot* lockobj) :
	m_projectionTypeU( PROJECTION_REPEAT ),
	m_projectionTypeV( PROJECTION_REPEAT ),
	m_materialSource( SOURCE_MATERIAL1 ),
	m_isTargetMtl1( true ),
	m_isTargetMtl2( true ),
	m_isTargetMtl3( true ),
	m_isTargetMtl4( true )
{}

EveSOFDataPatternLayerProperties::EveSOFDataPatternLayerProperties( IRoot* lockobj ) :
	m_projectionTypeU( PROJECTION_REPEAT ),
	m_projectionTypeV( PROJECTION_REPEAT ),
	m_isTargetMtl1( true ),
	m_isTargetMtl2( true ),
	m_isTargetMtl3( true ),
	m_isTargetMtl4( true )
{
	memset( m_applicableAreas, 1, EveSOFDataArea::AreaType::TYPE_MAX * sizeof( bool ) );
}

EveSOFDataPatternApplicationGroup::EveSOFDataPatternApplicationGroup( IRoot* lockobj ) :
	PARENTLOCK(m_projections)
{
}

EveSOFDataPatternTransform::EveSOFDataPatternTransform( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_isMirrored( false )
{}


EveSOFDataPattern::EveSOFDataPattern( IRoot* lockobj ) :
	PARENTLOCK( m_projections ),
	PARENTLOCK( m_applicationGroups ),
	m_sof6( false )
{
}


EveSOFDataHullArea::EveSOFDataHullArea( IRoot* lockobj ) :
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters ),
	m_index( 0 ),
	m_count( 1 ),
	m_blockedMaterials( 0 ),
	m_areaType( EveSOFDataArea::TYPE_PRIMARY )
{}


static BlueStructureDefinition s_eveSOFDecalIndexDef[] =
{ 
	{ "index",	Be::UINT32_1,	0 }, 
	{0} 
};


EveSOFDataHullLocator::EveSOFDataHullLocator( IRoot* lockobj )
	:m_transform( IdentityMatrix() )
{
}

EveSOFDataHullLocatorSetGroup::EveSOFDataHullLocatorSetGroup( IRoot* lockobj ) :
	PARENTLOCK( m_locatorSets )
{
}

EveSOFDataHullLocatorSet::EveSOFDataHullLocatorSet( IRoot* lockobj ) :
	PARENTLOCK( m_locators )
{
}

EveSOFDataTransform::EveSOFDataTransform( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_boneIndex( -1 )
{
}

EveSOFDataHullChildSetItem::EveSOFDataHullChildSetItem( IRoot* lockobj ) :
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_buildFilter( EveSOFDataHullBuildFilter::DEFAULT_FILTER )
{
}

std::string EveSOFDataHullChildSetItem::GetName()
{
	auto slash = m_redFilePath.rfind( '/' );
	if( slash == std::string::npos )
	{
		return "";
	}
	std::string result = m_redFilePath.substr( slash + 1 );
	auto dot = result.rfind( '.' );
	if( dot != std::string::npos )
	{
		result = result.substr( 0, dot );
	}
	return result;
}

EveSOFDataHullChildSet::EveSOFDataHullChildSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}

std::string EveSOFDataHullChildSet::GetName()
{
	return m_visibilityGroup.c_str();
}


EveSOFDataHullChild::EveSOFDataHullChild( IRoot* lockobj ) :
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_id( -1 ),
	m_groupIndex( -1 ),
	m_buildFilter( EveSOFDataHullBuildFilter::DEFAULT_FILTER )
{}

std::string EveSOFDataHullChild::GetName()
{
	auto slash = m_redFilePath.rfind( '/' );
	if( slash == std::string::npos )
	{
		return "";
	}
	std::string result = m_redFilePath.substr( slash + 1 );
	auto dot = result.rfind( '.' );
	if( dot != std::string::npos )
	{
		result = result.substr( 0, dot );
	}
	return result;
}


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

EveSOFDataPointLightAttachment::EveSOFDataPointLightAttachment( IRoot* lockobj ) :
	m_saturation( 1.0f ),
	m_intensity( 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_innerScaleMultiplier( 1.0f ),
	m_outerScaleMultiplier( 2.0f ),
	m_noiseAmplitude( 0.0 ),
	m_noiseFrequency( 1.0 ),
	m_noiseOctaves( 1 ),
	m_lightProfilePath( L"" ),
	m_rotation( Quaternion( 0, 0, 0, 1 ) )
{}

EveSOFDataSpotLightAttachment::EveSOFDataSpotLightAttachment( IRoot* lockobj ) :
	m_saturation( 1.0f ),
	m_intensity( 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_innerAngleMultiplier( 0.5f ),
	m_outerAngleMultiplier( 1.0f ),
	m_innerScaleMultiplier( 1.0f ),
	m_outerScaleMultiplier( 1.0f ),
	m_noiseAmplitude( 0.0 ),
	m_noiseFrequency( 1.0 ),
	m_noiseOctaves( 1 ),
	m_lightProfilePath( L"" )
{}

EveSOFDataHullSpotlightSet::EveSOFDataHullSpotlightSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP ),
	m_skinned( false ),
	m_zOffset( 0.f )
{}


EveSOFDataHullSpotlightSetItem::EveSOFDataHullSpotlightSetItem( IRoot* lockobj ) :
	m_boneIndex( 0 ), m_groupIndex( -1 ),
	m_boosterGainInfluence( false ),
	m_spriteScale( 1.f, 1.f, 1.f ),
	m_coneIntensity( 0.f ),
	m_flareIntensity( 0.f ),
	m_spriteIntensity( 0.f ),
	m_saturation( 1.f ),
	m_transform( IdentityMatrix() ),
	m_colorType( SOFDataFactionColorChooser::TYPE_HULL )
{
	m_light = nullptr;
}


EveSOFDataHullPlaneSet::EveSOFDataHullPlaneSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP ),
	m_skinned( false ),
	m_usage( USAGE_STANDARD ),
	m_atlasSize( 1 ),
	m_atlasAspectRatio( 1, 1 )
{}


EveSOFDataHullPlaneSetItem::EveSOFDataHullPlaneSetItem( IRoot* lockobj ) :
	PARENTLOCK( m_lights ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_colorType( SOFDataFactionColorChooser::TYPE_PRIMARY ),
	m_intensity( 1.f ),
	m_saturation( 1.f ),
	m_layer1Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer1Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_boneIndex( -1 ),
	m_groupIndex( -1 ),
	m_maskMapAtlasIndex( 0 ),
	// Blink data
	m_rate( 1.f ),
	m_phase( 0.f ),
	m_dutyCycle( 1.f ),
	m_blinkMode( 0 )
{
}


EveSOFDataHullSpriteSet::EveSOFDataHullSpriteSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}


EveSOFDataHullSpriteSetItem::EveSOFDataHullSpriteSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_blinkRate( 0.1f ), m_blinkPhase( 0.f ), m_minScale( 1.f ), m_maxScale( 10.f ), m_falloff( 0.f ), m_intensity( 1.f ),
	m_saturation( 1.f ),
	m_boneIndex( 0 ),
	m_colorType( SOFDataFactionColorChooser::TYPE_PRIMARY )
{
	m_light = nullptr;
}


EveSOFDataHullSpriteLineSet::EveSOFDataHullSpriteLineSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}

EveSOFDataHullSpriteLineSetItem::EveSOFDataHullSpriteLineSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ), m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_spacing( 1.f ), m_blinkRate( 0.1f ), m_blinkPhase( 0.f ), m_blinkPhaseShift( 0.f ), m_minScale( 1.f ), m_maxScale( 10.f ), m_falloff( 0.f ), m_intensity( 1.f ),
	m_saturation( 1.f ),
	m_boneIndex( 0 ),
	m_isCircle( false ),
	m_colorType( SOFDataFactionColorChooser::TYPE_PRIMARY )
{
	m_light = nullptr;
}

EveSOFDataHullHazeSet::EveSOFDataHullHazeSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP ),
	m_skinned( false ),
	m_hazeType( EveSOFDataHullHazeSet::TYPE_SPHERICAL )
{}

EveSOFDataHullHazeSetItem::EveSOFDataHullHazeSetItem( IRoot* lockobj ) :
	PARENTLOCK( m_lights ),
	m_position( 0.f, 0.f, 0.f ), m_scaling( 1.f, 1.f, 1.f ), 
	m_boneIndex( -1 ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_colorType( SOFDataFactionColorChooser::TYPE_PRIMARY ),
	m_hazeBrightness( 1.f ), m_hazeFalloff( 6.f ), m_sourceSize( 0.2f ), m_sourceBrightness( 2.f ),
	m_saturation( 1.f ),
	m_boosterGainInfluence( false )
{
}

/// <summary>
/// Banners
/// </summary>

EveSOFDataHullBannerLight::EveSOFDataHullBannerLight( IRoot* ) 
	:m_radiusMultiplier( 1 ),
	m_brightness( 1 ),
	m_innerRadiusMultiplier( 0.3 ),
	m_noiseAmplitude( 0 ),
	m_noiseFrequency( 1 ),
	m_noiseOctaves( 1 ),
	m_saturation( 1 )
{}

EveSOFDataHullBanner::EveSOFDataHullBanner( IRoot* )
	:m_usage( VERTICAL_BANNER ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP ),
	m_position( 0, 0, 0 ),
	m_scaling( 1, 1, 1 ),
	m_rotation( 0, 0, 0, 1 ),
	m_angleX( 0 ),
	m_angleY( 0 ),
	m_boneIndex( -1 ),
	m_maintainAspectRatio( true )
{
	m_lightOverride.CreateInstance();
}

float EveSOFDataHullBanner::GetTargetAspectRatio() const
{
	switch( m_usage )
	{
	case VERTICAL_BANNER:
	case TARGET_SYSTEM_VERTICAL_BANNER:
	case CURRENT_SYSTEM_VERTICAL_BANNER:
		return 0.25f;
	case PUBLICITY_POSTER:
		return 3.f / 4.f;
	case HORIZONTAL_BANNER:
	case TARGET_SYSTEM_HORIZONTAL_BANNER:
	case CURRENT_SYSTEM_HORIZONTAL_BANNER:
	case TARGET_SYSTEM_STATUS:
		return 4.f;
	default:
		return 1.f;
	}
}

float EveSOFDataHullBanner::GetAspectRatio() const
{
	EveBannerItem banner;
	banner.scaling = m_scaling;
	banner.angleX = m_angleX;
	banner.angleY = m_angleY;

	return EveBannerSet::GetBannerAspectRatio( banner );
}

float EveSOFDataHullBanner::GetAngleX() const
{
	return m_angleX;
}

void EveSOFDataHullBanner::SetAngleX( float angle )
{
	m_angleX = angle;
	if( m_maintainAspectRatio )
	{
		m_scaling.y *= GetAspectRatio() / GetTargetAspectRatio();
	}
}

float EveSOFDataHullBanner::GetAngleY() const
{
	return m_angleY;
}

void EveSOFDataHullBanner::SetAngleY( float angle )
{
	m_angleY = angle;
	if( m_maintainAspectRatio )
	{
		float ratio = GetAspectRatio();
		if( ratio != 0 )
		{
			m_scaling.x *= GetTargetAspectRatio() / ratio;
		}
	}
}

Vector3 EveSOFDataHullBanner::GetScaling() const
{
	return m_scaling;
}

void EveSOFDataHullBanner::SetScaling( const Vector3& scaling )
{
	m_scaling = scaling;
	if( m_maintainAspectRatio )
	{
		float ratio = GetAspectRatio();
		if( GetTargetAspectRatio() < 1 )
		{
			if( ratio != 0 )
			{
				m_scaling.x *= GetTargetAspectRatio() / ratio;
			}
		}
		else
		{
			m_scaling.y *= ratio / GetTargetAspectRatio();
		}
	}
}

EveSOFDataHullBannerSet::EveSOFDataHullBannerSet( IRoot* lockobj ):
	PARENTLOCK( m_banners ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}


std::string EveSOFDataHullBannerSet::GetName()
{
	return m_visibilityGroup.c_str();
}


EveSOFDataHullBannerSetItem::EveSOFDataHullBannerSetItem( IRoot* ) :
	m_usage( VERTICAL_BANNER ),
	m_position( 0, 0, 0 ),
	m_scaling( 1, 1, 1 ),
	m_rotation( 0, 0, 0, 1 ),
	m_angleX( 0 ),
	m_angleY( 0 ),
	m_boneIndex( -1 ),
	m_maintainAspectRatio( true )
{
	m_light = nullptr;
}

float EveSOFDataHullBannerSetItem::GetTargetAspectRatio() const
{
	switch( m_usage )
	{
	case VERTICAL_BANNER:
	case TARGET_SYSTEM_VERTICAL_BANNER:
	case CURRENT_SYSTEM_VERTICAL_BANNER:
		return 0.25f;
	case PUBLICITY_POSTER:
		return 3.f / 4.f;
	case HORIZONTAL_BANNER:
	case TARGET_SYSTEM_HORIZONTAL_BANNER:
	case CURRENT_SYSTEM_HORIZONTAL_BANNER:
	case TARGET_SYSTEM_STATUS:
		return 4.f;
	default:
		return 1.f;
	}
}

float EveSOFDataHullBannerSetItem::GetAspectRatio() const
{
	EveBannerItem banner;
	banner.scaling = m_scaling;
	banner.angleX = m_angleX;
	banner.angleY = m_angleY;

	return EveBannerSet::GetBannerAspectRatio( banner );
}

float EveSOFDataHullBannerSetItem::GetAngleX() const
{
	return m_angleX;
}

void EveSOFDataHullBannerSetItem::SetAngleX( float angle )
{
	m_angleX = angle;
	if( m_maintainAspectRatio )
	{
		m_scaling.y *= GetAspectRatio() / GetTargetAspectRatio();
	}
}

float EveSOFDataHullBannerSetItem::GetAngleY() const
{
	return m_angleY;
}

void EveSOFDataHullBannerSetItem::SetAngleY( float angle )
{
	m_angleY = angle;
	if( m_maintainAspectRatio )
	{
		float ratio = GetAspectRatio();
		if( ratio != 0 )
		{
			m_scaling.x *= GetTargetAspectRatio() / ratio;
		}
	}
}

Vector3 EveSOFDataHullBannerSetItem::GetScaling() const
{
	return m_scaling;
}

void EveSOFDataHullBannerSetItem::SetScaling( const Vector3& scaling )
{
	m_scaling = scaling;
	if( m_maintainAspectRatio )
	{
		float ratio = GetAspectRatio();
		if( GetTargetAspectRatio() < 1 )
		{
			if( ratio != 0 )
			{
				m_scaling.x *= GetTargetAspectRatio() / ratio;
			}
		}
		else
		{
			m_scaling.y *= ratio / GetTargetAspectRatio();
		}
	}
}

EveSOFDataHullDecalSet::EveSOFDataHullDecalSet( IRoot* lockobj ):
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}

EveSOFDataHullDecalSetItem::EveSOFDataHullDecalSetItem( IRoot* lockobj ) :
	m_usage( USAGE_STANDARD ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_boneIndex( -1 ),
	m_meshIndex( -1 ),
	m_glowColorType( SOFDataFactionColorChooser::TYPE_PRIMARY ),
	m_logoType( EveSOFDataLogoSet::TYPE_PRIMARY ),
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_indexBuffers )
{
}

EveSOFDataDecalIndexBuffer::EveSOFDataDecalIndexBuffer( IRoot* lockobj )
{
}

void EveSOFDataDecalIndexBuffer::AddIndex( uint32_t index )
{
	m_indexBuffer.push_back( index );
}

std::vector<uint32_t> EveSOFDataDecalIndexBuffer::GetIndices()
{
	return m_indexBuffer;
}

void EveSOFDataDecalIndexBuffer::GetWriteBufferAndSize( const char* propertyName, unsigned char** buffer, size_t* bufferSize )
{
	*buffer = reinterpret_cast<unsigned char*>( &m_indexBuffer[0] );
	*bufferSize = m_indexBuffer.size() * sizeof( uint32_t );
}

void EveSOFDataDecalIndexBuffer::ReleaseWriteBuffer( unsigned char* buffer )
{
}

void EveSOFDataDecalIndexBuffer::SetBufferAndSize( const char* propertyName, unsigned char* buffer, size_t bufferSize )
{
	// The set buffer will always be smaller than the allocated read buffer,
	// so we can just trivially resize our indexBuffer to be smaller
	m_indexBuffer.resize( bufferSize / sizeof( uint32_t ) );
}

unsigned char* EveSOFDataDecalIndexBuffer::AllocateReadBuffer( const char* memberName, size_t bufferSize )
{
	m_indexBuffer.resize( bufferSize / sizeof( uint32_t ) );
	return reinterpret_cast<unsigned char*>( &m_indexBuffer[0] );
}


EveSOFDataHullLightSet::EveSOFDataHullLightSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_visibilityGroup( PRIMARY_VISIBILITY_GROUP )
{}


EveSOFDataHullLightSetItem::EveSOFDataHullLightSetItem( IRoot* lockobj ) :
	m_name("")
{
	m_data.brightness = 0.0f;
	m_data.innerRadius = 0.0f;
	m_data.innerAngle = 0.0f;
	m_data.outerAngle = 0.0f;
	m_data.lightColor = SOFDataFactionColorChooser::TYPE_PRIMARY;
	m_data.noiseAmplitude = 0.0f;
	m_data.noiseFrequency = 1.0f;
	m_data.noiseOctaves = 1;
	m_data.position = Vector3( 0.f, 0.f, 0.f );
	m_data.radius = 0.0f;
	m_data.rotation = Quaternion( 0.f, 0.f, 0.f, 1.f );
	m_data.texturePath = L"";
	m_data.boneIndex = -1;
	m_data.flags = Tr2LightManager::FLAG_DEFAULT;
	m_data.type = POINT_LIGHT;
}

EveSOFDataHullLightSetTexturedPointLight::EveSOFDataHullLightSetTexturedPointLight( IRoot* lockobj ) :
	EveSOFDataHullLightSetItem( lockobj )
{
	m_data.type = TEXTURED_POINT_LIGHT;
}


EveSOFDataHullLightSetSpotLight::EveSOFDataHullLightSetSpotLight( IRoot* lockobj ) :
	EveSOFDataHullLightSetItem( lockobj )
{
	m_data.type = SPOT_LIGHT;
}


EveSOFDataFactionVisibilityGroupSet::EveSOFDataFactionVisibilityGroupSet( IRoot* lockobj ) :
	PARENTLOCK( m_visibilityGroups )
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
	m_atlasIndex1( 0 ),
	m_lightScale( 1 ),
	m_transform( IdentityMatrix() )
{
}

EveSOFDataHullSoundEmitter::EveSOFDataHullSoundEmitter( IRoot* ): 
	m_position( Vector3(0.0, 0.0, 0.0) ),
	m_rotation( Quaternion( 0.f, 0.f, 0.f, 1.f ) ),
	m_attenuationScalingFactor( 1.0f )
{
}

EveSOFDNADescriptor::EveSOFDNADescriptor( IRoot* ) :
	m_hull( "" ),
	m_faction( "" ),
	m_race( "" ),
	m_layout( "" ),
	m_pattern( "" ),
	m_material1( "" ),
	m_material2( "" ),
	m_material3( "" ),
	m_material4( "" )
{
}

EveSOFDataHullExtensionPlacement::EveSOFDataHullExtensionPlacement( IRoot*  lockobj ):
	PARENTLOCK( m_distributionConditions ),
	m_name( "" ),
	m_locatorSetName( "" ),
	m_offset( 0.f, 0.f, 0.f ),
	m_isInstanced( true ),
	m_extendsShieldEllipsoid( true ),
	m_extendsBoundingSphere( true ),
	m_enabled( true )
{
	m_distribution.CreateInstance();
	m_descriptor.CreateInstance();
}

EveSOFDataHullExtensionPlacementGroup::EveSOFDataHullExtensionPlacementGroup(IRoot* lockobj) :
	PARENTLOCK( m_placements ),
	PARENTLOCK( m_distributionConditions ),
	PARENTLOCK( m_depletionCounters ),
	m_name( "" ),
	m_enabled( true )
{
}


EveSOFDataHullExtensionBucket::EveSOFDataHullExtensionBucket( IRoot* lockobj ) :
	PARENTLOCK( m_depletionCounters ),
	PARENTLOCK( m_placements ),
	m_name( "" )
{
}

EveSOFDataHullExtensionPlacementDistributionParentMatch::EveSOFDataHullExtensionPlacementDistributionParentMatch( IRoot* )
{
	m_parentDescriptor.CreateInstance();
}

EveSOFDataDistributionDepletionCounter::EveSOFDataDistributionDepletionCounter( IRoot* ) :
	m_name( "" ),
	m_value( 1 )
{
}

EveSOFDataHullExtensionPlacementDistributionDepletionCounter::EveSOFDataHullExtensionPlacementDistributionDepletionCounter( IRoot* lockobj ) :
	PARENTLOCK( m_depletionCounters )
{
}

EveSOFDataHullExtensionPlacementDistributionRandomChance::EveSOFDataHullExtensionPlacementDistributionRandomChance( IRoot* ) :
	m_chanceOfUsage( 1.f )
{
}

EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings( IRoot* ) :
	m_displayFilter( SHADER_ALL )
{
}

EveSOFDataHullExtensionPlacementDistributionPlacement::EveSOFDataHullExtensionPlacementDistributionPlacement( IRoot* ) :
	m_name( "" ),
	m_completeness( 1.f ),
	m_placementBias( 0.f, 0.f, 0.f ),
	m_centerBias( 0.0f ),
	m_cap( 0 ),
	m_randomRotationStepSizeYPR( 0.008802f, 0.0086497f, 0.0086497f, 0.9998864f ),
	m_randomRotationMaxSteps( 0.f, 0.f, 0.f ),
	m_randomScaleMin( 1.f, 1.f, 1.f ),
	m_randomScaleMax( 1.f, 1.f, 1.f ),
	m_uniformScale( true ),
	m_occupyLocators( true )
{
}

EveSOFDataLayout::EveSOFDataLayout( IRoot* lockobj ) :
	PARENTLOCK( m_depletionCounters ),
	PARENTLOCK( m_placements ),
	m_randomizeSeedOnLoad( false ),
	m_seed( 1337 ),
	m_name( "" )
{
}
