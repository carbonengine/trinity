// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "Type.h"
#include "SymbolTable.h"
#include "HLSLParser.h"
#include "ParserUtils.h"
#include "ASTNode.h"

bool Type::operator==( const Type& type ) const
{
	if( symbol || type.symbol )
	{
		return symbol == type.symbol;
	}
	return builtInType == type.builtInType &&
		width == type.width &&
		height == type.height &&
		( ( templateParameter == nullptr && type.templateParameter == nullptr ) ||
		  ( templateParameter != nullptr && type.templateParameter != nullptr && *templateParameter == *type.templateParameter && templateSamples == type.templateSamples ) ) &&
		( !IsTexture() || isDepthTexture == type.isDepthTexture );
}

bool Type::operator!=( const Type& type ) const
{
	return !( *this == type );
}

bool Type::FromToken( const ScannerToken& token )
{
	// TODO: check if token.type is a type name
	symbol = nullptr;
	builtInType = token.type;
	width = int( std::max( token.intValue & 0xf, 1L ) );
	height = int( std::max( token.intValue >> 8, 1L ) );
	templateParameter = nullptr;
	modifier = 0;
	storageClass = 0;
	arrayDimensions = 0;
	metalTextureAccess = 0;
	templateSamples = -1;
	isDepthTexture = false;
	return true;
}

bool Type::FromTokenType( int type )
{
	// TODO: check if token.type is a type name
	symbol = nullptr;
	builtInType = type;
	width = 1;
	height = 1;
	templateParameter = nullptr;
	modifier = 0;
	storageClass = 0;
	arrayDimensions = 0;
	metalTextureAccess = 0;
	templateSamples = -1;
	isDepthTexture = false;
	return true;
}

bool Type::FromSymbol( const Symbol* asymbol )
{
	if( !asymbol->isTypeName )
	{
		return false;
	}
	symbol = asymbol;
	builtInType = 0;
	width = 1;
	height = 1;
	templateParameter = nullptr;
	modifier = 0;
	storageClass = 0;
	arrayDimensions = 0;
	metalTextureAccess = 0;
	templateSamples = -1;
	isDepthTexture = false;
	return true;
}

bool Type::IsScalar() const
{
	return IsScalarOrVector() && width == 1 && height == 1;
}

bool Type::IsVector() const
{
	return IsScalarOrVector() && width > 1 && height == 1;
}

bool Type::IsMatrix() const
{
	return IsScalarOrVector() && width > 1 && height > 1;
}

bool Type::IsStruct() const
{
	return symbol != nullptr;
}

bool Type::IsBindlessHandle() const
{
	return IsScalar() && ( builtInType == OP_BINDLESSHANDLETEXTURE2D || builtInType == OP_BINDLESSHANDLETEXTURE3D || builtInType == OP_BINDLESSHANDLETEXTURECUBE || builtInType == OP_BINDLESSHANDLESAMPLER );
}

bool Type::IsScalarOrVector() const
{
	return symbol == nullptr &&
		( builtInType == OP_INT ||
		  builtInType == OP_UINT ||
		  builtInType == OP_BOOL ||
		  builtInType == OP_HALF ||
		  builtInType == OP_FLOAT ||
		  builtInType == OP_DOUBLE ||
		  builtInType == OP_BINDLESSHANDLETEXTURE2D ||
		  builtInType == OP_BINDLESSHANDLETEXTURE3D ||
		  builtInType == OP_BINDLESSHANDLETEXTURECUBE ||
		  builtInType == OP_BINDLESSHANDLESAMPLER );
}

bool Type::IsTexture() const
{
	return symbol == nullptr &&
		( builtInType == OP_TEXTURE ||
		  builtInType == OP_TEXTURE1D ||
		  builtInType == OP_TEXTURE2D ||
		  builtInType == OP_TEXTURE3D ||
		  builtInType == OP_TEXTURECUBE ||
		  builtInType == OP_TEXTURE1DARRAY ||
		  builtInType == OP_TEXTURE2DARRAY ||
		  builtInType == OP_TEXTURE3DARRAY ||
		  builtInType == OP_TEXTURECUBEARRAY ||
		  builtInType == OP_TEXTURE2DMS ||
		  builtInType == OP_TEXTURE2DMSARRAY ||
		  builtInType == OP_RWTEXTURE1D ||
		  builtInType == OP_RWTEXTURE2D ||
		  builtInType == OP_RWTEXTURE3D ||
		  builtInType == OP_RWTEXTURE1DARRAY ||
		  builtInType == OP_RWTEXTURE2DARRAY ||
		  builtInType == OP_RWTEXTURE3DARRAY );
}

