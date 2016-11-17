#include "stdafx.h"
#include "EffectCompilerDX11.h"
#include "CompileMessageQueue.h"
#include "StringTable.h"
#include "EffectData.h"

#include "SymbolTable.h"
#include "ASTNode.h"

#include "ParserUtils.h"
#include "HLSLParser.h"
#include "ParserState.h"

#include "FXAnalyzer.h"

#include "TextureFunctionConversionDX11.h"

extern bool g_generateListing;
extern std::string g_listing;
extern CRITICAL_SECTION g_listingCS;
extern int g_maxClipPlanes;

extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern bool g_printWarnings;
extern unsigned g_optimizationLevel;
extern bool g_printShaderStats;
extern bool g_avoidFlowControl;

void PrintHLSL11( CodeStream& os, ASTNode* node, int level );

void PrintChildrenHLSL11( CodeStream& os, ASTNode* node, const char* glue, int level, unsigned start = 0 )
{
	for( unsigned i = start; i < node->GetChildrenCount(); ++i )
	{
		if( i )
		{
			os << glue;
		}
		PrintHLSL11( os, node->GetChild( i ), level );
	}
}

void PrintTypeHLSL11( std::ostream& os, Type type )
{
	switch( type.storageClass )
	{
	case OP_EXTERN:
		os << "extern ";
		break;
	case OP_NOINTERPOLATION:
		os << "nointerpolation ";
		break;
	case OP_PRECISE:
		os << "precise ";
		break;
	case OP_SHARED:
		os << "shared ";
		break;
	case OP_GROUPSHARED:
		os << "groupshared ";
		break;
	case OP_STATIC:
		os << "static ";
		break;
	case OP_UNIFORM:
		os << "uniform ";
		break;
	case OP_VOLATILE:
		os << "volatile ";
		break;
	}
	switch( type.modifier )
	{
	case OP_CONST:
		os << "const ";
		break;
	case OP_ROW_MAJOR:
		os << "row_major ";
		break;
	case OP_COLUMN_MAJOR:
		os << "column_major ";
		break;
	}
	if( type.symbol )
	{
		os << type.symbol->name;
	}
	else
	{
		switch( type.builtInType )
		{
		case OP_FLOAT:
			os << "float";
			break;
		case OP_INT:
			os << "int";
			break;
		case OP_UINT:
			os << "uint";
			break;
		case OP_HALF:
			os << "half";
			break;
		case OP_DOUBLE:
			os << "double";
			break;
		case OP_BOOL:
			os << "bool";
			break;
		case OP_STRING:
			os << "string";
			break;
		case OP_VOID:
			os << "void";
			return;
		case OP_SAMPLER2D:
		case OP_SAMPLER3D:
		case OP_SAMPLERCUBE:
		case OP_SAMPLER:
			os << "sampler";
			return;
		case OP_SAMPLERCOMPARISON:
			os << "SamplerComparisonState";
			return;
		case OP_TEXTURE1D:
			os << "Texture1D";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE2D:
			os << "Texture2D";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE3D:
			os << "Texture3D";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURECUBE:
			os << "TextureCube";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE1DARRAY:
			os << "Texture1DArray";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE2DARRAY:
			os << "Texture2DArray";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE3DARRAY:
			os << "Texture3DArray";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURECUBEARRAY:
			os << "TextureCubeArray";
			if( type.templateParameter )
			{
				os << '<';
				PrintTypeHLSL11( os, *type.templateParameter );
				os << '>';
			}
			return;
		case OP_TEXTURE2DMS:
			os << "Texture2DMS";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			if( type.templateSamples > 0 )
			{
				os << ", " << type.templateSamples;
			}
			os << '>';
			return;
		case OP_TEXTURE2DMSARRAY:
			os << "Texture2DMSArray";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			if( type.templateSamples > 0 )
			{
				os << ", " << type.templateSamples;
			}
			os << '>';
			return;
		case OP_TEXTURE:
			os << "Texture2D";
			return;
		case OP_BUFFER:
			os << "Buffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_APPENDSTRUCTUREDBUFFER:
			os << "AppendStructuredBuffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_BYTEADDRESSBUFFER:
			os << "ByteAddressBuffer";
			return;
		case OP_CONSUMESTRUCTUREDBUFFER:
			os << "ConsumeStructuredBuffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_INPUTPATCH:
			os << "InputPatch";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << ", " << type.templateSamples << '>';
			return;
		case OP_OUTPUTPATCH:
			os << "OutputPatch";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << ", " << type.templateSamples << '>';
			return;
		case OP_RWBUFFER:
			os << "RWBuffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWBYTEADDRESSBUFFER:
			os << "RWByteAddressBuffer";
			return;
		case OP_RWSTRUCTUREDBUFFER:
			os << "RWStructuredBuffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE1D:
			os << "RWTexture1D";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE1DARRAY:
			os << "RWTexture1DArray";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE2D:
			os << "RWTexture2D";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE2DARRAY:
			os << "RWTexture2DArray";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE3D:
			os << "RWTexture3D";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_RWTEXTURE3DARRAY:
			os << "RWTexture3DArray";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_STRUCTUREDBUFFER:
			os << "StructuredBuffer";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_POINTSTREAM:
			os << "PointStream";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_LINESTREAM:
			os << "LineStream";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		case OP_TRIANGLESTREAM:
			os << "TriangleStream";
			os << '<';
			PrintTypeHLSL11( os, *type.templateParameter );
			os << '>';
			return;
		default:
			os << "!!error_type!!";
			return;
		}
		if( type.width > 1 || type.height > 1 )
		{
			os << type.width;
			if( type.height > 1 )
			{
				os << 'x' << type.height;
			}
		}
	}
}

static const char* GetOperatorSymbol( int operatorID )
{
	static std::map<int, std::string> operators;
	if( operators.empty() )
	{
		operators[OP_MUL_ASSIGN] = "*=";
		operators[OP_DIV_ASSIGN] = "/=";
		operators[OP_MOD_ASSIGN] = "%=";
		operators[OP_ADD_ASSIGN] = "+=";
		operators[OP_SUB_ASSIGN] = "-=";
		operators[OP_LEFT_ASSIGN] = "<<=";
		operators[OP_RIGHT_ASSIGN] = ">>=";
		operators[OP_AND_ASSIGN] = "&=";
		operators[OP_XOR_ASSIGN] = "^=";
		operators[OP_OR_ASSIGN] = "|=";

		operators[OP_INC_OP] = "++";
		operators[OP_DEC_OP] = "--";
		operators[OP_LEFT_OP] = "<<";
		operators[OP_RIGHT_OP] = ">>";

		operators[OP_LE_OP] = "<=";
		operators[OP_GE_OP] = ">=";
		operators[OP_EQ_OP] = "==";
		operators[OP_NE_OP] = "!=";

		operators[OP_AND_OP] = "&&";
		operators[OP_OR_OP] = "||";

		operators[OP_PLUS] = "+";
		operators[OP_DASH] = "-";
		operators[OP_BANG] = "!";
		operators[OP_TILDE] = "~";
		operators[OP_STAR] = "*";
		operators[OP_SLASH] = "/";
		operators[OP_PERCENT] = "%";
		operators[OP_COMA] = ",";
		operators[OP_LESS] = "<";
		operators[OP_MORE] = ">";
		operators[OP_AMPERSAND] = "&";
		operators[OP_CARET] = "^";
		operators[OP_VERTICAL_BAR] = "|";
		operators[OP_EQUAL] = "=";
	}
	auto it = operators.find( operatorID );
	if( it == operators.end() )
	{
		return "";
	}
	return it->second.c_str();
}

bool HasUsedDeclarations( ASTNode* node )
{
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		if( node->GetChild( i ) )
		{
			if( node->GetChild( i )->GetNodeType() == NT_VAR_DECLARATION_LIST )
			{
				if( HasUsedDeclarations( node->GetChild( i ) ) )
				{
					return true;
				}
			}
			else if( node->GetChild( i )->GetNodeType() == NT_NAME_DECLARATION )
			{
				if( node->GetChild( i )->GetSymbol() && node->GetChild( i )->GetSymbol()->used )
				{
					return true;
				}
			}
		}
	}
	return false;
}

