// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "GeometryUtils.h"
#include "Tr2Renderer.h"
#include "Tr2VertexDefinitionUtilities.h"

#include <numeric>

#if WITH_GRANNY
void GetVertexPositionOffsetAndType( granny_mesh* grannyMesh, unsigned int& positionOffset, Tr2VertexDefinition::DataType& positionType )
{
	positionOffset = 0;
	positionType = Tr2VertexDefinition::DT_UNKNOWN_TYPE;

	if( !grannyMesh )
	{
		return;
	}

	granny_data_type_definition* grannyVertexDecl = grannyMesh->PrimaryVertexData->VertexType;

	if( !grannyVertexDecl )
	{
		return;
	}

	while( grannyVertexDecl->Type != GrannyEndMember )
	{
		if( !strcmp( grannyVertexDecl->Name, GrannyVertexPositionName ) )
		{
			positionType = ConvertGrannyTypeToDataType( *grannyVertexDecl );
			return;
		}

		positionOffset += GrannyGetTotalTypeSize( grannyVertexDecl );
		grannyVertexDecl++;
	}

	positionOffset = 0;
}
#endif

void ConvertShort4ToVector3( const void* ptr, Vector3* dest )
{
	short* vdata = (short*)( ptr );
	float rcp = 1.0f / (float)vdata[3];
	dest->x = (float)vdata[0] * rcp;
	dest->y = (float)vdata[1] * rcp;
	dest->z = (float)vdata[2] * rcp;
}

void ConvertUByte4ToVector3( const void* ptr, Vector3* dest )
{
	unsigned char* vdata = (unsigned char*)( ptr );

	dest->x = (float)vdata[2] / 255.0f * 2.0f - 1.0f;
	dest->y = (float)vdata[1] / 255.0f * 2.0f - 1.0f;
	dest->z = (float)vdata[0] / 255.0f * 2.0f - 1.0f;
}


#if WITH_GRANNY
void GetMeshVertexPosition( granny_mesh* grannyMesh, unsigned index, Vector3& position, unsigned grannyBytesPerVertex, unsigned positionOffset, Tr2VertexDefinition::DataType positionType )
{
	if( !grannyBytesPerVertex )
	{
		return;
	}

	granny_uint8* positionPtr = grannyMesh->PrimaryVertexData->Vertices + index * grannyBytesPerVertex + positionOffset;

	switch( positionType )
	{
	case Tr2VertexDefinition::FLOAT16_4:
		position = *reinterpret_cast<const Vector3_16*>( positionPtr );
		break;

	case Tr2VertexDefinition::FLOAT32_3:
		memcpy( &position, positionPtr, 12 );
		break;

	case Tr2VertexDefinition::SHORT_4:
		ConvertShort4ToVector3( positionPtr, &position );
		break;

	default:
		CCP_ASSERT_M( false, "Unsupported position type in GetMeshVertexPosition" );
		break;
	}
}
#endif

const char* VertexDeclTypeToString( Tr2VertexDefinition::DataType type )
{
#define VD_CASE( x )                  \
	case Tr2VertexDefinition::x: {    \
		static const char* text = #x; \
		return text;                  \
	}

	switch( type )
	{
		VD_CASE( BYTE_1 );
		VD_CASE( BYTE_2 );
		VD_CASE( BYTE_3 );
		VD_CASE( BYTE_4 );

		VD_CASE( UBYTE_1 );
		VD_CASE( UBYTE_2 );
		VD_CASE( UBYTE_3 );
		VD_CASE( UBYTE_4 );

		VD_CASE( SHORT_1 );
		VD_CASE( SHORT_2 );
		VD_CASE( SHORT_3 );
		VD_CASE( SHORT_4 );

		VD_CASE( USHORT_1 );
		VD_CASE( USHORT_2 );
		VD_CASE( USHORT_3 );
		VD_CASE( USHORT_4 );

		VD_CASE( INT32_1 );
		VD_CASE( INT32_2 );
		VD_CASE( INT32_3 );
		VD_CASE( INT32_4 );

		VD_CASE( UINT32_1 );
		VD_CASE( UINT32_2 );
		VD_CASE( UINT32_3 );
		VD_CASE( UINT32_4 );

		VD_CASE( FLOAT16_1 );
		VD_CASE( FLOAT16_2 );
		VD_CASE( FLOAT16_3 );
		VD_CASE( FLOAT16_4 );

		VD_CASE( UFLOAT16_1 );
		VD_CASE( UFLOAT16_2 );
		VD_CASE( UFLOAT16_3 );
		VD_CASE( UFLOAT16_4 );

		VD_CASE( FLOAT32_1 );
		VD_CASE( FLOAT32_2 );
		VD_CASE( FLOAT32_3 );
		VD_CASE( FLOAT32_4 );

		VD_CASE( UFLOAT32_1 );
		VD_CASE( UFLOAT32_2 );
		VD_CASE( UFLOAT32_3 );
		VD_CASE( UFLOAT32_4 );

		VD_CASE( UBYTE_4_NORM );
		VD_CASE( SHORT_2_NORM );
		VD_CASE( USHORT_2_NORM );
		VD_CASE( SHORT_4_NORM );
		VD_CASE( USHORT_4_NORM );

	default:
		static const char* text = "Unknown";
		return text;
	}
}

