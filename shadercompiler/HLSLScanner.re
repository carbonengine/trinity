#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <vector>
#include <map>
#define _IN_SCANNER
#include "HLSLParser.h"
#include "ParserUtils.h"
#include "SymbolTable.h"
#include "ParserState.h"


#define RETURN_PP(x) { token.type = (x); token.stringValue.start = start; token.stringValue.end = YYCURSOR;token.fileLocation = state.GetCurrentLocation(); return SCAN_PREPROCESSOR; }


/*!re2c

re2c:indent:top = 2;
re2c:yyfill:enable = 0;
		
ANY = [\000-\377]; 
LETTER = [a-zA-Z_]; 
DECIMAL = [0-9]; 
OCTAL = [0-7]; 
HEX = [a-fA-F0-9]; 
INT_SUFFIX = [uUlL]*; 
ESC	= [\\] ([abfnrtv?'"\\] | "x" HEX+ | OCTAL+); 
EXPONENT	= [Ee] [+-]? DECIMAL+; 
FLOAT_SUFFIX	= [fFlL];
SPACE = [ \t\v\f\r]+;
PP = "\n" SPACE* "#" [ \t]*;
ID = LETTER(LETTER|DECIMAL)*;
*/

#define YYCTYPE char
#define YYCURSOR (state.GetStreamPosition())
#define YYLIMIT (state.GetStreamEnd())
#define	YYMARKER (marker)
#define	YYCTXMARKER (ctxMarker)

bool GetIncludePath( ParserState &state, InlineString& path )
{
	bool local;
	if( *YYCURSOR == '"' )
	{
		local = true;
	}
	else if( *YYCURSOR == '<' )
	{
		local = false;
	}
	else
	{
		return false;
	}
	path.start = YYCURSOR++;
	while( YYCURSOR < YYLIMIT )
	{
		if( *YYCURSOR == '"' )
		{
			if( local )
			{
				path.end = ++YYCURSOR;
				return true;
			}
		}
		else if( *YYCURSOR == '>' )
		{
			if( !local )
			{
				path.end = ++YYCURSOR;
				return true;
			}
		}
		++YYCURSOR;
	}
	return false;
}

bool ExpectToken( ParserState &state, PreprocessorTokenType type, InlineString &result )
{
	PreprocessorToken token;
	PreprocessorScanResult code = state.GetPreprocessorToken( token );
	if( code == PPSR_ERROR )
	{
		return false;
	}
	else if( code == PPSR_EOF )
	{
		state.ShowMessage( EC_UNEXPECTED_EOF );
		return false;
	}
	if( token.type != type )
	{
		state.ShowMessage( EC_SYNTAX_ERROR, ToString( token.string ).c_str() );
		return false;
	}
	result = token.string;
	return true;
}

#define RETURN_PPT(x) { token.type = (x); token.string = MakeInlineString( start, YYCURSOR ); token.fileLocation = state.GetCurrentLocation(); return PPSR_OK; }

