// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2AtlasTexture.h"

BLUE_DEFINE_INTERFACE( ITr2Sprite2dTexture );
BLUE_DEFINE( Tr2Sprite2dTexture );

const Be::ClassInfo* Tr2Sprite2dTexture::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dTexture, "" )
		MAP_INTERFACE( ITr2Sprite2dTexture )
		MAP_INTERFACE( Tr2Sprite2dTexture )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name of this texture object",
			Be::READWRITE )

		MAP_PROPERTY(
			"resPath",
			GetResPath,
			SetResPath,
			"Resource path for texture" )

		MAP_PROPERTY(
			"srcX",
			GetSrcX,
			SetSrcX,
			"X-coordinate of top-left of source rectangle for this texture" )

		MAP_PROPERTY(
			"srcY",
			GetSrcY,
			SetSrcY,
			"Y-coordinate of top-left of source rectangle for this texture" )
		MAP_PROPERTY(
			"srcWidth",
			GetSrcWidth,
			SetSrcWidth,
			"Width of source rectangle for this texture" )

		MAP_PROPERTY(
			"srcHeight",
			GetSrcHeight,
			SetSrcHeight,
			"Height of source rectangle for this texture" )

		MAP_PROPERTY(
			"tileX",
			GetTileX,
			SetTileX,
			"If set, the texture is tiled along the x-axis rather than stretching" )

		MAP_PROPERTY(
			"tileY",
			GetTileY,
			SetTileY,
			"If set, the texture is tiled along the y-axis rather than stretching" )

		MAP_PROPERTY(
			"repeatMode",
			GetTextureRepeatMode,
			SetTextureRepeatMode,
			"0 - default, tiled\n"
			"1 - mirrored\n"
			"2 - clamp" )


		MAP_PROPERTY(
			"atlasTexture",
			GetAtlasTexture,
			SetAtlasTexture,
			"Atlas texture held by this texture object" )

		MAP_ATTRIBUTE(
			"useTransform",
			m_useTransform,
			"If set, the texture is transformed by the translation, rotation and scaling values.\n"
			"Otherwise no transformation takes place.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"translation",
			m_translation,
			"Translation applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"rotationCenter",
			m_rotationCenter,
			"Center point for rotation applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"rotation",
			m_rotation,
			"Rotation applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"scalingCenter",
			m_scalingCenter,
			"Center point for scale applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"scalingRotation",
			m_scalingRotation,
			"Scaling rotation factor for scale applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"scale",
			m_scale,
			"Scale applied to texture coordinates",
			Be::READWRITE | Be::NOTIFY )

	EXPOSURE_END()
}
