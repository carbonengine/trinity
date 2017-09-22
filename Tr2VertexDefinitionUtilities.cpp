////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2VertexDefinitionUtilities.h"

// --------------------------------------------------------------------------------------
// Description:
//   Converts Granny data type definition to Trinity vertex type.  
// Arguments:
//   src - Granny data type definition
// Return Value:
//   Trinity vertex type corresponding to input Granny type
// --------------------------------------------------------------------------------------
Tr2VertexDefinition::DataType ConvertGrannyTypeToDataType( const granny_data_type_definition& src )
{
	unsigned type = 0;

	switch (src.Type)
	{
	case GrannyInt8Member:
		type = Tr2VertexDefinition::DT_INT8;
		break;
	case GrannyUInt8Member:
		type = Tr2VertexDefinition::DT_INT8 | Tr2VertexDefinition::DT_UNSIGNED_BIT;
		break;
	case GrannyInt16Member:
		type = Tr2VertexDefinition::DT_INT16;
		break;
	case GrannyUInt16Member:
		type = Tr2VertexDefinition::DT_INT16 | Tr2VertexDefinition::DT_UNSIGNED_BIT;
		break;
	case GrannyInt32Member:
		type = Tr2VertexDefinition::DT_INT32;
		break;
	case GrannyUInt32Member:
		type = Tr2VertexDefinition::DT_INT32 | Tr2VertexDefinition::DT_UNSIGNED_BIT;
		break;
	case GrannyReal16Member:
		type = Tr2VertexDefinition::DT_FLOAT16;
		break;
	case GrannyNormalUInt8Member:
		type = Tr2VertexDefinition::DT_INT8 | Tr2VertexDefinition::DT_UNSIGNED_BIT | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case GrannyNormalUInt16Member:
		type = Tr2VertexDefinition::DT_INT16 | Tr2VertexDefinition::DT_UNSIGNED_BIT | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case GrannyReal32Member:
		type = Tr2VertexDefinition::DT_FLOAT32;
		break;
	default:
		return Tr2VertexDefinition::DT_UNKNOWN_TYPE;
	}

	unsigned size = std::max( 1, src.ArrayWidth ) - 1;
	type |= size << Tr2VertexDefinition::DT_SIZE_OFFSET;

	return static_cast<Tr2VertexDefinition::DataType>(type);
}

// --------------------------------------------------------------------------------------
// Description:
//   Converts Granny vertex definition to Trinity vertex definition.  
// Arguments:
//   grannyVertexDecl - Granny vertex definition
// Return Value:
//   Trinity vertex definition corresponding to input Granny vertex definition
// --------------------------------------------------------------------------------------
Tr2VertexDefinition BuildFromGrannyVertexDecl( const granny_data_type_definition* grannyVertexDecl )
{
	Tr2VertexDefinition vd;

	while( grannyVertexDecl->Type != GrannyEndMember )
	{
		const granny_data_type_definition& src = *grannyVertexDecl++;

		Tr2VertexDefinition::Item item;

		item.m_stream = 0;
		item.m_offset = vd.m_nextOffset[0];
		item.m_dataType = ConvertGrannyTypeToDataType(src);
		item.m_usageIndex = 0;

		vd.m_nextOffset[0] += vd.GetDataTypeSizeInBytes(item.m_dataType);

		if( !strncmp( src.Name, GrannyVertexPositionName, strlen( GrannyVertexPositionName ) ) )
		{
			item.m_usage = vd.POSITION;
			char C = src.Name[ strlen( GrannyVertexPositionName ) ];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexDiffuseColorName, strlen( GrannyVertexDiffuseColorName ) ) )
		{
			item.m_usage = vd.COLOR;
			char C = src.Name[ strlen( GrannyVertexDiffuseColorName ) ];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexNormalName, strlen( GrannyVertexNormalName ) ) )
		{
			item.m_usage = vd.NORMAL;
			char C = src.Name[ strlen( GrannyVertexNormalName ) ];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}		
		else if( !strcmp( src.Name, GrannyVertexTangentName ) )
		{
			item.m_usage = vd.TANGENT;
		}
		else if( !strcmp( src.Name, GrannyVertexBinormalName ) )
		{
			item.m_usage = vd.BITANGENT;
		}
		else if( !strncmp( src.Name, GrannyVertexTextureCoordinatesName, strlen( GrannyVertexTextureCoordinatesName ) ) )
		{
			item.m_usage = vd.TEXCOORD;
			char C = src.Name[ strlen( GrannyVertexTextureCoordinatesName ) ];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}		
		else if( !strcmp( src.Name, GrannyVertexBoneIndicesName ) )
		{
			item.m_usage = vd.BLENDINDICES;
		}
		else if( !strcmp( src.Name, GrannyVertexBoneWeightsName ) )
		{
			item.m_usage = vd.BLENDWEIGHTS;
		}

		vd.m_items.push_back( item );
	}

	return vd;
}

