// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2VertexDefinitionUtilities.h"

#if WITH_GRANNY
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

	switch( src.Type )
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

	return static_cast<Tr2VertexDefinition::DataType>( type );
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
		item.m_dataType = ConvertGrannyTypeToDataType( src );
		item.m_usageIndex = 0;

		vd.m_nextOffset[0] += vd.GetDataTypeSizeInBytes( item.m_dataType );

		if( !strncmp( src.Name, GrannyVertexPositionName, strlen( GrannyVertexPositionName ) ) )
		{
			item.m_usage = vd.POSITION;
			char C = src.Name[strlen( GrannyVertexPositionName )];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexDiffuseColorName, strlen( GrannyVertexDiffuseColorName ) ) )
		{
			item.m_usage = vd.COLOR;
			char C = src.Name[strlen( GrannyVertexDiffuseColorName )];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexNormalName, strlen( GrannyVertexNormalName ) ) )
		{
			item.m_usage = vd.NORMAL;
			char C = src.Name[strlen( GrannyVertexNormalName )];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexTangentName, strlen( GrannyVertexTangentName ) ) )
		{
			item.m_usage = vd.TANGENT;
			char C = src.Name[strlen( GrannyVertexTangentName )];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexBinormalName, strlen( GrannyVertexBinormalName ) ) )
		{
			item.m_usage = vd.BITANGENT;
			char C = src.Name[strlen( GrannyVertexBinormalName )];
			item.m_usageIndex = C ? unsigned( C - '0' ) : 0;
		}
		else if( !strncmp( src.Name, GrannyVertexTextureCoordinatesName, strlen( GrannyVertexTextureCoordinatesName ) ) )
		{
			item.m_usage = vd.TEXCOORD;
			char C = src.Name[strlen( GrannyVertexTextureCoordinatesName )];
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
	std::sort( begin( vd.m_items ), end( vd.m_items ) );

	// shorten the namespace...
	typedef Tr2VertexDefinition tvd;

	for( size_t i = 0; i != std::min( maxSize, (unsigned int)vd.m_items.size() ); ++i )
	{
		const auto& src = vd.m_items[i];

		granny_data_type_definition& dst = grannyVertexDecl[i];

		dst.ArrayWidth = ( ( src.m_dataType & tvd::DT_SIZE_MASK ) >> tvd::DT_SIZE_OFFSET ) + 1;
		const bool isUnsigned = ( src.m_dataType & tvd::DT_UNSIGNED_BIT ) != 0;
		const bool isNormalized = ( src.m_dataType & tvd::DT_NORMALIZED_BIT ) != 0;

		switch( src.m_dataType & tvd::DT_TYPE_MASK )
		{
		case tvd::DT_INT8:
			dst.Type = isUnsigned ? isNormalized ? GrannyNormalUInt8Member : GrannyUInt8Member : /*isNormalized ? GrannyNormalInt8Member	:*/ GrannyInt8Member;
			break;

		case tvd::DT_INT16:
			dst.Type = isUnsigned ? isNormalized ? GrannyNormalUInt16Member : GrannyUInt16Member : /*isNormalized ? GrannyNormalInt8Member	:*/ GrannyInt16Member;
			break;

		case tvd::DT_INT32:
			dst.Type = isUnsigned ? GrannyUInt32Member : GrannyInt16Member;
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

		static const char* grannyTexcoordNames[8] = {
			GrannyVertexTextureCoordinatesName "0",
			GrannyVertexTextureCoordinatesName "1",
			GrannyVertexTextureCoordinatesName "2",
			GrannyVertexTextureCoordinatesName "3",
			GrannyVertexTextureCoordinatesName "4",
			GrannyVertexTextureCoordinatesName "5",
			GrannyVertexTextureCoordinatesName "6",
			GrannyVertexTextureCoordinatesName "7",
		};

		static const char* grannyPositionNames[4] = {
			GrannyVertexPositionName,
			GrannyVertexPositionName "1",
			GrannyVertexPositionName "2",
			GrannyVertexPositionName "3",
		};

		static const char* grannyNormalNames[4] = {
			GrannyVertexNormalName,
			GrannyVertexNormalName "1",
			GrannyVertexNormalName "2",
			GrannyVertexNormalName "3",
		};

		switch( src.m_usage )
		{
		case tvd::POSITION:
			CCP_ASSERT( src.m_usageIndex < 4 );
			dst.Name = grannyPositionNames[src.m_usageIndex];
			break;
		case tvd::NORMAL:
			CCP_ASSERT( src.m_usageIndex < 4 );
			dst.Name = grannyNormalNames[src.m_usageIndex];
			break;
		case tvd::TANGENT:
			dst.Name = GrannyVertexTangentName;
			break;
		case tvd::BITANGENT:
			dst.Name = GrannyVertexBinormalName;
			break;
		case tvd::TEXCOORD:
			CCP_ASSERT( src.m_usageIndex < 8 );
			dst.Name = grannyTexcoordNames[src.m_usageIndex]; //GrannyVertexTextureCoordinatesName;
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
#endif


// --------------------------------------------------------------------------------------
// Description:
//   Converts CMF data type definition to Trinity vertex type.
// Arguments:
//   src - CMF data type definition
// Return Value:
//   Trinity vertex type corresponding to input CMF type
// --------------------------------------------------------------------------------------
Tr2VertexDefinition::DataType ConvertCMFTypeToDataType( const cmf::VertexElement& src )
{
	unsigned type = 0;

	switch( src.type )
	{
	case cmf::ElementType::Int8:
		type = Tr2VertexDefinition::DT_INT8;
		break;
	case cmf::ElementType::UInt8:
		type = Tr2VertexDefinition::DT_INT8 | Tr2VertexDefinition::DT_UNSIGNED_BIT;
		break;
	case cmf::ElementType::Int16:
		type = Tr2VertexDefinition::DT_INT16;
		break;
	case cmf::ElementType::UInt16:
		type = Tr2VertexDefinition::DT_INT16 | Tr2VertexDefinition::DT_UNSIGNED_BIT;
		break;
	case cmf::ElementType::Float16:
		type = Tr2VertexDefinition::DT_FLOAT16;
		break;
	case cmf::ElementType::Int8Norm:
		type = Tr2VertexDefinition::DT_INT8 | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case cmf::ElementType::UInt8Norm:
		type = Tr2VertexDefinition::DT_INT8 | Tr2VertexDefinition::DT_UNSIGNED_BIT | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case cmf::ElementType::Int16Norm:
		type = Tr2VertexDefinition::DT_INT16 | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case cmf::ElementType::UInt16Norm:
		type = Tr2VertexDefinition::DT_INT16 | Tr2VertexDefinition::DT_UNSIGNED_BIT | Tr2VertexDefinition::DT_NORMALIZED_BIT;
		break;
	case cmf::ElementType::Float32:
		type = Tr2VertexDefinition::DT_FLOAT32;
		break;
	default:
		return Tr2VertexDefinition::DT_UNKNOWN_TYPE;
	}

	unsigned size = std::max( uint8_t{ 1 }, src.elementCount ) - 1;
	type |= size << Tr2VertexDefinition::DT_SIZE_OFFSET;

	return static_cast<Tr2VertexDefinition::DataType>( type );
}

// --------------------------------------------------------------------------------------
// Description:
//   Converts CMF vertex definition to Trinity vertex definition.
// Arguments:
//   cmfVertexDecl - CMF vertex definition
// Return Value:
//   Trinity vertex definition corresponding to input CMF vertex definition
// --------------------------------------------------------------------------------------
Tr2VertexDefinition BuildFromCMFVertexDecl( const cmf::Span<cmf::VertexElement>& cmfVertexDecl )
{
	Tr2VertexDefinition vd;

	for( const auto& src : cmfVertexDecl )
	{
		Tr2VertexDefinition::Item item;

		item.m_stream = 0;
		item.m_offset = vd.m_nextOffset[0];
		item.m_dataType = ConvertCMFTypeToDataType( src );
		item.m_usageIndex = src.usageIndex;

		vd.m_nextOffset[0] += vd.GetDataTypeSizeInBytes( item.m_dataType );

		switch( src.usage )
		{
		case cmf::Usage::Position:
			item.m_usage = Tr2VertexDefinition::POSITION;
			break;
		case cmf::Usage::Normal:
			item.m_usage = Tr2VertexDefinition::NORMAL;
			break;
		case cmf::Usage::Tangent:
			item.m_usage = Tr2VertexDefinition::TANGENT;
			break;
		case cmf::Usage::Binormal:
			item.m_usage = Tr2VertexDefinition::BITANGENT;
			break;
		case cmf::Usage::TexCoord:
			item.m_usage = Tr2VertexDefinition::TEXCOORD;
			break;
		case cmf::Usage::Color:
			item.m_usage = Tr2VertexDefinition::COLOR;
			break;
		case cmf::Usage::BoneIndices:
			item.m_usage = Tr2VertexDefinition::BLENDINDICES;
			break;
		case cmf::Usage::BoneWeights:
			item.m_usage = Tr2VertexDefinition::BLENDWEIGHTS;
			break;
		case cmf::Usage::PackedTangent:
			item.m_usage = Tr2VertexDefinition::TANGENT;
			break;
		case cmf::Usage::PackedTangentLegacy:
			item.m_usage = Tr2VertexDefinition::TANGENT;
			break;
		}

		vd.m_items.push_back( item );
	}

	return vd;
}

// --------------------------------------------------------------------------------------
// Description:
//   Convert Trinity vertex definition back to a CMF layout.
// Arguments:
//   vd - input definition
//   cmfVertexDecl - pointer to at least maxSize elements
//   maxSize - size of cmfVertexDecl array
// Return Value:
//   true - If successful
//   false - On error
// --------------------------------------------------------------------------------------
bool ConvertVertexDeclToCMF( Tr2VertexDefinition vd, cmf::Span<cmf::VertexElement>& cmfVertexDecl, unsigned maxSize )
{
	// Note: This function assumes the D3D vertex layout is described in increasing offset order
	// ... so make sure.
	std::sort( begin( vd.m_items ), end( vd.m_items ) );

	for( size_t i = 0; i != std::min( maxSize, (unsigned int)vd.m_items.size() ); ++i )
	{
		const auto& src = vd.m_items[i];

		cmf::VertexElement& dst = cmfVertexDecl.begin()[i];

		dst.elementCount = ( ( src.m_dataType & Tr2VertexDefinition::DT_SIZE_MASK ) >> Tr2VertexDefinition::DT_SIZE_OFFSET ) + 1;
		const bool isUnsigned = ( src.m_dataType & Tr2VertexDefinition::DT_UNSIGNED_BIT ) != 0;
		const bool isNormalized = ( src.m_dataType & Tr2VertexDefinition::DT_NORMALIZED_BIT ) != 0;

		switch( src.m_dataType & Tr2VertexDefinition::DT_TYPE_MASK )
		{
		case Tr2VertexDefinition::DT_INT8:
			dst.type = isUnsigned ?
				( isNormalized ? cmf::ElementType::UInt8Norm : cmf::ElementType::UInt8 ) :
				( isNormalized ? cmf::ElementType::Int8Norm : cmf::ElementType::Int8 );
			break;

		case Tr2VertexDefinition::DT_INT16:
			dst.type = isUnsigned ?
				( isNormalized ? cmf::ElementType::UInt16Norm : cmf::ElementType::UInt16 ) :
				( isNormalized ? cmf::ElementType::Int16Norm : cmf::ElementType::Int16 );
			break;

		case Tr2VertexDefinition::DT_INT32:
			CCP_ASSERT_M( false, "ConvertVertexDeclToCMF: INT32 is not supported by cmf" );
			return false;

		case Tr2VertexDefinition::DT_FLOAT16:
			dst.type = cmf::ElementType::Float16;
			break;

		case Tr2VertexDefinition::DT_FLOAT32:
			dst.type = cmf::ElementType::Float32;
			break;

		default:
			CCP_ASSERT( false && "ConvertVertexDeclToCMF: Missing datatype support in cmf conversion" );
			return false;
		}

		dst.offset = src.m_offset;
		dst.usageIndex = src.m_usageIndex;

		switch( src.m_usage )
		{
		case Tr2VertexDefinition::POSITION:
			dst.usage = cmf::Usage::Position;
			break;
		case Tr2VertexDefinition::COLOR:
			dst.usage = cmf::Usage::Color;
			break;
		case Tr2VertexDefinition::NORMAL:
			dst.usage = cmf::Usage::Normal;
			break;
		case Tr2VertexDefinition::TANGENT:
			dst.usage = cmf::Usage::Tangent;
			break;
		case Tr2VertexDefinition::BITANGENT:
			dst.usage = cmf::Usage::Binormal;
			break;
		case Tr2VertexDefinition::TEXCOORD:
			dst.usage = cmf::Usage::TexCoord;
			break;
		case Tr2VertexDefinition::BLENDINDICES:
			dst.usage = cmf::Usage::BoneIndices;
			break;
		case Tr2VertexDefinition::BLENDWEIGHTS:
			dst.usage = cmf::Usage::BoneWeights;
			break;
		default:
			CCP_ASSERT( false && "ConvertVertexDeclToCMF: Missing usage support in cmf conversion" );
			return false;
		}
	}

	return true;
}