PreprocessorScanResult GetPreprocessorToken( ParserState &state, PreprocessorToken& token )
{
	const char* marker = 0;
std:

	while( YYCURSOR < YYLIMIT )
	{
		const char* start = YYCURSOR;

		/*!re2c
		LETTER(LETTER|DECIMAL)*	
		{
			RETURN_PPT( PPT_ID );
		}

		("0" [xX] HEX+ INT_SUFFIX?) | ("0" OCTAL+ INT_SUFFIX?) | (DECIMAL+ INT_SUFFIX? )
		{
			RETURN_PPT( PPT_INT_CONST );
		}

		(DECIMAL+ EXPONENT FLOAT_SUFFIX?) | (DECIMAL* "." DECIMAL+ EXPONENT? FLOAT_SUFFIX?) | (DECIMAL+ "." DECIMAL* EXPONENT? FLOAT_SUFFIX?) 
		{
			RETURN_PPT( PPT_FLOAT_CONST );
		}

		["](ESC|ANY\[\n\\"])*["]
		{
			RETURN_PPT( PPT_STRING_CONST );
		}

		"//" { goto cppComment; }
		"/*" { goto comment; }

		PP "include" SPACE+ 
		{
			state.GetCurrentLocation().lineNumber++;
			InlineString path;
			if( !GetIncludePath( state, path ) )
			{
				state.ShowMessage( EC_INVALID_INCLUDE_PATH );
				return PPSR_ERROR;
			}
			if( !state.InSkipMode() )
			{
				state.IncludeFile( path );
			}
			start = YYCURSOR;
			continue;
		}

		PP "define" SPACE+
		{ 
			state.GetCurrentLocation().lineNumber++;
			if( state.InSkipMode() )
			{
				bool done = false;
				bool slash = false;
				for( ; !done && YYCURSOR < YYLIMIT; ++YYCURSOR )
				{
					switch( *YYCURSOR )
					{
					case '\\':
						slash = true;
						break;
					case ' ':
					case '\t':
					case '\v':
					case '\f':
					case '\r':
						break;
					case '\n':
						if( slash )
						{
							state.GetCurrentLocation().lineNumber++;
							slash = false;
						}
						else
						{
							done = true;
						}
						break;
					default:
						slash = false;
					}
					if( done )
					{
						break;
					}
				}
			}
			else
			{
				PreprocessorDefine define;
				define.location = state.GetCurrentLocation();
				InlineString name;
				InlineString idToken;
				state.m_expandMacros = false;
				if( !ExpectToken( state, PPT_ID, name ) )
				{
					state.m_expandMacros = true;
					return PPSR_ERROR;
				}
				state.m_expandMacros = true;
				if( YYCURSOR < YYLIMIT && *YYCURSOR == '(' )
				{
					++YYCURSOR;
					while( true )
					{
						if( !ExpectToken( state, PPT_ID, idToken ) )
						{
							return PPSR_ERROR;
						}
						define.parameters.push_back( idToken );
						if( !ExpectToken( state, PPT_OPERATOR, idToken ) )
						{
							return PPSR_ERROR;
						}
						if( idToken.end - idToken.start != 1 )
						{
							state.ShowMessage( EC_SYNTAX_ERROR, ToString( idToken ).c_str() );
							return PPSR_ERROR;
						}
						if( *idToken.start == ',' )
						{
							continue;
						}
						if( *idToken.start == ')' )
						{
							break;
						}
						state.ShowMessage( EC_SYNTAX_ERROR, ToString( idToken ).c_str() );
						return PPSR_ERROR;
					}
				}
				{
					start = YYCURSOR;
					bool done = false;
					bool slash = false;
					for( ; !done && YYCURSOR < YYLIMIT; ++YYCURSOR )
					{
						switch( *YYCURSOR )
						{
						case '\\':
							slash = true;
							break;
						case ' ':
						case '\t':
						case '\v':
						case '\f':
						case '\r':
							break;
						case '\n':
							if( slash )
							{
								state.GetCurrentLocation().lineNumber++;
								slash = false;
							}
							else
							{
								done = true;
							}
							break;
						default:
							slash = false;
						}
						if( done )
						{
							break;
						}
					}
					define.value = MakeInlineString( start, YYCURSOR );
				}
				state.m_defines[name] = define;
			}
			continue;
		}

		PP "if"
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}

			PreprocessorCondition condition;
			condition.inheridedSkipMode = false;
			if( !state.m_prepocessorConditions.empty() )
			{
				condition.inheridedSkipMode = state.m_prepocessorConditions.back().skipMode;
			}
			if( condition.inheridedSkipMode )
			{
				condition.skipMode = true;
			}
			else
			{
				PreprocessorScanResult code = state.EvaluatePreprocessorCondition( MakeInlineString( start, YYCURSOR ), condition.skipMode );
				condition.skipMode = !condition.skipMode;
				if( code != PPSR_OK )
				{
					return code;
				}
			}
			condition.used = !condition.skipMode;
			condition.seenElse = false;
			state.m_prepocessorConditions.push_back( condition );
			start = YYCURSOR;
			continue;
		}

		PP "ifdef"
		{
			state.GetCurrentLocation().lineNumber++;
			if( state.InSkipMode() )
			{
				start = YYCURSOR;
				while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
				{
					++YYCURSOR;
				}
				PreprocessorCondition condition;
				condition.inheridedSkipMode = true;
				condition.skipMode = true;
				condition.used = !condition.skipMode;
				condition.seenElse = false;
				state.m_prepocessorConditions.push_back( condition );
			}
			else
			{
				start = YYCURSOR;
				bool done = false;
				while( !done && YYCURSOR < YYLIMIT )
				{
					switch( *YYCURSOR )
					{
					case ' ':
					case '\t':
					case '\v':
					case '\f':
					case '\r':
						++YYCURSOR;
						break;
					case '\n':
						state.ShowMessage( EC_UNEXPECTED_EOL );
						return PPSR_ERROR;
					default:
						done = true;
						break;
					}
				}

				state.m_expandMacros = false;
				InlineString name;
				if( !ExpectToken( state, PPT_ID, name ) )
				{
					state.m_expandMacros = true;
					return PPSR_ERROR;
				}
				state.m_expandMacros = true;

				PreprocessorCondition condition;
				condition.inheridedSkipMode = false;
				if( !state.m_prepocessorConditions.empty() )
				{
					condition.inheridedSkipMode = state.m_prepocessorConditions.back().skipMode;
				}
				if( condition.inheridedSkipMode )
				{
					condition.skipMode = true;
				}
				else
				{
					condition.skipMode = state.m_defines.find( name ) == state.m_defines.end();
				}
				condition.used = !condition.skipMode;
				condition.seenElse = false;
				state.m_prepocessorConditions.push_back( condition );
				start = YYCURSOR;
			}
			continue;
		}

		PP "ifndef"
		{
			state.GetCurrentLocation().lineNumber++;
			if( state.InSkipMode() )
			{
				start = YYCURSOR;
				while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
				{
					++YYCURSOR;
				}
				PreprocessorCondition condition;
				condition.inheridedSkipMode = true;
				condition.skipMode = true;
				condition.used = !condition.skipMode;
				condition.seenElse = false;
				state.m_prepocessorConditions.push_back( condition );
			}
			else
			{
				start = YYCURSOR;
				bool done = false;
				while( !done && YYCURSOR < YYLIMIT )
				{
					switch( *YYCURSOR )
					{
					case ' ':
					case '\t':
					case '\v':
					case '\f':
					case '\r':
						++YYCURSOR;
						break;
					case '\n':
						state.ShowMessage( EC_UNEXPECTED_EOL );
						return PPSR_ERROR;
					default:
						done = true;
						break;
					}
				}

				state.m_expandMacros = false;
				InlineString name;
				if( !ExpectToken( state, PPT_ID, name ) )
				{
					state.m_expandMacros = true;
					return PPSR_ERROR;
				}
				state.m_expandMacros = true;

				PreprocessorCondition condition;
				condition.inheridedSkipMode = false;
				if( !state.m_prepocessorConditions.empty() )
				{
					condition.inheridedSkipMode = state.m_prepocessorConditions.back().skipMode;
				}
				if( condition.inheridedSkipMode )
				{
					condition.skipMode = true;
				}
				else
				{
					condition.skipMode = state.m_defines.find( name ) != state.m_defines.end();
				}
				condition.used = !condition.skipMode;
				condition.seenElse = false;
				state.m_prepocessorConditions.push_back( condition );
				start = YYCURSOR;
			}
			continue;
		}

		PP "elif"
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}

			if( state.m_prepocessorConditions.empty() )
			{
				state.ShowMessage( EC_PREPROCESSOR_MISPLACED_ELIF );
				return PPSR_ERROR;
			}
			else
			{
				PreprocessorCondition& condition = state.m_prepocessorConditions.back();
				if( condition.seenElse )
				{
				    state.ShowMessage( EC_PREPROCESSOR_MISPLACED_ELIF );
					return PPSR_ERROR;
				}
				else if( condition.used || condition.inheridedSkipMode )
				{
					condition.skipMode = true;
				}
				else
				{
					PreprocessorScanResult code = state.EvaluatePreprocessorCondition( MakeInlineString( start, YYCURSOR ), condition.skipMode );
					condition.skipMode = !condition.skipMode;
					if( code != PPSR_OK )
					{
						return code;
					}
					condition.used = !condition.skipMode;
				}
			}
			start = YYCURSOR;
			continue;
		}

		PP "else"
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}

			if( state.m_prepocessorConditions.empty() )
			{
				state.ShowMessage( EC_PREPROCESSOR_MISPLACED_ELSE );
				return PPSR_ERROR;
			}
			else
			{
				PreprocessorCondition& condition = state.m_prepocessorConditions.back();
				if( condition.seenElse )
				{
				   state.ShowMessage( EC_PREPROCESSOR_MISPLACED_ELSE );
					return PPSR_ERROR;
				}
				else if( condition.inheridedSkipMode )
				{
					condition.skipMode = true;
				}
				else
				{
					condition.skipMode = condition.used;
				}
			}
			start = YYCURSOR;
			continue;
		}

		PP "endif"
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}
			if( state.m_prepocessorConditions.empty() )
			{
				state.ShowMessage( EC_PREPROCESSOR_MISPLACED_ENDIF );
				return PPSR_ERROR;
			}
			else
			{
				state.m_prepocessorConditions.pop_back();
			}
			start = YYCURSOR;
			continue;
		}

		PP "error"
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}

			if( !state.InSkipMode() )
			{
				state.ShowMessage( EC_USER_ERROR, std::string( start, YYCURSOR ).c_str() );
				return PPSR_ERROR;
			}
			continue;
		}

		PP "line" SPACE+ DECIMAL+ SPACE+ ["](ESC|ANY\[\n\\"])*["]
		{
			state.GetCurrentLocation().lineNumber++;
			if( !state.InSkipMode() )
			{
				const char* c = YYCURSOR - 2;
				while( *c != '\"' )
				{
					--c;
				}
				state.GetCurrentLocation().fileName = MakeInlineString( c + 1, YYCURSOR - 1 );
				while ( *c == ' ' || *c == '\t' || *c == '\v' || *c == '\f' || *c == '\r' )
				{
					--c;
				}
				const char* end = c;
				while ( *c != ' ' && *c != '\t' && *c != '\v' && *c != '\f' && *c != '\r' )
				{
					--c;
				}
				state.GetCurrentLocation().lineNumber = unsigned( ParseNumber( c, end + 1 ) - 1 );
			}
			continue;
		}

		PP "line" SPACE+ DECIMAL+
		{
			state.GetCurrentLocation().lineNumber++;
			if( !state.InSkipMode() )
			{
				const char* c = YYCURSOR - 1;
				while ( *c != ' ' && *c != '\t' && *c != '\v' && *c != '\f' && *c != '\r' )
				{
					--c;
				}
				state.GetCurrentLocation().lineNumber = unsigned( ParseNumber( c, YYCURSOR ) - 1 );
			}
			continue;
		}

		PP "undef" 
		{
			state.GetCurrentLocation().lineNumber++;
			if( !state.InSkipMode() )
			{
				InlineString name;
				state.m_expandMacros = false;
				if( !ExpectToken( state, PPT_ID, name ) )
				{
					state.m_expandMacros = true;
					return PPSR_ERROR;
				}
				state.m_expandMacros = true;
				
				auto found = state.m_defines.find( name );
				if( found != state.m_defines.end() )
				{
					state.m_defines.erase( found );
				}
			}
			else
			{
				while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
				{
					++YYCURSOR;
				}
			}
			continue;
		}

		PP "pragma" SPACE+
		{
			state.GetCurrentLocation().lineNumber++;
			start = YYCURSOR;
			while( YYCURSOR < YYLIMIT && *YYCURSOR != '\n' )
			{
				++YYCURSOR;
			}
			if( !state.InSkipMode() )
			{
				InlineString pragma = MakeInlineString( start, YYCURSOR );
				state.AddPragma( pragma );
			}
			continue;
		}

		"*=" { RETURN_PPT( PPT_OPERATOR ); }
		"/=" { RETURN_PPT( PPT_OPERATOR ); }
		"%=" { RETURN_PPT( PPT_OPERATOR ); }
		"+=" { RETURN_PPT( PPT_OPERATOR ); }
		"-=" { RETURN_PPT( PPT_OPERATOR ); }
		"<<=" { RETURN_PPT( PPT_OPERATOR ); }
		">>=" { RETURN_PPT( PPT_OPERATOR ); }
		"&=" { RETURN_PPT( PPT_OPERATOR ); }
		"^=" { RETURN_PPT( PPT_OPERATOR ); }
		"|=" { RETURN_PPT( PPT_OPERATOR ); }

		"++" { RETURN_PPT( PPT_OPERATOR ); }
		"--" { RETURN_PPT( PPT_OPERATOR ); }
		"<<" { RETURN_PPT( PPT_OPERATOR ); }
		">>" { RETURN_PPT( PPT_OPERATOR ); }

		"<=" { RETURN_PPT( PPT_OPERATOR ); }
		">=" { RETURN_PPT( PPT_OPERATOR ); }
		"==" { RETURN_PPT( PPT_OPERATOR ); }
		"!=" { RETURN_PPT( PPT_OPERATOR ); }
		"&&" { RETURN_PPT( PPT_OPERATOR ); }
		"||" { RETURN_PPT( PPT_OPERATOR ); }

		"##" { RETURN_PPT( PPT_PP_OPERATOR ); }
		"#@" { RETURN_PPT( PPT_PP_OPERATOR ); }
		"#" { RETURN_PPT( PPT_PP_OPERATOR ); }

		"+" { RETURN_PPT( PPT_OPERATOR ); }
		"-" { RETURN_PPT( PPT_OPERATOR ); }
		"!" { RETURN_PPT( PPT_OPERATOR ); }
		"~" { RETURN_PPT( PPT_OPERATOR ); }
		"*" { RETURN_PPT( PPT_OPERATOR ); }
		"/" { RETURN_PPT( PPT_OPERATOR ); }
		"%" { RETURN_PPT( PPT_OPERATOR ); }
		"(" { RETURN_PPT( PPT_OPERATOR ); }
		")" { RETURN_PPT( PPT_OPERATOR ); }
		"[" { RETURN_PPT( PPT_OPERATOR ); }
		"]" { RETURN_PPT( PPT_OPERATOR ); }
		"." { RETURN_PPT( PPT_OPERATOR ); }
		"," { RETURN_PPT( PPT_OPERATOR ); }
		"<" { RETURN_PPT( PPT_OPERATOR ); }
		">" { RETURN_PPT( PPT_OPERATOR ); }
		"&" { RETURN_PPT( PPT_OPERATOR ); }
		"^" { RETURN_PPT( PPT_OPERATOR ); }
		"|" { RETURN_PPT( PPT_OPERATOR ); }
		"?" { RETURN_PPT( PPT_OPERATOR ); }
		":" { RETURN_PPT( PPT_OPERATOR ); }
		"=" { RETURN_PPT( PPT_OPERATOR ); }
		";" { RETURN_PPT( PPT_OPERATOR ); }
		"{" { RETURN_PPT( PPT_OPERATOR ); }
		"}" { RETURN_PPT( PPT_OPERATOR ); }

		SPACE+ { continue; }
		"\\" SPACE* "\n"
		{
			if( state.InMacro() )
			{
				start = YYCURSOR;
				continue;
			}
			else
			{
				state.ShowMessage( EC_UNEXPECTED_CHARACTER, *start );
				return PPSR_ERROR;
			} 			
		}
		"\n" 
		{ 
			state.GetCurrentLocation().lineNumber++; 
			start = YYCURSOR; 
			state.m_newLine = true; 
			continue; 
		}
		"\000" { return PPSR_EOF; }
		[^] 
	    {
			state.ShowMessage( EC_UNEXPECTED_CHARACTER, *start );
			return PPSR_ERROR;
	    } 			
		*/
	}

comment:
	while( YYCURSOR < YYLIMIT )
	{
		/*!re2c
		"*/" { goto std; }
		"\n"
		{
			state.GetCurrentLocation().lineNumber++; 
			continue;
		}
		"\000" { return PPSR_EOF; }
		[^]	{ continue; }
		*/ 
	}
	return PPSR_EOF;

cppComment:
	while( YYCURSOR < YYLIMIT )
	{
		/*!re2c
		"\n"
		{
			state.m_newLine = true;
			--YYCURSOR;
			goto std;
		}
		"\000" { return PPSR_EOF; }
		[^]	{ continue; }
		*/ 
	}

	return PPSR_EOF;

#undef YYCTYPE
#undef YYCURSOR
#undef YYLIMIT
#undef YYMARKER
#undef YYCTXMARKER
}



