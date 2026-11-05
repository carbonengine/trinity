// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelTexture.h"


BLUE_DEFINE( Tr2VoxelTexture );

const Be::ClassInfo* Tr2VoxelTexture::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelTexture, "Texture composed from voxel data" )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_ATTRIBUTE( "voxelData", m_voxelData, "The voxel data associated with this texture", Be::PERSISTONLY )
		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "The voxel data associated with this texture" )
		MAP_ATTRIBUTE( "channel", m_channel, "The channel of the voxel data to use", Be::READWRITE | Be::PERSIST | Be::NOTIFY );

		MAP_PROPERTY_READONLY( "width", GetWidth, "The width of the texture" );
		MAP_PROPERTY_READONLY( "height", GetHeight, "The height of the texture" );
		MAP_PROPERTY_READONLY( "depth", GetDepth, "The depth of the texture" );
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "The number of mip levels of the texture" );
		MAP_PROPERTY_READONLY( "format", GetFormat, "The pixel format of the texture" );
		MAP_PROPERTY_READONLY( "type", GetType, "The type of the texture" );
		MAP_PROPERTY_READONLY( "valid", IsValid, "Whether the texture generated from the voxel data is valid" );

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			2,
			"Initialize the voxel texture with the given voxel data and channel.\n"
			"Raises ValueError if the channel does not exist on the voxel data.\n"
			":param voxelData: a Tr2VoxelData (default None)\n"
			":param channel: index of the voxel data channel to use (default 0)" );
	EXPOSURE_END();
}