#include "StdAfx.h"
#include "GeometryUtils.h"
#include "Tr2Renderer.h"
#include "Tr2VertexDefinitionUtilities.h"

void GetVertexPositionOffsetAndType(granny_mesh* grannyMesh, unsigned int &positionOffset, Tr2VertexDefinition::DataType &positionType )
{
	positionOffset = 0;
	positionType = Tr2VertexDefinition::DT_UNKNOWN_TYPE;

	if ( !grannyMesh )
	{		
		return;
	}

	granny_data_type_definition* grannyVertexDecl = grannyMesh->PrimaryVertexData->VertexType;

	if ( !grannyVertexDecl )
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

void ConvertShort4ToVector3( const void *ptr, Vector3* dest )
{
	short *vdata = (short*)(ptr);
	float rcp = 1.0f / (float)vdata[3];
	dest->x = (float)vdata[0] * rcp;
	dest->y = (float)vdata[1] * rcp;
	dest->z = (float)vdata[2] * rcp;
}

void ConvertUByte4ToVector3( const void *ptr, Vector3* dest )
{
	unsigned char * vdata = (unsigned char *)(ptr);
	
	dest->x = (float)vdata[2] / 255.0f * 2.0f - 1.0f;
	dest->y = (float)vdata[1] / 255.0f * 2.0f - 1.0f;
	dest->z = (float)vdata[0] / 255.0f * 2.0f - 1.0f;
}

void GetMeshVertexPosition(	granny_mesh* grannyMesh, unsigned index, 
							Vector3 & position,
							unsigned grannyBytesPerVertex, 
							unsigned positionOffset, 
							Tr2VertexDefinition::DataType positionType )
{	
	if( !grannyBytesPerVertex )
	{
		return;
	}

	granny_uint8 *positionPtr = grannyMesh->PrimaryVertexData->Vertices + index * grannyBytesPerVertex + positionOffset;

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

const char* VertexDeclTypeToString( Tr2VertexDefinition::DataType type )
{
#define VD_CASE(x)	case Tr2VertexDefinition:: x : { static const char* text = #x ; return text; }

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

	return map[ usage ];
}

void DescribeVertexDecl( unsigned int decl )
{
	Tr2VertexDefinition vd;
	bool result = Tr2EffectStateManager::GetVertexDeclarationElements( decl, vd );

	if( !result )
	{
		CCP_LOG( "Invalid vertex declaration");
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
