#include "stdafx.h"
#include "OutputHLSL.h"
#include "ParserUtils.h"
#include "ASTNode.h"
#include "HLSLParser.h"

namespace
{

	struct Children
	{
		Children( HLSL parent_, const char* glue_, size_t offset_ = 0 )
			:parent( parent_ ),
			glue( glue_ ),
			offset( offset_ )
		{
		}

		HLSL parent;
		const char* glue;
		size_t offset;
	};

	HLSL HLSLChild( const HLSL& parent, size_t childIndex )
	{
		return HLSL{ parent.node->GetChild( childIndex ), parent.symbolTable };
	}

	CodeStream& operator<<( CodeStream& os, const Children& children )
	{
		for( size_t i = children.offset; i < children.parent.node->GetChildrenCount(); ++i )
		{
			if( i )
			{
				os << children.glue;
			}
			os << HLSLChild( children.parent, i );
		}
		return os;
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

	std::map<int, std::string> s_operators = {
		{ OP_MUL_ASSIGN, "*=" },
		{ OP_DIV_ASSIGN, "/=" },
		{ OP_MOD_ASSIGN, "%=" },
		{ OP_ADD_ASSIGN, "+=" },
		{ OP_SUB_ASSIGN, "-=" },
		{ OP_LEFT_ASSIGN, "<<=" },
		{ OP_RIGHT_ASSIGN, ">>=" },
		{ OP_AND_ASSIGN, "&=" },
		{ OP_XOR_ASSIGN, "^=" },
		{ OP_OR_ASSIGN, "|=" },
		{ OP_INC_OP, "++" },
		{ OP_DEC_OP, "--" },
		{ OP_LEFT_OP, "<<" },
		{ OP_RIGHT_OP, ">>" },
		{ OP_LE_OP, "<=" },
		{ OP_GE_OP, ">=" },
		{ OP_EQ_OP, "==" },
		{ OP_NE_OP, "!=" },
		{ OP_AND_OP, "&&" },
		{ OP_OR_OP, "||" },
		{ OP_PLUS, "+" },
		{ OP_DASH, "-" },
		{ OP_BANG, "!" },
		{ OP_TILDE, "~" },
		{ OP_STAR, "*" },
		{ OP_SLASH, "/" },
		{ OP_PERCENT, "%" },
		{ OP_COMA, "," },
		{ OP_LESS, "<" },
		{ OP_MORE, ">" },
		{ OP_AMPERSAND, "&" },
		{ OP_CARET, "^" },
		{ OP_VERTICAL_BAR, "|" },
		{ OP_EQUAL, "=" }
	};

	static const char* GetOperatorSymbol( int operatorID )
	{
		auto it = s_operators.find( operatorID );
		if( it == s_operators.end() )
		{
			return "";
		}
		return it->second.c_str();
	}


	void PrintTypeHLSL11( CodeStream& os, Type type );

	bool IsResourceType( Type type )
	{
		if( type.symbol )
		{
			return false;
		}
		switch( type.builtInType )
		{
		case OP_SAMPLER2D:
		case OP_SAMPLER3D:
		case OP_SAMPLERCUBE:
		case OP_SAMPLER:
		case OP_SAMPLERCOMPARISON:
		case OP_TEXTURE1D:
		case OP_TEXTURE2D:
		case OP_TEXTURE3D:
		case OP_TEXTURECUBE:
		case OP_TEXTURE1DARRAY:
		case OP_TEXTURE2DARRAY:
		case OP_TEXTURE3DARRAY:
		case OP_TEXTURECUBEARRAY:
		case OP_TEXTURE2DMS:
		case OP_TEXTURE2DMSARRAY:
		case OP_TEXTURE:
		case OP_BUFFER:
		case OP_APPENDSTRUCTUREDBUFFER:
		case OP_BYTEADDRESSBUFFER:
		case OP_CONSUMESTRUCTUREDBUFFER:
		case OP_RWBUFFER:
		case OP_RWBYTEADDRESSBUFFER:
		case OP_RWSTRUCTUREDBUFFER:
		case OP_RWTEXTURE1D:
		case OP_RWTEXTURE1DARRAY:
		case OP_RWTEXTURE2D:
		case OP_RWTEXTURE2DARRAY:
		case OP_RWTEXTURE3D:
		case OP_RWTEXTURE3DARRAY:
		case OP_STRUCTUREDBUFFER:
			return true;
		default:
			return false;
		}
	}

	void PrintTypeHLSL11( CodeStream& os, Type type )
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
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE2D:
				os << "Texture2D";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE3D:
				os << "Texture3D";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURECUBE:
				os << "TextureCube";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE1DARRAY:
				os << "Texture1DArray";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE2DARRAY:
				os << "Texture2DArray";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE3DARRAY:
				os << "Texture3DArray";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURECUBEARRAY:
				os << "TextureCubeArray";
				if( type.templateParameter )
				{
					os << '<' << *type.templateParameter << '>';
				}
				return;
			case OP_TEXTURE2DMS:
				os << "Texture2DMS";
				os << '<' << *type.templateParameter;
				if( type.templateSamples > 0 )
				{
					os << ", " << type.templateSamples;
				}
				os << '>';
				return;
			case OP_TEXTURE2DMSARRAY:
				os << "Texture2DMSArray";
				os << '<' << *type.templateParameter;
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
				os << "Buffer" << '<' << *type.templateParameter << '>';
				return;
			case OP_APPENDSTRUCTUREDBUFFER:
				os << "AppendStructuredBuffer" << '<' << *type.templateParameter << '>';
				return;
			case OP_BYTEADDRESSBUFFER:
				os << "ByteAddressBuffer";
				return;
			case OP_CONSUMESTRUCTUREDBUFFER:
				os << "ConsumeStructuredBuffer" << '<' << *type.templateParameter << '>';
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

}



CompilerInputStream::CompilerInputStream( ParserState& state )
	:m_state( state )
{
	m_location.fileName = MakeInlineString( "" );
	m_location.lineNumber = -1;
}

void CompilerInputStream::Endl()
{
}

void CompilerInputStream::ChangeLocation( const FileLocation& location )
{
	if( m_location.lineNumber != location.lineNumber ||
		m_location.fileName != location.fileName )
	{
		if( m_location.fileName == location.fileName &&
			m_location.lineNumber + 1 == location.lineNumber )
		{
			*this << "\n";
		}
		else
		{
			std::string fileName = ToString( location.fileName );
			for( size_t i = 0; i < fileName.length(); ++i )
			{
				if( fileName[i] == '\\' )
				{
					fileName[i] = '/';
				}
			}

			*this << "\n#line " << location.lineNumber << " \"" << fileName << "\"\n";
		}
		flush();
		m_location = location;
		InlineString pragma;
		while( m_state.GetPragma( m_location, pragma ) )
		{
			*this << "#pragma " << pragma << "\n";
			m_location.lineNumber++;
		}
	}
}

void CompilerInputStream::Indent()
{
}

void CompilerInputStream::Unindent()
{
}



void ListingStream::Endl() 
{ 
	*this << "\n" << m_indent;
}

void ListingStream::ChangeLocation( const FileLocation& location ) 
{
}

void ListingStream::Indent()
{
	m_indent += "\t";
}

void ListingStream::Unindent()
{
	m_indent.pop_back();
}


CodeStream& operator<<( CodeStream& os, const Type& type )
{
	PrintTypeHLSL11( os, type );
	return os;
}


CodeStream& operator<<( CodeStream& os, const HLSL& hlsl )
{
	auto node = hlsl.node;

	os << node->GetLocation();

	auto Child = [&]( size_t index ) -> HLSL { return HLSLChild( hlsl, index ); };

	switch( node->GetNodeType() )
	{
	case NT_VAR_IDENTIFIER:
		os << node->GetSymbol()->name;
		break;
	case NT_CONSTANT:
		os << node->GetToken()->stringValue;
		break;
	case NT_INLINE_CONSTRUCTOR:
		os << "{ " << Indent() << Children( hlsl, ", " ) << Unindent() << " }";
		break;
	case NT_PREFIX_EXPRESSION:
		os << GetOperatorSymbol( node->GetToken()->type ) << "( " << Child( 0 ) << " )";
		break;
	case NT_POSTFIX_EXPRESSION:
		os << "( " << Child( 0 ) << " )";
		switch( node->GetToken()->type )
		{
		case OP_LEFT_BRACKET:
			os << "[" << Child( 1 ) << "]";
			break;
		case OP_DOT:
			os << "." << Child( 1 );
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
		os << "( " << Child( 0 ) << " )";
		if( node->GetToken()->type  != OP_LEFT_PAREN )
		{
			os << " " << GetOperatorSymbol( node->GetToken()->type ) << " ( " << Child( 1 ) << " )";
		}
		break;
	case NT_CONDITIONAL_EXPRESSION:
		os << "( " << Child( 0 ) << " ) ? ( " << Child( 1 ) << " ) : ( " << Child( 2 ) << " )";
		break;
	case NT_CAST_EXPRESSION:
		os << "(" << node->GetType() << ")( " << Child( 0 ) << " )";
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
				os << t;
			}
		}
		else
		{
			os << node->GetSymbol()->name;
		}
		os << "( " << Indent() << Children( hlsl , ", " ) << Unindent() << " )";
		break;
	case NT_FUNCTION_HEADER:
		if( !node->GetSymbol()->used )
		{
			break;
		}
		os.Endl();
		os << node->GetType() << " " << node->GetSymbol()->name << "( " << Indent() << Children( hlsl, ", " ) << Unindent() << " )";
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
			os << Child( 2 ) << ' ';
		}
		os << node->GetType();
		if( node->GetSymbol() )
		{
			os << " " << node->GetSymbol()->name;
		}
		if( node->GetChildOrNull( 0 ) )
		{
			os << "[" << Child( 0 ) << "]";
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
			os << " = " << Child( 1 );
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
		os << node->GetSymbol()->type << " " << node->GetSymbol()->name;
		if( node->GetChildOrNull( 0 ) )
		{
			os << "[" << Child( 0 ) << "]";
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
				reg.registerType == 'u' || reg.registerType == 'U' ||
				reg.registerType == 's' || reg.registerType == 'S' )
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
				if( reg.space >= 0 )
				{
					os << ", space" << reg.space;
				}
				os << " )";
			}
		}
		if( node->GetChildOrNull( 1 ) )
		{
			os << " = " << Child( 1 );
		}
		os << ";";
		os.Endl();
	}
	break;
	case NT_VAR_DECLARATION_LIST:
		os << Indent() << Children( hlsl, "" ) << Unindent();
		break;
	case NT_STRUCT:
		if( node->GetSymbol()->used )
		{
			os << "struct " << " " << node->GetSymbol()->name << Endl();
			os << "{" << Indent() << Endl();
			os << Children( hlsl, "" ) << Unindent() << Endl();
			os << "};" << Endl();
		}
		break;
	case NT_STRUCT_MEMBER:
		os << Children( hlsl, "" );
		break;
	case NT_PROGRAM:
		for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
		{
			os << Child( i );
			if( node->GetChild( i ) && node->GetChild( i )->GetNodeType() == NT_FUNCTION_HEADER )
			{
				os << ';';
			}
		}
		break;
	case NT_BLOCK:
		os << "{" << Indent() << Endl() << Children( hlsl, "" ) << Unindent() << Endl() << "}" << Endl();
		break;
	case NT_EXPRESSION_STATEMENT:
		if( node->GetChildrenCount() )
		{
			os << Child( 0 );
		}
		os << ";" << Endl();
		break;
	case NT_IF:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "if( " << Child( 0 ) << " )" << Endl();
		os << Child( 1 );
		if( node->GetChildrenCount() > 2 )
		{
			os << "else" << Endl() << Child( 2 );
		}
		break;
	case NT_WHILE:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "while( " << Child( 0 ) << " )" << Endl();
		os << Child( 1 );
		break;
	case NT_DO:
		os << "do" << Endl();
		os << Child( 1 );
		os << "while( " << Child( 0 ) << " );" << Endl();
		break;
	case NT_FOR:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "for( " << Child( 0 );
		if( node->GetChild( 1 ) )
		{
			os << Child( 1 );
		}
		os << "; ";
		if( node->GetChild( 2 ) )
		{
			os << Child( 2 );
		}
		os << " )" << Endl();
		os << Child( 3 );
		break;
	case NT_SWITCH:
		if( node->GetToken() )
		{
			os << "[" << node->GetToken()->stringValue << "] ";
		}
		os << "switch( " << Child( 0 ) << " )" << Endl();
		os << "{" << Endl();
		os << Children( hlsl, "", 1 );
		os << "}" << Endl();
		break;
	case NT_CASE:
		if( node->GetChildOrNull( 0 ) == nullptr )
		{
			os << "default:" << Endl();
		}
		else
		{
			os << "case " << Child( 0 ) << ":" << Endl();
		}
		os << Indent() << Child( 1 ) << Unindent();
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
				os << "return " << Child( 0 ) << ";";
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
				os << Child( 2 );
			}
			os << Child( 0 ) << Child( 1 );
		}
		break;
	case NT_SAMPLER_STATE_LIST:
		os << "sampler_state" << Endl();
		os << "{" << Endl();
		os << "}" << Endl();
		break;
	case NT_STATE_ASSIGNMENT:
		os << node->GetToken()->stringValue << " = ";
		if( node->GetSymbol() )
		{
			os << '<' << node->GetSymbol()->name << '>';
		}
		else
		{
			os << Child( 0 );
		}
		os << ";" << Endl();
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
			os << " : register( " << it->second.registerType << it->second.registerNumber;
			if( it->second.space >= 0 )
			{
				os << ", space" << it->second.space;
			}
			os << " )";
		}
		os.Endl();
		os << "{" << Endl();
		os << Children( hlsl, "" ) << Endl();
		os << "}";
		os.Endl();
	}
	break;
	case NT_STATE_VALUE:
		os << node->GetToken()->stringValue;
		break;
	case NT_FUNCTION_ATTRIBUTE_LIST:
		os << Children( hlsl, "" );
		break;
	case NT_FUNCTION_ATTRIBUTE:
		os << '[' << node->GetToken()->stringValue;
		if( node->GetChildrenCount() )
		{
			os << '(' << Children( hlsl, ", " ) << ')';
		}
		os << ']' << Endl();
		break;
	case NT_FUNCTION_ATTRIBUTE_VALUE:
	case NT_PRIMITIVE_TYPE:
		os << node->GetToken()->stringValue;
		break;
	}
	return os;
}
