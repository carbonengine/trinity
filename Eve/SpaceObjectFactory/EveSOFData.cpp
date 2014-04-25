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
	PARENTLOCK( m_race )
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
	PARENTLOCK( m_textures )
{}


EveSOFDataTexture::EveSOFDataTexture( IRoot* lockobj )
{}


EveSOFDataFaction::EveSOFDataFaction( IRoot* lockobj ) :
	PARENTLOCK( m_opaqueAreas ),
	PARENTLOCK( m_transparentAreas ),
	PARENTLOCK( m_additiveAreas ),
	PARENTLOCK( m_depthAreas ),
	PARENTLOCK( m_distortionAreas ),
	PARENTLOCK( m_decalUsageData ),
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets )
{}


EveSOFDataBooster::EveSOFDataBooster( IRoot* lockobj ) :
	m_color( 0.f, 0.f, 0.f, 0.f ),
	m_scale( 1.f, 1.f, 1.f, 1.f ),
	m_glowScale( 1.f ),
	m_glowColor( 0.f, 0.f, 0.f, 0.f ),
	m_haloColor( 0.f, 0.f, 0.f, 0.f ),
	m_haloScaleX( 1.f ),
	m_haloScaleY( 1.f ),
	m_symHaloScale( 1.f ),
	m_trailColor( 0.f, 0.f, 0.f, 0.f ),
	m_trailSize( 0.f, 0.f, 0.f, 0.f )
{}


EveSOFDataHull::EveSOFDataHull( IRoot* lockobj ) :
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_hullDecals ),
	PARENTLOCK( m_opaqueAreas ),
	PARENTLOCK( m_transparentAreas ),
	PARENTLOCK( m_additiveAreas ),
	PARENTLOCK( m_depthAreas ),
	PARENTLOCK( m_distortionAreas ),
	PARENTLOCK( m_locatorTurrets ),
	PARENTLOCK( m_locatorAudio ),
	PARENTLOCK( m_children ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_isSkinned( false )
{}


EveSOFDataRace::EveSOFDataRace( IRoot* lockobj )
{}


EveSOFDataHullArea::EveSOFDataHullArea( IRoot* lockobj ) :
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters ),
	m_index( 0 ),
	m_count( 1 )
{}


EveSOFDataHullDecal::EveSOFDataHullDecal( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_usageID( -1 ),
	PARENTLOCK( m_textures ),
	PARENTLOCK( m_parameters )
{}


EveSOFDataHullLocator::EveSOFDataHullLocator( IRoot* lockobj )
{
	D3DXMatrixIdentity( &m_transform );
}


EveSOFDataHullChild::EveSOFDataHullChild( IRoot* lockobj ) :
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f )
{}


EveSOFDataHullSpotlightSet::EveSOFDataHullSpotlightSet( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_skinned( false ),
	m_zOffset( 0.f )
{}


EveSOFDataHullSpotlightSetItem::EveSOFDataHullSpotlightSetItem( IRoot* lockobj ) :
	m_boneIndex( 0 ), m_groupIndex( -1 ),
	m_boosterGainInfluence( false ),
	m_spriteScale( 1.f, 1.f, 1.f )
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
	m_layer1Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Transform( 0.f, 0.f, 0.f, 0.f ),
	m_layer1Scroll( 0.f, 0.f, 0.f, 0.f ),
	m_layer2Scroll( 0.f, 0.f, 0.f, 0.f )
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

EveSOFDataHullBooster::EveSOFDataHullBooster( IRoot* lockobj ) :
	PARENTLOCK( m_items ),
	m_alwaysOn( false ),
	m_hasTrails( true )
{}


EveSOFDataHullBoosterItem::EveSOFDataHullBoosterItem( IRoot* lockobj ) :
	m_functionality( 0.f, 1.f, 1.f, 1.f )
{
	D3DXMatrixIdentity( &m_transform );
}


