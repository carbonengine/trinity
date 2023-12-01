#pragma once
#ifndef GeometryUtils_h
#define GeometryUtils_h

struct granny_mesh;
struct granny_file;
struct granny_file_info;

void DescribeVertexDecl( unsigned int decl );
void DescribeVertexDecl( const Tr2VertexDefinition& vd );

void GetVertexPositionOffsetAndType(granny_mesh* grannyMesh, unsigned int &positionOffset, Tr2VertexDefinition::DataType &positionType);
void GetMeshVertexPosition(granny_mesh* grannyMesh, unsigned int index, Vector3 &p, unsigned int bytesPerVertex, unsigned int positionOffset, Tr2VertexDefinition::DataType positionType);

void ConvertShort4ToVector3( const void * src, Vector3* dest );
void ConvertUByte4ToVector3( const void * src, Vector3* dest );

// Read a Granny file inside a structured exception handler.
// This might help recover from reading a corrupt file.
granny_file* ProtectedGrannyReadEntireFileFromMemory( const wchar_t* path, uint32_t dataSize, void* data );

#endif