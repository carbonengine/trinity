// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelTexture.h"


BLUE_DEFINE( Tr2VoxelTexture );

const Be::ClassInfo* Tr2VoxelTexture::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelTexture, "Texture composed from voxel data" )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2TextureProvider )
		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "The voxel data associated with this texture" );
		MAP_ATTRIBUTE( "channel", m_channel, "The channel of the voxel data to use", Be::READWRITE | Be::NOTIFY );

		MAP_PROPERTY_READONLY( "width", GetWidth, "The width of the texture" );
		MAP_PROPERTY_READONLY( "height", GetHeight, "The height of the texture" );
		MAP_PROPERTY_READONLY( "depth", GetDepth, "The depth of the texture" );
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "The number of mip levels of the texture" );
		MAP_PROPERTY_READONLY( "format", GetFormat, "The pixel format of the texture" );
		MAP_PROPERTY_READONLY( "type", GetType, "The type of the texture" );
		MAP_PROPERTY_READONLY( "valid", IsValid, "Whether the texture generated from the voxel data is valid" );
	EXPOSURE_END();
}