void PrintHLSL11( CodeStream& os, ASTNode* node, int level )
{
	os << node->GetLocation();

	switch( node->GetNodeType() )
	{
	case NT_VAR_IDENTIFIER:
		os << node->GetSymbol()->name;
		break;
	case NT_CONSTANT:
		os << node->GetToken()->stringValue;
		break;
	case NT_INLINE_CONSTRUCTOR:
		os << "{ ";
		PrintChildrenHLSL11( os, node, ", ", level + 1 );
		os << " }";
		break;
	case NT_PREFIX_EXPRESSION:
		os << GetOperatorSymbol( node->GetToken()->type ) << "( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		break;
	case NT_POSTFIX_EXPRESSION:
		os << "( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		switch( node->GetToken()->type )
		{
		case OP_LEFT_BRACKET:
			os << "[";
			PrintHLSL11( os, node->GetChild( 1 ), level );
			os << "]";
			break;
		case OP_DOT:
			os << ".";
			PrintHLSL11( os, node->GetChild( 1 ), level );
			break;
		case OP_ID:
			os << "." << node->GetToken()->stringValue;
			break;
		default:
			os << GetOperatorSymbol( node->GetToken()->type );
			break;
		}
		break;
	case NT_EXPRESSION:
		os << "( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		switch( node->GetToken()->type )
		{
		case OP_LEFT_PAREN:
			break;
		default:
			os << " " << GetOperatorSymbol( node->GetToken()->type ) << " ( ";
			PrintHLSL11( os, node->GetChild( 1 ), level );
			os << " )";
			break;
		}
		break;
	case NT_CONDITIONAL_EXPRESSION:
		os << "( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " ) ? ( ";
		PrintHLSL11( os, node->GetChild( 1 ), level );
		os << " ) : ( ";
		PrintHLSL11( os, node->GetChild( 2 ), level );
		os << " )";
		break;
	case NT_CAST_EXPRESSION:
		os << "(";
		PrintTypeHLSL11( os, node->GetType() );
		os << ")( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		break;
	case NT_FUNCTION_CALL:
		if( node->GetSymbol() == nullptr )
		{
			if( node->GetToken()->type == OP_ID )
			{
				os << node->GetToken()->stringValue;
			}
			else
			{
				Type t;
				t.FromToken( *node->GetToken() );
				PrintTypeHLSL11( os, t );
			}
		}
		else
		{
			os << node->GetSymbol()->name;
		}
		os << "( ";
		PrintChildrenHLSL11( os, node, ", ", level + 1 );
		os << " )";
		break;
	case NT_FUNCTION_HEADER:
		if( !node->GetSymbol()->used )
		{
			break;
		}
		PrintTypeHLSL11( os, node->GetType() );
		os << " " << node->GetSymbol()->name << "( ";
		PrintChildrenHLSL11( os, node, ", ", level + 1 );
		os << " )";
		if( node->GetSymbol()->semantic.start )
		{
			os << " : " << node->GetSymbol()->semantic;
		}
		os.Endl();
		break;
	case NT_FUNCTION_PARAMETER:
		if( node->GetToken() )
		{
			switch( node->GetToken()->type )
			{
			case OP_OUT:
				os << "out ";
				break;
			case OP_INOUT:
				os << "inout ";
				break;
			}
		}
		if( node->GetSymbol() )
		{
			switch( node->GetSymbol()->interpolationModifier )
			{
			case OP_LINEAR:
				os << "linear ";
				break;
			case OP_CENTROID:
				os << "centroid ";
				break;
			case OP_NOINTERPOLATION:
				os << "nointerpolation ";
				break;
			case OP_NOPERSPECTIVE:
				os << "noperspective ";
				break;
			}
		}
		if( node->GetChildOrNull( 2 ) )
		{
			PrintHLSL11( os, node->GetChild( 2 ), level + 1 );
			os << ' ';
		}
		PrintTypeHLSL11( os, node->GetType() );
		if( node->GetSymbol() )
		{
			os << " " << node->GetSymbol()->name;
		}
		if( node->GetChildOrNull( 0 ) )
		{
			os << "[";
			PrintHLSL11( os, node->GetChild( 0 ), level );
			os << "]";
		}
		if( node->GetSymbol() )
		{
			if( node->GetSymbol()->semantic.start )
			{
				os << " : " << node->GetSymbol()->semantic;
			}
		}
		if( node->GetChildOrNull( 1 ) )
		{
			os << " = ";
			PrintHLSL11( os, node->GetChild( 1 ), level );
		}
		break;
	case NT_NAME_DECLARATION:
		{
			if( !node->GetSymbol()->used )
			{
				break;
			}
			switch( node->GetSymbol()->interpolationModifier )
			{
			case OP_LINEAR:
				os << "linear ";
				break;
			case OP_CENTROID:
				os << "centroid ";
				break;
			case OP_NOINTERPOLATION:
				os << "nointerpolation ";
				break;
			case OP_NOPERSPECTIVE:
				os << "noperspective ";
				break;
			}
			PrintTypeHLSL11( os, node->GetSymbol()->type );
			os << " " << node->GetSymbol()->name;
			if( node->GetChildOrNull( 0 ) )
			{
				os << "[";
				PrintHLSL11( os, node->GetChild( 0 ), level );
				os << "]";
			}
			if( node->GetSymbol()->semantic.start )
			{
				os << " : " << node->GetSymbol()->semantic;
			}
			if( node->GetSymbol()->packOffset.subComponent >= 0 )
			{
				os << " : packoffset( c" << node->GetSymbol()->packOffset.subComponent;
				if( node->GetSymbol()->packOffset.component.start )
				{
					os << "." << node->GetSymbol()->packOffset.component;
				}
				os << " )";
			}
			for( auto it = node->GetSymbol()->registerSpecifier.begin(); it != node->GetSymbol()->registerSpecifier.end(); ++it )
			{
				const RegisterSpecifier& reg = it->second;
				if( reg.registerType == 't' || reg.registerType == 'T' ||
					reg.registerType == 'b' || reg.registerType == 'B' ||
					reg.registerType == 'u' || reg.registerType == 'U' )
				{
					os << " : register( ";
					if( reg.shaderProfile.start != reg.shaderProfile.end )
					{
						os << reg.shaderProfile << ", ";
					}
					os << reg.registerType << reg.registerNumber;
					if( reg.subComponent >= 0 )
					{
						os << "[" << reg.subComponent << "]";
					}
					os << " )";
				}
			}
			if( node->GetChildOrNull( 1 ) )
			{
				os << " = ";
				PrintHLSL11( os, node->GetChild( 1 ), level );
			}
			os << ";";
			os.Endl();
		}
		break;
	case NT_VAR_DECLARATION_LIST:
		PrintChildrenHLSL11( os, node, "", level + 1 );
		break;
	case NT_STRUCT:
		if( node->GetSymbol()->used )
		{
			os << "struct " << " " << node->GetSymbol()->name;
			os.Endl();
			os << "{";
			os.Endl();
			PrintChildrenHLSL11( os, node, "", level + 1 );
			os << "};";
			os.Endl();
		}
		break;
	case NT_STRUCT_MEMBER:
		PrintChildrenHLSL11( os, node, "", level + 1 );
		break;
	case NT_PROGRAM:
		for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
		{
			PrintHLSL11( os, node->GetChild( i ), level + 1 );
			if( node->GetChild( i ) && node->GetChild( i )->GetNodeType() == NT_FUNCTION_HEADER )
			{
				os << ';';
			}
		}
		break;
	case NT_BLOCK:
		os << "{";
		os.Endl();
		PrintChildrenHLSL11( os, node, "", level + 1 );
		os << "}";
		os.Endl();
		break;
	case NT_EXPRESSION_STATEMENT:
		if( node->GetChildrenCount() )
		{
			PrintHLSL11( os, node->GetChild( 0 ), level );
		}
		os << ";";
		os.Endl();
		break;
	case NT_IF:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "if( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		os.Endl();
		PrintHLSL11( os, node->GetChild( 1 ), level + 1 );
		if( node->GetChildrenCount() > 2 )
		{
			os << "else";
			os.Endl();
			PrintHLSL11( os, node->GetChild( 2 ), level );
		}
		break;
	case NT_WHILE:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "while( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " )";
		os.Endl();
		PrintHLSL11( os, node->GetChild( 1 ), level + 1 );
		break;
	case NT_DO:
		os << "do";
		os.Endl();
		PrintHLSL11( os, node->GetChild( 1 ), level + 1 );
		os << "while( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		os << " );";
		os.Endl();
		break;
	case NT_FOR:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "for( ";
		PrintHLSL11( os, node->GetChild( 0 ), level );
		if( node->GetChild( 1 ) )
		{
			PrintHLSL11( os, node->GetChild( 1 ), level );
		}
		os << "; ";
		if( node->GetChild( 2 ) )
		{
			PrintHLSL11( os, node->GetChild( 2 ), level );
		}
		os << " )";
		os.Endl();
		PrintHLSL11( os, node->GetChild( 3 ), level + 1 );
		break;
	case NT_SWITCH:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "switch( ";
		PrintHLSL11( os, node->GetChild( 0 ), level + 1 );
		os << " )";
		os.Endl();
		os << "{";
		os.Endl();
		PrintChildrenHLSL11( os, node, "", level + 1, 1 );
		os << "}";
		os.Endl();
		break;
	case NT_CASE:
		if( node->GetChildOrNull( 0 ) == nullptr )
		{
			os << "default:";
			os.Endl();
		}
		else
		{
			os << "case ";
			PrintHLSL11( os, node->GetChild( 0 ), level + 1 );
			os << ":";
			os.Endl();
		}
		PrintHLSL11( os, node->GetChild( 1 ), level + 1 );
		break;
	case NT_JUMP:
		switch( node->GetToken()->type )
		{
		case OP_CONTINUE:
			os << "continue;";
			break;
		case OP_BREAK:
			os << "break;";
			break;
		case OP_RETURN:
			if( node->GetChildrenCount() == 0 )
			{
				os << "return;";
			}
			else
			{
				os << "return ";
				PrintHLSL11( os, node->GetChild( 0 ), level + 1 );
				os << ";";
			}
			break;
		case OP_DISCARD:
			os << "discard;";
			break;
		}
		os.Endl();
		break;
	case NT_FUNCTION_DEFINITION:
		if( node->GetChild( 0 )->GetSymbol()->used )
		{
			if( node->GetChildOrNull( 2 ) )
			{
				PrintHLSL11( os, node->GetChild( 2 ), level + 1 );
			}
			PrintHLSL11( os, node->GetChild( 0 ), level + 1 );
			PrintHLSL11( os, node->GetChild( 1 ), level + 1 );
		}
		break;
	case NT_SAMPLER_STATE_LIST:
		os << "sampler_state";
		os.Endl();
		os << "{";
		os.Endl();
		os << "}";
		os.Endl();
		break;
	case NT_STATE_ASSIGNMENT:
		os << node->GetToken()->stringValue << " = ";
		if( node->GetSymbol() )
		{
			os << '<' << node->GetSymbol()->name << '>';
		}
		else
		{
			PrintHLSL11( os, node->GetChild( 0 ), level + 1 );
		}
		os << ";";
		os.Endl();
		break;
	case NT_CBUFFER:
		{
			if( !HasUsedDeclarations( node ) )
			{
				break;
			}
			if( node->GetToken()->type == OP_TBUFFER )
			{
				os << "tbuffer ";
			}
			else
			{
				os << "cbuffer ";
			}
			os << node->GetSymbol()->name;
			for( auto it = node->GetSymbol()->registerSpecifier.begin(); it != node->GetSymbol()->registerSpecifier.end(); ++it )
			{
				os << " : register( " << it->second.registerType << it->second.registerNumber << " )";
			}
			os.Endl();
			os << "{";
			os.Endl();
			PrintChildrenHLSL11( os, node, "", level + 1 );
			os << "}";
			os.Endl();
		}
		break;
	case NT_STATE_VALUE:
		os << node->GetToken()->stringValue;
		break;
	case NT_FUNCTION_ATTRIBUTE_LIST:
		PrintChildrenHLSL11( os, node, "", level + 1 );
		break;
	case NT_FUNCTION_ATTRIBUTE:
		os << '[' << node->GetToken()->stringValue;
		if( node->GetChildrenCount() )
		{
			os << '(';
			PrintChildrenHLSL11( os, node, ", ", level + 1 );
			os << ')';
		}
		os << ']';
		os.Endl();
		break;
	case NT_FUNCTION_ATTRIBUTE_VALUE:
		os << node->GetToken()->stringValue;
		break;
	case NT_PRIMITIVE_TYPE:
		os << node->GetToken()->stringValue;
		break;
	}
}

