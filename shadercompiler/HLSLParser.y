//--------------------------------------------------------- ./parser.y

%include {

	#include <cstdio>
	#include <cstdlib>
	#include <cassert>
	#include <vector>
	#include <map>
	#include "HLSLParser.h"
	#include "SymbolTable.h"
	#include "ASTNode.h"
	#include "ParserUtils.h"
	#include "ParserState.h"
	#include "FXAnalyzer.h"

	#ifdef _MSC_VER
	#pragma warning(disable: 4065 4100 4189)
	#endif
}

%extra_argument {ParserState* parserState}

%token_type {ScannerToken}
%default_type {ASTNode*}

%syntax_error 
{ 
	if( TOKEN.type == 0 )
	{
		parserState->ShowMessage( EC_UNEXPECTED_EOF );
	}
	else
	{
		parserState->ShowMessage( TOKEN, EC_SYNTAX_ERROR, ToString( TOKEN.stringValue ).c_str() ); 
	}
}

%stack_overflow 
{
	parserState->ShowMessage( EC_CUSTOM_ERROR, "Compiler stack overflow :/" );
}

%stack_size 2000

in ::= translation_unit(A). {parserState->SetTree( A );}



variable_identifier(A) ::= OP_ID(B). 
{ 
	if( parserState->m_mode == ParserState::FX )
	{
		A = new ASTNode( NT_STATE_VALUE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
		Type type;
		type.FromTokenType( OP_INT );
		A->SetType( type );
	}
	else
	{
		Symbol* symbol = parserState->GetSymbolTable().Lookup( B.stringValue ); 
		if( !symbol ) 
		{ 
			parserState->ShowMessage(B.fileLocation, EC_UNDECLARED_IDENTIFIER, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
		}
		A = new ASTNode( NT_VAR_IDENTIFIER, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
		A->SetSymbol( symbol );
		if( symbol )
		{
			A->SetType( symbol->type );
		}
	}
}


%type literal_constant {ScannerToken}

literal_constant(A) ::= OP_INT_CONST(B).
{
	A = B;
}

literal_constant(A) ::= OP_FLOAT_CONST(B).
{
	A = B;
}

literal_constant(A) ::= OP_BOOL_CONST(B).
{
	A = B;
}

literal_constant(A) ::= OP_STRING_CONST(B).
{
	A = B;
}


inline_constructor(A) ::= OP_LEFT_BRACE(T) inline_initializer_list(B) OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
}

inline_constructor(A) ::= OP_LEFT_BRACE(T) inline_initializer_list(B) OP_COMA OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
}


inline_initializer_list(A) ::= constant_expression(B).
{
	A = new ASTNode( NT_INLINE_CONSTRUCTOR, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

inline_initializer_list(A) ::= inline_constructor(B).
{
	A = new ASTNode( NT_INLINE_CONSTRUCTOR, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

inline_initializer_list(A) ::= inline_initializer_list(B) OP_COMA constant_expression(C).
{
	B->AddChild( C );
	A = B;
}

inline_initializer_list(A) ::= inline_initializer_list(B) OP_COMA inline_constructor(C).
{
	B->AddChild( C );
	A = B;
}


primary_expression(A) ::= variable_identifier(B).
{
	A = B;
}

primary_expression(A) ::= literal_constant(B).
{
	A = new ASTNode( NT_CONSTANT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	Type type;
	switch( B.type )
	{
	case OP_INT_CONST:
		type.FromTokenType( OP_INT );
		break;
	case OP_FLOAT_CONST:
		type.FromTokenType( OP_FLOAT );
		break;
	case OP_BOOL_CONST:
		type.FromTokenType( OP_BOOL );
		break;
	case OP_STRING_CONST:
		type.FromTokenType( OP_STRING );
		break;
	}
	A->SetType( type );
}

primary_expression(A) ::= OP_LEFT_PAREN(C) expression(B) OP_RIGHT_PAREN.
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->SetType( B->GetType() );
}


postfix_expression(A) ::= primary_expression(B).
{
	A = B;
}

postfix_expression(A) ::= function_call(B).
{
	A = B;

	if( B->GetToken()->type == OP_ID )
	{
		std::string diagnosticMessage;
		Symbol* symbol = parserState->GetSymbolTable().LookupFunction( B->GetToken()->stringValue, B, diagnosticMessage ); 
		if( !symbol ) 
		{ 
			parserState->ShowMessage( B->GetToken()->fileLocation, EC_NO_OVERRIDE, std::string( B->GetToken()->stringValue.start, B->GetToken()->stringValue.end ).c_str(), diagnosticMessage.c_str() );
		}
		else
		{
			if( symbol->intrinsicType )
			{
				A->SetType( ( *symbol->intrinsicType )( A, -1 ) );
			}
			else
			{
				A->SetType( symbol->type );
			}
		}
		A->SetSymbol( symbol );
	}
	else
	{
		Type type;
		type.FromToken( *A->GetToken() );
		A->SetType( type );
	}
}

postfix_expression(A) ::= postfix_expression(B) OP_LEFT_BRACKET(D) integer_expression(C) OP_RIGHT_BRACKET.
{
	A = new ASTNode( NT_POSTFIX_EXPRESSION, D.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &D );
	A->AddChild( B );
	A->AddChild( C );
	Type type;
	if( !B->GetType().GetIndexedType( type ) )
	{
		parserState->ShowMessage( D, EC_INVALID_INDEXING );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}

postfix_expression(A) ::= postfix_expression(B) OP_DOT OP_ID(C).
{
	A = new ASTNode( NT_POSTFIX_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	Type type;
	Symbol* symbol;
	if( !ComputeMemberType( B->GetType(), C.stringValue, type, symbol ) )
	{
		parserState->ShowMessage( C, EC_INVALID_SUBSCRIPT, ToString( C.stringValue ).c_str() );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
	A->SetSymbol( symbol );
}

postfix_expression(A) ::= postfix_expression(B) OP_DOT(C) function_call(D).
{
	A = new ASTNode( NT_POSTFIX_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );

	Type type;
	if( B->GetType().GetMethodType( D, type ) )
	{
		A->SetType( type );
	}
	else
	{
		parserState->ShowMessage( C, EC_INVALID_SUBSCRIPT, ToString( D->GetToken()->stringValue ).c_str() );
	}
}

postfix_expression(A) ::= postfix_expression(B) OP_INC_OP(C).
{
	A = new ASTNode( NT_POSTFIX_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->SetType( B->GetType() );
}

postfix_expression(A) ::= postfix_expression(B) OP_DEC_OP(C).
{
	A = new ASTNode( NT_POSTFIX_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->SetType( B->GetType() );
}


integer_expression(A) ::= expression(B).
{
	A = B;
}


function_call(A) ::= function_call_header_with_parameters(B) OP_RIGHT_PAREN.
{
	A = B;
}

function_call(A) ::= function_call_header_no_parameters(B) OP_RIGHT_PAREN.
{
	A = B;
}


function_call_header_no_parameters(A) ::= function_call_header(B) OP_VOID.
{
	A = B;
}

function_call_header_no_parameters(A) ::= function_call_header(B).
{
	A = B;
}


function_call_header_with_parameters(A) ::= function_call_header(B) assignment_expression(C).
{
	A = B;
	A->AddChild( C );
}

function_call_header_with_parameters(A) ::= function_call_header_with_parameters(B) OP_COMA assignment_expression(C).
{
	A = B;
	A->AddChild( C );
}


function_call_header(A) ::= constructor_identifier(B) OP_LEFT_PAREN.
{
	A = new ASTNode( NT_FUNCTION_CALL, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

function_call_header(A) ::= OP_ID(B) OP_LEFT_PAREN.
{
	A = new ASTNode( NT_FUNCTION_CALL, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}


%type constructor_identifier {ScannerToken}

constructor_identifier(A) ::= OP_BOOL(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_INT(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_UINT(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_HALF(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_FLOAT(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_DOUBLE(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_STRING(B).
{
	A = B;
}

constructor_identifier(A) ::= OP_VECTOR OP_LESS constructor_identifier(B) OP_COMA OP_INT_CONST(C) OP_MORE.
{
	A = B;
	A.intValue = C.intValue;
}

constructor_identifier(A) ::= OP_MATRIX OP_LESS constructor_identifier(B) OP_COMA OP_INT_CONST(C) OP_COMA OP_INT_CONST(D) OP_MORE.
{
	A = B;
	A.intValue = D.intValue | ( C.intValue << 8 );
}


unary_expression(A) ::= postfix_expression(B).
{
	A = B;
}

unary_expression(A) ::= OP_INC_OP(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}

unary_expression(A) ::= OP_DEC_OP(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}

unary_expression(A) ::= OP_PLUS(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}

unary_expression(A) ::= OP_DASH(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}

unary_expression(A) ::= OP_BANG(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}

unary_expression(A) ::= OP_TILDE(B) unary_expression(C).
{
	A = new ASTNode( NT_PREFIX_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( C->GetType() );
}


cast_expression(A) ::= unary_expression(C).
{
	A = C;
}

cast_expression(A) ::= OP_LEFT_PAREN constructor_identifier(B) OP_RIGHT_PAREN cast_expression(C).
{
	Type type;
	type.FromToken( B );
	A = new ASTNode( NT_CAST_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( type );
}

cast_expression(A) ::= OP_LEFT_PAREN OP_TYPE_NAME(B) OP_RIGHT_PAREN cast_expression(C).
{
	const Symbol* symbol = parserState->GetSymbolTable().LookupType( B.stringValue ); 

	Type type;
	type.FromSymbol( symbol );

	A = new ASTNode( NT_CAST_EXPRESSION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
	A->SetType( type );
}


multiplicative_expression(A) ::= cast_expression(B).
{
	A = B;
}

multiplicative_expression(A) ::= multiplicative_expression(B) OP_STAR(C) cast_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}

multiplicative_expression(A) ::= multiplicative_expression(B) OP_SLASH(C) cast_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}

multiplicative_expression(A) ::= multiplicative_expression(B) OP_PERCENT(C) cast_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}


additive_expression(A) ::= multiplicative_expression(B).
{
	A = B;
}

additive_expression(A) ::= additive_expression(B) OP_PLUS(C) multiplicative_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}

additive_expression(A) ::= additive_expression(B) OP_DASH(C) multiplicative_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}


shift_expression(A) ::= additive_expression(B).
{
	A = B;
}

shift_expression(A) ::= shift_expression(B) OP_LEFT_OP(C) additive_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( parserState->m_mode == ParserState::HLSL && ( type.symbol || ( type.builtInType != OP_INT && type.builtInType != OP_UINT ) ) )
	{
		parserState->ShowMessage( C, EC_INT_TYPE_REQUIRED );
	}
	A->SetType( type );
}

shift_expression(A) ::= shift_expression(B) OP_RIGHT_OP(C) additive_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( parserState->m_mode == ParserState::HLSL && ( type.symbol || ( type.builtInType != OP_INT && type.builtInType != OP_UINT ) ) )
	{
		parserState->ShowMessage( C, EC_INT_TYPE_REQUIRED );
	}
	A->SetType( type );
}


relational_expression(A) ::= shift_expression(B).
{
	A = B;
}

relational_expression(A) ::= relational_expression(B) OP_LESS(C) shift_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}

relational_expression(A) ::= relational_expression(B) OP_MORE(C) shift_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}

relational_expression(A) ::= relational_expression(B) OP_LE_OP(C) shift_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}

relational_expression(A) ::= relational_expression(B) OP_GE_OP(C) shift_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}


equality_expression(A) ::= relational_expression(B).
{
	A = B;
}

equality_expression(A) ::= equality_expression(B) OP_EQ_OP(C) relational_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}

equality_expression(A) ::= equality_expression(B) OP_NE_OP(C) relational_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}


and_expression(A) ::= equality_expression(B).
{
	A = B;
}

and_expression(A) ::= and_expression(B) OP_AMPERSAND(C) equality_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( parserState->m_mode == ParserState::HLSL && ( type.symbol || ( type.builtInType != OP_INT && type.builtInType != OP_UINT ) ) )
	{
		parserState->ShowMessage( C, EC_INT_TYPE_REQUIRED );
	}
	A->SetType( type );
}


exclusive_or_expression(A) ::= and_expression(B).
{
	A = B;
}

exclusive_or_expression(A) ::= exclusive_or_expression(B) OP_CARET(C) and_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( parserState->m_mode == ParserState::HLSL && ( type.symbol || ( type.builtInType != OP_INT && type.builtInType != OP_UINT ) ) )
	{
		parserState->ShowMessage( C, EC_INT_TYPE_REQUIRED );
	}
	A->SetType( type );
}


inclusive_or_expression(A) ::= exclusive_or_expression(B).
{
	A = B;
}

inclusive_or_expression(A) ::= inclusive_or_expression(B) OP_VERTICAL_BAR(C) exclusive_or_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( parserState->m_mode == ParserState::HLSL && ( type.symbol || ( type.builtInType != OP_INT && type.builtInType != OP_UINT ) ) )
	{
		parserState->ShowMessage( C, EC_INT_TYPE_REQUIRED );
	}
	A->SetType( type );
}


logical_and_expression(A) ::= inclusive_or_expression(B).
{
	A = B;
}

logical_and_expression(A) ::= logical_and_expression(B) OP_AND_OP(C) inclusive_or_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}


logical_or_expression(A) ::= logical_and_expression(B).
{
	A = B;
}

logical_or_expression(A) ::= logical_or_expression(B) OP_OR_OP(C) logical_and_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	Type type;
	if( !GetCommonType( B->GetType(), D->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	else if( !type.IsScalarOrVector() )
	{
		parserState->ShowMessage( C, EC_SCALAR_VECTOR_TYPE_REQUIRED );
	}
	type.builtInType = OP_BOOL;
	A->SetType( type );
}


conditional_expression(A) ::= logical_or_expression(B).
{
	A = B;
}

conditional_expression(A) ::= logical_or_expression(B) OP_QUESTION(C) expression(D) OP_COLON assignment_expression(E).
{
	A = new ASTNode( NT_CONDITIONAL_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	A->AddChild( E );
	Type type;
	if( !GetCommonType( D->GetType(), E->GetType(), type ) )
	{
		parserState->ShowMessage( C, EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	A->SetType( type );
}


assignment_expression(A) ::= conditional_expression(B).
{
	A = B;
}

assignment_expression(A) ::= unary_expression(B) assigment_operator(C) assignment_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );

	int casts = 0;
	if( !D->GetType().CanImplicitCast( B->GetType(), casts ) )
	{
		parserState->ShowMessage( C, EC_INVALID_CAST, D->GetType().ToString().c_str(), B->GetType().ToString().c_str() );
	}
	A->SetType( B->GetType() );
}


%type assigment_operator {ScannerToken}

assigment_operator(A) ::= OP_EQUAL(B).
{
	A = B;
}

assigment_operator(A) ::= OP_MUL_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_DIV_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_MOD_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_ADD_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_SUB_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_LEFT_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_RIGHT_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_AND_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_XOR_ASSIGN(B).
{
	A = B;
}

assigment_operator(A) ::= OP_OR_ASSIGN(B).
{
	A = B;
}


expression(A) ::= assignment_expression(B).
{
	A = B;
}

expression(A) ::= expression(B) OP_COMA(C) assignment_expression(D).
{
	A = new ASTNode( NT_EXPRESSION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->AddChild( B );
	A->AddChild( D );
	A->SetType( D->GetType() );
}


constant_expression(A) ::= conditional_expression(B).
{
	A = B;
}


%type semantics {InlineString}

semantics(A) ::= OP_COLON OP_ID(B).
{
	A = B.stringValue;
}


declaration(A) ::= function_prototype(B) OP_SEMICOLON.
{
	A = B;
	parserState->GetSymbolTable().LeaveScope();
}

declaration(A) ::= init_declarator_list(B) OP_SEMICOLON.
{
	A = B;
}

declaration(A) ::= struct_named_specifier(B) OP_SEMICOLON.
{
	A = B;
}


function_prototype(A) ::= function_declarator(B) OP_RIGHT_PAREN.
{
	Symbol* symbol = parserState->GetSymbolTable().LookupFunctionDeclaration( B->GetToken()->stringValue, B );
	if( !symbol )
	{
		symbol = parserState->GetSymbolTable().AddSymbol( B->GetToken()->stringValue, ALLOW_OVERRIDES ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( B->GetToken()->fileLocation, EC_IDENTIFIER_REDEFINITION, ToString( B->GetToken()->stringValue ).c_str() );
	}
	symbol->isFunction = true;
	symbol->type = B->GetType();
	A = B;
	A->SetSymbol( symbol );
	symbol->definition = A;
}

function_prototype(A) ::= function_declarator(B) OP_RIGHT_PAREN semantics(C).
{
	Symbol* symbol = parserState->GetSymbolTable().LookupFunctionDeclaration( B->GetToken()->stringValue, B );
	if( !symbol )
	{
		symbol = parserState->GetSymbolTable().AddSymbol( B->GetToken()->stringValue, ALLOW_OVERRIDES ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( B->GetToken()->fileLocation, EC_IDENTIFIER_REDEFINITION, ToString( B->GetToken()->stringValue ).c_str() );
	}
	symbol->isFunction = true;
	symbol->type = B->GetType();
	A = B;
	A->SetSymbol( symbol );
	symbol->definition = A;

	symbol->semantic = C;
}


function_declarator(A) ::= function_header(B).
{
	A = B;
}

function_declarator(A) ::= function_header_with_parameters(B).
{
	A = B;
}


function_header_with_parameters(A) ::= function_header(B) parameter_declaration(C).
{
	A = B;
	A->AddChild( C );
}

function_header_with_parameters(A) ::= function_header_with_parameters(B) OP_COMA parameter_declaration(C).
{
	A = B;
	A->AddChild( C );
}


function_header(A) ::= fully_specified_type(B) OP_ID(C) OP_LEFT_PAREN.
{
	A = new ASTNode( NT_FUNCTION_HEADER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->SetType( B );
	parserState->GetSymbolTable().EnterScope();
}


%type interpolation_modifier {int}
%type interpolation_modifier2 {int}

interpolation_modifier(A) ::= .
{
	A = 0;
}

interpolation_modifier(A) ::= interpolation_modifier2(B).
{
	A = B;
}

interpolation_modifier2(A) ::= OP_LINEAR(B).
{
	A = B.type;
}

interpolation_modifier2(A) ::= OP_CENTROID(B).
{
	A = B.type;
}

interpolation_modifier2(A) ::= OP_NOINTERPOLATION(B).
{
	A = B.type;
}

interpolation_modifier2(A) ::= OP_NOPERSPECTIVE(B).
{
	A = B.type;
}


parameter_declarator(A) ::= fully_specified_type(B) OP_ID(C) semantics(D) interpolation_modifier(E).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_PARAMETER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}
	else
	{
		symbol->semantic = D;
		symbol->interpolationModifier = E;
		symbol->type = B;
		symbol->definition = A;
	}
	A->SetType( B );
	A->SetSymbol( symbol );
	A->AddChild( nullptr );
}

parameter_declarator(A) ::= fully_specified_type(B) OP_ID(C) interpolation_modifier(E).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_PARAMETER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}
	else
	{
		symbol->interpolationModifier = E;
		symbol->type = B;
		symbol->definition = A;
	}
	A->SetType( B );
	A->SetSymbol( symbol );
	A->AddChild( nullptr );
}

parameter_declarator(A) ::= fully_specified_type(B) OP_ID(C) bracket_list(D) semantics(E) interpolation_modifier(F).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	B.arrayDimensions = static_cast<int>( D->GetChildrenCount() );
	for( int i = 0; i < B.arrayDimensions; i++ )
	{
		B.arraySizes[i] = EvaluateIntegerExpression( *parserState, D->GetChild( i ), 0 );
	}
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_PARAMETER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}
	else
	{
		symbol->semantic = E;
		symbol->interpolationModifier = F;
		symbol->type = B;
		symbol->definition = A;
	}
	A->SetType( B );
	A->SetSymbol( symbol );
	A->AddChild( D );
}

parameter_declarator(A) ::= fully_specified_type(B) OP_ID(C) bracket_list(D) interpolation_modifier(F).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	B.arrayDimensions = static_cast<int>( D->GetChildrenCount() );
	for( int i = 0; i < B.arrayDimensions; i++ )
	{
		B.arraySizes[i] = EvaluateIntegerExpression( *parserState, D->GetChild( i ), 0 );
	}
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(C, EC_PARAMETER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}
	else
	{
		symbol->interpolationModifier = F;
		symbol->type = B;
		symbol->definition = A;
	}
	A->SetType( B );
	A->SetSymbol( symbol );
	A->AddChild( D );
}


parameter_declaration(A) ::= primitive_type(P) parameter_qualifier(C) parameter_declarator(D).
{
	D->SetToken( &C );
	A = D;
	A->AddChild( nullptr );
	A->AddChild( P );
}

parameter_declaration(A) ::= primitive_type(P) parameter_qualifier(C) parameter_type_specifier(D).
{
	D->SetToken( &C );
	A = D;
	A->AddChild( nullptr );
	A->AddChild( P );
}


parameter_declaration(A) ::= primitive_type(P) parameter_qualifier(C) parameter_declarator(D) OP_EQUAL initializer(E).
{
	D->SetToken( &C );
	A = D;
	A->AddChild( E );
	A->AddChild( P );
}

parameter_declaration(A) ::= primitive_type(P) parameter_qualifier(C) parameter_type_specifier(D) OP_EQUAL initializer(E).
{
	D->SetToken( &C );
	A = D;
	A->AddChild( E );
	A->AddChild( P );
}


primitive_type(A) ::= .
{
	A = nullptr;
}

primitive_type(A) ::= OP_POINT(B).
{
	A = new ASTNode( NT_PRIMITIVE_TYPE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

primitive_type(A) ::= OP_LINE(B).
{
	A = new ASTNode( NT_PRIMITIVE_TYPE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

primitive_type(A) ::= OP_TRIANGLE(B).
{
	A = new ASTNode( NT_PRIMITIVE_TYPE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

primitive_type(A) ::= OP_LINEADJ(B).
{
	A = new ASTNode( NT_PRIMITIVE_TYPE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

primitive_type(A) ::= OP_TRIANGLEADJ(B).
{
	A = new ASTNode( NT_PRIMITIVE_TYPE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}


%type parameter_qualifier {ScannerToken}

parameter_qualifier(A) ::= .
{
	A.type = 0;
}

parameter_qualifier(A) ::= OP_IN(B).
{
	A = B;
}

parameter_qualifier(A) ::= OP_OUT(B).
{
	A = B;
}

parameter_qualifier(A) ::= OP_INOUT(B).
{
	A = B;
}


parameter_type_specifier(A) ::= fully_specified_type(B).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, parserState->GetCurrentLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->SetType( B );
	A->AddChild( nullptr );
}

parameter_type_specifier(A) ::= fully_specified_type(B) bracket_list(C).
{
	A = new ASTNode( NT_FUNCTION_PARAMETER, C->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	B.arrayDimensions = static_cast<int>( C->GetChildrenCount() );
	for( int i = 0; i < B.arrayDimensions; i++ )
	{
		B.arraySizes[i] = EvaluateIntegerExpression( *parserState, C->GetChild( i ), 0 );
	}
	A->SetType( B );
	A->AddChild( C );
}


%type pack_offset {PackOffset}

pack_offset(A) ::= OP_COLON OP_PACKOFFSET OP_LEFT_PAREN OP_ID(B) OP_RIGHT_PAREN.
{
	A.Create( *parserState, B, nullptr );
}

pack_offset(A) ::= OP_COLON OP_PACKOFFSET OP_LEFT_PAREN OP_ID(B) OP_DOT OP_ID(C) OP_RIGHT_PAREN.
{
	A.Create( *parserState, B, &C );
}


%type register_specifier {RegisterSpecifier}

register_specifier(A) ::= OP_COLON OP_REGISTER OP_LEFT_PAREN OP_ID(B) OP_RIGHT_PAREN.
{
	A.shaderProfile = MakeInlineString( "" );
	A.subComponent = -1;
	A.space = -1;
	A.explicitSpace = false;
	A.explicitRegister = true;
	if( !ParseRegisterID( B.stringValue.start, B.stringValue.end, A.registerType, A.registerNumber ) )
	{
		parserState->ShowMessage( B, EC_INVALID_REGISTER );
	}
}

register_specifier(A) ::= OP_COLON OP_REGISTER OP_LEFT_PAREN OP_ID(B) OP_LEFT_BRACKET OP_INT_CONST(D) OP_RIGHT_BRACKET OP_RIGHT_PAREN.
{
	A.shaderProfile = MakeInlineString( "" );
	A.subComponent = -1;
	A.space = -1;
	A.explicitSpace = false;
	A.explicitRegister = true;
	if( !ParseRegisterID( B.stringValue.start, B.stringValue.end, A.registerType, A.registerNumber ) )
	{
		parserState->ShowMessage( B, EC_INVALID_REGISTER );
	}
	A.subComponent = ParseNumber( D.stringValue.start, D.stringValue.end );
}

register_specifier(A) ::= OP_COLON OP_REGISTER OP_LEFT_PAREN OP_ID(B) OP_COMA OP_ID(T) OP_RIGHT_PAREN.
{
	A.explicitRegister = true;
	A.subComponent = -1;
	auto space = ToString( T.stringValue );
	if( space.substr( 0, 5 ) == "space" )
	{
		A.shaderProfile = MakeInlineString( "" );
		A.space = atoi( space.substr( 5 ).c_str() );
		A.explicitSpace = true;
		if( !ParseRegisterID( B.stringValue.start, B.stringValue.end, A.registerType, A.registerNumber ) )
		{
			parserState->ShowMessage( B, EC_INVALID_REGISTER );
		}
	}
	else
	{
		A.shaderProfile = B.stringValue;
		A.space = -1;
		A.explicitSpace = false;
		if( !ParseRegisterID( T.stringValue.start, T.stringValue.end, A.registerType, A.registerNumber ) )
		{
			parserState->ShowMessage( T, EC_INVALID_REGISTER );
		}
	}
}

register_specifier(A) ::= OP_COLON OP_REGISTER OP_LEFT_PAREN OP_ID(B) OP_COMA OP_ID(T) OP_LEFT_BRACKET OP_INT_CONST(D) OP_RIGHT_BRACKET OP_RIGHT_PAREN.
{
	A.explicitSpace = false;
	A.explicitRegister = true;
	A.shaderProfile = MakeInlineString( "" );
	A.subComponent = -1;
	A.space = -1;
	A.explicitSpace = false;
	if( !ParseRegisterID( T.stringValue.start, T.stringValue.end, A.registerType, A.registerNumber ) )
	{
		parserState->ShowMessage( T, EC_INVALID_REGISTER );
	}
	A.shaderProfile = B.stringValue;
	A.subComponent = ParseNumber( D.stringValue.start, D.stringValue.end );
}


%type annotation_declaration {SymbolAnnotations*}

annotation_declaration(A) ::= OP_LESS annotation_list(B) OP_MORE.
{
	A = B;
}

annotation_declaration(A) ::= OP_LESS OP_MORE.
{
	A = nullptr;
}


%type annotation_list {SymbolAnnotations*}

annotation_list(A) ::= single_annotation(B).
{
	A = new SymbolAnnotations;
	A->push_back( B );
}

annotation_list(A) ::= annotation_list(B) single_annotation(C).
{
	B->push_back( C );
	A = B;
}
 

%type single_annotation {SymbolAnnotation}

single_annotation(A) ::= constructor_identifier(B) OP_ID(C) OP_EQUAL literal_constant(D) OP_SEMICOLON.
{
	switch( B.type )
	{
	case OP_BOOL:
		if( D.type != OP_BOOL_CONST )
		{
			parserState->ShowMessage( D, EC_ANNOTATION_TYPE_MISMATCH, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
		}
		break;
	case OP_INT:
	case OP_UINT:
		if( D.type != OP_INT_CONST )
		{
			parserState->ShowMessage( D, EC_ANNOTATION_TYPE_MISMATCH, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
		}
		break;
	case OP_HALF:
	case OP_FLOAT:
	case OP_DOUBLE:
		if( D.type != OP_FLOAT_CONST )
		{
			parserState->ShowMessage( D, EC_ANNOTATION_TYPE_MISMATCH, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
		}
		break;
	case OP_STRING:
		if( D.type != OP_STRING_CONST )
		{
			parserState->ShowMessage( D, EC_ANNOTATION_TYPE_MISMATCH, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
		}
		break;
	}
	A.type = B.type;
	A.name = C.stringValue;
	A.value = D;
}


%type name_decoration {Symbol*}

name_decoration(A) ::= semantics(B).
{
	A = new Symbol;
	A->semantic = B;
}

name_decoration(A) ::= pack_offset(B).
{
	A = new Symbol;
	A->packOffset = B;
}

name_decoration(A) ::= register_specifier(B).
{
	A = new Symbol;
	A->registerSpecifier[B.shaderProfile] = B;
}


%type name_decoration_list {Symbol*}

name_decoration_list(A) ::= name_decoration(B).
{
	A = B;
}

name_decoration_list(A) ::= name_decoration_list(B) name_decoration(C).
{
	if( C->semantic.start )
	{
		B->semantic = C->semantic;
	}
	else if( C->packOffset.subComponent >= 0 )
	{
		B->packOffset = C->packOffset;
	}
	else if( !C->registerSpecifier.empty() )
	{
		RegisterSpecifier& reg = C->registerSpecifier.begin()->second;
		if( !B->registerSpecifier.empty() )
		{
			auto found = B->registerSpecifier.find( reg.shaderProfile );
			if( found == B->registerSpecifier.end() )
			{
				found = B->registerSpecifier.find( MakeInlineString( "" ) );
			}
			if( found != B->registerSpecifier.end() )
			{
				parserState->ShowMessage( EC_MULTIPLE_REGISTER_BINDINGS );
			}
		}
		B->registerSpecifier[reg.shaderProfile] = reg;
	}
	delete C;
	A = B;
}


%type name_post_declaration {Symbol*}

name_post_declaration(A) ::= .
{
	A = nullptr;
}

name_post_declaration(A) ::= name_decoration_list(B).
{
	A = B;
}

name_post_declaration(A) ::= annotation_declaration(B).
{
	A = new Symbol;
	A->annotations = B;
}

name_post_declaration(A) ::= name_decoration_list(B) annotation_declaration(C).
{
	A = B;
	A->annotations = C;
}


name_declaration(A) ::= OP_ID(C) name_post_declaration(B).
{
	Symbol* symbol = nullptr;
	if( B )
	{
		B->name = C.stringValue;
		symbol = parserState->GetSymbolTable().AddSymbol( B ); 
	}
	else
	{
		symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_IDENTIFIER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C );
	A->SetSymbol( symbol );
}

bracket_expression(A) ::= OP_LEFT_BRACKET OP_RIGHT_BRACKET.
{
	A = nullptr;
}

bracket_expression(A) ::= OP_LEFT_BRACKET constant_expression(B) OP_RIGHT_BRACKET.
{
	A = B;
}

bracket_list(A) ::= bracket_expression(B).
{
	A = new ASTNode( NT_BRACKET_LIST, B ? B->GetLocation() : FileLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

bracket_list(A) ::= bracket_list(B) bracket_expression(C).
{
	A = B;
	A->AddChild( C );
}

name_declaration(A) ::= OP_ID(C) bracket_list(D) name_post_declaration(B).
{
	Symbol* symbol = nullptr;
	if( B )
	{
		B->name = C.stringValue;
		symbol = parserState->GetSymbolTable().AddSymbol( B ); 
	}
	else
	{
		symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_IDENTIFIER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( D );
	A->AddChild( nullptr );
	A->SetSymbol( symbol );
}

name_declaration(A) ::= OP_ID(C) bracket_list(D) name_post_declaration(B) OP_EQUAL initializer(E).
{
	Symbol* symbol = nullptr;
	if( B )
	{
		B->name = C.stringValue;
		symbol = parserState->GetSymbolTable().AddSymbol( B ); 
	}
	else
	{
		symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_IDENTIFIER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( D );
	A->AddChild( E );
	A->SetSymbol( symbol );
}

name_declaration(A) ::= OP_ID(C) name_post_declaration(B) OP_EQUAL initializer(D).
{
	Symbol* symbol = nullptr;
	if( B )
	{
		B->name = C.stringValue;
		symbol = parserState->GetSymbolTable().AddSymbol( B ); 
	}
	else
	{
		symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_IDENTIFIER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( nullptr );
	A->AddChild( D );
	A->SetSymbol( symbol );
}

name_declaration(A) ::= OP_ID(C) name_post_declaration(B) sampler_initializer_10(D).
{
	Symbol* symbol = nullptr;
	if( B )
	{
		B->name = C.stringValue;
		symbol = parserState->GetSymbolTable().AddSymbol( B ); 
	}
	else
	{
		symbol = parserState->GetSymbolTable().AddSymbol( C.stringValue ); 
	}
	if( !symbol ) 
	{ 
		parserState->ShowMessage( C, EC_IDENTIFIER_REDEFINITION, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( nullptr );
	A->AddChild( D );
	A->SetSymbol( symbol );
}


init_declarator_list(A) ::= single_declaration(B).
{
	A = B;
}

init_declarator_list(A) ::= init_declarator_list(B) OP_COMA name_declaration(C).
{
	Type type = B->GetType();
	auto child = C->GetChildOrNull( 0 );
	if( child )
	{
		type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
		for( int i = 0; i < type.arrayDimensions; i++ )
		{
			type.arraySizes[i] = EvaluateIntegerExpression( *parserState, child->GetChild( i ), 0 );
		}
	}
	Symbol* symbol = C->GetSymbol();
	if( symbol )
	{
		symbol->type = type;
		symbol->definition = C;
	}
	C->SetType( type );
	B->AddChild( C );
	A = B;
}


single_declaration(A) ::= fully_specified_type(B) name_declaration(C).
{
	A = new ASTNode( NT_VAR_DECLARATION_LIST, C->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( C );
	Type type = B;
	auto child = C->GetChildOrNull( 0 );
	if( child )
	{
		type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
		for( int i = 0; i < type.arrayDimensions; i++ )
		{
			type.arraySizes[i] = EvaluateIntegerExpression( *parserState, child->GetChild( i ), 0 );
		}
	}
	C->SetType( type );

	Symbol* symbol = C->GetSymbol();
	if( symbol )
	{
		symbol->type = type;
		symbol->definition = C;
	}
	A->SetType( B );
}

%type fully_specified_type {Type}

fully_specified_type(A) ::= type_specifier(B).
{
	A = B;
}

fully_specified_type(A) ::= type_modifier(B) type_specifier(C).
{
	C.modifier = B.type;
	A = C;
}

fully_specified_type(A) ::= storage_class(B) type_specifier(D).
{
	D.storageClass = B.type;
	A = D;
}

fully_specified_type(A) ::= storage_class(B) type_modifier(C) type_specifier(D).
{
	D.storageClass = B.type;
	D.modifier = C.type;
	A = D;
}


%type storage_class {ScannerToken}

storage_class(A) ::= OP_EXTERN(B).
{
	A = B;
}

storage_class(A) ::= OP_NOINTERPOLATION(B).
{
	A = B;
}

storage_class(A) ::= OP_PRECISE(B).
{
	A = B;
}

storage_class(A) ::= OP_SHARED(B).
{
	A = B;
}

storage_class(A) ::= OP_GROUPSHARED(B).
{
	A = B;
}

storage_class(A) ::= OP_STATIC(B).
{
	A = B;
}

storage_class(A) ::= OP_UNIFORM(B).
{
	A = B;
}

storage_class(A) ::= OP_VOLATILE(B).
{
	A = B;
}


%type type_modifier {ScannerToken}

type_modifier(A) ::= OP_CONST(B).
{
	A = B;
}

type_modifier(A) ::= OP_ROW_MAJOR(B).
{
	A = B;
}

type_modifier(A) ::= OP_COLUMN_MAJOR(B).
{
	A = B;
}


%type type_specifier {Type}

type_specifier(A) ::= OP_VOID(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= constructor_identifier(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLER(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLERCOMPARISON(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLER1D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLER2D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLER3D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_SAMPLERCUBE(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE1D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE1D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE1DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE1DARRAY(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE2D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE2D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE2DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE2DARRAY(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE3D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE3D(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE3DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURE3DARRAY(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURECUBE(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURECUBE(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURECUBEARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TEXTURECUBEARRAY(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_TEXTURE2DMS(B) OP_LESS type_specifier(C) OP_COMA OP_INT_CONST(D) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = ParseNumber( D.stringValue.start, D.stringValue.end );
}

type_specifier(A) ::= OP_TEXTURE2DMS(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = -1;
}

type_specifier(A) ::= OP_TEXTURE2DMSARRAY(B) OP_LESS type_specifier(C) OP_COMA OP_INT_CONST(D) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = ParseNumber( D.stringValue.start, D.stringValue.end );
}

type_specifier(A) ::= OP_TEXTURE2DMSARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = -1;
}

type_specifier(A) ::= OP_BUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_APPENDSTRUCTUREDBUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_BYTEADDRESSBUFFER(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_CONSUMESTRUCTUREDBUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_INPUTPATCH(B) OP_LESS type_specifier(C) OP_COMA OP_INT_CONST(D) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = ParseNumber( D.stringValue.start, D.stringValue.end );
}

type_specifier(A) ::= OP_OUTPUTPATCH(B) OP_LESS type_specifier(C) OP_COMA OP_INT_CONST(D) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
	A.templateSamples = ParseNumber( D.stringValue.start, D.stringValue.end );
}

type_specifier(A) ::= OP_RWBUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWBYTEADDRESSBUFFER(B).
{
	A.FromToken( B );
}

type_specifier(A) ::= OP_RWSTRUCTUREDBUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE1D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE2D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE3D(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE1DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE2DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_RWTEXTURE3DARRAY(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_STRUCTUREDBUFFER(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_POINTSTREAM(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_LINESTREAM(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TRIANGLESTREAM(B) OP_LESS type_specifier(C) OP_MORE.
{
	A.FromToken( B );
	A.templateParameter = new Type( C );
}

type_specifier(A) ::= OP_TYPE_NAME(B).
{
	const Symbol* symbol = parserState->GetSymbolTable().LookupType( B.stringValue ); 
	A.FromSymbol( symbol );
}

type_specifier(A) ::= struct_specifier(B).
{
	parserState->AddOfflineStatement( B );
	A = B->GetType();
}

type_specifier(A) ::= struct_named_specifier(B).
{
	parserState->AddOfflineStatement( B );
	A = B->GetType();
}


struct_named_specifier(A) ::= OP_STRUCT enter_block OP_ID(B) OP_LEFT_BRACE struct_declaration_list(C) OP_RIGHT_BRACE.
{
	parserState->GetSymbolTable().LeaveScope();

	A = C;

	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( B.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_IDENTIFIER_REDEFINITION, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
	}
	else
	{
		symbol->isTypeName = true;
		symbol->definition = C;
		
		Type type;
		type.FromSymbol( symbol );

		A->SetSymbol( symbol );
		A->SetType( type );
	}
}


struct_specifier(A) ::= OP_STRUCT enter_block OP_LEFT_BRACE struct_declaration_list(C) OP_RIGHT_BRACE.
{
	parserState->GetSymbolTable().LeaveScope();

	A = C;

	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( parserState->AllocateName() );
	symbol->definition = C;
	symbol->isTypeName = true;

	Type type;
	type.FromSymbol( symbol );

	A->SetSymbol( symbol );
	A->SetType( type );
}


struct_declaration_list(A) ::= struct_declaration(B).
{
	A = new ASTNode( NT_STRUCT, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

struct_declaration_list(A) ::= struct_declaration_list(B) struct_declaration(C).
{
	B->AddChild(C);
	A = B;
}

struct_declaration(A) ::= type_specifier(B) struct_declarator_list(C) OP_SEMICOLON.
{
	for( unsigned i = 0; i < C->GetChildrenCount(); ++i )
	{
		Type type = B;
		auto child = C->GetChild( i )->GetChildOrNull( 0 );
		if( child )
		{
			type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
			for( int j = 0; j < type.arrayDimensions; j++ )
			{
				type.arraySizes[j] = EvaluateIntegerExpression( *parserState, child->GetChild( j ), 0 );
			}
		}
		C->GetChild( i )->SetType( type );
		Symbol* symbol = C->GetChild( i )->GetSymbol();
		if( symbol )
		{
			symbol->type = type;
		}
	}
	A = C;
	A->SetType( B );
}

struct_declaration(A) ::= type_modifier(D) type_specifier(B) struct_declarator_list(C) OP_SEMICOLON.
{
	B.modifier = D.type;
	for( unsigned i = 0; i < C->GetChildrenCount(); ++i )
	{
		Type type = B;
		auto child = C->GetChild( i )->GetChildOrNull( 0 );
		if( child )
		{
			type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
			for( int j = 0; j < type.arrayDimensions; j++ )
			{
				type.arraySizes[j] = EvaluateIntegerExpression( *parserState, child->GetChild( j ), 0 );
			}
		}
		C->GetChild( i )->SetType( type );
		Symbol* symbol = C->GetChild( i )->GetSymbol();
		if( symbol )
		{
			symbol->type = type;
		}
	}
	A = C;
	A->SetType( B );
}

struct_declaration(A) ::= interpolation_modifier2(E) type_modifier(D) type_specifier(B) struct_declarator_list(C) OP_SEMICOLON.
{
	B.modifier = D.type;
	for( unsigned i = 0; i < C->GetChildrenCount(); ++i )
	{
		Type type = B;
		auto child = C->GetChild( i )->GetChildOrNull( 0 );
		if( child )
		{
			type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
			for( int j = 0; j < type.arrayDimensions; j++ )
			{
				type.arraySizes[j] = EvaluateIntegerExpression( *parserState, child->GetChild( j ), 0 );
			}
		}
		C->GetChild( i )->SetType( type );
		Symbol* symbol = C->GetChild( i )->GetSymbol();
		if( symbol )
		{
			symbol->type = type;
			symbol->interpolationModifier = E;
		}
	}
	A = C;
	A->SetType( B );
}

struct_declaration(A) ::= interpolation_modifier2(E) type_specifier(B) struct_declarator_list(C) OP_SEMICOLON.
{
	for( unsigned i = 0; i < C->GetChildrenCount(); ++i )
	{
		Type type = B;
		auto child = C->GetChild( i )->GetChildOrNull( 0 );
		if( child )
		{
			type.arrayDimensions = static_cast<int>( child->GetChildrenCount() );
			for( int j = 0; j < type.arrayDimensions; j++ )
			{
				type.arraySizes[j] = EvaluateIntegerExpression( *parserState, child->GetChild( j ), 0 );
			}
		}
		C->GetChild( i )->SetType( type );
		Symbol* symbol = C->GetChild( i )->GetSymbol();
		if( symbol )
		{
			symbol->type = type;
			symbol->interpolationModifier = E;
		}
	}
	A = C;
	A->SetType( B );
}

struct_declarator_list(A) ::= struct_declarator(B).
{
	A = new ASTNode( NT_STRUCT_MEMBER, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

struct_declarator_list(A) ::= struct_declarator_list(B) OP_COMA struct_declarator(C).
{
	B->AddChild( C );
	A = B;
}

struct_declarator(A) ::= OP_ID(B).
{
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( B.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_IDENTIFIER_REDEFINITION, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	symbol->definition = A;
	A->SetSymbol( symbol );
}

struct_declarator(A) ::= OP_ID(B) semantics(C).
{
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( B.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_IDENTIFIER_REDEFINITION, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
	}
	symbol->semantic = C;

	A = new ASTNode( NT_NAME_DECLARATION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	symbol->definition = A;
	A->SetSymbol( symbol );
}

struct_declarator(A) ::= OP_ID(B) bracket_list(C).
{
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( B.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_IDENTIFIER_REDEFINITION, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
	}

	A = new ASTNode( NT_NAME_DECLARATION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	symbol->definition = A;
	A->AddChild( C );
	A->SetSymbol( symbol );
}

struct_declarator(A) ::= OP_ID(B) bracket_list(C) semantics(D).
{
	Symbol* symbol = parserState->GetSymbolTable().AddSymbol( B.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_IDENTIFIER_REDEFINITION, std::string( B.stringValue.start, B.stringValue.end ).c_str() );
	}
	else
	{
		symbol->semantic = D;
	}
	A = new ASTNode( NT_NAME_DECLARATION, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	symbol->definition = A;
	A->AddChild( C );
	A->SetSymbol( symbol );
}


initializer(A) ::= assignment_expression(B).
{
	A = B;
}

initializer(A) ::= sampler_initializer(B).
{
	A = B;
}

initializer(A) ::= inline_constructor(B).
{
	A = B;
}

enter_fx_mode ::= .
{
	parserState->m_mode = ParserState::FX;
}

sampler_initializer(A) ::= OP_SAMPLER_STATE enter_fx_mode OP_LEFT_BRACE sampler_state_list(B) OP_RIGHT_BRACE.
{
	A = B;
	parserState->m_mode = ParserState::HLSL;
}


sampler_initializer_10(A) ::= enter_fx_mode OP_LEFT_BRACE sampler_state_list(B) OP_RIGHT_BRACE.
{
	A = B;
	parserState->m_mode = ParserState::HLSL;
}


sampler_state_list(A) ::= state_assignment(B).
{
	A = new ASTNode( NT_SAMPLER_STATE_LIST, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

sampler_state_list(A) ::= sampler_state_list(B) state_assignment(C).
{
	B->AddChild(C);
	A = B;
}

state_assignment(A) ::= OP_ID(B) OP_EQUAL constant_expression(C) OP_SEMICOLON.
{
	A = new ASTNode( NT_STATE_ASSIGNMENT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
}

state_assignment(A) ::= OP_ID(B) OP_EQUAL inline_constructor(C) OP_SEMICOLON.
{
	A = new ASTNode( NT_STATE_ASSIGNMENT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
}

state_assignment(A) ::= OP_ID(B) OP_EQUAL OP_LESS OP_ID(C) OP_MORE OP_SEMICOLON.
{
	Symbol* symbol = parserState->GetSymbolTable().Lookup( C.stringValue ); 
	if( !symbol ) 
	{ 
		parserState->ShowMessage(B, EC_UNDECLARED_IDENTIFIER, std::string( C.stringValue.start, C.stringValue.end ).c_str() );
	} 
	ASTNode* child = new ASTNode( NT_VAR_IDENTIFIER, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	child->SetSymbol( symbol );
	A = new ASTNode( NT_STATE_ASSIGNMENT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( child );
}


statement_no_new_scope(A) ::= compount_statement_with_scope(B).
{
	A = B;
}

statement_no_new_scope(A) ::= simple_statement(B).
{
	A = B;
}


simple_statement(A) ::= declaration(B).
{
	A = B;
}

simple_statement(A) ::= expression_statement(B).
{
	A = B;
}

simple_statement(A) ::= selection_statement(B).
{
	A = B;
}

simple_statement(A) ::= iteration_statement(B).
{
	A = B;
}

simple_statement(A) ::= switch_statement(B).
{
	A = B;
}

simple_statement(A) ::= jump_statement(B).
{
	A = B;
}


compount_statement_with_scope(A) ::= OP_LEFT_BRACE(B) OP_RIGHT_BRACE.
{
	A = new ASTNode( NT_BLOCK, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
}

compount_statement_with_scope(A) ::= OP_LEFT_BRACE enter_block statement_list(C) OP_RIGHT_BRACE.
{
	A = C;
	parserState->GetSymbolTable().LeaveScope(); 
}


enter_block ::= .
{
	parserState->GetSymbolTable().EnterScope(); 
}


compount_statement_no_new_scope(A) ::= OP_LEFT_BRACE(B) OP_RIGHT_BRACE.
{
	A = new ASTNode( NT_BLOCK, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
}

compount_statement_no_new_scope(A) ::= OP_LEFT_BRACE statement_list(C) OP_RIGHT_BRACE.
{
	A = C;
}


statement_list(A) ::= statement_no_new_scope(B).
{
	A = new ASTNode( NT_BLOCK, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	parserState->MoveOfflineStatements( A );
	A->AddChild( B );
}

statement_list(A) ::= statement_list(B) statement_no_new_scope(C).
{
	parserState->MoveOfflineStatements( B );
	B->AddChild(C);
	A = B;
}


expression_statement(A) ::= OP_SEMICOLON(B).
{
	A = new ASTNode( NT_EXPRESSION_STATEMENT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
}

expression_statement(A) ::= expression(B) OP_SEMICOLON(C).
{
	A = new ASTNode( NT_EXPRESSION_STATEMENT, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}


%type statement_attribute {ScannerToken}
statement_attribute(A) ::= OP_LEFT_BRACKET OP_ID(B) OP_RIGHT_BRACKET.
{
	A = B;
}


selection_statement(A) ::= statement_attribute(T) OP_IF(D) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN selection_rest_statement(C).
{
	A = C;
	A->SetLocation( D.fileLocation );
	A->SetToken( &T );
	A->InsertChild( 0, B );
}

selection_statement(A) ::= OP_IF(D) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN selection_rest_statement(C).
{
	A = C;
	A->SetLocation( D.fileLocation );
	A->InsertChild( 0, B );
}


selection_rest_statement(A) ::= statement_no_new_scope(B) OP_ELSE(D) statement_no_new_scope(C).
{
	A = new ASTNode( NT_IF, D.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
}

selection_rest_statement(A) ::= statement_no_new_scope(B).
{
	A = new ASTNode( NT_IF, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}


iteration_statement(A) ::= statement_attribute(T) OP_WHILE(D) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN statement_no_new_scope(C).
{
	A = new ASTNode( NT_WHILE, D.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &T );
	A->AddChild( B );
	A->AddChild( C );
}

iteration_statement(A) ::= OP_WHILE(D) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN statement_no_new_scope(C).
{
	A = new ASTNode( NT_WHILE, D.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
}

iteration_statement(A) ::= OP_DO(D) statement_no_new_scope(B) OP_WHILE OP_LEFT_PAREN expression(C) OP_RIGHT_PAREN.
{
	A = new ASTNode( NT_DO, D.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( C );
	A->AddChild( B );
}

iteration_statement(A) ::= OP_FOR(F) enter_block OP_LEFT_PAREN for_init_statement(B) for_rest_statement(C) OP_RIGHT_PAREN statement_no_new_scope(D).
{
	A = C;
	A->InsertChild( 0, B );
	A->AddChild( D );
	A->SetLocation( F.fileLocation );
	parserState->GetSymbolTable().LeaveScope(); 
}

iteration_statement(A) ::= statement_attribute(T) OP_FOR(F) enter_block OP_LEFT_PAREN for_init_statement(B) for_rest_statement(C) OP_RIGHT_PAREN statement_no_new_scope(D).
{
	A = C;
	A->SetToken( &T );
	A->InsertChild( 0, B );
	A->AddChild( D );
	A->SetLocation( F.fileLocation );
	parserState->GetSymbolTable().LeaveScope(); 
}


for_init_statement(A) ::= expression_statement(B).
{
	A = B;
}

for_init_statement(A) ::= declaration(B).
{
	A = B;
}


conditionopt(A) ::= expression(B).
{
	A = B;
}

conditionopt(A) ::= .
{
	A = nullptr;
}


for_rest_statement(A) ::= conditionopt(B) OP_SEMICOLON.
{
	A = new ASTNode( NT_FOR, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( nullptr );
}

for_rest_statement(A) ::= conditionopt(B) OP_SEMICOLON expression(C).
{
	A = new ASTNode( NT_FOR, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
}


switch_statement(A) ::= statement_attribute(T) OP_SWITCH(S) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN OP_LEFT_BRACE case_statements(C) OP_RIGHT_BRACE.
{
	C->InsertChild( 0, B );
	C->SetToken( &T );
	A = C;
	A->SetLocation( S.fileLocation );
}

switch_statement(A) ::= OP_SWITCH(S) OP_LEFT_PAREN expression(B) OP_RIGHT_PAREN OP_LEFT_BRACE case_statements(C) OP_RIGHT_BRACE.
{
	C->InsertChild( 0, B );
	A = C;
	A->SetLocation( S.fileLocation );
}


case_statements(A) ::= case_statement(B).
{
	A = new ASTNode( NT_SWITCH, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

case_statements(A) ::= case_statements(B) case_statement(C).
{
	A = B;
	A->AddChild( C );
}


case_statement(A) ::= case_label(B) statement_list(C).
{
	B->AddChild( C );
	A = B;
}

case_statement(A) ::= case_label(B).
{
	A = B;
}


case_label(A) ::= OP_CASE(C) constant_expression(B) OP_COLON.
{
	A = new ASTNode( NT_CASE, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

case_label(A) ::= OP_DEFAULT(B) OP_COLON.
{
	A = new ASTNode( NT_CASE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( nullptr );
}



jump_statement(A) ::= OP_CONTINUE(B) OP_SEMICOLON.
{
	A = new ASTNode( NT_JUMP, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

jump_statement(A) ::= OP_BREAK(B) OP_SEMICOLON.
{
	A = new ASTNode( NT_JUMP, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

jump_statement(A) ::= OP_RETURN(B) OP_SEMICOLON.
{
	A = new ASTNode( NT_JUMP, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

jump_statement(A) ::= OP_DISCARD(B) OP_SEMICOLON.
{
	A = new ASTNode( NT_JUMP, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

jump_statement(A) ::= OP_RETURN(B) expression(C) OP_SEMICOLON.
{
	A = new ASTNode( NT_JUMP, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( C );
}


external_declaration(A) ::= function_definition(B).
{
	A = B;
}

external_declaration(A) ::= declaration(B).
{
	A = B;
}

external_declaration(A) ::= technique(B).
{
	A = B;
}

external_declaration(A) ::= cbuffer_declaration(B).
{
	A = B;
}

external_declaration(A) ::= OP_SEMICOLON.
{
	A = nullptr;
}


function_attribute_list(A) ::= function_attribute(B).
{
	A = new ASTNode( NT_FUNCTION_ATTRIBUTE_LIST, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

function_attribute_list(A) ::= function_attribute_list(B) function_attribute(C).
{
	A = B;
	A->AddChild( C );
}


function_attribute(A) ::= OP_LEFT_BRACKET OP_ID(B) OP_LEFT_PAREN function_atrribute_value_list(C) OP_RIGHT_PAREN OP_RIGHT_BRACKET.
{
	A = C;
	A->SetToken( &B );
}

function_attribute(A) ::= OP_LEFT_BRACKET OP_ID(B) OP_RIGHT_BRACKET.
{
	A = new ASTNode( NT_FUNCTION_ATTRIBUTE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}


function_atrribute_value_list(A) ::= literal_constant(B).
{
	A = new ASTNode( NT_FUNCTION_ATTRIBUTE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( new ASTNode( NT_FUNCTION_ATTRIBUTE_VALUE, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B ) );
}

function_atrribute_value_list(A) ::= function_atrribute_value_list(B) OP_COMA literal_constant(C).
{
	A = B;
	A->AddChild( new ASTNode( NT_FUNCTION_ATTRIBUTE_VALUE, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C ) );
}


function_definition(A) ::= function_attribute_list(L) function_prototype(B) annotation_declaration(N) compount_statement_no_new_scope(C).
{
	A = new ASTNode( NT_FUNCTION_DEFINITION, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
	A->AddChild( L );
	B->GetSymbol()->definition = A;
	B->GetSymbol()->annotations = N;
	parserState->GetSymbolTable().LeaveScope(); 
}

function_definition(A) ::= function_attribute_list(L) function_prototype(B) compount_statement_no_new_scope(C).
{
	A = new ASTNode( NT_FUNCTION_DEFINITION, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
	A->AddChild( L );
	B->GetSymbol()->definition = A;
	parserState->GetSymbolTable().LeaveScope(); 
}

function_definition(A) ::= function_prototype(B) annotation_declaration(N) compount_statement_no_new_scope(C).
{
	A = new ASTNode( NT_FUNCTION_DEFINITION, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
	B->GetSymbol()->definition = A;
	B->GetSymbol()->annotations = N;
	parserState->GetSymbolTable().LeaveScope(); 
}

function_definition(A) ::= function_prototype(B) compount_statement_no_new_scope(C).
{
	A = new ASTNode( NT_FUNCTION_DEFINITION, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
	A->AddChild( C );
	B->GetSymbol()->definition = A;
	parserState->GetSymbolTable().LeaveScope(); 
}


technique(A) ::= OP_TECHNIQUE(T) OP_ID(B) OP_LEFT_BRACE OP_RIGHT_BRACE.
{
	A = new ASTNode( NT_TECHNIQUE, T.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
}

technique(A) ::= OP_TECHNIQUE(T) OP_ID(B) OP_LEFT_BRACE pass_list(C) OP_RIGHT_BRACE.
{
	A = C;
	A->SetToken( &B );
	A->SetLocation( T.fileLocation );
}


pass_list(A) ::= pass_declaration(B).
{
	A = new ASTNode( NT_TECHNIQUE, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

pass_list(A) ::= pass_list(B) pass_declaration(C).
{
	B->AddChild( C );
	A = B;
}


pass_declaration(A) ::= OP_PASS(P) OP_ID(B) enter_fx_mode OP_LEFT_BRACE state_list(C) OP_RIGHT_BRACE.
{
	A = C;
	A->SetToken( &B );
	A->SetLocation( P.fileLocation );
	parserState->m_mode = ParserState::HLSL;
}


pass_declaration(A) ::= OP_PASS(P) OP_ID(B) enter_fx_mode OP_LEFT_BRACE OP_RIGHT_BRACE.
{
	A = new ASTNode( NT_PASS, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->SetToken( &B );
	A->SetLocation( P.fileLocation );
	parserState->m_mode = ParserState::HLSL;
}


state_list(A) ::= state_assignment(B).
{
	A = new ASTNode( NT_PASS, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

state_list(A) ::= shader_assignment(B).
{
	A = new ASTNode( NT_PASS, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

state_list(A) ::= state_list(B) state_assignment(C).
{
	B->AddChild( C );
	A = B;
}

state_list(A) ::= state_list(B) shader_assignment(C).
{
	B->AddChild( C );
	A = B;
}


leave_fx_mode ::= .
{
	parserState->m_mode = ParserState::HLSL;
}

shader_assignment(A) ::= OP_ID(B) OP_EQUAL OP_COMPILE leave_fx_mode OP_ID(C) function_call(D) OP_SEMICOLON.
{
	if( D->GetToken()->type == OP_ID )
	{
		std::string diagnosticMessage;
		Symbol* symbol = parserState->GetSymbolTable().LookupFunction( D->GetToken()->stringValue, D, diagnosticMessage ); 
		if( !symbol ) 
		{ 
            parserState->ShowMessage( *D->GetToken(), EC_NO_OVERRIDE, std::string( D->GetToken()->stringValue.start, D->GetToken()->stringValue.end ).c_str(), diagnosticMessage.c_str() );
		}
		D->SetSymbol( symbol );
	}

	A = new ASTNode( NT_SHADER_ASSIGNMENT, B.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &B );
	A->AddChild( new ASTNode( NT_SHADER_PROFILE, C.fileLocation, parserState->GetSymbolTable().GetCurrentScope(), &C ) );
	A->AddChild( D );
	parserState->m_mode = ParserState::FX;
}


%type buffer_type {ScannerToken}

buffer_type(A) ::= OP_CBUFFER(B).
{
	A = B;
}

buffer_type(A) ::= OP_TBUFFER(B).
{
	A = B;
}

buffer_type(A) ::= OP_SHARED OP_CBUFFER(B).
{
	A = B;
}

buffer_type(A) ::= OP_SHARED OP_TBUFFER(B).
{
	A = B;
}


cbuffer_declaration(A) ::= buffer_type(T) OP_ID(C) register_specifier(R) OP_LEFT_BRACE cbuffer_members(B)  OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
	Symbol* symbol = parserState->GetSymbolTable().AddBufferSymbol( C.stringValue );
	symbol->registerSpecifier[R.shaderProfile] = R;
	A->SetSymbol( symbol );
	A->SetLocation( C.fileLocation );
}

cbuffer_declaration(A) ::= buffer_type(T) OP_ID(C) OP_LEFT_BRACE cbuffer_members(B)  OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
	A->SetSymbol( parserState->GetSymbolTable().AddBufferSymbol( C.stringValue ) );
	A->SetLocation( C.fileLocation );
}

cbuffer_declaration(A) ::= buffer_type(T) register_specifier(R) OP_LEFT_BRACE cbuffer_members(B)  OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
	Symbol* symbol = parserState->GetSymbolTable().AddBufferSymbol( parserState->AllocateName() );
	symbol->registerSpecifier[R.shaderProfile] = R;
	A->SetSymbol( symbol );
	A->SetLocation( T.fileLocation );
}

cbuffer_declaration(A) ::= buffer_type(T) OP_LEFT_BRACE cbuffer_members(B)  OP_RIGHT_BRACE.
{
	A = B;
	A->SetToken( &T );
	A->SetSymbol( parserState->GetSymbolTable().AddBufferSymbol( parserState->AllocateName() ) );
	A->SetLocation( T.fileLocation );
}


cbuffer_members(A) ::= declaration(B).
{
	A = new ASTNode( NT_CBUFFER, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	A->AddChild( B );
}

cbuffer_members(A) ::= cbuffer_members(C) declaration(B).
{
	A = C;
	A->AddChild( B );
}


translation_unit(A) ::= external_declaration(B).
{
	A = new ASTNode( NT_PROGRAM, B->GetLocation(), parserState->GetSymbolTable().GetCurrentScope(), nullptr );
	parserState->MoveOfflineStatements( A );
	if( B )
	{
		A->AddChild( B );
	}
}

translation_unit(A) ::= translation_unit(B) external_declaration(C).
{
	parserState->MoveOfflineStatements( B );
	if( C )
	{
		B->AddChild(C);
	}
	A = B;
}
