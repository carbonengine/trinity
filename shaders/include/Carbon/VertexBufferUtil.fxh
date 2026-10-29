// Copyright © 2026 CCP ehf.

#ifndef CARBON_VERTEX_BUFFER_UTIL_FXH
#define CARBON_VERTEX_BUFFER_UTIL_FXH

// ------------------------------------------------------------------------
// --- mirrors Tr2VertexDefinition::DataType, see Tr2VertexDefinition.h ---
// ------------------------------------------------------------------------
static const uint DT_INT8 = 0u;
static const uint DT_INT16 = 1u;
static const uint DT_INT32 = 2u;
static const uint DT_FLOAT16 = 3u;
static const uint DT_FLOAT32 = 4u;
static const uint DT_TYPE_MASK = 7u;
static const uint DT_UNSIGNED_BIT = ( 1u << 3u );
static const uint DT_NORMALIZED_BIT = ( 1u << 4u );
static const uint DT_SIZE_OFFSET = 5u;
static const uint DT_SIZE_MASK = ( 3u << DT_SIZE_OFFSET );
static const uint DT_SIZE_1 = ( 0u << DT_SIZE_OFFSET );
static const uint DT_SIZE_2 = ( 1u << DT_SIZE_OFFSET );
static const uint DT_SIZE_3 = ( 2u << DT_SIZE_OFFSET );
static const uint DT_SIZE_4 = ( 3u << DT_SIZE_OFFSET );
static const uint BYTE_1 = ( DT_INT8 | DT_SIZE_1 );
static const uint BYTE_2 = ( DT_INT8 | DT_SIZE_2 );
static const uint BYTE_3 = ( DT_INT8 | DT_SIZE_3 );
static const uint BYTE_4 = ( DT_INT8 | DT_SIZE_4 );
static const uint UBYTE_1 = ( DT_INT8 | DT_SIZE_1 | DT_UNSIGNED_BIT );
static const uint UBYTE_2 = ( DT_INT8 | DT_SIZE_2 | DT_UNSIGNED_BIT );
static const uint UBYTE_3 = ( DT_INT8 | DT_SIZE_3 | DT_UNSIGNED_BIT );
static const uint UBYTE_4 = ( DT_INT8 | DT_SIZE_4 | DT_UNSIGNED_BIT );
static const uint SHORT_1 = ( DT_INT16 | DT_SIZE_1 );
static const uint SHORT_2 = ( DT_INT16 | DT_SIZE_2 );
static const uint SHORT_3 = ( DT_INT16 | DT_SIZE_3 );
static const uint SHORT_4 = ( DT_INT16 | DT_SIZE_4 );
static const uint USHORT_1 = ( DT_INT16 | DT_SIZE_1 | DT_UNSIGNED_BIT );
static const uint USHORT_2 = ( DT_INT16 | DT_SIZE_2 | DT_UNSIGNED_BIT );
static const uint USHORT_3 = ( DT_INT16 | DT_SIZE_3 | DT_UNSIGNED_BIT );
static const uint USHORT_4 = ( DT_INT16 | DT_SIZE_4 | DT_UNSIGNED_BIT );
static const uint INT32_1 = ( DT_INT32 | DT_SIZE_1 );
static const uint INT32_2 = ( DT_INT32 | DT_SIZE_2 );
static const uint INT32_3 = ( DT_INT32 | DT_SIZE_3 );
static const uint INT32_4 = ( DT_INT32 | DT_SIZE_4 );
static const uint UINT32_1 = ( DT_INT32 | DT_SIZE_1 | DT_UNSIGNED_BIT );
static const uint UINT32_2 = ( DT_INT32 | DT_SIZE_2 | DT_UNSIGNED_BIT );
static const uint UINT32_3 = ( DT_INT32 | DT_SIZE_3 | DT_UNSIGNED_BIT );
static const uint UINT32_4 = ( DT_INT32 | DT_SIZE_4 | DT_UNSIGNED_BIT );
static const uint FLOAT16_1 = ( DT_FLOAT16 | DT_SIZE_1 );
static const uint FLOAT16_2 = ( DT_FLOAT16 | DT_SIZE_2 );
static const uint FLOAT16_3 = ( DT_FLOAT16 | DT_SIZE_3 );
static const uint FLOAT16_4 = ( DT_FLOAT16 | DT_SIZE_4 );
static const uint UFLOAT16_1 = ( DT_FLOAT16 | DT_SIZE_1 | DT_UNSIGNED_BIT );
static const uint UFLOAT16_2 = ( DT_FLOAT16 | DT_SIZE_2 | DT_UNSIGNED_BIT );
static const uint UFLOAT16_3 = ( DT_FLOAT16 | DT_SIZE_3 | DT_UNSIGNED_BIT );
static const uint UFLOAT16_4 = ( DT_FLOAT16 | DT_SIZE_4 | DT_UNSIGNED_BIT );
static const uint FLOAT32_1 = ( DT_FLOAT32 | DT_SIZE_1 );
static const uint FLOAT32_2 = ( DT_FLOAT32 | DT_SIZE_2 );
static const uint FLOAT32_3 = ( DT_FLOAT32 | DT_SIZE_3 );
static const uint FLOAT32_4 = ( DT_FLOAT32 | DT_SIZE_4 );
static const uint UFLOAT32_1 = ( DT_FLOAT32 | DT_SIZE_1 | DT_UNSIGNED_BIT );
static const uint UFLOAT32_2 = ( DT_FLOAT32 | DT_SIZE_2 | DT_UNSIGNED_BIT );
static const uint UFLOAT32_3 = ( DT_FLOAT32 | DT_SIZE_3 | DT_UNSIGNED_BIT );
static const uint UFLOAT32_4 = ( DT_FLOAT32 | DT_SIZE_4 | DT_UNSIGNED_BIT );
static const uint BYTE_1_NORM = ( BYTE_1 | DT_NORMALIZED_BIT );
static const uint BYTE_2_NORM = ( BYTE_2 | DT_NORMALIZED_BIT );
static const uint BYTE_3_NORM = ( BYTE_3 | DT_NORMALIZED_BIT );
static const uint BYTE_4_NORM = ( BYTE_4 | DT_NORMALIZED_BIT );
static const uint UBYTE_1_NORM = ( UBYTE_1 | DT_NORMALIZED_BIT );
static const uint UBYTE_2_NORM = ( UBYTE_2 | DT_NORMALIZED_BIT );
static const uint UBYTE_3_NORM = ( UBYTE_3 | DT_NORMALIZED_BIT );
static const uint UBYTE_4_NORM = ( UBYTE_4 | DT_NORMALIZED_BIT );
static const uint SHORT_1_NORM = ( SHORT_1 | DT_NORMALIZED_BIT );
static const uint SHORT_2_NORM = ( SHORT_2 | DT_NORMALIZED_BIT );
static const uint SHORT_3_NORM = ( SHORT_3 | DT_NORMALIZED_BIT );
static const uint SHORT_4_NORM = ( SHORT_4 | DT_NORMALIZED_BIT );
static const uint USHORT_1_NORM = ( USHORT_1 | DT_NORMALIZED_BIT );
static const uint USHORT_2_NORM = ( USHORT_2 | DT_NORMALIZED_BIT );
static const uint USHORT_3_NORM = ( USHORT_3 | DT_NORMALIZED_BIT );
static const uint USHORT_4_NORM = ( USHORT_4 | DT_NORMALIZED_BIT );
static const uint DT_UNKNOWN_TYPE = 0xffFFffFFu;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// the following assumes addresses are aligned with size of the respective datatype
// I couldn't find vertex buffer alignment rules for dx12, so I'm basing this on vulkan...
// https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#fxvertex-input-extraction
// "attribAddress must be a multiple of the size in bytes of the size of the format"