bool Type::IsTextureArray() const
{
	return symbol == nullptr &&
		( builtInType == OP_TEXTURE1DARRAY ||
		  builtInType == OP_TEXTURE2DARRAY ||
		  builtInType == OP_TEXTURE3DARRAY ||
		  builtInType == OP_TEXTURECUBEARRAY ||
		  builtInType == OP_TEXTURE2DMSARRAY ||
		  builtInType == OP_RWTEXTURE1DARRAY ||
		  builtInType == OP_RWTEXTURE2DARRAY ||
		  builtInType == OP_RWTEXTURE3DARRAY );
}


bool Type::IsSampler() const
{
	return symbol == nullptr &&
		( builtInType == OP_SAMPLER ||
		  builtInType == OP_SAMPLER1D ||
		  builtInType == OP_SAMPLER2D ||
		  builtInType == OP_SAMPLER3D ||
		  builtInType == OP_SAMPLERCUBE ||
		  builtInType == OP_SAMPLERCOMPARISON );
}

bool Type::IsBuffer() const
{
	return symbol == nullptr &&
		( builtInType == OP_BUFFER ||
		  builtInType == OP_BYTEADDRESSBUFFER ||
		  builtInType == OP_STRUCTUREDBUFFER ||
		  builtInType == OP_APPENDSTRUCTUREDBUFFER ||
		  builtInType == OP_CONSUMESTRUCTUREDBUFFER ||
		  builtInType == OP_RWBUFFER ||
		  builtInType == OP_RWSTRUCTUREDBUFFER ||
		  builtInType == OP_RWBYTEADDRESSBUFFER );
}