// --------------------------------------------------------------------------------------
// Description:
//   Convert Trinity vertex definition back to a granny layout.  
// Arguments:
//   vd - input definition
//   grannyVertexDecl - pointer to at least maxSize elements
//   maxSize - size of grannyVertexDecl array
// Return Value:
//   true - If successful
//   false - On error
// --------------------------------------------------------------------------------------
bool ConvertVertexDeclToGranny( Tr2VertexDefinition vd, granny_data_type_definition* grannyVertexDecl, unsigned maxSize )
{
	// Note: This function assumes the D3D vertex layout is described in increasing offset order
	// ... so make sure.
	std::sort(	begin( vd.m_items ), end( vd.m_items ) );

	// shorten the namespace...
	typedef Tr2VertexDefinition tvd;

	for( size_t i = 0; i != std::min( maxSize, (unsigned int)vd.m_items.size() ); ++i )
	{
		const auto& src = vd.m_items[i];
	
		granny_data_type_definition& dst = grannyVertexDecl[i];

		dst.ArrayWidth				= ( ( src.m_dataType & tvd::DT_SIZE_MASK ) >> tvd::DT_SIZE_OFFSET ) + 1;
		const bool isUnsigned		= ( src.m_dataType & tvd::DT_UNSIGNED_BIT ) != 0;
		const bool isNormalized		= ( src.m_dataType & tvd::DT_NORMALIZED_BIT ) != 0;

		switch( src.m_dataType & tvd::DT_TYPE_MASK )
		{
			case tvd::DT_INT8:
				dst.Type = isUnsigned	?	isNormalized ? GrannyNormalUInt8Member	: GrannyUInt8Member
										:	/*isNormalized ? GrannyNormalInt8Member	:*/ GrannyInt8Member;
				break;

			case tvd::DT_INT16:
				dst.Type = isUnsigned	?	isNormalized ? GrannyNormalUInt16Member	: GrannyUInt16Member
										:	/*isNormalized ? GrannyNormalInt8Member	:*/ GrannyInt16Member;
				break;

			case tvd::DT_INT32:
				dst.Type = isUnsigned	?	GrannyUInt32Member : GrannyInt16Member;
				break;
		
			case tvd::DT_FLOAT16:
				dst.Type = GrannyReal16Member;
				break;
		
			case tvd::DT_FLOAT32:
				dst.Type = GrannyReal32Member;
				break;
		
			default:
				CCP_ASSERT( false && "Missing datatype support in granny conversion" );
				return false;
		}

		static const char * grannyTexcoordNames[8]=
		{
			GrannyVertexTextureCoordinatesName "0",
			GrannyVertexTextureCoordinatesName "1",
			GrannyVertexTextureCoordinatesName "2",
			GrannyVertexTextureCoordinatesName "3",
			GrannyVertexTextureCoordinatesName "4",
			GrannyVertexTextureCoordinatesName "5",
			GrannyVertexTextureCoordinatesName "6",
			GrannyVertexTextureCoordinatesName "7",
		};

		static const char * grannyPositionNames[4]=
		{
			GrannyVertexPositionName,
			GrannyVertexPositionName "1",
			GrannyVertexPositionName "2",
			GrannyVertexPositionName "3",
		};

		static const char * grannyNormalNames[4]=
		{
			GrannyVertexNormalName,
			GrannyVertexNormalName "1",
			GrannyVertexNormalName "2",
			GrannyVertexNormalName "3",
		};

		switch( src.m_usage )
		{
		case tvd::POSITION:
			CCP_ASSERT( src.m_usageIndex < 4 );
			dst.Name = grannyPositionNames[ src.m_usageIndex ];
			break;
		case tvd::NORMAL:
			CCP_ASSERT( src.m_usageIndex < 4 );
			dst.Name = grannyNormalNames[ src.m_usageIndex ];
			break;
		case tvd::TANGENT:
			dst.Name = GrannyVertexTangentName;
			break;
		case tvd::BITANGENT:
			dst.Name = GrannyVertexBinormalName;
			break;
		case tvd::TEXCOORD:
			CCP_ASSERT( src.m_usageIndex < 8 );
			dst.Name = grannyTexcoordNames[ src.m_usageIndex ]; //GrannyVertexTextureCoordinatesName;
			break;
		case tvd::BLENDINDICES:
			dst.Name = GrannyVertexBoneIndicesName;
			break;
		case tvd::BLENDWEIGHTS:
			dst.Name = GrannyVertexBoneWeightsName;
			break;
		default:
			CCP_ASSERT( false && "Missing usage support in granny conversion" );
			return false;
		}

		dst.ReferenceType = 0;
	}

	grannyVertexDecl[std::min( maxSize, (unsigned int)vd.m_items.size() )].Type = GrannyEndMember;

	return true;
}

