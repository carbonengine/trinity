// Copyright © 2023 CCP ehf.

#pragma once
#ifndef GeometryUtils_h
#define GeometryUtils_h

struct granny_mesh;
struct granny_file;
struct granny_file_info;

void DescribeVertexDecl( unsigned int decl );
void DescribeVertexDecl( const Tr2VertexDefinition& vd );

#if WITH_GRANNY
void GetVertexPositionOffsetAndType( granny_mesh* grannyMesh, unsigned int& positionOffset, Tr2VertexDefinition::DataType& positionType );
void GetMeshVertexPosition( granny_mesh* grannyMesh, unsigned int index, Vector3& p, unsigned int bytesPerVertex, unsigned int positionOffset, Tr2VertexDefinition::DataType positionType );
#endif

void ConvertShort4ToVector3( const void* src, Vector3* dest );
void ConvertUByte4ToVector3( const void* src, Vector3* dest );

#if WITH_GRANNY
// Read a Granny file inside a structured exception handler.
// This might help recover from reading a corrupt file.
granny_file* ProtectedGrannyReadEntireFileFromMemory( const wchar_t* path, uint32_t dataSize, void* data );

//////////////////////////////////////////////////////////////////////////
//
// Structures used for pulling bounds information out of extended data
// in the granny file. There are data type definitions here as well so
// we can use the future proof method of calling GrannyConvertTree to
// pull the data out, even if we change the processing done off-line.

struct BoundingBox
{
	granny_real32 min[3];
	granny_real32 max[3];
};

struct AreaBoundsInfo
{
	BoundingBox bounds;
	granny_int32 vertexCount;
};


#ifdef _WIN32
#pragma pack( push, 4 )
#endif

struct MeshBoundsInfo
{
	const char* typeName;
	BoundingBox bounds;
	granny_int32 areaCount;
	AreaBoundsInfo* areaInfos;
	granny_int32 sourceMeshIndex;
	granny_int32 maxScreenSize;
	granny_int32 uvDensityCount;
	granny_real32* uvDensities;
}
#ifndef _WIN32
// On non-windows x64 platforms areaInfos maybe 64bit aligned
__attribute__( ( packed ) )
#endif
;

#ifdef _WIN32
#pragma pack( pop )
#endif


extern granny_data_type_definition BoundingBoxType[];
extern granny_data_type_definition AreaBoundsInfoType[];
extern granny_data_type_definition UvDensityInfoType[];
extern granny_data_type_definition MeshBoundsInfoType[];

//
//////////////////////////////////////////////////////////////////////////
#endif

#endif