// Read Write Buffers

uint ReadUInt32( RWBuffer<uint> buffer, uint address )
{
    return buffer[( address + 0 ) >> 2];
}

uint2 ReadUInt32_2( RWBuffer<uint> buffer, uint address )
{
    uint2 result;
    result.x = buffer[( address + 0 ) >> 2];
    result.y = buffer[( address + 4 ) >> 2];
    return result;
}

uint3 ReadUInt32_3( RWBuffer<uint> buffer, uint address )
{
    uint3 result;
    result.x = buffer[( address + 0 ) >> 2];
    result.y = buffer[( address + 4 ) >> 2];
    result.z = buffer[( address + 8 ) >> 2];
    return result;
}

float ReadFloat32( RWBuffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32( buffer, address ) );
}

float2 ReadFloat32_2( RWBuffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32_2( buffer, address ) );
}

float3 ReadFloat32_3( RWBuffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32_3( buffer, address ) );
}

uint4 ReadUInt16_4( RWBuffer<uint> buffer, uint address )
{
    uint4 result;
    uint packed0 = buffer[( address + 0 ) >> 2];
    uint packed1 = buffer[( address + 4 ) >> 2];
    result.x = packed0 & 0xFFFF;
    result.y = packed0 >> 16;
    result.z = packed1 & 0xFFFF;
    result.w = packed1 >> 16;
    return result;
}

