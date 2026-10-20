// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelData.h"

BLUE_DEFINE( Tr2VoxelData );

const Be::ClassInfo* Tr2VoxelData::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelData, "" )

		MAP_ATTRIBUTE( "width", m_dimensions.width, "The width of the voxel data", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "height", m_dimensions.height, "The height of the voxel data", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "depth", m_dimensions.depth, "The depth of the voxel data", Be::READ | Be::PERSIST );

		MAP_PROPERTY_READONLY( "payloadChannelCount", GetPayloadChannelCount, "Number of channels in the payload" )

		MAP_METHOD_AND_WRAP( 
			"__init__", 
			__init__, 
			"Initialize the voxel data with the given dimensions and payload definition\n\n"
			":param width: The width of the voxel data\n"
			":param height: The height of the voxel data\n"
			":param depth: The depth of the voxel data\n"
			":param payload: The payload definition for the voxel data\n"
		);
		MAP_METHOD_AND_WRAP(
			"PopulateSparse",
			PopulateSparse,
			"Populate the voxel data with the given generator and remove functions\n\n"
			":param generator: A sequence of voxel data in a form of tuples (x, y, z, p0, p1, ...) where p# are payload elements\n"
			":type generator: Sequence[Tuple]\n"
		);
		MAP_METHOD_AND_WRAP(
			"SetVoxels",
			SetVoxels,
			"Set the voxel data with the given set and remove functions\n\n"
			":param mod: A sequence of voxel data in a form of tuples (x, y, z, p0, p1, ...) where p# are payload elements\n"
			":type mod: Sequence[Tuple]\n"
			":param remove: A sequence of voxel coordinates that are removed in a form of tuples (x, y, z)\n"
			":type remove: Sequence[Tuple[int, int, int]]\n" );
		MAP_METHOD_AND_WRAP( 
			"IsOccupied", 
			IsOccupied, 
			"Check if a voxel is occupied\n\n"
			":param x: The x coordinate of the voxel\n"
			":param y: The y coordinate of the voxel\n"
			":param z: The z coordinate of the voxel\n"
			":return: True if the voxel is occupied, False otherwise\n" )
	EXPOSURE_END()
}
