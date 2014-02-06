#include "stdafx.h"
#include "FXAnalyzer.h"
#include "EffectData.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "ParserUtils.h"
#include "EffectData.h"
#include "Type.h"
#include "ParserState.h"

#define DEFINE_VALUE( prefix, value ) { #value, prefix##_##value }
static StateValue s_addressValues[] = {
	DEFINE_VALUE( D3DTADDRESS, WRAP ),
	DEFINE_VALUE( D3DTADDRESS, MIRROR ),
	DEFINE_VALUE( D3DTADDRESS, CLAMP ),
	DEFINE_VALUE( D3DTADDRESS, BORDER ),
	DEFINE_VALUE( D3DTADDRESS, MIRRORONCE ),
	{ nullptr,			0 },
};
static StateValue s_filterValues[] = {
	DEFINE_VALUE( D3DTEXF, NONE ),
	DEFINE_VALUE( D3DTEXF, POINT ),
	DEFINE_VALUE( D3DTEXF, LINEAR ),
	DEFINE_VALUE( D3DTEXF, ANISOTROPIC ),
	{ nullptr,			0 },
};
static StateValue s_filter11Values[] = {
	DEFINE_VALUE( D3D11_FILTER, MIN_MAG_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, MIN_MAG_POINT_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, MIN_POINT_MAG_LINEAR_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, MIN_POINT_MAG_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, MIN_LINEAR_MAG_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, MIN_LINEAR_MAG_POINT_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, MIN_MAG_LINEAR_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, MIN_MAG_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, ANISOTROPIC ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_MAG_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_MAG_POINT_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_POINT_MAG_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_LINEAR_MAG_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_MAG_LINEAR_MIP_POINT ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_MIN_MAG_MIP_LINEAR ),
	DEFINE_VALUE( D3D11_FILTER, COMPARISON_ANISOTROPIC ),
	{ nullptr,			0 },
};
static StateValue s_comparisonValues[] = {
	DEFINE_VALUE( D3D11_COMPARISON, NEVER ),
	DEFINE_VALUE( D3D11_COMPARISON, LESS ),
	DEFINE_VALUE( D3D11_COMPARISON, EQUAL ),
	DEFINE_VALUE( D3D11_COMPARISON, LESS_EQUAL ),
	DEFINE_VALUE( D3D11_COMPARISON, GREATER ),
	DEFINE_VALUE( D3D11_COMPARISON, NOT_EQUAL  ),
	DEFINE_VALUE( D3D11_COMPARISON, GREATER_EQUAL ),
	DEFINE_VALUE( D3D11_COMPARISON, ALWAYS ),

	DEFINE_VALUE( D3DCMP, LESSEQUAL ),
	DEFINE_VALUE( D3DCMP, NOTEQUAL  ),
	DEFINE_VALUE( D3DCMP, GREATEREQUAL ),
	{ nullptr,			0 },
};
static StateValue s_blendValues[] = {
	DEFINE_VALUE( D3DBLEND, ZERO ),
	DEFINE_VALUE( D3DBLEND, ONE ),
	DEFINE_VALUE( D3DBLEND, SRCCOLOR ),
	DEFINE_VALUE( D3DBLEND, INVSRCCOLOR ),
	DEFINE_VALUE( D3DBLEND, SRCALPHA ),
	DEFINE_VALUE( D3DBLEND, INVSRCALPHA ),
	DEFINE_VALUE( D3DBLEND, DESTALPHA ),

	DEFINE_VALUE( D3DBLEND, INVDESTALPHA ),
	DEFINE_VALUE( D3DBLEND, DESTCOLOR ),
	DEFINE_VALUE( D3DBLEND, INVDESTCOLOR ),
	DEFINE_VALUE( D3DBLEND, SRCALPHASAT ),
	DEFINE_VALUE( D3DBLEND, BOTHSRCALPHA ),
	DEFINE_VALUE( D3DBLEND, BOTHINVSRCALPHA ),
	DEFINE_VALUE( D3DBLEND, BLENDFACTOR ),
	DEFINE_VALUE( D3DBLEND, INVBLENDFACTOR ),
	DEFINE_VALUE( D3DBLEND, SRCCOLOR2 ),
	DEFINE_VALUE( D3DBLEND, INVSRCCOLOR2 ),
	{ nullptr,			0 },
};
static StateValue s_blendOpValues[] = {
	DEFINE_VALUE( D3DBLENDOP, ADD ),
	DEFINE_VALUE( D3DBLENDOP, SUBTRACT ),
	DEFINE_VALUE( D3DBLENDOP, REVSUBTRACT ),
	DEFINE_VALUE( D3DBLENDOP, MIN ),
	DEFINE_VALUE( D3DBLENDOP, MAX ),
	{ nullptr,			0 },
};
static StateValue s_fillModeValues[] = {
	DEFINE_VALUE( D3DFILL, POINT ),
	DEFINE_VALUE( D3DFILL, WIREFRAME ),
	DEFINE_VALUE( D3DFILL, SOLID ),
	{ nullptr,			0 },
};
// This was an elegant way to convert effects from LH to RH
static StateValue s_cullModeValues[] = {
	DEFINE_VALUE( D3DCULL, NONE ),
	{ "CW", D3DCULL_CCW },
	{ "CCW", D3DCULL_CW },
	{ nullptr,			0 },
};
static StateValue s_colorWriteValues[] = {
	DEFINE_VALUE( D3DCOLORWRITEENABLE, RED ),
	DEFINE_VALUE( D3DCOLORWRITEENABLE, GREEN ),
	DEFINE_VALUE( D3DCOLORWRITEENABLE, BLUE ),
	DEFINE_VALUE( D3DCOLORWRITEENABLE, ALPHA ),
	{ "false", 0 },
	{ "true", 0xf },
	{ nullptr,			0 },
};