int4 ReadInt16_4( RWBuffer<uint> buffer, uint address )
{
    int4 result;
    uint packed0 = buffer[( address + 0 ) >> 2];
    uint packed1 = buffer[( address + 4 ) >> 2];
    result.x = asint( ( packed0 & 0xFFFF ) << 16 ) >> 16;
    result.y = asint( packed0 & 0xFFFF0000 ) >> 16;
    result.z = asint( ( packed1 & 0xFFFF ) << 16 ) >> 16;
    result.w = asint( packed1 & 0xFFFF0000 ) >> 16;
    return result;
}

// Read Only Buffers

uint ReadUInt16( Buffer<uint> buffer, uint address )
{
    uint result;
    uint packed = buffer.Load( address >> 2 );
    if( ( address & 3 ) == 0 )
    {
        result = packed & 0xFFFF;
    }
    else
    {
        result = packed >> 16;
    }
    return result;
}

uint2 ReadUInt16_2( Buffer<uint> buffer, uint address )
{
    uint2 result;
    uint packed = buffer.Load( ( address + 0 ) >> 2 );
    result.x = packed & 0xFFFF;
    result.y = packed >> 16;
    return result;
}

uint3 ReadUInt16_3( Buffer<uint> buffer, uint address )
{
    uint3 result;
    uint packed0 = buffer.Load( ( address + 0 ) >> 2 );
    uint packed1 = buffer.Load( ( address + 4 ) >> 2 );
    if( ( address & 3 ) == 0 )
    {
        result.x = packed0 & 0xFFFF;
        result.y = packed0 >> 16;
        result.z = packed1 & 0xFFFF;
    }
    else
    {
        result.x = packed0 >> 16;
        result.y = packed1 & 0xFFFF;
        result.z = packed1 >> 16;
    }
    return result;
}

uint4 ReadUInt16_4( Buffer<uint> buffer, uint address )
{
    uint4 result;
    uint packed0 = buffer.Load( ( address + 0 ) >> 2 );
    uint packed1 = buffer.Load( ( address + 4 ) >> 2 );
    result.x = packed0 & 0xFFFF;
    result.y = packed0 >> 16;
    result.z = packed1 & 0xFFFF;
    result.w = packed1 >> 16;
    return result;
}

int4 ReadInt16_4( Buffer<uint> buffer, uint address )
{
    int4 result;
    uint packed0 = buffer.Load( ( address + 0 ) >> 2 );
    uint packed1 = buffer.Load( ( address + 4 ) >> 2 );
    result.x = asint( ( packed0 & 0xFFFF ) << 16 ) >> 16;
    result.y = asint( packed0 & 0xFFFF0000 ) >> 16;
    result.z = asint( ( packed1 & 0xFFFF ) << 16 ) >> 16;
    result.w = asint( packed1 & 0xFFFF0000 ) >> 16;
    return result;
}


