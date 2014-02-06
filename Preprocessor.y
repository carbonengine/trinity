%include {

	#include <cstdio>
	#include <cstdlib>
	#include <cassert>
	#include <vector>
	#include <map>
	#include "HLSLParser.h"
	#include "ParserUtils.h"
	#include "ParserState.h"
}

%extra_argument {ParserState* parserState}

%token_type {ScannerToken}
%default_type {signed long}

%syntax_error { parserState->ShowMessage( TOKEN, EC_SYNTAX_ERROR, ToString( TOKEN.stringValue ).c_str() ); }

in ::= expression(B).
{
	parserState->m_preprocessorConditionResult = B;
}


literal_constant(A) ::= PP_INT_CONST(B).
{
	A = ParseNumber( B.stringValue.start, B.stringValue.end );
}

literal_constant(A) ::= PP_CHAR_CONST.
{
	A = 1;
}

%type id {ScannerToken}

id(A) ::= PP_ID(C).
{
	if( C.stringValue == MakeInlineString( "defined" ) )
	{
		parserState->m_expandMacros = false;
	}
	A = C;
}

primary_expression(A) ::= id(C).
{
	if( C.stringValue == MakeInlineString( "defined" ) )
	{
		parserState->m_expandMacros = false;
	}
	else
	{
		A = 0;
	}
}

primary_expression(A) ::= literal_constant(B).
{
	A = B;
}

primary_expression(A) ::= PP_LEFT_PAREN expression(B) PP_RIGHT_PAREN.
{
	A = B;
}


postfix_expression(A) ::= primary_expression(B).
{
	A = B;
}

postfix_expression(A) ::= id(C) PP_LEFT_PAREN PP_ID(B) PP_RIGHT_PAREN.
{
	if( C.stringValue != MakeInlineString( "defined" ) )
	{
		parserState->ShowMessage( EC_SYNTAX_ERROR, ToString( C.stringValue ).c_str() );
	}
	else
	{
		if( parserState->m_defines.find(B.stringValue)  != parserState->m_defines.end() )
		{
			A = 1;
		}
		else
		{
			A = 0;
		}
	}
	parserState->m_expandMacros = true;
}


unary_expression(A) ::= postfix_expression(B).
{
	A = B;
}

unary_expression(A) ::= PP_PLUS unary_expression(C).
{
	A = C;
}

unary_expression(A) ::= PP_DASH unary_expression(C).
{
	A = -C;
}

unary_expression(A) ::= PP_BANG unary_expression(C).
{
	A = !C;
}

unary_expression(A) ::= PP_TILDE unary_expression(C).
{
	A = ~C;
}


multiplicative_expression(A) ::= unary_expression(B).
{
	A = B;
}

multiplicative_expression(A) ::= multiplicative_expression(B) PP_STAR unary_expression(D).
{
	A = B * D;
}

multiplicative_expression(A) ::= multiplicative_expression(B) PP_SLASH unary_expression(D).
{
	A = B / D;
}

multiplicative_expression(A) ::= multiplicative_expression(B) PP_PERCENT unary_expression(D).
{
	A = B % D;
}


additive_expression(A) ::= multiplicative_expression(B).
{
	A = B;
}

additive_expression(A) ::= additive_expression(B) PP_PLUS multiplicative_expression(D).
{
	A = B + D;
}

additive_expression(A) ::= additive_expression(B) PP_DASH multiplicative_expression(D).
{
	A = B - D;
}


shift_expression(A) ::= additive_expression(B).
{
	A = B;
}

shift_expression(A) ::= shift_expression(B) PP_LEFT_OP additive_expression(D).
{
	A = B << D;
}

shift_expression(A) ::= shift_expression(B) PP_RIGHT_OP additive_expression(D).
{
	A = B << D;
}


relational_expression(A) ::= shift_expression(B).
{
	A = B;
}

relational_expression(A) ::= relational_expression(B) PP_LESS shift_expression(D).
{
	A = B < D;
}

relational_expression(A) ::= relational_expression(B) PP_MORE shift_expression(D).
{
	A = B > D;
}

relational_expression(A) ::= relational_expression(B) PP_LE_OP shift_expression(D).
{
	A = B <= D;
}

relational_expression(A) ::= relational_expression(B) PP_GE_OP shift_expression(D).
{
	A = B >= D;
}


equality_expression(A) ::= relational_expression(B).
{
	A = B;
}

equality_expression(A) ::= equality_expression(B) PP_EQ_OP relational_expression(D).
{
	A = B == D;
}

equality_expression(A) ::= equality_expression(B) PP_NE_OP relational_expression(D).
{
	A = B != D;
}


and_expression(A) ::= equality_expression(B).
{
	A = B;
}

and_expression(A) ::= and_expression(B) PP_AMPERSAND equality_expression(D).
{
	A = B & D;
}


exclusive_or_expression(A) ::= and_expression(B).
{
	A = B;
}

exclusive_or_expression(A) ::= exclusive_or_expression(B) PP_CARET and_expression(D).
{
	A = B ^ D;
}


inclusive_or_expression(A) ::= exclusive_or_expression(B).
{
	A = B;
}

inclusive_or_expression(A) ::= inclusive_or_expression(B) PP_VERTICAL_BAR exclusive_or_expression(D).
{
	A = B | D;
}


logical_and_expression(A) ::= inclusive_or_expression(B).
{
	A = B;
}

logical_and_expression(A) ::= logical_and_expression(B) PP_AND_OP inclusive_or_expression(D).
{
	A = B && D;
}


logical_or_expression(A) ::= logical_and_expression(B).
{
	A = B;
}

logical_or_expression(A) ::= logical_or_expression(B) PP_OR_OP logical_and_expression(D).
{
	A = B || D;
}


conditional_expression(A) ::= logical_or_expression(B).
{
	A = B;
}

conditional_expression(A) ::= logical_or_expression(B) PP_QUESTION expression(D) PP_COLON expression(E).
{
	A = B ? D : E;
}


expression(A) ::= conditional_expression(B).
{
	A = B;
}