const char* VertexDeclUsageToString( Tr2VertexDefinition::UsageCode usage )
{
	static const char* map[Tr2VertexDefinition::NUM_USAGE_CODE] = {
		"POSITION",
		"COLOR",
		"NORMAL",
		"TANGENT",
		"BITANGENT",
		"TEXCOORD",
		"BLENDINDICES",
		"BLENDWEIGHTS"
	};

	return map[usage];
}

void DescribeVertexDecl( unsigned int decl )
{
	Tr2VertexDefinition vd;
	bool result = Tr2EffectStateManager::GetVertexDeclarationElements( decl, vd );

	if( !result )
	{
		CCP_LOG( "Invalid vertex declaration" );
		return;
	}

	DescribeVertexDecl( vd );
}

void DescribeVertexDecl( const Tr2VertexDefinition& vd )
{
	for( auto it = begin( vd.m_items ); it != end( vd.m_items ); ++it )
	{
		const char* type = VertexDeclTypeToString( it->m_dataType );
		const char* usage = VertexDeclUsageToString( it->m_usage );
		CCP_LOG( "%d\t%d\t%-12.12s\t%-12.12s\t%d", it->m_stream, it->m_offset, type, usage, it->m_usageIndex );
	}
}

#if WITH_GRANNY
granny_file* ProtectedGrannyReadEntireFileFromMemory( const wchar_t* path, uint32_t dataSize, void* data )
{
	granny_file* result = NULL;
#ifdef _MSC_VER
	__try
#endif
	{
		result = GrannyReadEntireFileFromMemory( dataSize, data );
	}
#ifdef _MSC_VER
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		CCP_LOGERR( "Exception caught while reading Granny file %S", path );
		CCP_LOGERR( "Files might be corrupt - try running the repair tool" );
	}
#endif

	return result;
}


//////////////////////////////////////////////////////////////////////////
//

granny_data_type_definition BoundingBoxType[] = {
	{ GrannyReal32Member, "min", 0, 3 },
	{ GrannyReal32Member, "max", 0, 3 },
	{ GrannyEndMember }
};

granny_data_type_definition AreaBoundsInfoType[] = {
	{ GrannyInlineMember, "bounds", BoundingBoxType },
	{ GrannyInt32Member, "vertexCount" },
	{ GrannyEndMember }
};

granny_data_type_definition UvDensityInfoType[] = {
	{ GrannyReal32Member, "density" },
	{ GrannyEndMember }
};

granny_data_type_definition MeshBoundsInfoType[] = {
	{ GrannyStringMember, "typeName" },
	{ GrannyInlineMember, "bounds", BoundingBoxType },
	{ GrannyReferenceToArrayMember, "areaInfo", AreaBoundsInfoType },
	{ GrannyInt32Member, "sourceMeshIndex" },
	{ GrannyInt32Member, "maxScreenSize" },
	{ GrannyReferenceToArrayMember, "uvDensities", UvDensityInfoType },
	{ GrannyEndMember }
};

//
//////////////////////////////////////////////////////////////////////////
#endif