bool ConvertToScannerToken( ParserState &state, const PreprocessorToken& ppToken, ScannerToken& token ) 
{
#define YYCTYPE char
#define YYCURSOR (start)
#define YYLIMIT (ppToken.string.end)

#define RETURN(x) { token.type = (x); token.stringValue = ppToken.string; token.fileLocation = ppToken.fileLocation; return true; }

	const char* start = ppToken.string.start;

	switch( ppToken.type )
	{
	case PPT_INT_CONST:
		RETURN( OP_INT_CONST );
	case PPT_FLOAT_CONST:
		RETURN( OP_FLOAT_CONST );
	case PPT_STRING_CONST:
		RETURN( OP_STRING_CONST );
	default:
		break;
	}

	while( YYCURSOR < YYLIMIT )
	{
		token.intValue = 1;

		/*!re2c

		"true"
		{
			token.intValue = 1;
			RETURN( OP_BOOL_CONST );
		}

		"false"
		{
			token.intValue = 0;
			RETURN( OP_BOOL_CONST );
		}

		"void" { RETURN( OP_VOID ); }

		"float"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_FLOAT );
		}
		"float"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_FLOAT );
		}
		"float"
		{
			token.intValue = 1;
			RETURN( OP_FLOAT );
		}
		"int"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_INT );
		}
		"int"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_INT );
		}
		"int"
		{
			token.intValue = 1;
			RETURN( OP_INT );
		}
		"uint"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_UINT );
		}
		"uint"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_UINT );
		}
		"uint"
		{
			token.intValue = 1;
			RETURN( OP_UINT );
		}
		"half"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_HALF );
		}
		"half"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_HALF );
		}
		"half"
		{
			token.intValue = 1;
			RETURN( OP_HALF );
		}
		"double"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_DOUBLE );
		}
		"double"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_DOUBLE );
		}
		"double"
		{
			token.intValue = 1;
			RETURN( OP_DOUBLE );
		}
		"bool"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_BOOL );
		}
		"bool"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_BOOL );
		}
		"bool"
		{
			token.intValue = 1;
			RETURN( OP_BOOL );
		}
		"string"[1234]"x"[1234]
		{
			token.intValue = ( *(YYCURSOR - 1) - '0' ) << 8 | ( *(YYCURSOR - 3) - '0' );
			RETURN( OP_STRING );
		}
		"string"[1234]
		{
			token.intValue = *(YYCURSOR - 1) - '1' + 1;
			RETURN( OP_STRING );
		}
		"string"
		{
			token.intValue = 1;
			RETURN( OP_STRING );
		}
		"vector" { RETURN( OP_VECTOR ); }
		"matrix" { RETURN( OP_MATRIX ); }

		"sampler1D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER1D ); }
		"sampler2D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER2D ); }
		"sampler3D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER3D ); }
		"samplerCUBE" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLERCUBE ); }
		"sampler" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER ); }
		"SamplerState" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER ); }
		"SamplerComparisonState" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLERCOMPARISON ); }

		[tT]"exture1D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE1D ); }
		[tT]"exture2D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE2D ); }
		[tT]"exture3D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE3D ); }
		"TextureCube" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURECUBE ); }
		"textureCUBE" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURECUBE ); }
		"Texture1DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE1DARRAY ); }
		"Texture2DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE2DARRAY ); }
		"Texture3DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE3DARRAY ); }
		"TextureCubeArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURECUBEARRAY ); }
		"Texture2DMS" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE2DMS ); }
		"Texture2DMSArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE2DMSARRAY ); }
		"texture" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TEXTURE  ); }
		"Buffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BUFFER ); }
		"AppendStructuredBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_APPENDSTRUCTUREDBUFFER ); }
		"ByteAddressBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BYTEADDRESSBUFFER ); }
		"ConsumeStructuredBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_CONSUMESTRUCTUREDBUFFER ); }
		"InputPatch" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_INPUTPATCH ); }
		"OutputPatch" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_OUTPUTPATCH ); }

		"BindlessHandleTexture2D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BINDLESSHANDLETEXTURE2D ); }
		"BindlessHandleTexture3D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BINDLESSHANDLETEXTURE3D ); }
		"BindlessHandleTextureCube" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BINDLESSHANDLETEXTURECUBE ); }

		"RWBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWBUFFER ); }
		"RWByteAddressBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWBYTEADDRESSBUFFER ); }
		"RWStructuredBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWSTRUCTUREDBUFFER ); }
		"RWTexture1D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE1D ); }
		"RWTexture1DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE1DARRAY ); }
		"RWTexture2D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE2D ); }
		"RWTexture2DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE2DARRAY ); }
		"RWTexture3D" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE3D ); }
		"RWTexture3DArray" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RWTEXTURE3DARRAY ); }
		"StructuredBuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_STRUCTUREDBUFFER ); }

		"PointStream" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_POINTSTREAM ); }
		"LineStream" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_LINESTREAM ); }
		"TriangleStream" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TRIANGLESTREAM ); }

		"struct" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_STRUCT ); }
		"if" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_IF ); }
		"else" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_ELSE ); }
		"while" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_WHILE ); }
		"do" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_DO ); }
		"for" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_FOR ); }
		"continue" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_CONTINUE ); }
		"break" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_BREAK ); }
		"return" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_RETURN ); }
		"discard" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_DISCARD ); }
		"switch" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SWITCH ); }
		"case" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_CASE ); }
		"default" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_DEFAULT ); }

		"technique" { RETURN( OP_TECHNIQUE ); }
		"pass" { RETURN( OP_PASS ); }
		"compile" { RETURN( OP_COMPILE ); }

		"inout" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_INOUT ); }
		"in" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_IN ); }
		"out" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_OUT ); }

		"point" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_POINT ); }
		"line" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_LINE ); }
		"triangle" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TRIANGLE ); }
		"lineadj" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_LINEADJ ); }
		"triangleadj" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TRIANGLEADJ ); }

		"const" { RETURN( OP_CONST ); }
		"row_major" { RETURN( OP_ROW_MAJOR ); }
		"column_major" { RETURN( OP_COLUMN_MAJOR ); }

		"extern" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_EXTERN ); }
		"nointerpolation" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_NOINTERPOLATION ); }
		"precise" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_PRECISE ); }
		"shared" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SHARED ); }
		"groupshared" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_GROUPSHARED ); }
		"static" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_STATIC ); }
		"uniform" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_UNIFORM ); }
		"volatile" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_VOLATILE ); }

		"linear" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_LINEAR ); }
		"centroid" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_CENTROID ); }
		"nointerpolation" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_NOINTERPOLATION ); }
		"noperspective" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_NOPERSPECTIVE ); }

		"packoffset" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_PACKOFFSET ); }
		"register" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_REGISTER ); }
		"sampler_state" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_SAMPLER_STATE ); }

		"cbuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_CBUFFER ); }
		"tbuffer" { RETURN( state.m_mode == ParserState::FX ? OP_ID : OP_TBUFFER ); }

		"*=" { RETURN( OP_MUL_ASSIGN ); }
		"/=" { RETURN( OP_DIV_ASSIGN ); }
		"%=" { RETURN( OP_MOD_ASSIGN ); }
		"+=" { RETURN( OP_ADD_ASSIGN ); }
		"-=" { RETURN( OP_SUB_ASSIGN ); }
		"<<=" { RETURN( OP_LEFT_ASSIGN ); }
		">>=" { RETURN( OP_RIGHT_ASSIGN ); }
		"&=" { RETURN( OP_AND_ASSIGN ); }
		"^=" { RETURN( OP_XOR_ASSIGN ); }
		"|=" { RETURN( OP_OR_ASSIGN ); }

		"++" { RETURN( OP_INC_OP ); }
		"--" { RETURN( OP_DEC_OP ); }
		"<<" { RETURN( OP_LEFT_OP ); }
		">>" { RETURN( OP_RIGHT_OP ); }

		"<=" { RETURN( OP_LE_OP ); }
		">=" { RETURN( OP_GE_OP ); }
		"==" { RETURN( OP_EQ_OP ); }
		"!=" { RETURN( OP_NE_OP ); }
		"&&" { RETURN( OP_AND_OP ); }
		"||" { RETURN( OP_OR_OP ); }

		"##" { return false; }
		"#@" { return false; }
		"#" { return false; }

		"+" { RETURN( OP_PLUS ); }
		"-" { RETURN( OP_DASH ); }
		"!" { RETURN( OP_BANG ); }
		"~" { RETURN( OP_TILDE ); }
		"*" { RETURN( OP_STAR ); }
		"/" { RETURN( OP_SLASH ); }
		"%" { RETURN( OP_PERCENT ); }
		"(" { RETURN( OP_LEFT_PAREN ); }
		")" { RETURN( OP_RIGHT_PAREN ); }
		"[" { RETURN( OP_LEFT_BRACKET ); }
		"]" { RETURN( OP_RIGHT_BRACKET ); }
		"." { RETURN( OP_DOT ); }
		"," { RETURN( OP_COMA ); }
		"<" { RETURN( OP_LESS ); }
		">" { RETURN( OP_MORE ); }
		"&" { RETURN( OP_AMPERSAND ); }
		"^" { RETURN( OP_CARET ); }
		"|" { RETURN( OP_VERTICAL_BAR ); }
		"?" { RETURN( OP_QUESTION ); }
		":" { RETURN( OP_COLON ); }
		"=" { RETURN( OP_EQUAL ); }
		";" { RETURN( OP_SEMICOLON ); }
		"{" { RETURN( OP_LEFT_BRACE ); }
		"}" { RETURN( OP_RIGHT_BRACE ); }

		LETTER(LETTER|DECIMAL)*	
		{
			const Symbol* symbol = state.GetSymbolTable().LookupType( ppToken.string );
			if( symbol && symbol->isTypeName )
			{
				RETURN( OP_TYPE_NAME ); 
			}
			RETURN( OP_ID ); 
		} 
		*/
	}
	return false;
}
