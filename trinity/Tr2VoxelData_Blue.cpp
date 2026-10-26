// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelData.h"


Be::VarChooser Tr2VoxelDataTypeChooser[] = {
	{ "Boolean", BeCast( Tr2VoxelDataType::Boolean ), "" },
	{ "Float", BeCast( Tr2VoxelDataType::Float ), "" },
	{ "Float16", BeCast( Tr2VoxelDataType::Float16 ), "" },
	{ "UInt8", BeCast( Tr2VoxelDataType::UInt8 ), "" },
	{ "UInt16", BeCast( Tr2VoxelDataType::UInt16 ), "" },
	{ "UInt32", BeCast( Tr2VoxelDataType::UInt32 ), "" },
	{ "Int8", BeCast( Tr2VoxelDataType::Int8 ), "" },
	{ "Int16", BeCast( Tr2VoxelDataType::Int16 ), "" },
	{ "Int32", BeCast( Tr2VoxelDataType::Int32 ), "" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"Tr2VoxelDataType",
	Tr2VoxelDataType,
	Tr2VoxelDataTypeChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );


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
			Construct, 
			"Initialize the voxel data with the given dimensions and payload definition\n\n"
			":param dimensions: The dimensions of the voxel data (tuple of width, height, depth)\n"
			":param payload: The payload definition for the voxel data: a list of payload definitions dictionaries. Dictionary entries are:\n"
			"  type: The type of the payload channel (see trinity.Tr2VoxelDataType)\n"
			"  elementCount: The number of elements in the payload channel\n"
			"  normalized: Whether the payload channel is normalized\n"
		);
		MAP_METHOD_AND_WRAP(
			"SetVoxels",
			SetVoxels,
			"Populate the voxel data with the given generator\n\n"
			":param mod: A generator/sequence of voxel data in a form of tuples ((x, y, z), p0, p1, ...) where p# are payload elements\n"
			":type mod: Sequence[Tuple]\n" );
		MAP_METHOD_AND_WRAP( 
			"GetPayloadData", 
			PyGetPayloadData, 
			"Returns data for the specified payload channel at the given voxel coordinates\n\n"
			":param coord: The coordinates of the voxel (tuple of x, y, z)\n"
			":return: The payload data for the specified channel\n" )
	EXPOSURE_END()
}