bool Type::GetMethodType( ASTNode* methodCall, Type& returnType ) const
{
	if( symbol )
	{
		return false;
	}
	const InlineString& method = methodCall->GetToken()->stringValue;
	switch( builtInType )
	{
	case OP_TEXTURE2D:
	case OP_TEXTURE2DARRAY:
		if( method == MakeInlineString( "Gather" ) ||
			method == MakeInlineString( "GatherRed" ) ||
			method == MakeInlineString( "GatherGreen" ) ||
			method == MakeInlineString( "GatherBlue" ) ||
			method == MakeInlineString( "GatherAlpha" ) )
		{
			if( templateParameter )
			{
				returnType = *templateParameter;
			}
			else
			{
				returnType.FromTokenType( OP_FLOAT );
			}
			returnType.width = 4;
			return true;
		}
		if( method == MakeInlineString( "GatherCmp" ) ||
			method == MakeInlineString( "GatherCmpRed" ) ||
			method == MakeInlineString( "GatherCmpGreen" ) ||
			method == MakeInlineString( "GatherCmpBlue" ) ||
			method == MakeInlineString( "GatherCmpAlpha" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			returnType.width = 4;
			return true;
		}
	case OP_TEXTURE1D:
	case OP_TEXTURE3D:
	case OP_TEXTURECUBE:
	case OP_TEXTURE1DARRAY:
	case OP_TEXTURE3DARRAY:
	case OP_TEXTURECUBEARRAY:
		if( method == MakeInlineString( "CalculateLevelOfDetail" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			return true;
		}
		if( method == MakeInlineString( "CalculateLevelOfDetailUnclamped" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			return true;
		}
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "GetSamplePosition" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			returnType.width = 2;
			return true;
		}
		if( method == MakeInlineString( "Load" ) ||
			method == MakeInlineString( "Sample" ) ||
			method == MakeInlineString( "SampleBias" ) ||
			method == MakeInlineString( "SampleGrad" ) ||
			method == MakeInlineString( "SampleLevel" ) )
		{
			if( templateParameter )
			{
				returnType = *templateParameter;
			}
			else
			{
				returnType.FromTokenType( OP_FLOAT );
				if( isDepthTexture )
				{
					returnType.width = 1;
				}
				else
				{
					returnType.width = 4;
				}
			}
			return true;
		}
		if( method == MakeInlineString( "SampleCmp" ) ||
			method == MakeInlineString( "SampleCmpLevelZero" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			return true;
		}
		return false;
	case OP_TEXTURE2DMS:
	case OP_TEXTURE2DMSARRAY:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "GetSamplePosition" ) )
		{
			returnType.FromTokenType( OP_FLOAT );
			returnType.width = 2;
			return true;
		}
		if( method == MakeInlineString( "Load" ) )
		{
			returnType = *templateParameter;
			return true;
		}
		return false;
	case OP_BUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "Load" ) )
		{
			returnType = *templateParameter;
			return true;
		}
		return false;
	case OP_BYTEADDRESSBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "Load" ) )
		{
			returnType.FromTokenType( OP_UINT );
			return true;
		}
		if( method == MakeInlineString( "Load2" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 2;
			return true;
		}
		if( method == MakeInlineString( "Load3" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 3;
			return true;
		}
		if( method == MakeInlineString( "Load4" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 4;
			return true;
		}
		return false;
	case OP_APPENDSTRUCTUREDBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "Append" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
	case OP_CONSUMESTRUCTUREDBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "Consume" ) )
		{
			returnType = *templateParameter;
			return true;
		}
		return false;
	case OP_RWBUFFER:
	case OP_RWTEXTURE1D:
	case OP_RWTEXTURE1DARRAY:
	case OP_RWTEXTURE2D:
	case OP_RWTEXTURE2DARRAY:
	case OP_RWTEXTURE3D:
	case OP_RWTEXTURE3DARRAY:
	case OP_STRUCTUREDBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		return false;
	case OP_RWSTRUCTUREDBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "DecrementCounter" ) )
		{
			returnType.FromTokenType( OP_UINT );
			return true;
		}
		if( method == MakeInlineString( "IncrementCounter" ) )
		{
			returnType.FromTokenType( OP_UINT );
			return true;
		}
		return false;
	case OP_RWBYTEADDRESSBUFFER:
		if( method == MakeInlineString( "GetDimensions" ) ||
			method == MakeInlineString( "InterlockedAdd" ) ||
			method == MakeInlineString( "InterlockedAnd" ) ||

			method == MakeInlineString( "InterlockedCompareExchange" ) ||
			method == MakeInlineString( "InterlockedCompareStore" ) ||
			method == MakeInlineString( "InterlockedExchange" ) ||
			method == MakeInlineString( "InterlockedMax" ) ||
			method == MakeInlineString( "InterlockedMin" ) ||
			method == MakeInlineString( "InterlockedOr" ) ||
			method == MakeInlineString( "InterlockedXor" ) ||
			method == MakeInlineString( "Store" ) ||
			method == MakeInlineString( "Store2" ) ||
			method == MakeInlineString( "Store3" ) ||
			method == MakeInlineString( "Store4" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		if( method == MakeInlineString( "Load" ) )
		{
			returnType.FromTokenType( OP_UINT );
			return true;
		}
		if( method == MakeInlineString( "Load2" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 2;
			return true;
		}
		if( method == MakeInlineString( "Load3" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 3;
			return true;
		}
		if( method == MakeInlineString( "Load4" ) )
		{
			returnType.FromTokenType( OP_UINT );
			returnType.width = 4;
			return true;
		}
		return false;
	case OP_POINTSTREAM:
	case OP_LINESTREAM:
	case OP_TRIANGLESTREAM:
		if( method == MakeInlineString( "Append" ) ||
			method == MakeInlineString( "RestartStrip" ) )
		{
			returnType.FromTokenType( OP_VOID );
			return true;
		}
		return false;
	}
	return false;
}

std::string Type::ToString() const
{
	if( symbol )
	{
		return ::ToString( symbol->name );
	}
	std::string result;
	switch( builtInType )
	{
	case OP_FLOAT:
		result = "float";
		break;
	case OP_INT:
		result = "int";
		break;
	case OP_UINT:
		result = "uint";
		break;
	case OP_HALF:
		result = "half";
		break;
	case OP_DOUBLE:
		result = "double";
		break;
	case OP_BOOL:
		result = "bool";
		break;
	case OP_STRING:
		result = "string";
		break;
	case OP_VOID:
		return "void";
	case OP_SAMPLER2D:
	case OP_SAMPLER3D:
	case OP_SAMPLERCUBE:
	case OP_SAMPLER:
	case OP_SAMPLERCOMPARISON:
		return "sampler";
	case OP_TEXTURE1D:
		return "Texture1D";
	case OP_TEXTURE2D:
		return isDepthTexture ? "DepthTexture2D" : "Texture2D";
	case OP_TEXTURE3D:
		return "Texture3D";
	case OP_TEXTURECUBE:
		return "TextureCube";
	case OP_TEXTURE1DARRAY:
		return "Texture1DArray";
	case OP_TEXTURE2DARRAY:
		return isDepthTexture ? "DepthTexture2DArray" : "Texture2DArray";
	case OP_TEXTURE3DARRAY:
		return "Texture3DArray";
	case OP_TEXTURECUBEARRAY:
		return "TextureCubeArray";
	case OP_TEXTURE2DMS:
		return "Texture2DMS";
	case OP_TEXTURE2DMSARRAY:
		return "Texture2DMSArray";
	case OP_TEXTURE:
		return "Texture";
	case OP_BUFFER:
		return "Buffer";
	case OP_APPENDSTRUCTUREDBUFFER:
		return "AppendStructuredBuffer";
	case OP_BYTEADDRESSBUFFER:
		return "ByteAddressBuffer";
	case OP_CONSUMESTRUCTUREDBUFFER:
		return "ConsumeStructuredBuffer";
	case OP_INPUTPATCH:
		return "InputPatch";
	case OP_OUTPUTPATCH:
		return "OutputPatch";
	case OP_RWBUFFER:
		return "RWBuffer";
	case OP_RWBYTEADDRESSBUFFER:
		return "RWByteAddressBuffer";
	case OP_RWSTRUCTUREDBUFFER:
		return "RWStructuredBuffer";
	case OP_RWTEXTURE1D:
		return "RWTexture1D";
	case OP_RWTEXTURE1DARRAY:
		return "RWTexture1DArray";
	case OP_RWTEXTURE2D:
		return "RWTexture2D";
	case OP_RWTEXTURE2DARRAY:
		return "RWTexture2DArray";
	case OP_RWTEXTURE3D:
		return "RWTexture3D";
	case OP_RWTEXTURE3DARRAY:
		return "RWTexture3DArray";
	case OP_STRUCTUREDBUFFER:
		return "StructuredBuffer";
	case OP_POINTSTREAM:
		return "PointStream";
	case OP_LINESTREAM:
		return "LineStream";
	case OP_TRIANGLESTREAM:
		return "TriangleStream";
	case OP_RAYTRACING_ACCELERATION_STRUCTURE:
		return "RaytracingAccelerationStructure";
	case OP_RAY_DESC:
		return "RayDesc";
	}
	if( width > 1 || height > 1 )
	{
		char temp[64];
#if _WIN32
		_itoa_s( width, temp, 10 );
#else
		snprintf( temp, sizeof( temp ), "%d", width );
#endif
		result += temp;
		if( height > 1 )
		{
			result += "x";
#if _WIN32
			_itoa_s( height, temp, 10 );
#else
			snprintf( temp, sizeof( temp ), "%d", height );
#endif
			result += temp;
		}
	}
	for( int i = 0; i < arrayDimensions; ++i )
	{
		result += '[';
		result += std::to_string( arraySizes[i] );
		result += ']';
	}
	return result;
}

bool Type::CanImplicitCast( const Type& to, int& casts ) const
{
	// For user-defined types and resource types we don't have implicit casts
	if( symbol || to.symbol )
	{
		return symbol == to.symbol;
	}
	if( !IsScalarOrVector() || !to.IsScalarOrVector() )
	{
		return *this == to;
	}
	// Our types are scalars or vectors - check dimensions
	if( width == 1 && height == 1 )
	{
		if( to.width > 1 || to.height > 1 || to.builtInType != builtInType )
		{
			casts++;
		}
		return true;
	}
	if( width >= to.width && height >= to.height )
	{
		if( width != to.width || height != to.height )
		{
			casts++;
		}
		if( builtInType != to.builtInType )
		{
			casts++;
		}
		return true;
	}
	return false;
}

bool Type::GetIndexedType( Type& type ) const
{
	if( arrayDimensions > 0 )
	{
		type = *this;
		type.arrayDimensions--;
		return true;
	}
	if( symbol )
	{
		return false;
	}
	switch( builtInType )
	{
	case OP_TEXTURE1D:
	case OP_TEXTURE1DARRAY:
	case OP_TEXTURE2D:
	case OP_TEXTURE2DARRAY:
	case OP_TEXTURE3D:
	case OP_TEXTURE3DARRAY:
		if( templateParameter )
		{
			type = *templateParameter;
		}
		else
		{
			type.FromTokenType( OP_FLOAT );
			if( isDepthTexture )
			{
				type.width = 1;
			}
			else
			{
				type.width = 4;
			}
		}
		return true;
	case OP_BUFFER:
	case OP_INPUTPATCH:
	case OP_OUTPUTPATCH:

	case OP_RWBUFFER:
	case OP_RWSTRUCTUREDBUFFER:
	case OP_RWTEXTURE1D:
	case OP_RWTEXTURE1DARRAY:
	case OP_RWTEXTURE2D:
	case OP_RWTEXTURE2DARRAY:
	case OP_RWTEXTURE3D:
	case OP_RWTEXTURE3DARRAY:
	case OP_STRUCTUREDBUFFER:
		type = *templateParameter;
		return true;
	}
	if( width == 1 && height == 1 )
	{
		return false;
	}
	type = *this;
	if( height > 1 )
	{
		type.height = 1;
	}
	else
	{
		type.width = 1;
	}
	return true;
}

Type TypeFromSymbol( const Symbol* symbol )
{
	Type t;
	t.FromSymbol( symbol );
	return t;
}

Type TypeFromTokenType( int type, int width, int height )
{
	Type t;
	t.FromTokenType( type );
	t.width = width;
	t.height = height;
	return t;
}

bool GetCommonType( const Type& type0, const Type& type1, Type& type )
{
	int casts = 0;
	if( !type1.CanImplicitCast( type0, casts ) && !type0.CanImplicitCast( type1, casts ) )
	{
		return false;
	}
	if( type0.symbol )
	{
		type = type0;
		return true;
	}
	if( type1.symbol )
	{
		type = type1;
		return true;
	}
	//static const int typePrecedence[] = { OP_BOOL, OP_INT, OP_UINT, OP_HALF, OP_FLOAT, OP_DOUBLE };
	int prec0 = GetNumericTypePrecedence( type0.builtInType );
	int prec1 = GetNumericTypePrecedence( type1.builtInType );
	if( prec0 == -1 )
	{
		type = type0;
		return true;
	}
	if( prec1 == -1 )
	{
		type = type1;
		return true;
	}
	if( prec0 > prec1 )
	{
		type = type0;
	}
	else
	{
		type = type1;
	}
	if( type0.width == 1 || type1.width == 1 )
	{
		type.width = std::max( type0.width, type1.width );
	}
	else
	{
		type.width = std::min( type0.width, type1.width );
	}
	if( type0.height == 1 || type1.height == 1 )
	{
		type.height = std::max( type0.height, type1.height );
	}
	else
	{
		type.height = std::min( type0.height, type1.height );
	}
	return true;
}

int GetNumericTypePrecedence( int type )
{
	static const int typePrecedence[] = { OP_BOOL, OP_INT, OP_UINT, OP_HALF, OP_FLOAT, OP_DOUBLE };
	for( int i = 0; i < int( sizeof( typePrecedence ) / sizeof( int ) ); ++i )
	{
		if( type == typePrecedence[i] )
		{
			return i;
		}
	}
	return -1;
}



namespace hlsl
{

const Type void_t = TypeFromTokenType( OP_VOID );

const Type bool_t = TypeFromTokenType( OP_BOOL );
const Type int_t = TypeFromTokenType( OP_INT );
const Type uint_t = TypeFromTokenType( OP_UINT );
const Type float_t = TypeFromTokenType( OP_FLOAT );

const Type bool2_t = TypeFromTokenType( OP_BOOL, 2 );
const Type int2_t = TypeFromTokenType( OP_INT, 2 );
const Type uint2_t = TypeFromTokenType( OP_UINT, 2 );
const Type float2_t = TypeFromTokenType( OP_FLOAT, 2 );

const Type bool3_t = TypeFromTokenType( OP_BOOL, 3 );
const Type int3_t = TypeFromTokenType( OP_INT, 3 );
const Type uint3_t = TypeFromTokenType( OP_UINT, 3 );
const Type float3_t = TypeFromTokenType( OP_FLOAT, 3 );

const Type bool4_t = TypeFromTokenType( OP_BOOL, 4 );
const Type int4_t = TypeFromTokenType( OP_INT, 4 );
const Type uint4_t = TypeFromTokenType( OP_UINT, 4 );
const Type float4_t = TypeFromTokenType( OP_FLOAT, 4 );

}