uint ReadUInt32( Buffer<uint> buffer, uint address )
{
    return buffer.Load( ( address + 0 ) >> 2 );
}

uint2 ReadUInt32_2( Buffer<uint> buffer, uint address )
{
    uint2 result;
    result.x = buffer.Load( ( address + 0 ) >> 2 );
    result.y = buffer.Load( ( address + 4 ) >> 2 );
    return result;
}

uint3 ReadUInt32_3( Buffer<uint> buffer, uint address )
{
    uint3 result;
    result.x = buffer.Load( ( address + 0 ) >> 2 );
    result.y = buffer.Load( ( address + 4 ) >> 2 );
    result.z = buffer.Load( ( address + 8 ) >> 2 );
    return result;
}

float ReadFloat16( Buffer<uint> buffer, uint address )
{
    return f16tof32( ReadUInt16( buffer, address ) );
}

float2 ReadFloat16_2( Buffer<uint> buffer, uint address )
{
    return f16tof32( ReadUInt16_2( buffer, address ) );
}

float3 ReadFloat16_3( Buffer<uint> buffer, uint address )
{
    return f16tof32( ReadUInt16_3( buffer, address ) );
}

float ReadFloat32( Buffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32( buffer, address ) );
}

float2 ReadFloat32_2( Buffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32_2( buffer, address ) );
}

float3 ReadFloat32_3( Buffer<uint> buffer, uint address )
{
    return asfloat( ReadUInt32_3( buffer, address ) );
}

float2 ReadFloat_2( Buffer<uint> buffer, uint address, uint dataType )
{
    if( dataType == FLOAT16_2 )
    {
        return ReadFloat16_2( buffer, address );
    }
    else if( dataType == FLOAT32_2 )
    {
        return ReadFloat32_2( buffer, address );
    }
    else
    {
        return 0.0.xx;
    }
}

void WriteInt16_4( RWBuffer<uint> buffer, uint address, int4 data )
{
    uint packed0 = ( ( asuint( data.x ) & 0xFFFF ) | ( asuint( data.y ) << 16 ) );
    uint packed1 = ( ( asuint( data.z ) & 0xFFFF ) | ( asuint( data.w ) << 16 ) );
    buffer[( address + 0 ) >> 2] = packed0;
    buffer[( address + 4 ) >> 2] = packed1;
}

void WriteUInt16_4( RWBuffer<uint> buffer, uint address, uint4 data )
{
    uint packed0 = ( data.x & 0xFFFF ) | data.y << 16;
    uint packed1 = ( data.z & 0xFFFF ) | data.w << 16;

    buffer[( address + 0 ) >> 2] = packed0;
    buffer[( address + 4 ) >> 2] = packed1;
}

void WriteUInt32_2( RWBuffer<uint> buffer, uint address, uint2 data )
{
    buffer[( address + 0 ) >> 2] = data.x;
    buffer[( address + 4 ) >> 2] = data.y;
}

void WriteUInt32_3( RWBuffer<uint> buffer, uint address, uint3 data )
{
    buffer[( address + 0 ) >> 2] = data.x;
    buffer[( address + 4 ) >> 2] = data.y;
    buffer[( address + 8 ) >> 2] = data.z;
}

void WriteUInt32_4( RWBuffer<uint> buffer, uint address, uint4 data )
{
    buffer[( address + 0 ) >> 2] = data.x;
    buffer[( address + 4 ) >> 2] = data.y;
    buffer[( address + 8 ) >> 2] = data.z;
    buffer[( address + 12 ) >> 2] = data.w;
}

void WriteFloat32_2( RWBuffer<uint> buffer, uint address, float2 data )
{
    WriteUInt32_2( buffer, address, asuint( data ) );
}

void WriteFloat32_3( RWBuffer<uint> buffer, uint address, float3 data )
{
    WriteUInt32_3( buffer, address, asuint( data ) );
}

void WriteFloat32_4( RWBuffer<uint> buffer, uint address, float4 data )
{
    WriteUInt32_4( buffer, address, asuint( data ) );
}

#endif // VERTEX_BUFFER_UTIL_FXH