void ConvertDataToVector3( Tr2VertexDefinition::DataType elementType, const void* src, Vector3* dest )
{

	switch( elementType )
	{
	case Tr2VertexDefinition::FLOAT16_4: {
		*dest = *static_cast<const Vector3_16*>( src );
		break;
	}
	case Tr2VertexDefinition::FLOAT32_3: {
		memcpy( dest, src, 3 * sizeof( float ) );
		break;
	}
	case Tr2VertexDefinition::FLOAT32_4: {
		memcpy( dest, src, 3 * sizeof( float ) );
		break;
	}
	case Tr2VertexDefinition::SHORT_4: {
		ConvertShort4ToVector3( src, dest );
		break;
	}

	case Tr2VertexDefinition::UBYTE_4: {
		ConvertUByte4ToVector3( src, dest );
		break;
	}

	default: {
		dest->x = 0.0f;
		dest->y = 0.0f;
		dest->z = 0.0f;
	}
	}
}

bool IntersectTri(
	const Vector3& vertex0,
	const Vector3& vertex1,
	const Vector3& vertex2,
	const Vector3& rayPos,
	const Vector3& rayDir,
	float& u,
	float& v,
	float& dist )
{
	// Möller–Trumbore intersection algorithm
	Vector3 e1 = vertex1 - vertex0;
	Vector3 e2 = vertex2 - vertex0;
	Vector3 a = Cross( rayDir, e2 );

	float det = Dot( e1, a );
	if( std::abs( det ) < std::numeric_limits<float>::min() )
	{
		return false;
	}
	float invDet = 1.f / det;

	Vector3 t0 = rayPos - vertex0;
	float uu = Dot( t0, a ) * invDet;
	if( uu < 0.f || uu > 1.f )
	{
		return false;
	}

	Vector3 b = Cross( t0, e1 );
	float vv = Dot( rayDir, b ) * invDet;
	if( vv < 0.f || uu + vv > 1.f )
	{
		return false;
	}

	float t = Dot( e2, b ) * invDet;
	if( t < 0.f )
	{
		return false;
	}

	u = uu;
	v = vv;
	dist = t;
	return true;
}

bool IntersectTriXM(
	const XMVECTOR& vertex0,
	const XMVECTOR& edge1,
	const XMVECTOR& edge2,
	const XMVECTOR& rayPos,
	const XMVECTOR& rayDir,
	float& u,
	float& v,
	float& dist )
{
	// Möller–Trumbore intersection algorithm, SIMD
	XMVECTOR a = XMVector3Cross( rayDir, edge2 );

	float det = XMVectorGetX( XMVector3Dot( edge1, a ) );
	if( std::abs( det ) < std::numeric_limits<float>::min() )
	{
		return false;
	}
	float invDet = 1.f / det;

	XMVECTOR t0 = XMVectorSubtract( rayPos, vertex0 );
	float uu = XMVectorGetX( XMVector3Dot( t0, a ) ) * invDet;
	if( uu < 0.f || uu > 1.f )
	{
		return false;
	}

	XMVECTOR b = XMVector3Cross( t0, edge1 );
	float vv = XMVectorGetX( XMVector3Dot( rayDir, b ) ) * invDet;
	if( vv < 0.f || uu + vv > 1.f )
	{
		return false;
	}

	float t = XMVectorGetX( XMVector3Dot( edge2, b ) ) * invDet;
	if( t < 0.f )
	{
		return false;
	}

	u = uu;
	v = vv;
	dist = t;
	return true;
}

bool GetBoneIndex( Tr2VertexDefinition::DataType elementType, const void* src, int& dest )
{
	if( elementType != Tr2VertexDefinition::UBYTE_4 )
	{
		CCP_LOGERR( "BLENDINDICES using unsupported format." );
		return false;
	}

	const uint8_t* vdata = static_cast<const uint8_t*>( src );
	dest = vdata[0];
	return true;
}

bool GetColor( Tr2VertexDefinition::DataType elementType, const void* src, Color& dest )
{
	if( elementType == Tr2VertexDefinition::UBYTE_4_NORM )
	{
		const uint8_t* v = static_cast<const uint8_t*>( src );
		dest = Color( v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f );
		return true;
	}

	if( elementType == Tr2VertexDefinition::FLOAT16_4 )
	{
		dest = Color( *static_cast<const Vector4_16*>( src ) );
		return true;
	}

	CCP_LOGERR( "COLOR using unsupported format." );
	return false;
}
