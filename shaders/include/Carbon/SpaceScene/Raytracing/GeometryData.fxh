// Copyright © 2026 CCP ehf.

#ifndef CARBON_RAYTRACING_GEOMETRYDATA_FXH
#define CARBON_RAYTRACING_GEOMETRYDATA_FXH

#include "../../PackedTangents.fxh"
#include "../../VertexBufferUtil.fxh"

// Constant buffer layout for data describing the geometry of a mesh for ray tracing shaders. 
// This is used to read vertex and index data from buffers in a shader.
// Normally the constant buffer for this data is declared as:
//  cbuffer RtVertexBufferData : register( b5, space8 ) { RayTracingGeometryData geometryData; }
struct RayTracingGeometryData
{
    // Index into the buffer heap view array for the index buffer of the mesh.
    uint indexBufferId;
    // Index buffer stride in bytes (2 for 16-bit indices, 4 for 32-bit indices).
    uint indexBufferStride;

    // Offset in bytes to the start of the index data in the index buffer.
    uint indexOffset;

    // Index into the buffer heap view array for the vertex buffer of the mesh.
    uint vertexBufferId;
    // Vertex buffer stride in bytes.
    uint vertexBufferStride;

    // Offset in bytes to the start of the position (POSITION0 semantics) data in the vertex buffer.
    uint positionOffset;
    // Data type of the position attribute. See constants in VertexBufferUtil.fxh for possible values.
    uint positionType;
    
    // Offset in bytes to the start of the normal (NORMAL0 semantics) data in the vertex buffer.
    uint normalOffset;
    // Data type of the normal attribute. See constants in VertexBufferUtil.fxh for possible values.
    uint normalType;

    // Offset in bytes to the start of the tangent (TANGENT0 semantics) data in the vertex buffer.
    uint tangentOffset;
    // Data type of the tangent attribute. See constants in VertexBufferUtil.fxh for possible values.
    uint tangentType;

    // Offset in bytes to the start of the bitangent (BITANGENT0 semantics) data in the vertex buffer.
    uint bitangentOffset;
    // Data type of the bitangent attribute. See constants in VertexBufferUtil.fxh for possible values.
    uint bitangentType;

    // Offset in bytes to the start of the first texture coordinate (TEXCOORD0 semantics) data in the vertex buffer.
    uint texCoord0Offset;
    // Data type of the first texture coordinate attribute.
    uint texCoord0Type;

    // Offset in bytes to the start of the second texture coordinate (TEXCOORD1 semantics) data in the vertex buffer.
    uint texCoord1Offset;
    // Data type of the second texture coordinate attribute.
    uint texCoord1Type;

    // Offset in bytes to the start of the third texture coordinate (TEXCOORD2 semantics) data in the vertex buffer.
    uint texCoord2Offset;
    // Data type of the third texture coordinate attribute.
    uint texCoord2Type;

    uint padding;
};

// Reads trangle vertex indices from the index buffer for a given primitive index. Returns a uint3 containing the three vertex indices of the triangle.
// Handles both 16-bit and 32-bit index buffers based on the indexBufferStride in the RayTracingGeometryData. Assumes "triangle list" topology.
uint3 ReadIndices( Buffer<uint> indexBuffer, RayTracingGeometryData data, uint primitiveIndex )
{
    uint indicesAddress = ( 3 * primitiveIndex ) * data.indexBufferStride + data.indexOffset;
    if( data.indexBufferStride == 2 )
    {
        return ReadUInt16_3( indexBuffer, indicesAddress );
    }
    else
    {
        return ReadUInt32_3( indexBuffer, indicesAddress );
    }
}

// Reads the position of a vertex (POSITION0 semantics) from the vertex buffer for a given vertex index. Returns a float3 containing the position.
// Assumes positions are stored as float32_3 in the vertex buffer. The position offset and stride are taken from the RayTracingGeometryData.
float3 ReadPosition( Buffer<uint> vertexBuffer, RayTracingGeometryData data, uint vertexIndex )
{
    return ReadFloat32_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.positionOffset );
}