static StateValue s_boolValues[] = {
	{ "true",			1 },
	{ "false",			0 },
	{ nullptr,			0 },
};

static StateValue s_shaderValues[] = {
	{ "NULL",			0 },
	{ nullptr,			0 },
};

StateValue g_renderStateNames[] = {
	DEFINE_VALUE( D3DRS, ALPHATESTENABLE ),
	DEFINE_VALUE( D3DRS, SRCBLEND ),
	DEFINE_VALUE( D3DRS, DESTBLEND ),
	DEFINE_VALUE( D3DRS, ALPHAREF ),
	DEFINE_VALUE( D3DRS, ALPHAFUNC ),
	DEFINE_VALUE( D3DRS, ALPHABLENDENABLE ),
	DEFINE_VALUE( D3DRS, BLENDOP ),
	DEFINE_VALUE( D3DRS, ZENABLE ),
	DEFINE_VALUE( D3DRS, ZWRITEENABLE ),
	DEFINE_VALUE( D3DRS, ZFUNC ),
	DEFINE_VALUE( D3DRS, FILLMODE ),
	DEFINE_VALUE( D3DRS, COLORWRITEENABLE ),
	DEFINE_VALUE( D3DRS, DEPTHBIAS ),
	DEFINE_VALUE( D3DRS, SLOPESCALEDEPTHBIAS ),
	DEFINE_VALUE( D3DRS, SRGBWRITEENABLE ),
	DEFINE_VALUE( D3DRS, SEPARATEALPHABLENDENABLE ),
	DEFINE_VALUE( D3DRS, BLENDOPALPHA ),
	DEFINE_VALUE( D3DRS, SRCBLENDALPHA ),
	DEFINE_VALUE( D3DRS, DESTBLENDALPHA ),
	DEFINE_VALUE( D3DRS, COLORWRITEENABLE1 ),
	DEFINE_VALUE( D3DRS, COLORWRITEENABLE2 ),
	DEFINE_VALUE( D3DRS, COLORWRITEENABLE3 ),
	DEFINE_VALUE( D3DRS, CULLMODE ),
	{ "vertexshader", -1 },
	{ "pixelshader", -2 },
	{ "computeshader", -3 },
	{ "geometryshader", -4 },
	{ "hullshader", -5 },
	{ "domainshader", -6 },
	{ nullptr,			0 },
};

StateDescription g_samplerStates[] = {
	{ "AddressU",		SVT_BYTE,	s_addressValues,	offsetof( Sampler, addressU ) },
	{ "AddressV",		SVT_BYTE,	s_addressValues,	offsetof( Sampler, addressV ) },
	{ "AddressW",		SVT_BYTE,	s_addressValues,	offsetof( Sampler, addressW ) },
	{ "BorderColor",	SVT_COLOR,	nullptr,			offsetof( Sampler, borderColor ) },
	{ "MagFilter",		SVT_BYTE,	s_filterValues,		offsetof( Sampler, magFilter ) },
	{ "MinFilter",		SVT_BYTE,	s_filterValues,		offsetof( Sampler, minFilter ) },
	{ "MipFilter",		SVT_BYTE,	s_filterValues,		offsetof( Sampler, mipFilter ) },
	{ "Filter",			SVT_BYTE,	s_filter11Values,	offsetof( Sampler, filter ) },
	{ "MaxAnisotropy",	SVT_BYTE,	nullptr,			offsetof( Sampler, maxAnisotropy ) },
	{ "MaxMipLevel",	SVT_FLOAT,	nullptr,			offsetof( Sampler, minLOD ) },
	{ "MinLOD",			SVT_FLOAT,	nullptr,			offsetof( Sampler, minLOD ) },
	{ "MaxLOD",			SVT_FLOAT,	nullptr,			offsetof( Sampler, maxLOD ) },
	{ "MipMapLodBias",	SVT_FLOAT,	nullptr,			offsetof( Sampler, mipLODBias ) },
	{ "ComparisonFunc",	SVT_BYTE,	s_comparisonValues,	offsetof( Sampler, comparisonFunc ) },
	{ "SRGBTexture",	SVT_BOOL,	s_boolValues,		offsetof( Sampler, srgbTexture ) },
	{ nullptr,			SVT_BOOL,	nullptr,			0 },
};

