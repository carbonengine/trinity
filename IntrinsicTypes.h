#pragma once

#include "ASTNode.h"
#include "HLSLParser.h"
#include "Type.h"


inline Type MulIntrinsicType( ASTNode* call, int argIndex )
{
	ASTNode* arg0 = call->GetChildOrNull( 0 );
	ASTNode* arg1 = call->GetChildOrNull( 1 );
	if( arg0 == nullptr || arg1 == nullptr )
	{
		Type type;
		type.FromTokenType( OP_VOID );
		return type;
	}
	Type t0 = arg0->GetType();
	Type t1 = arg1->GetType();
	if( !t0.IsScalarOrVector() || !t1.IsScalarOrVector() )
	{
		Type type;
		type.FromTokenType( OP_VOID );
		return type;
	}
	bool isScalar0 = t0.width == 1 && t0.height == 1;
	bool isScalar1 = t1.width == 1 && t1.height == 1;
	bool isVector0 = t0.width > 1 && t0.height == 1;
	bool isVector1 = t1.width > 1 && t1.height == 1;
	bool isMatrix0 = t0.width > 1 && t0.height > 1;
	bool isMatrix1 = t1.width > 1 && t1.height > 1;
	if( GetNumericTypePrecedence( t1.builtInType ) > GetNumericTypePrecedence( t0.builtInType ) )
	{
		t0.builtInType = t1.builtInType;
	}
	else
	{
		t1.builtInType = t0.builtInType;
	}
	if( isScalar0 && isScalar1 )
	{
		return t0;
	}
	if( isScalar0 && isVector1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			return t1;
		default:
			return t1;
		}
	}
	if( isScalar0 && isMatrix1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			return t1;
		default:
			return t1;
		}
	}
	if( isVector0 && isScalar1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			return t1;
		default:
			return t0;
		}
	}
	if( isVector0 && isVector1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			return t0;
		default:
			t0.width = 1;
			t0.height = 1;
			return t0;
		}
	}
	if( isVector0 && isMatrix1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			t1.width = t0.width;
			return t1;
		default:
			t0.width = t1.height;
			return t0;
		}
	}
	if( isMatrix0 && isScalar1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			return t1;
		default:
			return t0;
		}
	}
	if( isMatrix0 && isVector1 )
	{
		switch( argIndex )
		{
		case 0:
			return t0;
		case 1:
			t1.width = t0.height;
			return t1;
		default:
			t1.width = t0.width;
			return t1;
		}
		t1.width = t0.width;
		return t1;
	}
	t0.height = t1.height;
	return t0;
}

inline Type TransposeIntrinsicType( ASTNode* call, int argIndex )
{
	ASTNode* arg0 = call->GetChildOrNull( 0 );
	if( arg0 == nullptr )
	{
		Type type;
		type.FromTokenType( OP_VOID );
		return type;
	}
	Type t0 = arg0->GetType();
	if( argIndex == 0 )
	{
		return t0;
	}
	std::swap( t0.width, t0.height );
	return t0;
}

inline Symbol* AddFunction( SymbolTable& table, const char* name, IntrinsicType intrinsicType )
{
	InlineString str;
	str.start = name;
	str.end = name + strlen( name );
	Symbol* symbol = table.AddSymbol( MakeInlineString( name ), ALLOW_OVERRIDES );
	symbol->isFunction = true;
	symbol->intrinsicType = intrinsicType;
	return symbol;
}

template<int Arg>
int TypeSameAsArg( ASTNode* call )
{
	ASTNode* arg0 = call->GetChildOrNull( Arg );
	if( arg0 == nullptr )
	{
		return OP_VOID;
	}
	return arg0->GetType().builtInType;
}


template<int type>
int TypeIs( ASTNode* call )
{
	return type;
}

inline int CommonArgType( ASTNode* call )
{
	if( call->GetChildrenCount() == 0 )
	{
		return OP_VOID;
	}
	int type = call->GetChild( 0 )->GetType().builtInType;
	int prec = GetNumericTypePrecedence( type );
	for( unsigned i = 1; i < call->GetChildrenCount(); ++i )
	{
		int p1 = GetNumericTypePrecedence( call->GetChild( i )->GetType().builtInType );
		if( p1 > prec )
		{
			type = call->GetChild( i )->GetType().builtInType;
		}
	}
	return type;
}

template<int Arg>
void DimSameAsArg( ASTNode* call, int& width, int& height )
{
	ASTNode* arg0 = call->GetChildOrNull( Arg );
	if( arg0 == nullptr )
	{
		width = 1;
		height = 1;
	}
	else
	{
		width = arg0->GetType().width;
		height = arg0->GetType().height;
	}
}

template<int Width, int Height>
void DimIs( ASTNode* call, int& width, int& height )
{
	width = Width;
	height = Height;
}