// Reads the normal, tangent, and bitangent of a vertex from the vertex buffer for a given vertex index. Returns a TangentSpace struct containing the three vectors.
// Supports both unpacked and packed tangent space representations based on the tangentType in the RayTracingGeometryData.
TangentSpace ReadNormalTangentBitangent( Buffer<uint> vertexBuffer, RayTracingGeometryData data, uint vertexIndex )
{
    TangentSpace ts;
    if( data.tangentType == FLOAT32_3 )
    {
        ts.normal = ReadFloat32_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.normalOffset );
        ts.tangent = ReadFloat32_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.tangentOffset );
        ts.bitangent = ReadFloat32_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.bitangentOffset );
    }
    else if( data.tangentType == FLOAT16_3 )
    {
        ts.normal = ReadFloat16_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.normalOffset );
        ts.tangent = ReadFloat16_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.tangentOffset );
        ts.bitangent = ReadFloat16_3( vertexBuffer, vertexIndex * data.vertexBufferStride + data.bitangentOffset );
    }
    else if( data.tangentType == UBYTE_4_NORM )
    {
		// untested...
        uint packed = vertexBuffer.Load( ( vertexIndex * data.vertexBufferStride + data.tangentOffset ) >> 2 );
        float4 tangents;
        tangents.x = float( ( packed >> 0 ) & 0xFF );
        tangents.y = float( ( packed >> 8 ) & 0xFF );
        tangents.z = float( ( packed >> 16 ) & 0xFF );
        tangents.w = float( ( packed >> 24 ) & 0xFF );
        tangents *= ( 1. / 255. );
        ts = UnpackTangentSpace( tangents );
    }
    else if( data.tangentType == USHORT_4_NORM )
    {
		// untested...
        uint packed0 = vertexBuffer.Load( ( vertexIndex * data.vertexBufferStride + data.tangentOffset + 0 ) >> 2 );
        uint packed1 = vertexBuffer.Load( ( vertexIndex * data.vertexBufferStride + data.tangentOffset + 4 ) >> 2 );
        float4 tangents;
        tangents.x = float( ( packed0 >> 0 ) & 0xFFFF );
        tangents.y = float( ( packed0 >> 16 ) & 0xFFFF );
        tangents.z = float( ( packed1 >> 0 ) & 0xFFFF );
        tangents.w = float( ( packed1 >> 16 ) & 0xFFFF );
        tangents *= ( 1. / 65535. );
        ts = UnpackTangentSpace( tangents );
    }
    else
    {
        ts.normal = 0;
        ts.tangent = 0;
        ts.bitangent = 0;
    }
    return ts;
}

// Reads the texture coordinate (TEXCOORD0 semantics) of a vertex from the vertex buffer for a given vertex index. Returns a float2 containing the texture coordinate.
float2 ReadTexcoord0( Buffer<uint> vertexBuffer, RayTracingGeometryData data, uint vertexIndex )
{
    return ReadFloat_2( vertexBuffer, vertexIndex * data.vertexBufferStride + data.texCoord0Offset, data.texCoord0Type );
}

// Reads the texture coordinate (TEXCOORD1 semantics) of a vertex from the vertex buffer for a given vertex index. Returns a float2 containing the texture coordinate.
float2 ReadTexcoord1( Buffer<uint> vertexBuffer, RayTracingGeometryData data, uint vertexIndex )
{
    return ReadFloat_2( vertexBuffer, vertexIndex * data.vertexBufferStride + data.texCoord1Offset, data.texCoord1Type );
}

// Reads the texture coordinate (TEXCOORD2 semantics) of a vertex from the vertex buffer for a given vertex index. Returns a float2 containing the texture coordinate.
float2 ReadTexcoord2( Buffer<uint> vertexBuffer, RayTracingGeometryData data, uint vertexIndex )
{
    return ReadFloat_2( vertexBuffer, vertexIndex * data.vertexBufferStride + data.texCoord2Offset, data.texCoord2Type );
}


#endif