StateDescription g_renderStates[] = {
	{ "AlphaTestEnable",			SVT_BOOL,	s_boolValues,	0 },
	{ "SrcBlend",					SVT_DWORD,	s_blendValues,	0 },
	{ "DestBlend",					SVT_DWORD,	s_blendValues,	0 },
	{ "AlphaRef",					SVT_DWORD,	nullptr,	0 },
	{ "AlphaFunc",					SVT_DWORD,	s_comparisonValues,	0 },
	{ "AlphaBlendEnable",			SVT_BOOL,	s_boolValues,	0 },
	{ "BlendOp",					SVT_DWORD,	s_blendOpValues,	0 },
	{ "ZEnable",					SVT_BOOL,	s_boolValues,	0 },
	{ "ZWriteEnable",				SVT_BOOL,	s_boolValues,	0 },
	{ "ZFunc",						SVT_DWORD,	s_comparisonValues,	0 },
	{ "FillMode",					SVT_DWORD,	s_fillModeValues,	0 },
	{ "ColorWriteEnable",			SVT_DWORD,	s_colorWriteValues,	0 },
	{ "DepthBias",					SVT_FLOAT,	nullptr,	0 },
	{ "SlopeScaleDepthBias",		SVT_FLOAT,	nullptr,	0 },
	{ "SRGBWriteEnable",			SVT_BOOL,	s_boolValues,	0 },
	{ "SeparateAlphaBlendEnable",	SVT_BOOL,	s_boolValues,	0 },
	{ "BlendOpAlpha",				SVT_DWORD,	s_blendOpValues,	0 },
	{ "SrcBlendAlpha",				SVT_DWORD,	s_blendValues,	0 },
	{ "DestBlendAlpha",				SVT_DWORD,	s_blendValues,	0 },
	{ "ColorWriteEnable1",			SVT_BOOL,	s_boolValues,	0 },
	{ "ColorWriteEnable2",			SVT_BOOL,	s_boolValues,	0 },
	{ "ColorWriteEnable3",			SVT_BOOL,	s_boolValues,	0 },
	{ "CullMode",					SVT_DWORD,	s_cullModeValues,	0 },
	{ "LocalViewer",				SVT_BOOL,	s_boolValues,	0 },

	{ "vertexshader",				SVT_DWORD,	s_shaderValues,	0 },
	{ "pixelshader",				SVT_DWORD,	s_shaderValues,	0 },
	{ "computeshader",				SVT_DWORD,	s_shaderValues,	0 },
	{ "geometryshader",				SVT_DWORD,	s_shaderValues,	0 },
	{ "hullshader",					SVT_DWORD,	s_shaderValues,	0 },
	{ "domainshader",				SVT_DWORD,	s_shaderValues,	0 },


	{ nullptr,						SVT_BOOL,	nullptr,			0 },
};





static void CastExpressionElementValue( ExpressionValueElement& value, int from, int to )
{
	if( from == to )
	{
		return;
	}
	switch( from )
	{
	case OP_INT:
		switch( to )
		{
		case OP_FLOAT:
			value.floatValue = double( value.intValue );
			return;
		}
		return;
	case OP_FLOAT:
		switch( to )
		{
		case OP_INT:
			value.intValue = int( value.floatValue );
			return;
		}
		return;
	}
}

bool CastExpressionValue( ExpressionValue& value, const Type& from, const Type& to )
{
	if( from == to )
	{
		return true;
	}

	if( to.width > from.width || to.height > from.height )
	{
		return false;
	}

	ExpressionValue result;
	for( int j = 0; j < to.height; ++j )
	{
		for( int i = 0; i < to.width; ++i )
		{
			ExpressionValueElement element = value[min( j, from.height ) * from.width + min( i, from.width )];
			if( from.builtInType != to.builtInType )
			{
				CastExpressionElementValue( element, from.builtInType, to.builtInType );
			}
			result.push_back( element );
		}
	}
	value.clear();
	value.assign( result.begin(), result.end() );
	return true;
}

static bool GetElementType( const Type& type, unsigned index, Type& elementType )
{
	if( type.symbol )
	{
		if( !type.symbol->definition )
		{
			return false;
		}
		for( unsigned i = 0; i < type.symbol->definition->GetChildrenCount(); ++i )
		{
			if( index <= type.symbol->definition->GetChild( i )->GetChildrenCount() )
			{
				elementType = type.symbol->definition->GetChild( i )->GetType();
				return true;
			}
			index -= type.symbol->definition->GetChild( i )->GetChildrenCount();
		}
		return false;
	}
	else
	{
		if( type.height > 1 )
		{
			if( int( index ) >= type.height )
			{
				return false;
			}
			elementType = type;
			elementType.height = 1;
			return true;
		}
		else
		{
			elementType = type;
			elementType.height = 1;
			elementType.width = 1;
			return true;
		}
	}
}