bool MakeEffectAnnotationFromSymbolAnnotation( const SymbolAnnotation& annotation, Annotation& result, bool& isSRGB, bool& isAutoregister )
{
	switch( annotation.type )
	{
	case OP_FLOAT:
	case OP_HALF:
	case OP_DOUBLE:
		result.type = ANNOTATION_TYPE_FLOAT;
		result.floatValue = float( ParseFloat( annotation.value.stringValue.start, annotation.value.stringValue.end ) );
		return true;

	case OP_UINT:
	case OP_INT:
		result.type = ANNOTATION_TYPE_INT;
		result.intValue = ParseNumber( annotation.value.stringValue.start, annotation.value.stringValue.end );
		return true;

	case OP_BOOL:
		result.type = ANNOTATION_TYPE_BOOL;
		result.intValue = annotation.value.intValue ? 1 : 0;
		if( ToString( annotation.name ) == "Tr2sRGB" )
		{
			isSRGB = result.intValue != 0;
		}
		else if( ToString( annotation.name ) == "AutoRegister" )
		{
			isAutoregister = result.intValue != 0;
		}
		return true;

	case OP_STRING:
		result.type = ANNOTATION_TYPE_STRING;
		result.stringValue = g_stringTable.AddString( ParseString( annotation.value.stringValue ).c_str() );
		return true;
	}

	return false;
}

static bool GetTextureType( const D3D11_SHADER_INPUT_BIND_DESC& desc, TextureType& type )
{
	switch( desc.Type )
	{
	case D3D_SIT_TEXTURE:
		type = TEX_TYPE_TYPELESS;
		switch( desc.Dimension )
		{
		case D3D10_SRV_DIMENSION_TEXTURE1D:
			type = TEX_TYPE_1D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURE2D:
			type = TEX_TYPE_2D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURE3D:
			type = TEX_TYPE_3D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURECUBE:
			type = TEX_TYPE_CUBE;
			break;
		case D3D10_SRV_DIMENSION_BUFFER:
			type = TEX_TYPE_BUFFER;
			break;
		}
		break;
	case D3D_SIT_TBUFFER:
		type = TEX_TYPE_TBUFFER;
		break;
	case D3D_SIT_STRUCTURED:
		type = TEX_TYPE_STRUCTURED_BUFFER;
		break;
	case D3D_SIT_BYTEADDRESS:
		type = TEX_TYPE_BYTEADDRESS_BUFFER;
		break;
	case D3D_SIT_UAV_RWTYPED:
		type = TEX_TYPE_UAV_RWTYPED;
		break;
	case D3D_SIT_UAV_RWSTRUCTURED:
		type = TEX_TYPE_UAV_RWSTRUCTURED;
		break;
	case D3D_SIT_UAV_RWBYTEADDRESS:
		type = TEX_TYPE_UAV_RWBYTEADDRESS;
		break;
	case D3D_SIT_UAV_APPEND_STRUCTURED:
		type = TEX_TYPE_UAV_APPEND_STRUCTURED;
		break;
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
		type = TEX_TYPE_UAV_CONSUME_STRUCTURED;
		break;
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		type = TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER;
		break;
	default:
		return false;
	}
	return true;
}

