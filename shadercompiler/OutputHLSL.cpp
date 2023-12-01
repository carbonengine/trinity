#include "stdafx.h"
#include "OutputHLSL.h"
#include "ParserUtils.h"
#include "ASTNode.h"
#include "HLSLParser.h"

#include "EffectCompilerMetal.h"

namespace
{

	// XSL = X Shading Language. Can be either HLSL or MSL.
	template< typename XSL >
	struct Children
	{
		Children( XSL parent_, const char* glue_, size_t offset_ = 0 )
			:parent( parent_ ),
			glue( glue_ ),
			offset( offset_ )
		{
		}

		XSL parent;
		const char* glue;
		size_t offset;
	};

	template< typename XSL >
	XSL XSLChild( const XSL& parent, size_t childIndex )
	{
		return XSL{ parent.node->GetChild( childIndex ), parent.symbolTable };
	}

	template< typename XSL >
	CodeStream& operator<<( CodeStream& os, const Children<XSL>& children )
	{
		for( size_t i = children.offset; i < children.parent.node->GetChildrenCount(); ++i )
		{
			if( i )
			{
				os << children.glue;
			}
			os << XSLChild( children.parent, i );
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
				os << "min16float";
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

	Type ScalarType( const Type& type )
	{
		Type scalarType = type;
		scalarType.width = scalarType.height = 1;
		return scalarType;
	}

	Type MslTextureTemplateType( const Type& textureType )
	{
		if( textureType.templateParameter )
		{
			// Metal expects type of only one texture component here.
			return ScalarType( *textureType.templateParameter );
		}
		else
		{
			return TypeFromTokenType( OP_FLOAT );
		}
	}

	void PrintTypeMSL( CodeStream& os, Type type )
	{
		switch( type.storageClass )
		{
		case OP_EXTERN:
			os << "extern ";
			break;
	//TODO
#if 0
		case OP_NOINTERPOLATION:
			os << "nointerpolation ";
			break;
		case OP_PRECISE:
			os << "precise ";
			break;
		case OP_SHARED:
			os << "shared ";
			break;
#endif
		case OP_GROUPSHARED:
			// Already processed and mapped to AddressSpace::Threadgroup.
			break;
		case OP_STATIC:
			os << "static ";
			break;
	//TODO
#if 0
		case OP_UNIFORM:
			os << "uniform ";
			break;
#endif
		case OP_VOLATILE:
			os << "volatile ";
			break;
		}
		if( type.modifier == OP_CONST )
		{
			os << "const ";
		}
		else if( type.modifier == OP_PACKOFFSET )
		{
			os << "packed_";
		}
		if( type.symbol )
		{
			os << type.symbol->name;
		}
		else
		{
			switch( type.builtInType )
			{
			case OP_BOOL:
				os << "bool";
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
			case OP_FLOAT:
				os << "float";
				break;
#if 0
			case OP_DOUBLE:
				os << "double";
				break;
#endif
			case OP_VOID:
				os << "void";
				return;
			case OP_STRING:
				os << "string";
				break;
			case OP_SAMPLER2D:
			case OP_SAMPLER3D:
			case OP_SAMPLERCUBE:
			case OP_SAMPLER:
				os << "sampler";
				return;
			// TODO
#if 0
			case OP_SAMPLERCOMPARISON:
				os << "SamplerComparisonState";
				return;
#endif
			case OP_TEXTURE1D:
				os << "texture1d<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURE1DARRAY:
				os << "texture1d_array<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURE:
			case OP_TEXTURE2D:
				os << "texture2d<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURE2DARRAY:
				os << "texture2d_array<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURE3D:
				os << "texture3d<" << MslTextureTemplateType( type ) << '>';
				return;
#if 0
			// There is no "texture3d_array" in MSL.
			case OP_TEXTURE3DARRAY:
				os << "texture3d_array<" << MslTextureTemplateType( type ) << '>';
				return;
#endif
			case OP_TEXTURECUBE:
				os << "texturecube<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURECUBEARRAY:
				os << "texturecube_array<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_TEXTURE2DMS:
				os << "texture2d_ms<" << MslTextureTemplateType( type ) << '>';
				return;
			// Supported since Metal 2.0 (macOS) and Metal 1.0 (iOS).
			case OP_TEXTURE2DMSARRAY:
				os << "texture2d_ms_array<" << MslTextureTemplateType( type ) << '>';
				return;
			case OP_BUFFER:
			case OP_STRUCTUREDBUFFER:
				os << "const " << *type.templateParameter << '*';
				return;
			case OP_RWBUFFER:
			case OP_RWSTRUCTUREDBUFFER:
				os << *type.templateParameter << '*';
				return;
	//TODO
#if 0
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
				os << '<' << *type.templateParameter << ", " << type.templateSamples << '>';
				return;
			case OP_OUTPUTPATCH:
				os << "OutputPatch";
				os << '<' << *type.templateParameter << ", " << type.templateSamples << '>';
				return;
			case OP_RWBYTEADDRESSBUFFER:
				os << "RWByteAddressBuffer";
				return;
#endif
			case OP_RWTEXTURE1D:
				os << "texture1d<" << MslTextureTemplateType( type ) << ", access::" << ( type.metalTextureAccess ? "read_": "" ) << "write>";
				return;
			case OP_RWTEXTURE1DARRAY:
				os << "texture1d_array<" << MslTextureTemplateType( type ) << ", access::" << ( type.metalTextureAccess ? "read_" : "" ) << "write>";
				return;
			case OP_RWTEXTURE2D:
				os << "texture2d<" << MslTextureTemplateType( type ) << ", access::" << ( type.metalTextureAccess ? "read_" : "" ) << "write>";
				return;
			case OP_RWTEXTURE2DARRAY:
				os << "texture2d_array<" << MslTextureTemplateType( type ) << ", access::" << ( type.metalTextureAccess ? "read_" : "" ) << "write>";
				return;
			case OP_RWTEXTURE3D:
				os << "texture3d<" << MslTextureTemplateType( type ) << ", access::" << ( type.metalTextureAccess ? "read_" : "" ) << "write>";
				return;
#if 0
			// There is no "texture3d_array" in MSL.
			case OP_RWTEXTURE3DARRAY:
				os << "texture3d_array<" << MslTextureTemplateType( type ) << ", access::write>";
				return;
			// Geometry shaders are not supported in Metal.
			case OP_POINTSTREAM:
				os << "PointStream";
				os << '<' << *type.templateParameter << '>';
				return;
			case OP_LINESTREAM:
				os << "LineStream";
				os << '<' << *type.templateParameter << '>';
				return;
			case OP_TRIANGLESTREAM:
				os << "TriangleStream";
				os << '<' << *type.templateParameter << '>';
				return;
#endif
			default:
				os << "!!unsupported type: " << type.ToString() << "!!";
				return;
			}
			if( type.width > 1 || type.height > 1 )
			{
				if( type.height > 1 )
				{
					os << type.height << 'x' << type.width;
				}
				else
				{
					os << type.width;
				}
			}
		}
	}

}

CompilerInputStream::CompilerInputStream( ParserState& state, const ShadingLanguage lang )
	:CodeStream( lang )
	,m_state( state )
{
	m_location.fileName = MakeInlineString( "" );
	m_location.lineNumber = unsigned( -1 );
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

CodeStream& operator<<( CodeStream& os, const Type& type )
{
	if( os.shadingLanguage == ShadingLanguage::HLSL )
	{
		PrintTypeHLSL11( os, type );
	}
	else if( os.shadingLanguage == ShadingLanguage::MSL )
	{
		PrintTypeMSL( os, type );
	}
	else
	{
		assert( false );
	}

	return os;
}

CodeStream& operator<<( CodeStream& os, const HLSL& hlsl )
{
	auto node = hlsl.node;

	os << node->GetLocation();

	auto Child = [&]( size_t index ) -> HLSL { return XSLChild( hlsl, index ); };

	switch( node->GetNodeType() )
	{
	case NT_VAR_IDENTIFIER:
		os << node->GetSymbol()->name;
		break;
	case NT_CONSTANT:
		os << node->GetToken()->stringValue;
		break;
	case NT_INLINE_CONSTRUCTOR:
		os << "{ " << Indent() << Children<HLSL>( hlsl, ", " ) << Unindent() << " }";
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
		os << "( " << Indent() << Children<HLSL>( hlsl , ", " ) << Unindent() << " )";
		break;
	case NT_FUNCTION_HEADER:
		if( !node->GetSymbol()->used )
		{
			break;
		}
		os.Endl();
		os << node->GetType() << " " << node->GetSymbol()->name << "( " << Indent() << Children<HLSL>( hlsl, ", " ) << Unindent() << " )";
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
			os << Child( 0 );
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
			os << Child( 0 );
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
		os << Indent() << Children<HLSL>( hlsl, "" ) << Unindent();
		break;
	case NT_STRUCT:
		if( node->GetSymbol()->used )
		{
			os << "struct " << " " << node->GetSymbol()->name << Endl();
			os << "{" << Endl();
			os << Indent() << Children<HLSL>( hlsl, "" ) << Unindent() << Endl();
			os << "};" << Endl();
		}
		break;
	case NT_STRUCT_MEMBER:
		os << Children<HLSL>( hlsl, "" );
		break;
	case NT_BRACKET_LIST:
		for( int i = 0; i < node->GetChildrenCount(); i++ )
		{
			os << "[";
			if( node->GetChildOrNull( i ) )
			{
				os << Child( i );
			}
			os << "]";
		}
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
		os << "{" << Indent() << Endl() << Children<HLSL>( hlsl, "" ) << Unindent() << Endl() << "}" << Endl();
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
		os << Children<HLSL>( hlsl, "", 1 );
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
		os << Children<HLSL>( hlsl, "" ) << Endl();
		os << "}";
		os.Endl();
	}
	break;
	case NT_STATE_VALUE:
		os << node->GetToken()->stringValue;
		break;
	case NT_FUNCTION_ATTRIBUTE_LIST:
		os << Children<HLSL>( hlsl, "" );
		break;
	case NT_FUNCTION_ATTRIBUTE:
		os << '[' << node->GetToken()->stringValue;
		if( node->GetChildrenCount() )
		{
			os << '(' << Children<HLSL>( hlsl, ", " ) << ')';
		}
		os << ']' << Endl();
		break;
	case NT_FUNCTION_ATTRIBUTE_VALUE:
	case NT_PRIMITIVE_TYPE:
		os << node->GetToken()->stringValue;
		break;
	default:
		break;
	}
	return os;
}

static bool IsVectorReferenceParameter( const Symbol* functionSymbol, size_t paramIndex, Type& paramType, AddressSpace& addressSpace )
{
	if( functionSymbol && functionSymbol->definition )
	{
		auto params = functionSymbol->definition->GetChild( 0 );
		auto param = params->GetChildOrNull( paramIndex );
		if( param && param->GetToken() && param->GetToken()->type != OP_IN && param->GetType().IsScalarOrVector() )
		{
			paramType = param->GetType();
			addressSpace = AddressSpace::Thread;
			if( param->GetSymbol() )
			{
				addressSpace = param->GetSymbol()->addressSpace;
			}
			return true;
		}
	}
	return false;
}

CodeStream& operator<<( CodeStream& os, const MSL& msl )
{
	auto node = msl.node;

	os << node->GetLocation();

	auto Child = [&]( size_t index ) -> MSL { return XSLChild( msl, index ); };

	switch( node->GetNodeType() )
	{
	case NT_VAR_IDENTIFIER:
		os << node->GetSymbol()->name;
		break;
	case NT_CONSTANT:
		os << node->GetToken()->stringValue;
		break;
	case NT_INLINE_CONSTRUCTOR:
		os << "{ " << Indent() << Children<MSL>( msl, ", " ) << Unindent() << " }";
		break;
	case NT_PREFIX_EXPRESSION:
		os << GetOperatorSymbol( node->GetToken()->type ) << "( " << Child( 0 ) << " )";
		break;
	case NT_POSTFIX_EXPRESSION:
		if( node->GetToken()->type == OP_ID )
		{
			if( node->GetChild( 0 )->GetType().IsScalarOrVector() && node->GetChild( 0 )->GetType().width == 1 && node->GetChild( 0 )->GetType().height == 1 )
			{
				// swizzle for a scalar "f"
				if( node->GetType().width == 1 && node->GetType().height == 1 )
				{
					// f.x -> x
					os << Child( 0 );
				}
				else
				{
					// f.xx -> float2(f)
					os << node->GetType() << '(' << Child( 0 ) << ")";
				}
				break;
			}
		}
		os << "( " << Child( 0 ) << " )";
		switch( node->GetToken()->type )
		{
		case OP_LEFT_BRACKET:
			{
				auto indexType = node->GetChild( 1 )->GetType();
				
				if( indexType.IsScalarOrVector() && indexType.builtInType != OP_INT && indexType.builtInType != OP_UINT )
				{
					indexType.builtInType = OP_INT;
					os << "[(" << indexType << ")(" << Child( 1 ) << ")]";
				}
				else
				{
					os << "[" << Child( 1 ) << "]";
				}
			}
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
		if( node->GetType().IsStruct() && node->GetChild( 0 )->GetType().IsScalarOrVector() )
		{
			os << node->GetType() << "{ " << Child( 0 ) << " }";
		}
		else if( node->GetType().IsMatrix() &&
				 ( node->GetChild( 0 )->GetType().IsMatrix() ||
				   ( node->GetChild( 0 )->GetNodeType() == NT_POSTFIX_EXPRESSION &&
				     node->GetChild( 0 )->GetChild( 1 )->GetType().IsMatrix() ) ) )
		{
			os << "to_" << node->GetType() << "( " << Child( 0 ) << " )";
		}
		else
		{
			os << "(" << node->GetType() << ")( " << Child( 0 ) << " )";
		}
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
		os << "( " << Indent();
		for( size_t i = 0; i < msl.node->GetChildrenCount(); ++i )
		{
			if( i )
			{
				os << ", ";
			}
			Type paramType;
			AddressSpace paramSpace;
			if( IsVectorReferenceParameter( node->GetSymbol(), i, paramType, paramSpace ) )
			{
				auto arg = msl.node->GetChild( i );
				if( arg->GetNodeType() == NT_POSTFIX_EXPRESSION && arg->GetToken() && arg->GetToken()->type == OP_ID && arg->GetChild( 0 )->GetType().IsVector() )
				{
					auto size = paramType.width;
					os << "VectorReference" << size << "<thread " << arg->GetChild( 0 )->GetType() << "&,";
					switch( paramSpace )
					{
					case AddressSpace::Constant:
						os << "constant ";
						break;
					case AddressSpace::Device:
						os << "device ";
						break;
					case AddressSpace::Threadgroup:
						os << "threadgroup ";
						break;
					default:
						os << "thread ";
						break;
					}
					os << " " << paramType << "&>(";
					os << XSLChild( XSLChild( msl, i ), 0 );
					os << ", int4( 0, 1, 2, 3 )." << arg->GetToken()->stringValue << ")";
					continue;
				}
				else
				{
					os << XSLChild( msl, i );
				}
			}
			else
			{
				os << XSLChild( msl, i );
			}
		}
		os << Unindent() << " )";
		break;
	case NT_FUNCTION_HEADER:
		if( !node->GetSymbol()->used )
		{
			break;
		}
		os.Endl();
		os << node->GetType();
		os << " " << node->GetSymbol()->name << "( " << Indent() << Children<MSL>( msl, ", " ) << Unindent() << " )";
		os.Endl();
		break;
	case NT_FUNCTION_PARAMETER:
	{
		// TODO
#if 0
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
#endif
		Symbol* symbol = node->GetSymbol();
		assert( symbol );

		bool isReference = false;

		if( node->GetToken() )
		{
			// Treat "out" and "inout" parameters as references.
			switch( node->GetToken()->type )
			{
				case OP_OUT:
				case OP_INOUT:
					// References need an address space. The default will be "thread".
					if( symbol->addressSpace == AddressSpace::None )
					{
						symbol->addressSpace = AddressSpace::Thread;
					}
					isReference = true;
					break;
			}
		}
		switch( symbol->addressSpace )
		{
			case AddressSpace::Constant:
				os << "constant ";
				break;
			case AddressSpace::Device:
				os << "device ";
				break;
			case AddressSpace::Thread:
				os << "thread ";
				break;
			case AddressSpace::Threadgroup:
				os << "threadgroup ";
				break;
			default:
				break;
		}
		// TODO: Do we need this? Child[2] is a primitive type for (unsupported) geometry shaders.
#if 0
		if( node->GetChildOrNull( 2 ) )
		{
			os << Child( 2 ) << ' ';
		}
#endif
		os << node->GetType();
		if( isReference )
		{
			// Don't need to add & for arrays.
			if( node->GetType().arrayDimensions == 0)
			{
				os << '&';
			}
		}
		os << " " << symbol->name;
		if( node->GetChildOrNull( 0 ) )
		{
			os << Child( 0 );
		}
		if( symbol )
		{
			for( auto it = symbol->registerSpecifier.begin(); it != symbol->registerSpecifier.end(); ++it )
			{
				const RegisterSpecifier& reg = it->second;
				if( reg.registerType == MetalRegister::StageIn )
				{
					os << " [[ stage_in ]]";
				}
				else if( reg.registerType == MetalRegister::Attribute )
				{
					os << "[[ attribute(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::CBuffer )
				{
					os << " [[ CBUFFER(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::SRV )
				{
					os << " [[ SRV(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::Texture )
				{
					os << " [[ texture(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::Sampler )
				{
					os << " [[ sampler(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::UAV )
				{
					if( node->GetType().IsTexture() )
					{
						os << " [[ UAVT(" << reg.registerNumber << ") ]]";
					}
					else
					{
						os << " [[ UAV(" << reg.registerNumber << ") ]]";
					}
				}
				else if( reg.registerType == MetalRegister::ThreadGroup )
				{
					os << " [[ threadgroup(" << reg.registerNumber << ") ]]";
				}
				else if( reg.registerType == MetalRegister::User )
				{
					os << " [[ user(" << symbol->semantic << ") ]]";
				}
				else if( reg.registerType == MetalRegister::System )
				{
					os << "[[ " << MetalSystemSemanticsType::GetString( reg.registerNumber ) << " ]]";
				}
				else
				{
					os << " [[ !!unsupported_attribute " << reg.registerType << "!! ]]";
				}
			}
		}
		if( node->GetChildOrNull( 1 ) )
		{
			os << " = " << Child( 1 );
		}
	}
	break;
	case NT_NAME_DECLARATION:
	{
		Symbol* symbol = node->GetSymbol();
		if( !symbol->used )
		{
			break;
		}
		// TODO
#if 0
		switch( symbol->interpolationModifier )
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
#endif
		switch( symbol->addressSpace )
		{
		case AddressSpace::Constant:
			os << "constant ";
			break;
		case AddressSpace::Device:
			os << "device ";
			break;
		case AddressSpace::Thread:
			os << "thread ";
			break;
		case AddressSpace::Threadgroup:
			os << "threadgroup ";
			break;
		default:
			break;
		}

		os << symbol->type << " " << symbol->name;
		if( node->GetChildOrNull( 0 ) )
		{
			os << Child( 0 );
		}
		// TODO
#if 0
		if( symbol->packOffset.subComponent >= 0 )
		{
			os << " : packoffset( c" << symbol->packOffset.subComponent;
			if( symbol->packOffset.component.start )
			{
				os << "." << symbol->packOffset.component;
			}
			os << " )";
		}
#endif
		for( auto it = symbol->registerSpecifier.begin(); it != symbol->registerSpecifier.end(); ++it )
		{
			const RegisterSpecifier& reg = it->second;
			if( reg.registerType == MetalRegister::Attribute )
			{
				os << "[[ attribute(" << reg.registerNumber << ") ]]";
			}
			else if( reg.registerType == MetalRegister::CBuffer )
			{
				// just ignore it
			}
			else if( reg.registerType == MetalRegister::User )
			{
				os << " [[ user(" << symbol->semantic << ") ]]";
			}
			else if( reg.registerType == MetalRegister::System )
			{
				os << "[[ " << MetalSystemSemanticsType::GetString( reg.registerNumber ) << " ]]";
			}
			else
			{
				os << "[[ !!unsupported_register " << reg.registerType << "!! ]]";
			}
		}

		if( node->GetChildOrNull( 1 ) )
		{
			if( !node->GetType().IsSampler() )
			{
				os << " = ";
			}
			os << Child( 1 );
		}
		os << ";";
		os.Endl();
	}
	break;
	case NT_VAR_DECLARATION_LIST:
		os << Indent() << Children<MSL>( msl, "" ) << Unindent();
		break;
	case NT_STRUCT:
		if( node->GetSymbol()->used )
		{
			os << "struct " << " " << node->GetSymbol()->name << Endl();
			os << "{" << Endl();
			os << Indent() << Children<MSL>( msl, "" ) << Unindent() << Endl();
			os << "};" << Endl();
		}
		break;
	case NT_STRUCT_MEMBER:
		os << Children<MSL>( msl, "" );
		break;
	case NT_BRACKET_LIST:
		for( int i = 0; i < node->GetChildrenCount(); i++ )
		{
			os << "[";
			if( node->GetChildOrNull( i ) )
			{
				os << Child( i );
			}
			os << "]";
		}
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
		os << "{" << Indent() << Endl() << Children<MSL>( msl, "" ) << Unindent() << Endl() << "}" << Endl();
		break;
	case NT_EXPRESSION_STATEMENT:
		if( node->GetChildrenCount() )
		{
			os << Child( 0 );
		}
		os << ";" << Endl();
		break;
	case NT_IF:
		os << "if( " << Child( 0 ) << " )" << Endl();
		os << Child( 1 );
		if( node->GetChildrenCount() > 2 )
		{
			os << "else" << Endl() << Child( 2 );
		}
		break;
	case NT_WHILE:
		os << "while( " << Child( 0 ) << " )" << Endl();
		os << Child( 1 );
		break;
	case NT_DO:
		os << "do" << Endl();
		os << Child( 1 );
		os << "while( " << Child( 0 ) << " );" << Endl();
		break;
	case NT_FOR:
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
		os << "switch( ";
		{
			auto childType = node->GetChild( 0 )->GetType();

			if( childType.IsScalarOrVector() && childType.builtInType != OP_INT && childType.builtInType != OP_UINT )
			{
				childType.builtInType = OP_INT;
				os << "(" << childType << ")(" << Child( 0 ) << ")";
			}
			else
			{
				os << Child( 0 );
			}
		}
		os << " )" << Endl();

		os << "{" << Endl();
		os << Children<MSL>( msl, "", 1 );
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
			os << "discard_fragment();";
			break;
		}
		os.Endl();
		break;
	case NT_FUNCTION_DEFINITION:
		if( node->GetChild( 0 )->GetSymbol()->used )
		{
			// function attributes
			if( node->GetChildOrNull( 2 ) )
			{
				os << Child( 2 );
			}
			else
			{
				os << " __attribute__((always_inline)) ";
			}
			// header
			os << Child( 0 ) << Endl();
			// body
			os << Child( 1 ) << Endl();
		}
		break;
	case NT_SAMPLER_STATE_LIST:
		// TODO: This must be improved.
#if 0
		{
			os << '(';
			int stateCount = 0;
			for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i)
			{
				ASTNode* assignment = node->GetChild( i );
				assert( assignment->GetNodeType() == NT_STATE_ASSIGNMENT );

				InlineString lhs = assignment->GetToken()->stringValue;

				static const InlineString texture = MakeInlineString( "Texture" );
				if( lhs != texture )
				{
					if( stateCount > 0 )
					{
						os << ", ";
					}

					InlineString rhs = assignment->GetChild( 0 )->GetToken()->stringValue;
					os << MapHLSLtoMSL( lhs ) << "::" << MapHLSLtoMSL( rhs );
					++stateCount;
				}

				ScannerToken token;
				token.type = OP_ID;
				token.intValue = 0;
				token.stringValue = MakeInlineString( "&" );
				token.fileLocation = assignment->GetLocation();

			}
			os << ')';
		}
#endif
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
		// Assert here. All NT_CBUFFER nodes should have been replaced with NT_STRUCT nodes
		// plus necessary adjustments in other places.
		assert( false );
		break;
	case NT_STATE_VALUE:
		os << node->GetToken()->stringValue;
		break;
	case NT_FUNCTION_ATTRIBUTE_LIST:
		os << Children<MSL>( msl, "" );
		break;
	case NT_FUNCTION_ATTRIBUTE:
		{
			auto attrib = ToString( node->GetToken()->stringValue );
			if( attrib == "vertex" || attrib == "fragment" || attrib == "kernel" )
			{
				os << ' ' << attrib << ' ';
			}
		}
		break;
	case NT_FUNCTION_ATTRIBUTE_VALUE:
	case NT_PRIMITIVE_TYPE:
		os << node->GetToken()->stringValue;
		break;
	default:
		break;
	}
	return os;
}