bool EvaluateInitializer( ParserState& state, ASTNode* node, Type& type, ExpressionValue& value, StateValue* stateValues )
{
	type.symbol = nullptr;
	if( node->GetNodeType() == NT_INLINE_CONSTRUCTOR )
	{
		int elements = type.width * type.height;

		for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
		{
			Type childType;
			ExpressionValue childValue;
			if( !EvaluateExpression( state, node->GetChild( i ), childType, childValue, stateValues ) )
			{
				return false;
			}

			Type newChildType = childType;
			newChildType.builtInType = type.builtInType;
			if( !CastExpressionValue( childValue, childType, newChildType ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}

			for( int j = 0; j < childType.width * childType.height; ++j )
			{
				value.push_back( childValue[j] );
				if( elements == 0 )
				{
					state.ShowMessage( node->GetToken()->fileLocation, EC_INCORRECT_NUMBER_OF_ARGS );
					return false;
				}
				elements--;
			}
		}
		if( elements > 0 )
		{
			state.ShowMessage( node->GetToken()->fileLocation, EC_INCORRECT_NUMBER_OF_ARGS );
			return false;
		}
	}
	else
	{
		Type exprType;
		if( !EvaluateExpression( state, node, exprType, value, stateValues ) )
		{
			return false;
		}
		if( exprType != type )
		{
			if( !CastExpressionValue( value, exprType, type ) )
			{
				return false;
			}
		}
	}
	return true;
}

bool EvaluateExpression( ParserState& state, ASTNode* node, Type& type, ExpressionValue& value, StateValue* stateValues )
{
	type.symbol = nullptr;
	switch( node->GetNodeType() )
	{
	case NT_CONSTANT:
		switch( node->GetToken()->type )
		{
		case OP_INT_CONST:
			{
				type.FromTokenType( OP_INT );
				ExpressionValueElement element;
				element.intValue = ParseNumber( node->GetToken()->stringValue.start, node->GetToken()->stringValue.end );
				value.push_back( element );
			}
			return true;
		case OP_BOOL_CONST:
			{
				type.FromTokenType( OP_INT );
				ExpressionValueElement element;
				element.intValue = node->GetToken()->intValue != 0 ? 1 : 0;
				value.push_back( element );
			}
			return true;
		case OP_FLOAT_CONST:
			{
				type.FromTokenType( OP_FLOAT );
				ExpressionValueElement element;
				element.floatValue = ParseFloat( node->GetToken()->stringValue.start, node->GetToken()->stringValue.end );
				value.push_back( element );
			}
			return true;
		case OP_STRING_CONST:
			state.ShowMessage( node->GetToken()->fileLocation, EC_UNSUPPORTED_TYPE, "string" );
			return false;
		}
		return false;
	case NT_VAR_IDENTIFIER:
		{
			if( node->GetSymbol()->definition == nullptr )
			{
				return false;
			}
			ASTNode* definition = node->GetSymbol()->definition;
			if( definition->GetChildOrNull( 1 ) == nullptr )
			{
				state.ShowMessage( definition->GetToken()->fileLocation, EC_UNINITIALIZED_VAR, ToString( definition->GetSymbol()->name ).c_str() );
				return false;
			}
			type = node->GetSymbol()->type;
			return EvaluateInitializer( state, definition->GetChildOrNull( 1 ), type, value, nullptr );
		}
		return false;
	case NT_STATE_VALUE:
		if( stateValues )
		{
			std::string name = ToString( node->GetToken()->stringValue );
			for( unsigned i = 0; stateValues[i].name; ++i )
			{
				if( _stricmp( name.c_str(), stateValues[i].name ) == 0 )
				{
					memset( &type, 0, sizeof( type ) );
					type.FromTokenType( OP_INT );
					ExpressionValueElement element;
					element.intValue = stateValues[i].value;
					value.push_back( element );
					return true;
				}
			}
			state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_STATE_VALUE, ToString( node->GetToken()->stringValue ).c_str() );
		}
		return false;
	case NT_EXPRESSION:
		{
			if( node->GetToken()->type == 0 )
			{
				if( !EvaluateExpression( state, node->GetChild( 0 ), type, value, stateValues ) )
				{
					return false;
				}
				return true;
			}
			ExpressionValue left, right;
			Type leftType, rightType;
			if( !EvaluateExpression( state, node->GetChild( 0 ), leftType, left, stateValues ) )
			{
				return false;
			}
			if( !EvaluateExpression( state, node->GetChild( 1 ), rightType, right, stateValues ) )
			{
				return false;
			}
			if( leftType.symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			if( rightType.symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			if( !GetCommonType( leftType, rightType, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}

			CastExpressionValue( left, leftType, type );
			CastExpressionValue( right, rightType, type );

			switch( node->GetToken()->type )
			{
			case OP_STAR:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue * right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = left[i].floatValue * right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_SLASH:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue / right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = left[i].floatValue / right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_PERCENT:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue % right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = fmod( left[i].floatValue, right[i].floatValue );
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_PLUS:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue + right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = left[i].floatValue + right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_DASH:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue - right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = left[i].floatValue - right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_LEFT_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue << right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			case OP_RIGHT_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue >> right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			case OP_LESS:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue < right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue < right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_MORE:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue > right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue > right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_LE_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue <= right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue <= right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_GE_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue >= right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue >= right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_EQ_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue == right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue == right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_NE_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue != right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].floatValue != right[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_AMPERSAND:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue & right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			case OP_CARET:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue ^ right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			case OP_VERTICAL_BAR:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue | right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			case OP_AND_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue && right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = ( left[i].floatValue != 0 && right[i].floatValue != 0 ) ? 1 : 0;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_OR_OP:
				switch( leftType.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue || right[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					type.builtInType = OP_INT;
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = ( left[i].floatValue != 0 || right[i].floatValue != 0 ) ? 1 : 0;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_EQUAL:
			case OP_MUL_ASSIGN:
			case OP_DIV_ASSIGN:
			case OP_MOD_ASSIGN:
			case OP_ADD_ASSIGN:
			case OP_SUB_ASSIGN:
			case OP_LEFT_ASSIGN:
			case OP_RIGHT_ASSIGN:
			case OP_AND_ASSIGN:
			case OP_XOR_ASSIGN:
			case OP_OR_ASSIGN:
				state.ShowMessage( node->GetToken()->fileLocation, EC_SIDE_EFFECT_IN_STATE_ASSIGMENT, ToString( node->GetToken()->stringValue ).c_str() );
				return true;
			case OP_COMA:
				for( size_t i = 0; i < right.size(); ++i )
				{
					value.push_back( right[i] );
				}
				return true;
			}
		}
		return false;
	case NT_PREFIX_EXPRESSION:
		{
			ExpressionValue left;
			if( !EvaluateExpression( state, node->GetChild( 0 ), type, left, stateValues ) )
			{
				return false;
			}
			switch( node->GetToken()->type )
			{
			case OP_INC_OP:
			case OP_DEC_OP:
				state.ShowMessage( node->GetToken()->fileLocation, EC_SIDE_EFFECT_IN_STATE_ASSIGMENT, ToString( node->GetToken()->stringValue ).c_str() );
				return false;
			case OP_DASH:
				switch( type.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = -left[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = -left[i].floatValue;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_BANG:
				switch( type.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = left[i].intValue ? 0 : 1;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.floatValue = left[i].floatValue == 0.0 ? 1.0 : 0.0;
						value.push_back( element );
					}
					return true;
				}
				return false;
			case OP_TILDE:
				switch( type.builtInType )
				{
				case OP_INT:
					for( size_t i = 0; i < left.size(); ++i )
					{
						ExpressionValueElement element;
						element.intValue = ~left[i].intValue;
						value.push_back( element );
					}
					return true;
				case OP_FLOAT:
					state.ShowMessage( node->GetToken()->fileLocation, EC_OPERATOR_TYPE_MISMATCH, ToString( node->GetToken()->stringValue ).c_str() );
				}
				return false;
			}
		}
		return false;
	case NT_POSTFIX_EXPRESSION:
		{
			Type leftType;
			ExpressionValue left;
			if( !EvaluateExpression( state, node->GetChild( 0 ), leftType, left, stateValues ) )
			{
				return false;
			}
			switch( node->GetToken()->type )
			{
			case OP_LEFT_BRACKET:
				{
					Type indexType;
					indexType.FromTokenType( OP_INT );

					ExpressionValue index;
					Type oldIndexType;
					if( !EvaluateExpression( state, node->GetChild( 1 ), oldIndexType, index, stateValues ) )
					{
						return false;
					}
					if( !CastExpressionValue( index, oldIndexType, indexType ) )
					{
						return false;
					}
					if( !GetElementType( leftType, index[0].intValue, type ) )
					{
						state.ShowMessage( node->GetToken()->fileLocation, EC_INDEX_OUT_OF_RANGE, index[0].intValue );
						return false;
					}
					type = leftType;
					if( leftType.height > 1 )
					{
						for( int i = 0; i < leftType.width; ++i )
						{
							value.push_back( left[index[0].intValue * leftType.width + i] );
						}
						type.height = 1;
					}
					else
					{
						value.push_back( left[index[0].intValue] );
						type.width = 1;
						type.height = 1;
					}
				}
				return true;
			case OP_ID:
				{
					if( leftType.symbol )
					{
						state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
						return false;
					}
					else
					{
						type = leftType;
						if( leftType.height != 1 )
						{
							type.height = 1;
							type.width = 0;
							int swizzleSet = -1;
							for( const char* c = node->GetToken()->stringValue.start; c != node->GetToken()->stringValue.end; ++c )
							{
								if( *c != '_' )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								++c;
								if( c == node->GetToken()->stringValue.end )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								if( *c == 'm' )
								{
									if( swizzleSet == -1 )
									{
										swizzleSet = 1;
									}
									else if( swizzleSet == 0 )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
									++c;
									if( c == node->GetToken()->stringValue.end )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
								}
								else
								{
									if( swizzleSet == -1 )
									{
										swizzleSet = 0;
									}
									else if( swizzleSet == 1 )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
								}
								if( *c < '0' || *c > '4' )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								int row = *c - '0';
								if( swizzleSet == 0 )
								{
									if( row == 0 )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
									--row;
								}
								else if( row >= 4 )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								++c;
								if( c == node->GetToken()->stringValue.end )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								int column = *c - '0';
								if( swizzleSet == 0 )
								{
									if( column == 0 )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
									--column;
								}
								else if( column >= 4 )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								type.width++;
								value.push_back( left[row * leftType.width + column] );
							}
							return true;
						}
						else
						{
							const char* swizzle[2] = { "xyzw", "rgba" };
							int swizzleIndex = -1;
							for( const char* c = node->GetToken()->stringValue.start; c != node->GetToken()->stringValue.end; ++c )
							{
								if( swizzleIndex == -1 )
								{
									for( int k = 0; k < 2; ++k )
									{
										if( strchr( swizzle[k], *c ) )
										{
											swizzleIndex = k;
											break;
										}
									}
									if( swizzleIndex == -1 )
									{
										state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
										return false;
									}
								}
								const char* s = strchr( swizzle[swizzleIndex], *c );
								if( s == nullptr )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								int index = s - swizzle[swizzleIndex];
								if( index > leftType.width )
								{
									state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_SWIZZLE, ToString( node->GetToken()->stringValue ).c_str() );
									return false;
								}
								value.push_back( left[index] );
							}
							type.width = node->GetToken()->stringValue.end - node->GetToken()->stringValue.start;
							type.height = 1;
							return true;
						}
					}
				}
				return false;
			case OP_INC_OP:
			case OP_DEC_OP:
				state.ShowMessage( node->GetToken()->fileLocation, EC_SIDE_EFFECT_IN_STATE_ASSIGMENT, ToString( node->GetToken()->stringValue ).c_str() );
				return false;
			}
		}
		return false;
	case NT_CAST_EXPRESSION:
		{
			if( node->GetType().symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			Type leftType;
			if( !EvaluateExpression( state, node->GetChild( 0 ), leftType, value, stateValues ) )
			{
				return false;
			}
			type = node->GetType();
			switch( type.builtInType )
			{
			case OP_BOOL:
			case OP_INT:
			case OP_UINT:
				type.builtInType = OP_INT;
				break;
			case OP_HALF:
			case OP_FLOAT:
			case OP_DOUBLE:
				type.builtInType = OP_FLOAT;
				break;
			default:
				state.ShowMessage( node->GetToken()->fileLocation, EC_UNSUPPORTED_TYPE, ToString( node->GetToken()->stringValue ).c_str() );
				return false;
			}
			return CastExpressionValue( value, leftType, type );
		}
		return false;
	case NT_CONDITIONAL_EXPRESSION:
		{
			ExpressionValue cond, left, right;
			Type condType, leftType, rightType;
			if( !EvaluateExpression( state, node->GetChild( 0 ), condType, cond, stateValues ) )
			{
				return false;
			}
			if( !EvaluateExpression( state, node->GetChild( 1 ), leftType, left, stateValues ) )
			{
				return false;
			}
			if( !EvaluateExpression( state, node->GetChild( 2 ), rightType, right, stateValues ) )
			{
				return false;
			}
			if( condType.symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			if( leftType.symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			if( rightType.symbol )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_STRUCTS_NOT_SUPPORTED );
				return false;
			}
			if( !GetCommonType( leftType, rightType, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}
			if( !GetCommonType( condType, type, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}
			
			if( !CastExpressionValue( cond, condType, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}
			if( !CastExpressionValue( left, leftType, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}
			if( !CastExpressionValue( right, rightType, type ) )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
				return false;
			}
			for( size_t i = 0; i < cond.size(); ++i )
			{
				switch( type.builtInType )
				{
				case OP_INT:
					value.push_back( cond[i].intValue ? left[i] : right[i] );
					break;
				case OP_FLOAT:
					value.push_back( cond[i].floatValue != 0.0 ? left[i] : right[i] );
					break;
				}
			}
		}
		return true;
	case NT_FUNCTION_CALL:
		{
			if( node->GetSymbol() )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_FUNCTIONS_NOT_SUPPORTED, ToString( node->GetSymbol()->name ).c_str() );
				return false;
			}
			memset( &type, 0, sizeof( type ) );
			type.FromToken( *node->GetToken() );
			switch( type.builtInType )
			{
			case OP_BOOL:
			case OP_INT:
			case OP_UINT:
				type.builtInType = OP_INT;
				break;
			case OP_HALF:
			case OP_FLOAT:
			case OP_DOUBLE:
				type.builtInType = OP_FLOAT;
				break;
			default:
				state.ShowMessage( node->GetToken()->fileLocation, EC_UNSUPPORTED_TYPE, ToString( node->GetToken()->stringValue ).c_str() );
				return false;
			}
		
			int elements = type.width * type.height;

			for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
			{
				Type childType;
				ExpressionValue childValue;
				if( !EvaluateExpression( state, node->GetChild( i ), childType, childValue, stateValues ) )
				{
					return false;
				}
				Type newChildType = childType;
				newChildType.builtInType = type.builtInType;
				if( !CastExpressionValue( childValue, childType, newChildType ) )
				{
					state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
					return false;
				}
				for( int j = 0; j < childType.width * childType.height; ++j )
				{
					value.push_back( childValue[j] );
					if( elements == 0 )
					{
						state.ShowMessage( node->GetToken()->fileLocation, EC_INCORRECT_NUMBER_OF_ARGS );
						return false;
					}
					elements--;
				}
			}
			if( elements > 0 )
			{
				state.ShowMessage( node->GetToken()->fileLocation, EC_INCORRECT_NUMBER_OF_ARGS );
				return false;
			}
		}
		return true;
	}
	return false;
}












bool GetStateValue( ParserState& state, ASTNode* value, const StateDescription& desc, void* dataBlob )
{
	BYTE* blob = reinterpret_cast<BYTE*>( dataBlob );
	if( value == nullptr )
	{
		return false;
	}
	Type expectedType, type;
	switch( desc.type )
	{
	case SVT_BYTE:
	case SVT_DWORD:
	case SVT_BOOL:
		expectedType.FromTokenType( OP_INT );
		break;
	case SVT_FLOAT:
		expectedType.FromTokenType( OP_FLOAT );
		break;
	case SVT_COLOR:
		expectedType.FromTokenType( OP_FLOAT );
		expectedType.width = 4;
		break;
	}
	ExpressionValue exprValue;

	if( value->GetNodeType() == NT_INLINE_CONSTRUCTOR )
	{
		if( !EvaluateInitializer( state, value, expectedType, exprValue, desc.stateValues ) )
		{
			return false;
		}
	}
	else
	{
		if( !EvaluateExpression( state, value, type, exprValue, desc.stateValues ) )
		{
			return false;
		}
		if( expectedType != type )
		{
			if( !CastExpressionValue( exprValue, type, expectedType ) )
			{
				if( desc.type == SVT_COLOR )
				{
					expectedType.builtInType = OP_INT;
					expectedType.width = 1;
					if( !CastExpressionValue( exprValue, type, expectedType ) )
					{
						return false;
					}
					*reinterpret_cast<D3DXCOLOR*>( blob + desc.offset ) = exprValue[0].intValue;
					return true;
				}
				else
				{
					return false;
				}
			}
		}
	}
	switch( desc.type )
	{
	case SVT_DWORD:
		*reinterpret_cast<DWORD*>( blob + desc.offset ) = DWORD( exprValue[0].intValue );
		return true;
	case SVT_BYTE:
		*reinterpret_cast<BYTE*>( blob + desc.offset ) = BYTE( exprValue[0].intValue );
		return true;
	case SVT_FLOAT:
		*reinterpret_cast<float*>( blob + desc.offset ) = float( exprValue[0].floatValue );
		return true;
	case SVT_BOOL:
		*reinterpret_cast<BYTE*>( blob + desc.offset ) = BYTE( exprValue[0].intValue ? 1 : 0 );
		return true;
	case SVT_COLOR:
		{
			reinterpret_cast<Vector4*>( blob + desc.offset )->x = float( exprValue[0].floatValue );
			reinterpret_cast<Vector4*>( blob + desc.offset )->y = float( exprValue[1].floatValue );
			reinterpret_cast<Vector4*>( blob + desc.offset )->z = float( exprValue[2].floatValue );
			reinterpret_cast<Vector4*>( blob + desc.offset )->w = float( exprValue[3].floatValue );
		}
		return true;
	}
	return false;
}

bool ParseStateAssignment( ParserState& parserState, ASTNode* state, StateDescription* states, void* dataBlob )
{
	if( state->GetNodeType() != NT_STATE_ASSIGNMENT )
	{
		return false;
	}
	std::string name = ToString( state->GetToken()->stringValue );
	ASTNode* value = state->GetChildOrNull( 0 );
	if( value == nullptr )
	{
		return false;
	}
	BYTE* blob = reinterpret_cast<BYTE*>( dataBlob );
	for( int i = 0; states[i].stateName; ++i )
	{
		if( _stricmp( name.c_str(), states[i].stateName ) == 0 )
		{
			return GetStateValue( parserState, value, states[i], dataBlob );
		}
	}
	parserState.ShowMessage( state->GetToken()->fileLocation, EC_INVALID_STATE, name.c_str() );
	return false;
}

bool GetSamplerState( ParserState& parserState, ASTNode* node, Sampler& sampler )
{
	sampler.comparison = 0;
	sampler.minFilter = D3DTEXF_LINEAR;
	sampler.magFilter = D3DTEXF_LINEAR;
	sampler.mipFilter = D3DTEXF_LINEAR;
	sampler.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.addressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.minLOD = -FLT_MAX;
	sampler.maxLOD = FLT_MAX;
	sampler.mipLODBias = 0.0f;
	sampler.maxAnisotropy = 16;
	sampler.comparisonFunc = D3D11_COMPARISON_NEVER;
	sampler.borderColor.x = 0.0f;
	sampler.borderColor.y = 0.0f;
	sampler.borderColor.z = 0.0f;
	sampler.borderColor.w = 0.0f;
	sampler.srgbTexture = 0;

	ASTNode* states = node->GetChildOrNull( 1 );
	if( states && states->GetNodeType() == NT_SAMPLER_STATE_LIST )
	{
		sampler.filter = 0xff;
		sampler.minFilter = 0xff;
		sampler.magFilter = 0xff;
		sampler.mipFilter = 0xff;

		for( size_t k = 0; k < states->GetChildrenCount(); ++k )
		{
			ASTNode* state = states->GetChild( k );
			std::string name = ToString( state->GetToken()->stringValue );
			if( _stricmp( name.c_str(), "texture" ) == 0 )
			{
				continue;
			}
			if( !ParseStateAssignment( parserState, state, g_samplerStates, &sampler ) )
			{
				return false;
			}
		}
		if( sampler.filter != 0xff )
		{
			if( D3D11_DECODE_IS_ANISOTROPIC_FILTER( sampler.filter ) )
			{
				sampler.minFilter = sampler.magFilter = sampler.mipFilter = D3DTEXF_ANISOTROPIC;
			}
			else
			{
				sampler.magFilter = D3D11_DECODE_MAG_FILTER( sampler.filter ) + 1;
				sampler.minFilter = D3D11_DECODE_MIN_FILTER( sampler.filter ) + 1;
				sampler.mipFilter = D3D11_DECODE_MIP_FILTER( sampler.filter ) + 1;
			}
			sampler.comparison = D3D11_DECODE_IS_COMPARISON_FILTER( sampler.filter ) ? 1 : 0;
		}
		else
		{
			if( sampler.mipFilter == 0xff )
			{
				sampler.mipFilter = D3DTEXF_LINEAR;
			}
			if( sampler.minFilter == 0xff )
			{
				sampler.minFilter = D3DTEXF_LINEAR;
			}
			if( sampler.magFilter == 0xff )
			{
				sampler.magFilter = D3DTEXF_LINEAR;
			}
			//if( sampler.minFilter == D3DTEXF_ANISOTROPIC || sampler.magFilter == D3DTEXF_ANISOTROPIC ||
			//	sampler.mipFilter == D3DTEXF_ANISOTROPIC )
			//{
			//	sampler.filter = D3D11_FILTER_ANISOTROPIC;
			//}
			//else
			//{
			//	sampler.filter = D3D11_ENCODE_BASIC_FILTER( 
			//		sampler.minFilter == D3DTEXF_LINEAR ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT, 
			//		sampler.magFilter == D3DTEXF_LINEAR ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT, 
			//		sampler.mipFilter == D3DTEXF_LINEAR ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT, 
			//		FALSE );
			//}
		}
	}
	return true;
}

int EvaluateIntegerExpression( ParserState& state, ASTNode* node, int defaultValue )
{
	Type type;
	ExpressionValue value;
	if( !EvaluateExpression( state, node, type, value, nullptr ) )
	{
		return defaultValue;
	}

	Type newType;
	newType.FromTokenType( OP_INT );
	if( !CastExpressionValue( value, type, newType ) )
	{
		state.ShowMessage( node->GetToken()->fileLocation, EC_INVALID_IMPLICIT_CAST );
		return defaultValue;
	}
	return value[0].intValue;
}