static bool GetStageData( ParserState& parserState, ID3D11ShaderReflection* reflection, StageInput& stage, std::map<StringReference, ParameterAnnotation> &annotations )
{
	D3D11_SHADER_DESC reflDesc;
	if( FAILED( reflection->GetDesc( &reflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}

	reflection->GetThreadGroupSize( &stage.threadGroupSize[0], &stage.threadGroupSize[1], &stage.threadGroupSize[2] );

	for( unsigned cbIndex = 0; cbIndex < reflDesc.ConstantBuffers; ++cbIndex )
	{
		ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex( cbIndex );
		// TODO: identify buffer by register
		D3D11_SHADER_BUFFER_DESC cbDesc;
		cb->GetDesc( &cbDesc );
		if( strcmp( cbDesc.Name, "$Globals" ) )
		{
			continue;
		}

		for( unsigned i = 0; i < cbDesc.Variables ; ++i )
		{
			ID3D11ShaderReflectionVariable* variable = cb->GetVariableByIndex( i );

			D3D11_SHADER_VARIABLE_DESC varDesc;
			if( FAILED( variable->GetDesc( &varDesc ) ) )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader constant #%i description", i );
				}
				continue;
			}
			if( ( varDesc.uFlags & D3D_SVF_USED ) == 0 )
			{
				continue;
			}

			ID3D11ShaderReflectionType* type = variable->GetType();
			D3D11_SHADER_TYPE_DESC typeDesc;
			if( FAILED( type->GetDesc( &typeDesc ) ) )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader constant \"%s\" type description", varDesc.Name );
				}
				continue;
			}
				
			Constant constant;
			constant.name = g_stringTable.AddString( varDesc.Name );
			constant.offset = varDesc.StartOffset;
			constant.size = varDesc.Size;

			switch( typeDesc.Type )
			{
			case D3D10_SVT_FLOAT:
				constant.type = CONSTANT_TYPE_FLOAT;
				break;
			case D3D10_SVT_INT:
				constant.type = CONSTANT_TYPE_INT;
				break;
			case D3D10_SVT_BOOL:
				constant.type = CONSTANT_TYPE_BOOL;
				break;
			default:
				constant.type = CONSTANT_TYPE_OTHER;
			}
			switch( typeDesc.Class )
			{
			case D3D10_SVC_SCALAR:
				constant.dimension = 1;
				break;
			case D3D10_SVC_VECTOR:
				constant.dimension = 4;
				break;
			case D3D10_SVC_MATRIX_ROWS:
			case D3D10_SVC_MATRIX_COLUMNS:
				constant.dimension = 16;
				break;
			default:
				constant.dimension = 1;
			}
			constant.elements = typeDesc.Elements;
			constant.isSRGB = false;
			constant.isAutoregister = false;

			if( varDesc.DefaultValue )
			{
				stage.defaultValues.resize( max( stage.defaultValues.size(), constant.offset + constant.size ) );
				memcpy( &stage.defaultValues[constant.offset], varDesc.DefaultValue, constant.size );
			}

			if( annotations.find( constant.name ) == annotations.end() )
			{
				ParameterAnnotation paramAnnotations;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( varDesc.Name );
				if( symbol && symbol->annotations )
				{
					for( auto a = symbol->annotations->begin(); a != symbol->annotations->end(); ++a )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *a, result, constant.isSRGB, constant.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( a->name ).c_str() )] = result;
						}
					}
					if( !paramAnnotations.annotations.empty() )
					{
						annotations[constant.name] = paramAnnotations;
					}
				}
			}

			stage.constants.push_back( constant );
		}
	}

	for( unsigned i = 0; i < reflDesc.BoundResources; ++i )
	{
		D3D11_SHADER_INPUT_BIND_DESC desc;
		if( FAILED( reflection->GetResourceBindingDesc(i, &desc) ) )
		{
			if( g_printWarnings )
			{
				g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader resource #%i description", i );
			}
			continue;
		}
		switch( desc.Type )
		{
		case D3D_SIT_SAMPLER:
			{
				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr || symbol->definition == nullptr )
				{
					if( g_printWarnings )
					{
						g_messages.AddMessage( "\\memory(0): warning X0000: Could not find sampler \"%s\" definition in the source code", desc.Name );
					}
					continue;
				}
				Sampler sampler;
				sampler.name = g_stringTable.AddString( desc.Name );
				if( !GetSamplerState( parserState, symbol->definition, sampler ) )
				{
					return false;
				}
				stage.samplers[desc.BindPoint] = sampler;
			}
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			{
				Uav uav;
				uav.isAutoregister = false;
				std::string textureName = desc.Name;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr )
				{
					continue;
				}
				if( symbol && symbol->annotations )
				{
					ParameterAnnotation paramAnnotations;

					bool srgb;
					uav.isAutoregister = false;

					for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *ai, result, srgb, uav.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;					
						}
					}

					if( !paramAnnotations.annotations.empty() )
					{
						annotations[ g_stringTable.AddString( textureName.c_str() ) ] = paramAnnotations;
					}
				}


				uav.name = g_stringTable.AddString( textureName.c_str() );
				if( !GetTextureType( desc, uav.type ) )
				{
					continue;
				}
				stage.uavs[desc.BindPoint] = uav;
			}
			break;
		default:
			{
				Texture texture;
				texture.isSRGB = false;
				texture.isAutoregister = false;
				std::string textureName = desc.Name;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr )
				{
					continue;
				}
				// For legacy sampling functions (like tex2D) the reflection will create "dummy"
				// texture object with the same name as the sampler, so we need to find the
				// actual texture object from sampler definition.
				if( symbol && symbol->type.IsSampler() )
				{
					if( symbol->definition )
					{
						ASTNode* states = symbol->definition->GetChildOrNull( 1 );
						if( states && states->GetNodeType() == NT_SAMPLER_STATE_LIST )
						{
							for( size_t k = 0; k < states->GetChildrenCount(); ++k )
							{
								ASTNode* state = states->GetChild( k );
								std::string name = ToString( state->GetToken()->stringValue );
								if( _stricmp( name.c_str(), "Texture" ) == 0 )
								{
									ASTNode* value = state->GetChildOrNull( 0 );
									if( value && value->GetSymbol() )
									{
										textureName = ToString( value->GetSymbol()->name );
									}
								}
								else if( _stricmp( name.c_str(), "SRGBTexture" ) == 0 )
								{
									Sampler sampler;
									sampler.srgbTexture = 0;
									ParseStateAssignment( parserState, state, g_samplerStates, &sampler );
									texture.isSRGB = sampler.srgbTexture != 0;
								}
							}
						}
					}
				}
				else if( symbol && symbol->annotations )
				{
					ParameterAnnotation paramAnnotations;
				
					texture.isSRGB = false;
					texture.isAutoregister = false;

					for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *ai, result, texture.isSRGB, texture.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;					
						}
					}

					if( !paramAnnotations.annotations.empty() )
					{
						annotations[ g_stringTable.AddString( textureName.c_str() ) ] = paramAnnotations;
					}
				}


				texture.name = g_stringTable.AddString( textureName.c_str() );
				if( !GetTextureType( desc, texture.type ) )
				{
					continue;
				}
				stage.textures[desc.BindPoint] = texture;
			}
		}
	}
	for( unsigned k = 0; k < reflDesc.InputParameters; ++k )
	{
		D3D11_SIGNATURE_PARAMETER_DESC desc;
		if( FAILED( reflection->GetInputParameterDesc( k, &desc ) ) )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader input parameter description" );
			return false;
		}
		if( desc.SystemValueType == D3D10_NAME_UNDEFINED )
		{
			static const char* usageNames[] = {
				"POSITION",
				"COLOR",
				"NORMAL",
				"TANGENT",
				"BINORMAL",
				"TEXCOORD",
				"BLENDINDICES",
				"BLENDWEIGHT"
				};
			bool found = false;
			InputDescription input;
			for( int n = 0; n < UC_NUM_USAGE_CODE; ++n )
			{
				if( _stricmp( desc.SemanticName, usageNames[n] ) == 0 )
				{
					input.name = n;
					input.registerIndex = desc.Register;
					input.index = desc.SemanticIndex;
					input.usedMask = desc.ReadWriteMask;
					input.componentType = desc.ComponentType;
					stage.inputs.push_back( input );
					found = true;
				}
			}
			if( !found && g_printWarnings )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: Shader uses unsupported input semantics \"%s\"", desc.SemanticName );
				return false;
			}
		}
	}
	return true;
}