inline void CommonArgDim( ASTNode* call, int& width, int& height )
{
	width = call->GetChild( 0 )->GetType().width;
	height = call->GetChild( 0 )->GetType().height;
	for( unsigned i = 1; i < call->GetChildrenCount(); ++i )
	{
		if( call->GetChild( i )->GetType().width > 1 || call->GetChild( i )->GetType().height > 1 )
		{
			width = min( call->GetChild( i )->GetType().width, width );
			height = min( call->GetChild( i )->GetType().height, height );
		}
	}
}

typedef int ( *TypeComponent )( ASTNode* call );
typedef void ( *DimComponent )( ASTNode* call, int& width, int& height );

template<TypeComponent ResultType, DimComponent ResultDim>
Type FunctionDescription0( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	if( argIndex >= 0 )
	{
		return type;
	}
	type.builtInType = ResultType( call );
	ResultDim( call, type.width, type.height );
	return type;
}

template<
	TypeComponent ResultType, DimComponent ResultDim,
	TypeComponent Arg0Type, DimComponent Arg0Dim
>
Type FunctionDescription1( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	switch( argIndex )
	{
	case 0:
		type.builtInType = Arg0Type( call );
		Arg0Dim( call, type.width, type.height );
		return type;
	case -1:
		type.builtInType = ResultType( call );
		ResultDim( call, type.width, type.height );
		return type;
	}
	return type;
}

template<
	TypeComponent ResultType, DimComponent ResultDim,
	TypeComponent Arg0Type, DimComponent Arg0Dim,
	TypeComponent Arg1Type, DimComponent Arg1Dim
>
Type FunctionDescription2( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	switch( argIndex )
	{
	case 0:
		type.builtInType = Arg0Type( call );
		Arg0Dim( call, type.width, type.height );
		return type;
	case 1:
		type.builtInType = Arg1Type( call );
		Arg1Dim( call, type.width, type.height );
		return type;
	case -1:
		type.builtInType = ResultType( call );
		ResultDim( call, type.width, type.height );
		return type;
	}
	return type;
}

template<
	TypeComponent ResultType, DimComponent ResultDim,
	TypeComponent Arg0Type, DimComponent Arg0Dim, int Arg0Modifier,
	TypeComponent Arg1Type, DimComponent Arg1Dim, int Arg1Modifier
>
Type FunctionDescription2Ex( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	switch( argIndex )
	{
	case 0:
		type.builtInType = Arg0Type( call );
		Arg0Dim( call, type.width, type.height );
		type.modifier = Arg0Modifier;
		return type;
	case 1:
		type.builtInType = Arg1Type( call );
		Arg1Dim( call, type.width, type.height );
		type.modifier = Arg1Modifier;
		return type;
	case -1:
		type.builtInType = ResultType( call );
		ResultDim( call, type.width, type.height );
		return type;
	}
	return type;
}

template<
	TypeComponent ResultType, DimComponent ResultDim,
	TypeComponent Arg0Type, DimComponent Arg0Dim,
	TypeComponent Arg1Type, DimComponent Arg1Dim,
	TypeComponent Arg2Type, DimComponent Arg2Dim
>
Type FunctionDescription3( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	switch( argIndex )
	{
	case 0:
		type.builtInType = Arg0Type( call );
		Arg0Dim( call, type.width, type.height );
		return type;
	case 1:
		type.builtInType = Arg1Type( call );
		Arg1Dim( call, type.width, type.height );
		return type;
	case 2:
		type.builtInType = Arg2Type( call );
		Arg2Dim( call, type.width, type.height );
		return type;
	case -1:
		type.builtInType = ResultType( call );
		ResultDim( call, type.width, type.height );
		return type;
	}
	return type;
}

template<
	TypeComponent ResultType, DimComponent ResultDim,
	TypeComponent Arg0Type, DimComponent Arg0Dim,
	TypeComponent Arg1Type, DimComponent Arg1Dim,
	TypeComponent Arg2Type, DimComponent Arg2Dim,
	TypeComponent Arg3Type, DimComponent Arg3Dim
>
Type FunctionDescription4( ASTNode* call, int argIndex )
{
	Type type;
	type.FromTokenType( OP_VOID );
	switch( argIndex )
	{
	case 0:
		type.builtInType = Arg0Type( call );
		Arg0Dim( call, type.width, type.height );
		return type;
	case 1:
		type.builtInType = Arg1Type( call );
		Arg1Dim( call, type.width, type.height );
		return type;
	case 2:
		type.builtInType = Arg2Type( call );
		Arg2Dim( call, type.width, type.height );
		return type;
	case 3:
		type.builtInType = Arg3Type( call );
		Arg3Dim( call, type.width, type.height );
		return type;
	case -1:
		type.builtInType = ResultType( call );
		ResultDim( call, type.width, type.height );
		return type;
	}
	return type;
}