static bool FindParameterBySemantics( ASTNode* node, const char** semantics, std::vector<Symbol*>* path, bool outParameter = false )
{
	for( unsigned j = 0; j < node->GetChildrenCount(); ++j )
	{
		Symbol* symbol = node->GetChild( j )->GetSymbol();
		if( symbol == nullptr )
		{
			continue;
		}
		if( outParameter )
		{
			if( node->GetChild( j )->GetToken() == 0 || node->GetChild( j )->GetToken()->type == OP_IN )
			{
				continue;
			}
		}
		for( unsigned k = 0; semantics[k]; ++k )
		{
			if( _stricmp( ToString( symbol->semantic ).c_str(), semantics[k] ) == 0 )
			{
				if( path )
				{
					path->clear();
					path->push_back( symbol );
				}
				return true;
			}
		}
		if( node->GetChild( j )->GetType().symbol && node->GetChild( j )->GetType().symbol->definition )
		{
			for( unsigned i = 0; i < node->GetChild( j )->GetType().symbol->definition->GetChildrenCount(); ++i )
			{
				if( FindParameterBySemantics( node->GetChild( j )->GetType().symbol->definition->GetChild( i ), semantics, path ) )
				{
					if( path )
					{
						path->insert( path->begin(), symbol );
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool FindOutputBySemantics( ASTNode* node, const char** semantics, std::vector<Symbol*>* path )
{
	// check "out" function arguments
	if( FindParameterBySemantics( node, semantics, path, true ) )
	{
		return true;
	}
	// check return value if it has semantics
	if( node->GetSymbol() && node->GetSymbol()->semantic.start )
	{
		std::string sematic = ToString( node->GetSymbol()->semantic );
		for( unsigned i = 0; semantics[i]; ++i )
		{
			if( _stricmp( sematic.c_str(), semantics[i] ) == 0 )
			{
				if( path )
				{
					path->push_back( nullptr );
				}
				return true;
			}
		}
	}
	// finally check return value as a structure
	if( node->GetType().symbol && 
		node->GetType().symbol->definition )
	{
		for( unsigned i = 0; i < node->GetType().symbol->definition->GetChildrenCount(); ++i )
		{
			if( FindParameterBySemantics( node->GetType().symbol->definition->GetChild( i ), semantics, path ) )
			{
				if( path )
				{
					path->insert( path->begin(), nullptr );
				}
				return true;
			}
		}
	}
	return false;
}

void PrintValuePath( std::ostream& os, const std::vector<Symbol*>& path, const char* functionSymbol )
{
	bool first = true;
	for( auto it = path.begin(); it != path.end(); ++it )
	{
		if( first )
		{
			first = false;
		}
		else
		{
			os << ".";
		}
		if( *it )
		{
			os << ( *it )->name;
		}
		else
		{
			os << functionSymbol;
		}
	}
}



static PatchAction PatchShader( InputStageType type, bool patchOutput, bool pixelOffset, ASTNode* callNode, ParserState& state, CodeStream& os, std::string& entryPointName, bool& hasShadowState )
{
	// 1. wrap uniforms
	// 2. fix VPOS
	// 3. alpha test
	// 4. fix VFACE: it appears that HLSL compiler may crash for cirtain usages of VFACE
	// 5. 0.5 pixel offset

	bool wrapUniforms = false;
	bool fixVPOS = false;
	bool fixVFACE = false;
	bool fixVPOSType = false;

	Symbol* entryPointSymbol = callNode->GetSymbol();

	if( entryPointSymbol == nullptr || entryPointSymbol->definition == nullptr )
	{
		return PATCH_ERROR;
	}

	ASTNode* functionHeader  = entryPointSymbol->definition->GetChildOrNull( 0 );
	if( functionHeader == nullptr )
	{
		return PATCH_ERROR;
	}

	std::vector<Symbol*> targetPath;
	std::vector<Symbol*> outPositionPath;
	std::vector<Symbol*> vfacePath;

	if( patchOutput )
	{
		if( type == PIXEL_STAGE )
		{
			const char* target[] = { "color", "color0", "sv_target", "sv_target0", nullptr };
			// check "out" parameters
			if( !FindOutputBySemantics( functionHeader, target, &targetPath ) )
			{
				return PATCH_SKIP;
			}
		}
		else if( type == VERTEX_STAGE )
		{
			const char* target[] = { "sv_clipdistance", "sv_clipdistance0", nullptr };
			if( FindOutputBySemantics( functionHeader, target, nullptr ) )
			{
				// shader outputs SV_ClipDistance0, so we skip patching for clip planes
				return PATCH_SKIP;
			}
			const char* position[] = { "position", "sv_position", nullptr };
			if( !FindOutputBySemantics( functionHeader, position, &outPositionPath ) )
			{
				// shader somehow doesn't output SV_Position, so we skip patching for clip planes
				return PATCH_SKIP;
			}
		}
		else
		{
			return PATCH_SKIP;
		}
	}
	else if( type == VERTEX_STAGE )
	{
		const char* position[] = { "position", "sv_position", nullptr };
		FindOutputBySemantics( functionHeader, position, &outPositionPath );
	}


	// check for uniform parameters
	if( callNode->GetChildrenCount() )
	{
		unsigned count = 0;
		for( unsigned i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
			{
				++count;
			}
		}
		if( count != callNode->GetChildrenCount() )
		{
			state.ShowMessage( callNode->GetLocation(), EC_NO_OVERRIDE, ToString( functionHeader->GetToken()->stringValue ).c_str() );
			return PATCH_ERROR;
		}
		wrapUniforms = true;
	}

	// find VPOS+POSITION parameters
	std::vector<Symbol*> positionPath;
	std::vector<Symbol*> vposPath;
	const char* vpos[] = { "vpos", nullptr };
	fixVPOSType = fixVPOS = FindParameterBySemantics( functionHeader, vpos, &vposPath );
	if( fixVPOS || patchOutput && type == VERTEX_STAGE )
	{
		const char* position[] = { "position", "sv_position", nullptr };
		bool foundPosition = FindParameterBySemantics( functionHeader, position, &positionPath );
		fixVPOS &= foundPosition;
	}

	if( fixVPOSType )
	{
		if( vposPath.back() )
		{
			if( vposPath.back()->type.symbol || 
				( vposPath.back()->type.builtInType == OP_FLOAT && vposPath.back()->type.width == 4 && vposPath.back()->type.height == 1 ) )
			{
				fixVPOSType = false;
			}
		}
	}

	const char* vface[] = { "vface", nullptr };
	fixVFACE = FindParameterBySemantics( functionHeader, vface, &vfacePath );

	if( !wrapUniforms && !fixVPOS && !patchOutput && !fixVFACE && !fixVPOSType && ( type != VERTEX_STAGE || outPositionPath.empty() ) )
	{
		entryPointName = ToString( entryPointSymbol->name );
		return PATCH_USE;
	}

	Symbol* perObjectPSInt = state.GetSymbolTable().LookupGlobal( "DX11ShadowState" );
	hasShadowState |= perObjectPSInt != nullptr && perObjectPSInt->used;
	if( !hasShadowState )
	{
		hasShadowState = true;
		os << "cbuffer DX11ShadowState : register( b10 )\n{\n";
		os << "struct {\n";
		os << "int invertedTest;\n";
		os << "int alphaTestRef;\n";
		os << "int alphaTestFunc;\n";
		os << "uint clipPlaneEnabled;\n";
		os << "float4 clipPlanes[4];\n";
		os << "float4 renderTargetSize;\n";
		os << "} DX11ShadowState;\n}\n";
	}

	if( entryPointSymbol->definition->GetChildOrNull( 2 ) )
	{
		PrintHLSL11( os, entryPointSymbol->definition->GetChildOrNull( 2 ), 0 );
	}
	// return type
	PrintTypeHLSL11( os, functionHeader->GetType() );
	// name
	InlineString name = state.AllocateName();
	os << ' ' << name << "( ";
	bool first = true;

	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			continue;
		}

		Symbol* symbol = functionHeader->GetChild( i )->GetSymbol();
		if( symbol == nullptr )
		{
			return PATCH_ERROR;
		}
		if( fixVPOS && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
		{
			continue;
		}
		if( !first )
		{
			os << ", ";
		}
		else
		{
			first = false;
		}
		if( fixVFACE && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vface" ) == 0 )
		{
			os << "bool " << symbol->name << ": SV_IsFrontFace";
		}
		else
		{
			if( fixVPOSType && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
			{
				os << "float4 " << functionHeader->GetChild( i )->GetSymbol()->name << ": VPOS";
			}
			else
			{
				PrintHLSL11( os, functionHeader->GetChild( i ), 1 );
			}
		}
	}
	if( patchOutput && type == VERTEX_STAGE )
	{
		if( !first )
		{
			os << ", ";
		}
		else
		{
			first = false;
		}
		os << "out float" << g_maxClipPlanes << " __clipDistance : SV_ClipDistance";
	}
	os << " )";
	// semantics
	if( functionHeader->GetSymbol()->semantic.start )
	{
		os << " : " << functionHeader->GetSymbol()->semantic;
	}
	os << "\n{\n";
	unsigned uniformIndex = 0;
	std::map<size_t, InlineString> defaultUniformArguments;
	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			if( callNode->GetChildrenCount() <= uniformIndex && functionHeader->GetChild( i )->GetChildOrNull( 1 ) != nullptr )
			{
				InlineString name = state.AllocateName();
				Type type = functionHeader->GetChild( i )->GetType();
				type.storageClass = 0;
				PrintTypeHLSL11( os, type );
				os << ' ' << name << " = ";
				PrintHLSL11( os, functionHeader->GetChild( i )->GetChildOrNull( 1 ), 1 );
				os << ";\n";
				defaultUniformArguments[i] = name;
			}
			uniformIndex++;
		}
	}

	if( functionHeader->GetSymbol()->type.symbol || functionHeader->GetSymbol()->type.builtInType != OP_VOID )
	{
		PrintTypeHLSL11( os, functionHeader->GetType() );
		os << " __returnValue = ";
	}
	// call original function
	uniformIndex = 0;
	os << entryPointSymbol->name << "(";
	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( i > 0 )
		{
			os << ", ";
		}
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			if( callNode->GetChildrenCount() > uniformIndex )
			{
				PrintHLSL11( os, callNode->GetChild( uniformIndex ), 1 );
			}
			else if( functionHeader->GetChild( i )->GetChildOrNull( 1 ) != nullptr )
			{
				os << defaultUniformArguments[i];
			}
			uniformIndex++;
		}
		else
		{
			Symbol* symbol = functionHeader->GetChild( i )->GetSymbol();
			if( symbol == nullptr )
			{
				return PATCH_ERROR;
			}
			if( fixVPOS && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
			{
				// pass VPOS value from POSITION
				PrintValuePath( os, positionPath, "" );
				if( symbol->type.symbol == nullptr && symbol->type.builtInType == OP_FLOAT && symbol->type.height == 1 &&
					positionPath.back()->type.symbol == nullptr && positionPath.back()->type.builtInType == OP_FLOAT && 
					positionPath.back()->type.height == 1 && positionPath.back()->type.width != symbol->type.width )
				{
					const char* swizzle = "xyzw";
					os << ".";
					for( int k = 0; k < symbol->type.width; ++k )
					{
						os << swizzle[min( k, positionPath.back()->type.width - 1 ) ];
					}
					os << " - 0.5";
				}
			}
			else if( fixVPOSType && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
			{
				Type type = symbol->type;
				PrintTypeHLSL11( os, type );
				os << "( " << symbol->name;
				if( type.width != 4 )
				{
					const char* swizzle = "xyzw";
					os << ".";
					for( int k = 0; k < symbol->type.width; ++k )
					{
						os << swizzle[min( k, symbol->type.width - 1 ) ];
					}
				}
				os << " )";
			}
			else if( fixVFACE && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vface" ) == 0 )
			{
				os << symbol->name << " ? 1.0 : -1.0";
			}
			else
			{
				os << symbol->name;
			}
		}
	}
	os << ");\n";
	if( type == VERTEX_STAGE && pixelOffset)
	{
		PrintValuePath( os, outPositionPath, "__returnValue" );
		os << ".xy += DX11ShadowState.renderTargetSize.xy * ";
		PrintValuePath( os, outPositionPath, "__returnValue" );
		os << ".w;\n";
	}
	if( patchOutput )
	{
		if( type == PIXEL_STAGE )
		{
			os << "int __alphaValue = int( saturate( ";
			PrintValuePath( os, targetPath, "__returnValue" );
			os << ".a ) * 255 + 0.5 );\n";

			os << "[branch] if( DX11ShadowState.alphaTestFunc == 0 )\n";
			os << "{\nif( __alphaValue * DX11ShadowState.invertedTest + DX11ShadowState.alphaTestRef < 0 ) discard;\n}\nelse\n";
			os << "{\nif( ( __alphaValue == DX11ShadowState.alphaTestRef ) == DX11ShadowState.invertedTest ) discard;\n}\n";
		}
		else
		{
			os << "float4 __position = ";
			PrintValuePath( os, outPositionPath, "__returnValue" );
			Symbol* lastPosition = outPositionPath.back();
			if( lastPosition == nullptr )
			{
				lastPosition = callNode->GetSymbol();
			}
			if( lastPosition->type.symbol == nullptr && lastPosition->type.builtInType == OP_FLOAT && 
				lastPosition->type.height == 1 && lastPosition->type.width != 4 )
			{
				const char* swizzle = "xyzw";
				os << ".";
				for( int k = 0; k < 4; ++k )
				{
					os << swizzle[min( k, outPositionPath.back()->type.width - 1 ) ];
				}
			}
			os << ";\n";
			os << "__clipDistance = 0.0;\n";
			for( int i = 0; i < g_maxClipPlanes; ++i )
			{
				static const char* swizzle = "xyzw";
				os << "[branch] if( DX11ShadowState.clipPlaneEnabled & " << ( 1 << i ) << " )\n";
				os << "{\n__clipDistance." << swizzle[i] << " = dot( __position, DX11ShadowState.clipPlanes[" << i << "] );\n}\n";
			}
		}
	}
	if( functionHeader->GetSymbol()->type.symbol || functionHeader->GetSymbol()->type.builtInType != OP_VOID )
	{
		os << "return __returnValue;\n";
	}
	os << "}\n";

	entryPointName = ToString( name );
	return PATCH_USE;
}

bool EffectCompilerDX11::Create()
{
	GetOperatorSymbol( 0 );
	return true;
}

bool MatchShaderInputOutput( ID3D11ShaderReflection* output, ID3D11ShaderReflection* input )
{
	D3D11_SHADER_DESC vsReflDesc;
	if( FAILED( output->GetDesc( &vsReflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}
	D3D11_SHADER_DESC psReflDesc;
	if( FAILED( input->GetDesc( &psReflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}

	for( unsigned k = 0; k < psReflDesc.InputParameters; ++k )
	{
		D3D11_SIGNATURE_PARAMETER_DESC psDesc;
		if( FAILED( input->GetInputParameterDesc( k, &psDesc ) ) )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader input parameter description" );
			return false;
		}
		if( psDesc.SystemValueType != D3D10_NAME_UNDEFINED )
		{
			continue;
		}
		bool found = false;
		for( unsigned n = 0; n < vsReflDesc.OutputParameters; ++n )
		{
			D3D11_SIGNATURE_PARAMETER_DESC vsDesc;
			if( FAILED( output->GetOutputParameterDesc( n, &vsDesc ) ) )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader output parameter description" );
				return false;
			}
			if( vsDesc.Register == psDesc.Register && ( ( ~vsDesc.Mask & psDesc.Mask ) == 0 ) )
			{
				if( _stricmp( vsDesc.SemanticName, psDesc.SemanticName ) || vsDesc.SemanticIndex != psDesc.SemanticIndex )
				{
					g_messages.AddMessage( "\\memory(0): error X0000: Vertex/pixel shader signature mismatch" );
					return false;
				}
				found = true;
				break;
			}
		}
		if( !found )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Vertex/pixel shader signature mismatch" );
			return false;
		}
	}
	return true;
}

void PrintPrettyCode( std::ostream& listing, const char* code, const char* indent )
{
	while( *code )
	{
		const char* end = code;
		while( *end && *end != '\n' )
		{
			++end;
		}
		if( strncmp( code, "#line", 5 ) && code != end )
		{
			listing << indent;
			listing.write( code, end - code );
			listing << std::endl;
		}
		code = end + 1;
		if( *end == 0 )
		{
			break;
		}
	}
}

void PrintShaderOutListing( std::ostream& listing, ID3DBlob* effectData, ID3D11ShaderReflection* reflection )
{
	CComPtr<ID3DBlob> disassembly;
	if( SUCCEEDED( D3DDisassemble( effectData->GetBufferPointer(), effectData->GetBufferSize(), D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, nullptr, &disassembly ) ) )
	{
		listing << "          asm: |" << std::endl;
		PrintPrettyCode( listing, reinterpret_cast<char*>( disassembly->GetBufferPointer() ), "            " );
		listing << std::endl;
	}
	D3D11_SHADER_DESC desc;
	if( reflection && SUCCEEDED( reflection->GetDesc( &desc ) ) )
	{
		listing << "          constantBuffers: " << desc.ConstantBuffers << std::endl;
		listing << "          boundResources: " << desc.BoundResources << std::endl;
		listing << "          inputParameters: " << desc.InputParameters << std::endl;
		listing << "          outputParameters: " << desc.OutputParameters << std::endl;
		listing << "          instructionCount: " << desc.InstructionCount << std::endl;
		listing << "          tempRegisterCount: " << desc.TempRegisterCount << std::endl;
		listing << "          tempArrayCount: " << desc.TempArrayCount << std::endl;
		listing << "          defCount: " << desc.DefCount << std::endl;
		listing << "          dclCount: " << desc.DclCount << std::endl;
		listing << "          textureNormalInstructions: " << desc.TextureNormalInstructions << std::endl;
		listing << "          textureLoadInstructions: " << desc.TextureLoadInstructions << std::endl;
		listing << "          textureCompInstructions: " << desc.TextureCompInstructions << std::endl;
		listing << "          textureBiasInstructions: " << desc.TextureBiasInstructions << std::endl;
		listing << "          textureGradientInstructions: " << desc.TextureGradientInstructions << std::endl;
		listing << "          floatInstructionCount: " << desc.FloatInstructionCount << std::endl;
		listing << "          intInstructionCount: " << desc.IntInstructionCount << std::endl;
		listing << "          uintInstructionCount: " << desc.UintInstructionCount << std::endl;
		listing << "          staticFlowControlCount: " << desc.StaticFlowControlCount << std::endl;
		listing << "          dynamicFlowControlCount: " << desc.DynamicFlowControlCount << std::endl;
		listing << "          macroInstructionCount: " << desc.MacroInstructionCount << std::endl;
		listing << "          arrayInstructionCount: " << desc.ArrayInstructionCount << std::endl;
		listing << "          cutInstructionCount: " << desc.CutInstructionCount << std::endl;
		listing << "          emitInstructionCount: " << desc.EmitInstructionCount << std::endl;
		listing << "          GSOutputTopology: " << desc.GSOutputTopology << std::endl;
		listing << "          inputPrimitive: " << desc.InputPrimitive << std::endl;
		listing << "          GSMaxOutputVertexCount: " << desc.GSMaxOutputVertexCount << std::endl; // gs | hs
		listing << "          patchConstantParameters: " << desc.PatchConstantParameters << std::endl;
		listing << "          cGSInstanceCount: " << desc.cGSInstanceCount << std::endl;
		listing << "          HSOutputPrimitive: " << desc.HSOutputPrimitive << std::endl; // hs | ds
		listing << "          HSPartitioning: " << desc.HSPartitioning << std::endl;
		listing << "          tessellatorDomain: " << desc.TessellatorDomain << std::endl; // ds
		listing << "          cBarrierInstructions: " << desc.cBarrierInstructions << std::endl; // cs
		listing << "          cInterlockedInstructions: " << desc.cInterlockedInstructions << std::endl; // cs
		listing << "          cTextureStoreInstructions: " << desc.cTextureStoreInstructions << std::endl; // cs
	}
}

void PrintAnnotations( std::ostream& listing, const std::map<StringReference, Annotation>& annotations )
{
	listing << "          annotations:" << std::endl;
	for( auto a = annotations.begin(); a != annotations.end(); ++a )
	{
		listing << "          -" << std::endl;
		listing << "              name: " << g_stringTable.GetString( a->first ) << std::endl;
		listing << "              annotationType: ";
		switch( a->second.type )
		{
		case ANNOTATION_TYPE_BOOL:
			listing << "bool" << std::endl;
			listing << "              value: " << ( a->second.intValue ? "true" : "false" ) << std::endl;
			break;
		case ANNOTATION_TYPE_INT:
			listing << "int" << std::endl;
			listing << "              value: " << a->second.intValue << std::endl;
			break;
		case ANNOTATION_TYPE_FLOAT:
			listing << "float" << std::endl;
			listing << "              value: " << a->second.floatValue << std::endl;
			break;
		default:
			listing << "string" << std::endl;
			listing << "              value: \'"; 
			{
				const char* value = g_stringTable.GetString( a->second.stringValue );
				while( *value )
				{
					if( *value == '\'' )
					{
						listing.put( '\'' );
					}
					if( *value != '\r' && *value != '\n' )
					{
						listing.put( *value );
					}
					++value;
				}
			}
			listing << "\'" << std::endl;
			break;
		}
	}
}

void PrintStageInfo( std::ostream& listing, const StageInput& stage, const EffectData& result )
{
	if( !stage.constants.empty() )
	{
		listing << "        constants: " << std::endl;
		for( auto it = stage.constants.begin(); it != stage.constants.end(); ++it )
		{
			listing << "        -" << std::endl;
			listing << "          name: " << g_stringTable.GetString( it->name ) << std::endl;
			listing << "          constantType: ";
			switch( it->type )
			{
			case CONSTANT_TYPE_FLOAT:
				listing << "float";
				break;
			case CONSTANT_TYPE_INT:
				listing << "int";
				break;
			case CONSTANT_TYPE_BOOL:
				listing << "bool";
				break;
			default:
				listing << "other (struct, etc.)";
				break;
			}
			listing << std::endl;
			listing << "          dimension: " << it->dimension << std::endl;
			listing << "          arrayElements: " << it->elements << std::endl;
			listing << "          autoregister: " << ( it->isAutoregister ? "true" : "false" ) << std::endl;
			listing << "          sRGB: " << ( it->isSRGB ? "true" : "false" ) << std::endl;
			auto annotations = result.annotations.find( it->name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
		}
	}
	if( !stage.samplers.empty() )
	{
		listing << "        samplers:" << std::endl;
		for( auto it = stage.samplers.begin(); it != stage.samplers.end(); ++it )
		{
			listing << "        -" << std::endl;
			listing << "          register: " << it->first << std::endl;
			listing << "          name: " << g_stringTable.GetString( it->second.name ) << std::endl;
			listing << "          filter: " << int( it->second.filter ) << std::endl;
			listing << "          comparison: " << int( it->second.comparison ) << std::endl;
			listing << "          minFilter: " << int( it->second.minFilter ) << std::endl;
			listing << "          magFilter: " << int( it->second.magFilter ) << std::endl;
			listing << "          mipFilter: " << int( it->second.mipFilter ) << std::endl;
			listing << "          addressU: " << int( it->second.addressU ) << std::endl;
			listing << "          addressV: " << int( it->second.addressV ) << std::endl;
			listing << "          addressW: " << int( it->second.addressW ) << std::endl;
			listing << "          mipLODBias: " << it->second.mipLODBias << std::endl;
			listing << "          maxAnisotropy: " << int( it->second.maxAnisotropy ) << std::endl;
			listing << "          comparisonFunc: " << int( it->second.comparisonFunc ) << std::endl;
			listing << "          borderColor: [" << it->second.borderColor.x << ", " << it->second.borderColor.y << ", " << it->second.borderColor.z << ", " << it->second.borderColor.w << "]" << std::endl;
			listing << "          minLOD: " << it->second.minLOD << std::endl;
			listing << "          maxLOD: " << it->second.maxLOD << std::endl;
			listing << "          srgbTexture: " << ( it->second.srgbTexture ? "true" : "false" ) << std::endl;
		}
	}
	if( !stage.textures.empty() )
	{
		listing << "        textures:" << std::endl;
		for( auto it = stage.textures.begin(); it != stage.textures.end(); ++it )
		{
			listing << "        -" << std::endl;
			listing << "          register: " << it->first << std::endl;
			listing << "          name: " << g_stringTable.GetString( it->second.name ) << std::endl;
			listing << "          textureType: ";
			switch( it->second.type )
			{
			case TEX_TYPE_1D:
				listing << "1D texture";
				break;
			case TEX_TYPE_2D:
				listing << "2D texture";
				break;
			case TEX_TYPE_3D:
				listing << "3D texture";
				break;
			case TEX_TYPE_CUBE:
				listing << "CUBE texture";
				break;
			case TEX_TYPE_TYPELESS:
				listing << "typeless texture";
				break;
			case TEX_TYPE_BUFFER:
				listing << "buffer";
				break;
			case TEX_TYPE_STRUCTURED_BUFFER:
				listing << "structured buffer";
				break;
			case TEX_TYPE_TBUFFER:
				listing << "TBuffer";
				break;
			case TEX_TYPE_BYTEADDRESS_BUFFER:
				listing << "byte address buffer";
				break;
			}
			listing << std::endl;
			listing << "          autoregister: " << ( it->second.isAutoregister ? "true" : "false" ) << std::endl;
			listing << "          sRGB: " << ( it->second.isSRGB ? "true" : "false" ) << std::endl;
			auto annotations = result.annotations.find( it->second.name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
		}
	}
	if( !stage.uavs.empty() )
	{
		listing << "        uavs:" << std::endl;
		for( auto it = stage.uavs.begin(); it != stage.uavs.end(); ++it )
		{
			listing << "        -" << std::endl;
			listing << "          register: " << it->first << std::endl;
			listing << "          name: " << g_stringTable.GetString( it->second.name ) << std::endl;
			listing << "          resourceType: ";
			switch( it->second.type )
			{
			case TEX_TYPE_1D:
				listing << "1D texture";
				break;
			case TEX_TYPE_2D:
				listing << "2D texture";
				break;
			case TEX_TYPE_3D:
				listing << "3D texture";
				break;
			case TEX_TYPE_CUBE:
				listing << "CUBE texture";
				break;
			case TEX_TYPE_TYPELESS:
				listing << "typeless texture";
				break;
			case TEX_TYPE_BUFFER:
				listing << "buffer";
				break;
			case TEX_TYPE_STRUCTURED_BUFFER:
				listing << "structured buffer";
				break;
			case TEX_TYPE_TBUFFER:
				listing << "TBuffer";
				break;
			case TEX_TYPE_BYTEADDRESS_BUFFER:
				listing << "byte address buffer";
				break;

			case TEX_TYPE_UAV_RWTYPED:
				listing << "UAV typed";
				break;
			case TEX_TYPE_UAV_RWSTRUCTURED:
				listing << "UAV structured";
				break;
			case TEX_TYPE_UAV_RWBYTEADDRESS:
				listing << "UAV RW byte address";
				break;
			case TEX_TYPE_UAV_APPEND_STRUCTURED:
				listing << "UAV append structured";
				break;
			case TEX_TYPE_UAV_CONSUME_STRUCTURED:
				listing << "UAV consume structured";
				break;
			case TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER:
				listing << "UAV structured with counter";
				break;
			}
			listing << std::endl;
			listing << "          autoregister: " << ( it->second.isAutoregister ? "true" : "false" ) << std::endl;
			auto annotations = result.annotations.find( it->second.name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
		}
	}
	if( !stage.inputs.empty() )
	{
		listing << "        inputs:" << std::endl;
		for( auto it = stage.inputs.begin(); it != stage.inputs.end(); ++it )
		{
			listing << "        -" << std::endl;
			listing << "          register: " << int( it->registerIndex ) << std::endl;
			listing << "          name: " << int( it->name ) << std::endl;
			listing << "          index: " << int( it->index ) << std::endl;
			listing << "          usedMask: " << int( it->usedMask ) << std::endl;
		}
	}
}

bool EffectCompilerDX11::CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result, bool patchShaders )
{
	CComPtr<ID3D10Blob> effectData;
	CComPtr<ID3D10Blob> errors;

	DWORD optimizationLevel;
	switch( g_optimizationLevel )
	{
	case 0:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL0;
		break;
	case 1:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL1;
		break;
	case 2:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL2;
		break;
	default:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL3;
	}

	ParserState state( MakeInlineString( source, source + sourceLength ) );
	for( const D3DXMACRO* define = defines; define->Name; ++define )
	{
		PreprocessorDefine d;
		d.location.fileName = MakeInlineString( "" );
		d.location.lineNumber = 0;
		d.value = MakeInlineString( define->Definition );
		state.m_defines[MakeInlineString( define->Name )] = d;
	}

	if( !state.Parse() )
	{
		return false;
	}
	PatchCBuffers( state );
	TransferSRGBToTexturesDX11( state );
	ConvertTextureFunctionsDX11( state );

	ASTNode* techniqueNode = state.GetTree()->FindNode( NT_TECHNIQUE );
	if( techniqueNode == nullptr )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: No technique found" );
		return false;
	}

	std::stringstream listing;
	if( g_generateListing )
	{
		listing << "permutation:" << std::endl;
		listing << "  platform: DX11" << std::endl;
		listing << "  id: 000" << std::endl;
		listing << "  defines:" << std::endl;
		for( int i = 0; defines[i].Name; ++i )
		{
			listing << "    " << defines[i].Name << ": " << defines[i].Definition << std::endl;
		}
		listing << "  passes:" << std::endl;
	}
	struct OutputListing
	{
		OutputListing( std::stringstream* listing )
			:m_listing( listing )
		{
		}
		~OutputListing()
		{
			if( m_listing )
			{
				EnterCriticalSection( &g_listingCS );
				g_listing += m_listing->str();
				LeaveCriticalSection( &g_listingCS );
			}
		}

		std::stringstream* m_listing;
	} outputListing( g_generateListing ? &listing : nullptr );

	std::string shaderStatistics;
	if( g_printShaderStats )
	{
		shaderStatistics = "Compiled shader instructions (approximate):\nPass  VS  PS  CS  GS  HS  DS";
	}

	for( size_t passIx = 0; passIx < techniqueNode->GetChildrenCount(); ++passIx )
	{
		if( g_generateListing )
		{
			listing << "  -" << std::endl;
		}
		Pass outPass;
		ASTNode* passNode = techniqueNode->GetChild( passIx );
		CComPtr<ID3D11ShaderReflection> reflections[6];
		for( size_t stateIx = 0; stateIx < passNode->GetChildrenCount(); ++stateIx )
		{
			if( passNode->GetChild( stateIx )->GetNodeType() == NT_STATE_ASSIGNMENT )
			{
				DWORD stateCode = 0;
				DWORD value = 0;
				if( ParseStateAssignment( state, passNode->GetChild( stateIx ), g_renderStates, &value ) )
				{
					std::string name = ToString( passNode->GetChild( stateIx )->GetToken()->stringValue );
					for( int i = 0; g_renderStateNames[i].name; ++i )
					{
						if( _stricmp( name.c_str(), g_renderStateNames[i].name ) == 0 )
						{
							stateCode = g_renderStateNames[i].value;
						}
					}
					switch( stateCode )
					{
					case -1:
					case -2:
					case -3:
					case -4:
					case -5:
					case -6:
						if( value != 0 )
						{
							state.ShowMessage( passNode->GetChild( stateIx )->GetLocation(), EC_INVALID_STATE_VALUE, name.c_str() );
						}
						break;
					case 0:
						state.ShowMessage( passNode->GetChild( stateIx )->GetLocation(), EC_STATE_DEPRECATED, name.c_str() );
						break;
					default:
						outPass.states[stateCode] = value;
					}
				}
				continue;
			}

			if( passNode->GetChild( stateIx )->GetNodeType() != NT_SHADER_ASSIGNMENT )
			{
				continue;
			}

			ASTNode* shaderNode = passNode->GetChild( stateIx );

			StageInput stage;
			if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "vertexshader" ) == 0 )
			{
				stage.type = VERTEX_STAGE;
			}
			else if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "pixelshader" ) == 0 )
			{
				stage.type = PIXEL_STAGE;
			}
			else if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "computeshader" ) == 0 )
			{
				stage.type = COMPUTE_STAGE;
			}
			else if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "geometryshader" ) == 0 )
			{
				stage.type = GEOMETRY_STAGE;
			}
			else if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "hullshader" ) == 0 )
			{
				stage.type = HULL_STAGE;
			}
			else if( _stricmp( ToString( shaderNode->GetToken()->stringValue ).c_str(), "domainshader" ) == 0 )
			{
				stage.type = DOMAIN_STAGE;
			}
			else
			{
				state.ShowMessage( shaderNode->GetToken()->fileLocation, EC_INVALID_STATE, ToString( shaderNode->GetToken()->stringValue ).c_str() );
				return false;
			}

			std::string profile = ToString( shaderNode->GetChild( 0 )->GetToken()->stringValue );
			if( profile[0] == 'v' )
			{
				profile = "vs_5_0";
			}
			else if( profile[0] == 'p' )
			{
				profile = "ps_5_0";
			}

			effectData = nullptr;
			errors = nullptr;


			if( shaderNode->GetChild( 1 )->GetSymbol() == nullptr )
			{
				return false;
			}

			state.GetSymbolTable().ResetUsedFlag();
			MarkUsedSymbols( shaderNode->GetChild( 1 ), state );
			{
				Symbol* symbol = state.GetSymbolTable().LookupGlobal( "DX11ShadowState" );
				if( symbol )
				{
					symbol->used = true;
					MarkUsedSymbols( symbol->definition, state );
				}
			}
			CompilerInputStream os( state );
			PrintHLSL11( os, state.GetTree(), 0 );
			
			std::string entryPoint = ToString( shaderNode->GetChild( 1 )->GetSymbol()->name );

			std::string patchEntryPoint = entryPoint;
			bool hasShadowState = false;
			switch( PatchShader( stage.type, false, patchShaders, shaderNode->GetChild( 1 ), state, os, patchEntryPoint, hasShadowState ) )
			{
			case PATCH_ERROR:
				return false;
			}
			state.ResetPragmaUsage();
			std::string code( os.str(), os.str() + os.pcount() );

			if( g_generateListing )
			{
				listing << "    -" << std::endl;
				listing << "        profile: " << profile << std::endl;
				listing << "        original: " << std::endl;
				listing << "          entryPoint: " << patchEntryPoint << std::endl;
				listing << "          source: |" << std::endl;
				PrintPrettyCode( listing, code.c_str(), "            " );
				listing << std::endl;
			}

			HRESULT hr = D3DX11CompileFromMemory(
				code.c_str(), 
				code.length(), 
				"\\memory", 
				nullptr, 
				nullptr, 
				patchEntryPoint.c_str(), 
				profile.c_str(),
				D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY | optimizationLevel | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR | ( g_avoidFlowControl ? D3D10_SHADER_AVOID_FLOW_CONTROL : 0 ), 
				0,
				nullptr,
				&effectData,
				&errors,
				nullptr );
			if( FAILED( hr ) )
			{
				if( errors )
				{
					g_messages.AddMessages( errors );
				}
				return false;
			}

			if( g_printWarnings && errors )
			{
				g_messages.AddMessages( errors );
			}

			CComPtr<ID3DBlob> strippedEffectData;
			if( SUCCEEDED( D3DStripShader( 
				effectData->GetBufferPointer(), 
				effectData->GetBufferSize(), 
				D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, 
				&strippedEffectData ) ) )
			{
				stage.shaderData = new char[strippedEffectData->GetBufferSize()];
				memcpy( stage.shaderData, strippedEffectData->GetBufferPointer(), strippedEffectData->GetBufferSize() );
				stage.shaderSize = strippedEffectData->GetBufferSize();
			}
			else
			{
				stage.shaderData = new char[effectData->GetBufferSize()];
				memcpy( stage.shaderData, effectData->GetBufferPointer(), effectData->GetBufferSize() );
				stage.shaderSize = effectData->GetBufferSize();
			}
			stage.shaderDataStr = g_stringTable.AddString( stage.shaderData, stage.shaderSize );
			stage.shadowShaderSize = 0;
			stage.shadowShaderData = nullptr;
			stage.shadowShaderDataStr = -1;

			CComPtr<ID3D11ShaderReflection> reflection;
			if( FAILED( D3DReflect( effectData->GetBufferPointer(), effectData->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &reflection.p ) ) )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection" );
				return false;
			}

			if( !GetStageData( state, reflection, stage, result.annotations ) )
			{
				return false;
			}
			if( !stage.defaultValues.empty() )
			{
				stage.defaultValuesStr = g_stringTable.AddString( &stage.defaultValues[0], stage.defaultValues.size() );
			}
			else
			{
				stage.defaultValuesStr = -1;
			}

			if( g_generateListing )
			{
				PrintShaderOutListing( listing, effectData, reflection );
			}

			reflections[stage.type] = reflection;

			if( stage.type == PIXEL_STAGE || g_maxClipPlanes > 0 ) 
			{
				os.freeze( false );
				patchEntryPoint = entryPoint;
				switch( PatchShader( stage.type, true, true, shaderNode->GetChild( 1 ), state, os, patchEntryPoint, hasShadowState ) )
				{
				case PATCH_ERROR:
					return false;
				case PATCH_USE:
					{
						state.ResetPragmaUsage();
						code = std::string( os.str(), os.str() + os.pcount() );
						effectData = nullptr;
						errors = nullptr;

						if( g_generateListing )
						{
							listing << "        patched:" << std::endl;
							listing << "          entryPoint: " << patchEntryPoint << std::endl;
							listing << "          source: |" << std::endl;
							PrintPrettyCode( listing, code.c_str(), "            " );
							listing << std::endl;
						}

						HRESULT hr = D3DX11CompileFromMemory(
							code.c_str(), 
							code.length(), 
							"\\memory", 
							nullptr, 
							nullptr, 
							patchEntryPoint.c_str(), 
							profile.c_str(),
							D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY | optimizationLevel | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR | ( g_avoidFlowControl ? D3D10_SHADER_AVOID_FLOW_CONTROL : 0 ), 
							0,
							nullptr,
							&effectData,
							&errors,
							nullptr );
						if( FAILED( hr ) )
						{
							if( errors )
							{
								g_messages.AddMessages( errors );
							}
							return false;
						}

						if( g_printWarnings && errors )
						{
							g_messages.AddMessages( errors );
						}
						
						CComPtr<ID3DBlob> strippedEffectData;
						if( SUCCEEDED( D3DStripShader( 
							effectData->GetBufferPointer(), 
							effectData->GetBufferSize(), 
							D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, 
							&strippedEffectData ) ) )
						{
							stage.shadowShaderData = new char[strippedEffectData->GetBufferSize()];
							memcpy( stage.shadowShaderData, strippedEffectData->GetBufferPointer(), strippedEffectData->GetBufferSize() );
							stage.shadowShaderSize = strippedEffectData->GetBufferSize();
						}
						else
						{
							stage.shadowShaderData = new char[effectData->GetBufferSize()];
							memcpy( stage.shadowShaderData, effectData->GetBufferPointer(), effectData->GetBufferSize() );
							stage.shadowShaderSize = effectData->GetBufferSize();
						}
						stage.shadowShaderDataStr = g_stringTable.AddString( stage.shadowShaderData, stage.shadowShaderSize );


						CComPtr<ID3D11ShaderReflection> reflection;
						D3DReflect( effectData->GetBufferPointer(), effectData->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &reflection.p );

						if( g_generateListing )
						{
							PrintShaderOutListing( listing, effectData, reflection );
						}
					}
					break;
				}
			}

			if( g_generateListing )
			{
				PrintStageInfo( listing, stage, result );
			}

			outPass.stages.push_back( stage );
		}

		if( g_printShaderStats )
		{
			char buffer[64];
			sprintf_s( buffer, "\n%4u", passIx );
			shaderStatistics += buffer;
			for( int k = 0; k < 6; ++k )
			{
				if( reflections[k] )
				{
					D3D11_SHADER_DESC desc;
					if( SUCCEEDED( reflections[k]->GetDesc( &desc ) ) )
					{
						char buffer[64];
						sprintf_s( buffer, "%4u", desc.InstructionCount );
						shaderStatistics += buffer;
					}
					else
					{
						shaderStatistics += "   ?";
					}
				}
				else
				{
					shaderStatistics += "   -";
				}
			}
		}
		// Check if PS input matches VS output
		InputStageType pipelineStages[] = {
			VERTEX_STAGE,
			HULL_STAGE,
			DOMAIN_STAGE,
			GEOMETRY_STAGE,
			PIXEL_STAGE, };
		for( int i = 0; i < 6; ++i )
		{
			if( reflections[i] )
			{
				for( int j = 0; j < sizeof( pipelineStages ) / sizeof( InputStageType ); ++j )
				{
					if( i == pipelineStages[j] )
					{
						for( int k = j - 1; k >= 0; --k )
						{
							if( reflections[pipelineStages[k]] )
							{
								if( !MatchShaderInputOutput( reflections[pipelineStages[k]], reflections[i] ) )
								{
									return false;
								}
								break;
							}
						}
						break;
					}
				}
			}
		}
		result.passes.push_back( outPass );
	}

	if( g_printShaderStats )
	{
		g_messages.AddMessage( shaderStatistics.c_str() );
	}

	return true;
}
