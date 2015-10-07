////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "stdafx.h"
#include "EffectCompilerGL2.h"
#include "EffectCompilerDx9.h"
#include "EffectData.h"
#include "CompileMessageQueue.h"
#include "InlineString.h"
#include <strstream>
#include <regex>
#include <iomanip>

extern EffectCompilerDX9 g_compilerDX9;
extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern int g_maxClipPlanes;
extern bool g_glesEmulateSampler;
extern GlesExtensionInfo g_glesExtensions;
extern bool g_generateListing;
extern std::string g_listing;
extern CRITICAL_SECTION g_listingCS;
extern bool g_printWarnings;

extern std::string g_glExternalCompilerSwitch;
extern std::string g_glExternalCompilerPath;

// Critical section for GL contexts
CRITICAL_SECTION s_glCS;
// Owner window for GL contexts
HWND s_glWnd;

namespace
{
const std::regex s_id( "([-!])?([[:alpha:]][[:alnum:]]*)(?:_([a-zA-Z0-9_]+))?(?:\\[([^\\]]*)\\])?(?:\\.([[:alpha:]]+))?" );
const std::regex s_number( "\\-?[[:digit:]]([[:digit:]\\.eE+-])*" );
const std::regex s_coma( "," );
const std::regex s_space( "[ \\t]+" );
const std::regex s_endl( "([ \\t]*\\n)+" );
const std::regex s_comment( "//.*\\n?" );
}

#define CBR_RETURN_VAL( c, val ) {													\
	if( !( c ) ) {																	\
	g_messages.AddMessage( "\\memory(0): error X0000: internal compiler error" );	\
	assert( false );																\
	return val; } }

// --------------------------------------------------------------------------------------
// Description:
//   Structure to define a token when parsing DX9 assembly code.
// --------------------------------------------------------------------------------------
struct DX9AsmToken
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Token type.
	// ----------------------------------------------------------------------------------
	enum Type
	{
		// ID: command, register specifier (along with all modifiers), boolean constant
		ID,
		// Coma
		Coma,
		// Numeric literal (including sign)
		Number,
		// End of line
		CommandEnd,
		// End of stream
		EndOfSteam,
	};
	// Token type
	Type type;
	// Token name (command/register name)
	InlineString name;
	// "-" or "!" modifier sign for registers
	char negate; // - or !
	// Modifier (everything appearing after "_" in command/register name
	InlineString modifier;
	// Register swizzle
	InlineString swizzle;
	// Register indexing expression (we don't parse it since it only contains literals and "a0" register
	InlineString index;
	// TODO: predicate mask? (see setp_comp)
};

// --------------------------------------------------------------------------------------
// Description:
//   Scans DX9 assembly code for next token; skips whitespaces and comments.
// Arguments:
//   stream - DX9 assembly code (start contains current scan position)
//   token - (out) read token
// Return Value:
//   true If token was successfully read from the steam
//   false On error
// --------------------------------------------------------------------------------------
bool GetDX9AsmToken( InlineString& stream, DX9AsmToken& token )
{
	while( stream.start < stream.end )
	{
		// Try regular expressions for each token type: not insanelly optimal, but
		// should work for now.
		std::cmatch match;
		if( std::regex_search( stream.start, stream.end, match, s_space, std::regex_constants::match_continuous ) )
		{
			stream.start = match[0].second;
			continue;
		}
		if( std::regex_search( stream.start, stream.end, match, s_comment, std::regex_constants::match_continuous ) )
		{
			stream.start = match[0].second;
			continue;
		}
		if( std::regex_search( stream.start, stream.end, match, s_id, std::regex_constants::match_continuous ) )
		{
			token.type = DX9AsmToken::ID;
			token.negate = match[1].matched ? match[1].first[0] : 0;
			token.name = match[2].matched ? MakeInlineString( match[2].first, match[2].second ) : MakeInlineString( nullptr, nullptr );
			token.modifier = match[3].matched ? MakeInlineString( match[3].first, match[3].second ) : MakeInlineString( nullptr, nullptr );
			token.index = match[4].matched ? MakeInlineString( match[4].first, match[4].second ) : MakeInlineString( nullptr, nullptr );
			token.swizzle = match[5].matched ? MakeInlineString( match[5].first, match[5].second ) : MakeInlineString( nullptr, nullptr );
			stream.start = match[0].second;
			return true;
		}
		if( std::regex_search( stream.start, stream.end, match, s_number, std::regex_constants::match_continuous ) )
		{
			token.type = DX9AsmToken::Number;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return true;
		}
		if( std::regex_search( stream.start, stream.end, match, s_coma, std::regex_constants::match_continuous ) )
		{
			token.type = DX9AsmToken::Coma;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return true;
		}
		if( std::regex_search( stream.start, stream.end, match, s_endl, std::regex_constants::match_continuous ) )
		{
			token.type = DX9AsmToken::CommandEnd;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return true;
		}
		CBR_RETURN_VAL( false, false );
	}
	token.type = DX9AsmToken::EndOfSteam;
	token.negate = 0;
	token.name.start = token.name.end = 0;
	token.modifier.start = token.modifier.end = 0;
	token.index.start = token.modifier.end = 0;
	token.swizzle.start = token.modifier.end = 0;
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the given command modifier is a comparison modifier.
// Arguments:
//   modifier - Asm command modifier
// Return Value:
//   true If the given command modifier is a comparison modifier
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool IsCompModifier( const InlineString& modifier )
{
	return
		modifier == MakeInlineString( "gt" ) ||
		modifier == MakeInlineString( "lt" ) ||
		modifier == MakeInlineString( "ge" ) ||
		modifier == MakeInlineString( "le" ) ||
		modifier == MakeInlineString( "eq" ) ||
		modifier == MakeInlineString( "ne" );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns vector dimension for a register token (depends on swizzle and register 
//   type).
// Arguments:
//   token - Token containing register ID
// Return Value:
//   Number of vector components for the given register symbol
// --------------------------------------------------------------------------------------
unsigned GetRegisterSwizzleDimension( const DX9AsmToken& token )
{
	unsigned length = 4;
	if( token.name.start[0] == 'b' || token.name.start[0] == 'p' )
	{
		length = 1;
	}
	if( token.swizzle )
	{
		length = token.swizzle.end - token.swizzle.start;
	}
	return length;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a swizzle for a destination register. If the register doesn't have a swizzle
//   the function returns a default swizzle for that register type.
// Arguments:
//   token - Token containing register ID
// Return Value:
//   Swizzle for a destination register
// --------------------------------------------------------------------------------------
InlineString GetRegisterDestSwizzle( const DX9AsmToken& token )
{
	if( token.swizzle )
	{
		return token.swizzle;
	}
	else
	{
		if( token.name.start[0] == 'b' || token.name.start[0] == 'p' )
		{
			return MakeInlineString( "x" );
		}
		else
		{
			return MakeInlineString( "xyzw" );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Information on register bank (register type)
// --------------------------------------------------------------------------------------
struct RegisterBankInfo
{
	// All register names in the bank that are referenced in the shader
	// (this might exclude indirect references with indexing)
	std::set<InlineString> registers;
	// True iff there are indexed fetches from registers in this bank
	bool hasIndexed;
	// Range of used register indexes (used when hasIndexed is true)
	unsigned indexesBegin;
	unsigned indexesEnd;
};

// --------------------------------------------------------------------------------------
// Description:
//   Outputs variable declarations for the given register bank. Registers will either be
//   declared as individual variables or as an array depending if the bank has indexed
//   fetches.
// Arguments:
//   registerInfo - Register bank information (indexed by register type)
//   os - Output stream
//   type - GLSL type name for registers in the bank
//   bank - Bank type to print
// Return Value:
//   Number of vector components for the given register symbol
// --------------------------------------------------------------------------------------
void PrintRegisterBank( std::map<char, RegisterBankInfo>& registerInfo, 
						std::ostream& os, 
						const char* type, 
						char bank )
{
	using std::endl;
	if( registerInfo.find( bank ) != registerInfo.end() )
	{
		auto& r = registerInfo[bank];
		if( r.hasIndexed )
		{
			os << type << ' ' << bank << '[' << ( r.indexesEnd - r.indexesBegin ) << "];" << endl;
		}
		else
		{
			for( auto i = r.registers.begin(); i != r.registers.end(); ++i )
			{
				os << type << ' ' << *i << ';' << endl;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Converts DX9 assebly code to GLSL code.
// Arguments:
//   source - DX9 assembly code
//   glCode - (out) GLSL code
//   glPatchedCode - (out) GLSL code for patched
//   stage - Stage information for current shader
// Return Value:
//   true If conversion is successful
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool AsmToGLES2( const char* source, std::string& glCode, const StageInput& stage, GlesExtensionInfo::ExtensionMap& usedExtensions )
{
	using std::endl;

#define EXPECT_TOKEN( stream, token, expectedType )  {			\
	CBR_RETURN_VAL( tokenIndex < tokens.size(), false );		\
	token = tokens[tokenIndex++];								\
	CBR_RETURN_VAL( token.type == expectedType, false );		\
	}

	// Code stream (code inside main())
	std::ostrstream os;
	// Declaration stream (ends up outside main())
	std::ostrstream declOs;
	std::ostrstream localsOs;
	unsigned tmpCount = 0;

	// Has "lit" helper function declaration
	bool hasLit = false;
	// Has "sign" helper function declaration
	bool hasSign = false;
	// Has "saturate" helper function declaration
	bool hasSaturate = false;
	// Has DX9-style "nrm" function
	bool hasNrm = false;

	// Has declared OES_texture_3D extension
	bool hasTexture3DExtension = false;
	// Has declared OES_standard_derivatives extension
	bool hasDerivativesExtension = false;
	// Has declared EXT_shader_texture_lod extension
	bool hasTextureLodExtension = false;
	// Uses borderXXX functions to simulate sampler border
	bool hasBorderFuncs = false;
	// Uses g2l function to emulate sRgbTexture sampler state
	bool hasGammaToLinear = false;

	// Sampler types for register names (types are strings "2D", "Cube", "3D")
	std::map<InlineString, std::string> samplerTypes;
	// Register bank information per register type (first letter of register name)
	std::map<char, RegisterBankInfo> registerInfo;
	// Remapping for output register names
	std::map<InlineString, InlineString> outputRemap;

	// Constant buffer register (c##) ranges
	std::pair<unsigned, unsigned> cbs[16];

	// Get constant buffer information from stage constants
	for( int i = 0; i < 16; ++i )
	{
		cbs[i].first = -1;
		cbs[i].second = -1;
	}
	for( auto constant = stage.constants.begin(); constant != stage.constants.end(); ++constant )
	{
		const char* name = g_stringTable.GetString( constant->name );
		if( strcmp( name, "PerObjectVS" ) == 0 )
		{
			if( constant->offset / sizeof( Vector4 ) == 180 )
			{
				cbs[5].first = 180;
				cbs[5].second = cbs[5].first + constant->size / sizeof( Vector4 );
			}
			else
			{
				cbs[3].first = constant->offset / sizeof( Vector4 );
				cbs[3].second = cbs[3].first + constant->size / sizeof( Vector4 );
			}
		}
		else if( strcmp( name, "PerFrameVS" ) == 0 )
		{
			cbs[1].first = constant->offset / sizeof( Vector4 );
			cbs[1].second = cbs[1].first + constant->size / sizeof( Vector4 );
		}
		else if( strcmp( name, "PerObjectPS" ) == 0 )
		{
			cbs[4].first = constant->offset / sizeof( Vector4 );
			cbs[4].second = cbs[4].first + constant->size / sizeof( Vector4 );
		}
		else if( strcmp( name, "PerFramePS" ) == 0 )
		{
			cbs[2].first = constant->offset / sizeof( Vector4 );
			cbs[2].second = cbs[2].first + constant->size / sizeof( Vector4 );
		}
		else if( strcmp( name, "g_uiTransforms" ) == 0 )
		{
			cbs[6].first = constant->offset / sizeof( Vector4 );
			cbs[6].second = cbs[6].first + constant->size / sizeof( Vector4 );
		}
		else
		{
			unsigned index = 0;
			if( stage.type == PIXEL_STAGE )
			{
				index = 7;
			}
			cbs[index].first = 0;
			unsigned last = ( constant->offset + constant->size ) / sizeof( Vector4 );
			if( cbs[index].second == -1 || cbs[index].second < last )
			{
				cbs[index].second = last;
			}
		}
	}
	for( int i = 0; i < 16; ++i )
	{
		if( cbs[i].first != -1 && cbs[i].second == -1 )
		{
			for( int j = 0; j < 16; ++j )
			{
				if( cbs[j].first != -1 && cbs[j].first > cbs[i].first )
				{
					cbs[i].second = min( cbs[i].second, cbs[j].first - 1 ) + 1;
				}
			}
		}
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Prints register symbols as GLSL code: chooses correct constant buffer ID if 
	//   needed; attempts to provide correct swizzle.
	// Arguments:
	//   os - Output stream
	//   token - Token containing register symbol
	//   dimension - Requested vector dimension of register (0 if doesn't matter)
	// ----------------------------------------------------------------------------------
	auto PrintRegister = [&]( std::ostream& os, const DX9AsmToken& token, unsigned dimension )
	{
		if( token.negate )
		{
			os << '(' << token.negate;
		}

		if( token.modifier == MakeInlineString( "abs" ) )
		{
			os << "abs(";
		}
		
		if( token.name.start[0] == 'c' )
		{
			InlineString index = token.name;
			index.start++;
			unsigned i = unsigned( atoi( ToString( index ).c_str() ) );
			// First search local constants
			if( registerInfo.find( 'c' ) != registerInfo.end() && 
				registerInfo['c'].registers.find( token.name ) != registerInfo['c'].registers.end() )
			{
				if( registerInfo['c'].hasIndexed )
				{
					os << "c[" << ( i - registerInfo['c'].indexesBegin );
					if( token.index )
					{
						os << '+' << token.index;
					}
					os << ']';
				}
				else
				{
					os << token.name;
				}
			}
			else
			{
				// If not a local constant, it should be in one of constant buffers
				bool found = false;
				for( int cb = 0; cb < 16; ++cb )
				{
					if( i >= cbs[cb].first && i < cbs[cb].second )
					{
						os << "cb" << cb << '['  << ( i - cbs[cb].first );
						if( token.index )
						{
							os << '+' << token.index;
						}
						os << ']';
						found = true;
						break;
					}
				}
				if( !found )
				{
					assert( false );
				}
			}
		}
		else if( token.name.start[0] == 'i' || token.name.start[0] == 'b' )
		{
			// "i" and "b" banks are not indexed (AFAIK) and can be used directly
			os << token.name;
			if( token.index )
			{
				os << '[' << token.index << ']';
			}
		}
		else
		{
			// "o" registers might need remapping to GLES output variables
			if( token.name.start[0] == 'o' )
			{
				os << outputRemap[token.name];
			}
			else
			{
				os << token.name;
			}
			if( token.index )
			{
				os << '[' << token.index << ']';
			}
		}
		// Prove correct swizzle: extend existing swizzle if needed
		unsigned length = GetRegisterSwizzleDimension( token );
		if( dimension == 0 )
		{
			length = dimension;
		}
		if( token.swizzle )
		{
			os << '.';
			if( dimension < length )
			{
				InlineString swizzle = token.swizzle;
				swizzle.end = swizzle.start + dimension;
				os << swizzle;
			}
			else
			{
				os << token.swizzle;
				for( int i = token.swizzle.end - token.swizzle.start; i < int( dimension ); ++i )
				{
					os << token.swizzle.end[-1];
				}
			}
		}
		else if( length != dimension )
		{
			const char* swizzles = "xyzw";
			InlineString swizzle;
			swizzle.start = swizzles;
			swizzle.end = swizzle.start + dimension;
			os << '.' << swizzle;
		}
		if( token.negate )
		{
			os << ')';
		}
		if( token.modifier == MakeInlineString( "abs" ) )
		{
			os << ')';
		}
	};
	auto PrintRegister2 = [&]( std::ostream& os, const DX9AsmToken& token, const InlineString& destSwizzle )
	{
		if( token.negate )
		{
			os << '(' << token.negate;
		}

		if( token.modifier == MakeInlineString( "abs" ) )
		{
			os << "abs(";
		}
		
		if( token.name.start[0] == 'c' )
		{
			InlineString index = token.name;
			index.start++;
			unsigned i = unsigned( atoi( ToString( index ).c_str() ) );
			// First search local constants
			if( registerInfo.find( 'c' ) != registerInfo.end() && 
				registerInfo['c'].registers.find( token.name ) != registerInfo['c'].registers.end() )
			{
				if( registerInfo['c'].hasIndexed )
				{
					os << "c[" << ( i - registerInfo['c'].indexesBegin );
					if( token.index )
					{
						os << '+' << token.index;
					}
					os << ']';
				}
				else
				{
					os << token.name;
				}
			}
			else
			{
				// If not a local constant, it should be in one of constant buffers
				bool found = false;
				for( int cb = 0; cb < 16; ++cb )
				{
					if( i >= cbs[cb].first && i < cbs[cb].second )
					{
						os << "cb" << cb << '['  << ( i - cbs[cb].first );
						if( token.index )
						{
							os << '+' << token.index;
						}
						os << ']';
						found = true;
						break;
					}
				}
				if( !found )
				{
					assert( false );
				}
			}
		}
		else if( token.name.start[0] == 'i' || token.name.start[0] == 'b' )
		{
			// "i" and "b" banks are not indexed (AFAIK) and can be used directly
			os << token.name;
			if( token.index )
			{
				os << '[' << token.index << ']';
			}
		}
		else
		{
			// "o" registers might need remapping to GLES output variables
			if( token.name.start[0] == 'o' )
			{
				os << outputRemap[token.name];
			}
			else
			{
				os << token.name;
			}
			if( token.index )
			{
				os << '[' << token.index << ']';
			}
		}
		// Prove correct swizzle: extend existing swizzle if needed
		InlineString srcSwizzle;
		if( token.swizzle )
		{
			srcSwizzle = token.swizzle;
		}
		else
		{
			if( token.name.start[0] == 'b' || token.name.start[0] == 'p' )
			{
				srcSwizzle = MakeInlineString( "x" );
			}
			else
			{
				srcSwizzle = MakeInlineString( "xyzw" );
			}
		}
		std::string swizzle;
		if( destSwizzle )
		{
			for( const char* e = destSwizzle.start; e != destSwizzle.end; ++e )
			{
				switch( *e )
				{
				case 'x':
					swizzle += srcSwizzle.start[0];
					break;
				case 'y':
					if( srcSwizzle.end - srcSwizzle.start > 1 )
					{
						swizzle += srcSwizzle.start[1];
					}
					else
					{
						swizzle += srcSwizzle.end[-1];
					}
					break;
				case 'z':
					if( srcSwizzle.end - srcSwizzle.start > 2 )
					{
						swizzle += srcSwizzle.start[2];
					}
					else
					{
						swizzle += srcSwizzle.end[-1];
					}
					break;
				case 'w':
					if( srcSwizzle.end - srcSwizzle.start > 3 )
					{
						swizzle += srcSwizzle.start[3];
					}
					else
					{
						swizzle += srcSwizzle.end[-1];
					}
					break;
				}
			}
		}
		if( !swizzle.empty() && swizzle != "xyzw" )
		{
			os << '.' << swizzle;
		}
		if( token.negate )
		{
			os << ')';
		}
		if( token.modifier == MakeInlineString( "abs" ) )
		{
			os << ')';
		}
	};

	// ----------------------------------------------------------------------------------
	// Description:
	//   Prints comparison operator, according to asm comparison modifier, for two given
	//   registers. 
	// Arguments:
	//   os - Output stream
	//   modifier - Modifier name, needs to be one of comparison modifiers
	//   src0 - Left register token
	//   src1 - Right register token
	// ----------------------------------------------------------------------------------
	auto PrintCompareOperator = [&]( std::ostream& os, 
									 const InlineString& modifier, 
									 const DX9AsmToken& src0, 
									 const DX9AsmToken& src1 )
	{
		// Get register dimensions: comparisons for vectors and scalars are
		// different in GLSL
		int length0 = 4;
		int length1 = 4;
		if( src0.swizzle )
		{
			length0 = src0.swizzle.end - src0.swizzle.start;
		}
		if( src1.swizzle )
		{
			length1 = src1.swizzle.end - src1.swizzle.start;
		}
		int length = min( length0, length1 );
		if( length == 1 )
		{
			os << '(';
			PrintRegister( os, src0, 1 );
			if( modifier == MakeInlineString( "gt" ) )
			{
				os << ">";
			}
			else if( modifier == MakeInlineString( "lt" ) )
			{
				os << "<";
			}
			else if( modifier == MakeInlineString( "ge" ) )
			{
				os << ">=";
			}
			else if( modifier == MakeInlineString( "le" ) )
			{
				os << "<=";
			}
			else if( modifier == MakeInlineString( "eq" ) )
			{
				os << "==";
			}
			else
			{
				os << "!=";
			}
			PrintRegister( os, src1, 1 );
			os << ')';
		}
		else
		{
			if( modifier == MakeInlineString( "gt" ) )
			{
				os << "greaterThan(";
			}
			else if( modifier == MakeInlineString( "lt" ) )
			{
				os << "lessThan(";
			}
			else if( modifier == MakeInlineString( "ge" ) )
			{
				os << "greaterThanEqual(";
			}
			else if( modifier == MakeInlineString( "le" ) )
			{
				os << "lessThanEqual(";
			}
			else if( modifier == MakeInlineString( "eq" ) )
			{
				os << "equal(";
			}
			else
			{
				os << "notEqual(";
			}
			PrintRegister( os, src0, 0 );
			os << ',';
			PrintRegister( os, src1, 0 );
			os << ')';
		}
	};

	// Pixel shader remappings for output registers
	if( stage.type == PIXEL_STAGE )
	{
		outputRemap[MakeInlineString( "oC0" )] = MakeInlineString( "gl_FragData[0]" );
		outputRemap[MakeInlineString( "oC1" )] = MakeInlineString( "gl_FragData[1]" );
		outputRemap[MakeInlineString( "oC2" )] = MakeInlineString( "gl_FragData[2]" );
		outputRemap[MakeInlineString( "oC3" )] = MakeInlineString( "gl_FragData[3]" );
		outputRemap[MakeInlineString( "oDepth" )] = MakeInlineString( "gl_FragDepth" );
	}

	InlineString stream;
	stream.start = source;
	stream.end = source + strlen( source );

	DX9AsmToken command, coma, eoc;

	std::vector<DX9AsmToken> tokens;
	while( true )
	{
		// All asm code has simple grammar: (command [register (comma register)*] endofcommand)*
		if( !GetDX9AsmToken( stream, command ) )
		{
			return false;
		}
		if( command.type == DX9AsmToken::EndOfSteam )
		{
			break;
		}
		tokens.push_back( command );
	}

	// Fist pass: get all used register names; check what register banks are used
	// for indexing ([] operators)
	for( size_t tokenIndex = 0; tokenIndex < tokens.size(); )
	{
		// All asm code has simple grammar: (command [register (comma register)*] endofcommand)*
		command = tokens[tokenIndex++];
		if( command.name == MakeInlineString( "def" ) ||
			command.name == MakeInlineString( "defb" ) ||
			command.name == MakeInlineString( "defi" )
			)
		{
			DX9AsmToken reg;
			EXPECT_TOKEN( stream, reg, DX9AsmToken::ID );
			if( reg.name != MakeInlineString( "vFace" ) && reg.name != MakeInlineString( "vPos" ) )
			{
				InlineString index = reg.name;
				index.start++;
				unsigned newIndex = unsigned( atoi( ToString( index ).c_str() ) );
				auto& info = registerInfo[reg.name.start[0]];
				info.registers.insert( reg.name );
				info.hasIndexed = reg.index;
			}
		}
		while( true )
		{
			DX9AsmToken reg = tokens[tokenIndex++];
			if( reg.type == DX9AsmToken::ID && reg.name.start[0] != 'c' && 
				reg.name != MakeInlineString( "vFace" ) && 
				reg.name != MakeInlineString( "vPos" ) )
			{
				InlineString index = reg.name;
				index.start++;
				unsigned newIndex = unsigned( atoi( ToString( index ).c_str() ) );
				auto& info = registerInfo[reg.name.start[0]];
				info.registers.insert( reg.name );
				info.hasIndexed = reg.index;
			}
			else if( reg.type == DX9AsmToken::ID && reg.name.start[0] == 'c' && reg.index )
			{
				auto info = registerInfo.find( reg.name.start[0] );
				if( info != registerInfo.end() )
				{
					InlineString index = reg.name;
					index.start++;
					//unsigned newIndex = unsigned( atoi( ToString( index ).c_str() ) );
					if( info->second.registers.find( reg.name ) != info->second.registers.end() )
					{
						info->second.hasIndexed = true;
					}
				}
			}
			if( reg.type == DX9AsmToken::CommandEnd )
			{
				break;;
			}
		}
	}

	// Updated index ranges for register banks
	for( auto bank = registerInfo.begin(); bank != registerInfo.end(); ++bank )
	{
		bank->second.indexesBegin = -1;
		bank->second.indexesEnd = 0;
		for( auto reg = bank->second.registers.begin(); reg != bank->second.registers.end(); ++reg )
		{
			InlineString index = *reg;
			index.start++;
			unsigned i = unsigned( atoi( ToString( index ).c_str() ) );
			if( bank->second.indexesBegin > i )
			{
				bank->second.indexesBegin = i;
			}
			if( bank->second.indexesEnd <= i )
			{
				bank->second.indexesEnd = i + 1;
			}
		}
	}

	// Second pass: output actual GLSL
	stream.start = source;
	for( size_t tokenIndex = 0; tokenIndex < tokens.size(); )
	{
		command = tokens[tokenIndex++];
		if( command.type == DX9AsmToken::EndOfSteam )
		{
			break;
		}
		CBR_RETURN_VAL( command.type == DX9AsmToken::ID, false );
		if( command.modifier == MakeInlineString( "sat" ) && !hasSaturate )
		{
			declOs << 
				"float saturate(float x){return clamp(x,0.0,1.0);}" << endl << 
				"vec2 saturate(vec2 x){return clamp(x,vec2(0.0),vec2(1.0));}" << endl << 
				"vec3 saturate(vec3 x){return clamp(x,vec3(0.0),vec3(1.0));}" << endl << 
				"vec4 saturate(vec4 x){return clamp(x,vec4(0.0),vec4(1.0));}" << endl;
			hasSaturate = true;
		}
		if( command.name == MakeInlineString( "abs" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "abs(";
			PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "add" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << '+';
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "break" ) )
		{
			if( IsCompModifier( command.modifier ) )
			{
				DX9AsmToken src0, src1;
				EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
				EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
				EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
				os << "if(";
				PrintCompareOperator( os, command.modifier, src0, src1 );
				os << ")break;" << endl;
			}
			else
			{
				os << "break;" << endl;
			}
		}
		else if( command.name == MakeInlineString( "breakp" ) )
		{
			DX9AsmToken src;
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			os << "if(";
			PrintRegister( os, src, 0 );
			os << ")break;" << endl;
		}
		else if( command.name == MakeInlineString( "call" ) )
		{
			// TODO: procedure call
			DX9AsmToken label;
			EXPECT_TOKEN( stream, label, DX9AsmToken::ID );
			CBR_RETURN_VAL( false, false );
		}
		else if( command.name == MakeInlineString( "callnz" ) )
		{
			// TODO: procedure call
			DX9AsmToken label, src;
			EXPECT_TOKEN( stream, label, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			CBR_RETURN_VAL( false, false );
		}
		else if( command.name == MakeInlineString( "cmp" ) )
		{
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			int length = GetRegisterSwizzleDimension( dst );
			if( length == 1 )
			{
				PrintRegister( os, dst, 0 );
				os << '=';
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
				os << ">=0.0";
				os << "?";
				PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
				os << ':';
				PrintRegister2( os, src2, GetRegisterDestSwizzle( dst ) );
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
			}
			else
			{
				os << "{bvec" << length << " tmp=greaterThanEqual(";
				PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
				os << ",vec" << length << "(0.0));";
				PrintRegister( os, dst, 0 );
				os << '=';
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				os << "vec" << length << "(";
				const char* components = "xyzw";
				InlineString swizzle = dst.swizzle ? dst.swizzle : MakeInlineString( "xyzw" );
				for( int l = 0; l < length; ++l )
				{
					if( l )
					{
						os << ',';
					}
					os << "tmp." << components[l] << '?';
					PrintRegister2( os, src1, MakeInlineString( swizzle.start + l, swizzle.start + l + 1 ) );
					os << ':';
					PrintRegister2( os, src2, MakeInlineString( swizzle.start + l, swizzle.start + l + 1 ) );
				}
				os << ")";
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				os << ";}";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "cnd" ) )
		{
			// TODO: remove cnd, as it's only supported on SM <2.0
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );

			int length = GetRegisterSwizzleDimension( src0 );
			if( length == 1 )
			{
				PrintRegister( os, dst, 0 );
				os << '=';
				if( dst.swizzle )
				{
					os << "vec4(";
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister( os, src0, 1 );
				os << ">0.5";
				os << "?";
				PrintRegister( os, src1, 4 );
				os << ':';
				PrintRegister( os, src2, 4 );
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				if( dst.swizzle )
				{
					os << ")." << dst.swizzle;
				}
			}
			else
			{
				os << "{bvec4 tmp=greaterThan(";
				PrintRegister( os, src0, 4 );
				os << ",vec4(0.5));";
				int length = 4;
				if( dst.swizzle )
				{
					length = dst.swizzle.end - dst.swizzle.start;
				}
				PrintRegister( os, dst, 0 );
				os << '=';
				if( dst.swizzle )
				{
					os << '(';
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				os << "vec4(tmp.x?"; 
				PrintRegister( os, src1, 4 );
				os << ".x:";
				PrintRegister( os, src2, 4 );
				os << ".x,";
				os << "tmp.y?"; 
				PrintRegister( os, src1, 4 );
				os << ".y:";
				PrintRegister( os, src2, 4 );
				os << ".y,";
				os << "tmp.z?"; 
				PrintRegister( os, src1, 4 );
				os << ".z:";
				PrintRegister( os, src2, 4 );
				os << ".z,";
				os << "tmp.w?"; 
				PrintRegister( os, src1, 4 );
				os << ".w:";
				PrintRegister( os, src2, 4 );
				os << ".w)";
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				if( dst.swizzle )
				{
					os << ")." << dst.swizzle;
				}
				os << ";}";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "crs" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "cross(";
			PrintRegister2( os, src0, MakeInlineString( "xyz" ) );
			os << ',';
			PrintRegister2( os, src1, MakeInlineString( "xyz" ) );
			os << ')';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << "." << dst.swizzle;
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dcl" ) )
		{
			DX9AsmToken reg;
			EXPECT_TOKEN( stream, reg, DX9AsmToken::ID );
			if( command.modifier == MakeInlineString( "2d" ) )
			{
				samplerTypes[reg.name] = "2D";
				declOs << "uniform sampler2D ";
				if( stage.type == VERTEX_STAGE )
				{
					declOs << 'v';
				}
				declOs << reg.name << ';' << endl;
			}
			else if( command.modifier == MakeInlineString( "cube" ) )
			{
				samplerTypes[reg.name] = "Cube";
				declOs << "uniform samplerCube ";
				if( stage.type == VERTEX_STAGE )
				{
					declOs << 'v';
				}
				declOs << reg.name << ';' << endl;
			}
			else if( command.modifier == MakeInlineString( "volume" ) )
			{
				switch( g_glesExtensions.Supports( "OES_texture_3D" ) )
				{
				case GlesExtensionInfo::WARN:
					usedExtensions["OES_texture_3D"] = GlesExtensionInfo::WARN;
					if( g_printWarnings )
					{
						g_messages.AddMessage( "\\memory(0): warning X0000: volume textures require OES_texture_3D GLES extension" );
					}
					break;
				case GlesExtensionInfo::DISABLE:
					g_messages.AddMessage( "\\memory(0): error X0000: volume textures require OES_texture_3D GLES extension" );
					return false;
				default:
					usedExtensions["OES_texture_3D"] = GlesExtensionInfo::ENABLE;
				}
				hasTexture3DExtension = true;
				samplerTypes[reg.name] = "3D";
				declOs << "uniform sampler3D ";
				if( stage.type == VERTEX_STAGE )
				{
					declOs << 'v';
				}
				declOs << reg.name << ';' << endl;
				declOs << "#ifndef GL_OES_texture_3D" << endl << "uniform float " << reg.name << "sl;" << endl << 
					"#else" << endl << "#define " << reg.name << "sl 0.0" << endl <<
					"#endif" << endl;
			}
			else if( reg.name == MakeInlineString( "vFace" ) )
			{
				os << "vec4 vFace = gl_FrontFacing ? vec4(1.0) : vec4(-1.0);" << endl;
			}
			else if( reg.name == MakeInlineString( "vPos" ) )
			{
				os << "vec4 vPos = gl_FragCoord;" << endl;
			}
			else if( reg.name.start[0] == 'o' && stage.type == VERTEX_STAGE )
			{
				outputRemap[reg.name] = command.modifier == MakeInlineString( "position" ) ? MakeInlineString( "gl_Position" ) : command.modifier;
				if( command.modifier != MakeInlineString( "position" ) )
				{
					declOs << "varying vec4 " << command.modifier << ';' << endl;
				}
			}
			else if( stage.type == VERTEX_STAGE )
			{
				InlineString name = reg.name;
				name.start++;
				declOs << "attribute vec4 attr" << name << ';' << endl;
				os << reg.name << "=attr" << name << ';' << endl;
			}
			else // PIXEL_STAGE input
			{
				declOs << "varying vec4 " << command.modifier << ';' << endl;
				os << reg.name << "=" << command.modifier << ';' << endl;
			}
		}
		else if( command.name == MakeInlineString( "def" ) )
		{
			DX9AsmToken dst, src0, src1, src2, src3;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src3, DX9AsmToken::Number );
			if( !registerInfo['c'].hasIndexed )
			{
				os << "vec4 ";
			}
			PrintRegister( os, dst, 0 );
			os << "=vec4(" << src0.name << ',' << src1.name << ',' << src2.name << ',' << src3.name << ");" << endl;
		}
		else if( command.name == MakeInlineString( "defb" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			os << "bool ";
			PrintRegister( os, dst, 0 );
			os << "=" << src.name << ";" << endl;
		}
		else if( command.name == MakeInlineString( "defi" ) )
		{
			DX9AsmToken dst, src0, src1, src2, src3;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::Number );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src3, DX9AsmToken::Number );
			os << "ivec4 ";
			PrintRegister( os, dst, 0 );
			os << "=ivec4(" << src0.name << ',' << src1.name << ',' << src2.name << ',' << src3.name << ");" << endl;
		}
		else if( command.name == MakeInlineString( "dp2add" ) )
		{
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "dot(";
			PrintRegister2( os, src0, MakeInlineString( "xy" ) );
			os << ",";
			PrintRegister2( os, src1, MakeInlineString( "xy" ) );
			os << ")+";
			PrintRegister( os, src2, 0 );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dim > 1 )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dp3" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "clamp(";
			}
			os << "dot(";
			PrintRegister2( os, src0, MakeInlineString( "xyz" ) );
			os << ",";
			PrintRegister2( os, src1, MakeInlineString( "xyz" ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ",0.0, 1.0)";
			}
			if( dim > 1 )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dp4" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "clamp(";
			}
			os << "dot(";
			PrintRegister2( os, src0, MakeInlineString( "xyzw" ) );
			os << ",";
			PrintRegister2( os, src1, MakeInlineString( "xyzw" ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ",0.0, 1.0)";
			}
			if( dim > 1 )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dsx" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			switch( g_glesExtensions.Supports( "OES_standard_derivatives" ) )
			{
			case GlesExtensionInfo::WARN:
				usedExtensions["OES_standard_derivatives"] = GlesExtensionInfo::WARN;
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: derivative operations (ddx, ddy) require OES_standard_derivatives GLES extension" );
				}
				break;
			case GlesExtensionInfo::DISABLE:
				g_messages.AddMessage( "\\memory(0): error X0000: derivative operations (ddx, ddy) require OES_standard_derivatives GLES extension" );
				return false;
			default:
				usedExtensions["OES_standard_derivatives"] = GlesExtensionInfo::ENABLE;
			}
			os << "dFdx(";
			PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
			os << ")";
			hasDerivativesExtension = true;
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dsy" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			switch( g_glesExtensions.Supports( "OES_standard_derivatives" ) )
			{
			case GlesExtensionInfo::WARN:
				usedExtensions["OES_standard_derivatives"] = GlesExtensionInfo::WARN;
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: derivative operations (ddx, ddy) require OES_standard_derivatives GLES extension" );
				}
				break;
			case GlesExtensionInfo::DISABLE:
				g_messages.AddMessage( "\\memory(0): error X0000: derivative operations (ddx, ddy) require OES_standard_derivatives GLES extension" );
				return false;
			default:
				usedExtensions["OES_standard_derivatives"] = GlesExtensionInfo::ENABLE;
			}
			os << "dFdy(";
			PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "dst" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "=vec4(1.0,";
			PrintRegister( os, src0, 4 );
			os << ".y*";
			PrintRegister( os, src1, 4 );
			os << ".y,";
			PrintRegister( os, src0, 4 );
			os << ".z,";
			PrintRegister( os, src1, 4 );
			os << ".w)";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << "." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "else" ) )
		{
			os << "}else{" << endl;
		}
		else if( command.name == MakeInlineString( "endif" ) )
		{
			os << "}" << endl;
		}
		else if( command.name == MakeInlineString( "endloop" ) )
		{
			os << "}" << endl;
		}
		else if( command.name == MakeInlineString( "endrep" ) )
		{
			os << "}" << endl;
		}
		else if( command.name == MakeInlineString( "exp" ) ||
			command.name == MakeInlineString( "expp" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "exp2(";
			PrintRegister2( os, src, MakeInlineString( "x" ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dim > 1 )
			{
				os << ')';
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "frc" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "fract(";
			PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "if" ) )
		{
			if( IsCompModifier( command.modifier ) )
			{
				DX9AsmToken src0, src1;
				EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
				EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
				EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
				os << "if(";
				PrintCompareOperator( os, command.modifier, src0, src1 );
				os << "){" << endl;
			}
			else
			{
				DX9AsmToken reg;
				EXPECT_TOKEN( stream, reg, DX9AsmToken::ID );
				os << "if(";
				PrintRegister( os, reg, 0 );
				os << "){" << endl;
			}
		}
		else if( command.name == MakeInlineString( "label" ) )
		{
			// TODO: procedure call
			DX9AsmToken label;
			EXPECT_TOKEN( stream, label, DX9AsmToken::ID );
			CBR_RETURN_VAL( false, false );
		}
		else if( command.name == MakeInlineString( "lit" ) )
		{
			if( !hasLit )
			{
				declOs << "vec4 lit(vec4 src){"
					"{vec4 dest=vec4(1.0,0.0,0.0,1.0);"
					"float power=clamp(src.w,-127.9961,127.9961);"
					"if(src.x>0.0){"
						"dest.y=src.x;"
						"if(src.y>0.0){"
							"dest.z=pow(src.y,power);"
						"}"
					"}"
					"}" << endl;
				hasLit = true;
			}
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "lit(";
			PrintRegister( os, src, 4 );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "log" ) ||
			command.name == MakeInlineString( "logp" ) )
		{
			/*
			... CommandEnd ID(mul) ID(dst) Coma ID(dst)
			... CommandEnd ID("exp") ID Coma ID(dst)
			*/
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );

			bool lvalueUse = false;
			bool foundMul = false;
			size_t mulIndex, expIndex;
			bool foundExp = false;
			for( size_t t = tokenIndex; t < tokens.size(); ++t )
			{
				if( tokens[t].type == DX9AsmToken::ID && tokens[t].name == dst.name && tokens[t].swizzle == dst.swizzle )
				{
					if( tokens[t].swizzle == dst.swizzle )
					{
						if( !foundMul )
						{
							if( t > 2 && t + 2 < tokens.size() &&
								tokens[t - 1].type == DX9AsmToken::ID && tokens[t - 1].name == MakeInlineString( "mul" ) &&
								tokens[t - 2].type == DX9AsmToken::CommandEnd &&
								tokens[t + 1].type == DX9AsmToken::Coma &&
								tokens[t + 2].type == DX9AsmToken::ID && tokens[t + 2].name == dst.name && tokens[t + 2].swizzle == dst.swizzle )
							{
								foundMul = true;
								mulIndex = t - 1;
								t += 2;
								continue;
							}
						}
						else
						{
							if( t > 4 &&
								tokens[t - 1].type == DX9AsmToken::Coma &&
								tokens[t - 2].type == DX9AsmToken::ID &&
								tokens[t - 3].type == DX9AsmToken::ID && tokens[t - 3].name == MakeInlineString( "exp" ) &&
								tokens[t - 4].type == DX9AsmToken::CommandEnd )
							{
								foundExp = true;
								expIndex = t - 3;
								continue;
							}
							if( t > 2 &&
								tokens[t - 1].type == DX9AsmToken::ID && tokens[t - 1].name == MakeInlineString( "exp" ) &&
								tokens[t - 2].type == DX9AsmToken::CommandEnd &&
								tokens[t + 1].type == DX9AsmToken::Coma &&
								tokens[t + 2].type == DX9AsmToken::ID && tokens[t + 2].name == dst.name && tokens[t + 2].swizzle == dst.swizzle )
							{
								foundExp = true;
								expIndex = t - 1;
								continue;
							}
						}
					}
					if( tokens[t].swizzle == dst.swizzle || tokens[t].swizzle.start == tokens[t].swizzle.end || strchr( ToString( tokens[t].swizzle ).c_str(), *dst.swizzle.start ) )
					{
						if( tokens[t - 1].type != DX9AsmToken::Coma )
						{
							lvalueUse = true;
						}
						break;
					}
				}
			}
			if( !lvalueUse && foundMul && foundExp )
			{
				char* tmpName = new char[64];
				sprintf_s( tmpName, 64, "tmp%u", tmpCount++ );
				localsOs << "vec4 " << tmpName << ";" << endl;
				tokens[mulIndex].name = MakeInlineString( "mov" );
				tokens[mulIndex + 1].name = MakeInlineString( tmpName );
				tokens[expIndex].name = MakeInlineString( "pow" );
				tokens.insert( tokens.begin() + expIndex + 4, tokens[expIndex + 2] );
				tokens.insert( tokens.begin() + expIndex + 5, tokens[mulIndex + 1] );
				tokens.erase( tokens.begin() + mulIndex + 3 );
				tokens.erase( tokens.begin() + mulIndex + 3 );
			}
			else
			{
				PrintRegister( os, dst, 0 );
				os << "=";
				int dim = GetRegisterSwizzleDimension( dst );
				if( dim > 1 )
				{
					os << "vec" << dim << '(';
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister2( os, src, MakeInlineString( "x" ) );
				os << ">0.0?";
				os << "log2(";
				PrintRegister2( os, src, MakeInlineString( "x" ) );
				os << ")";
				os << ":-3.402823466e+38";
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				if( dim > 1 )
				{
					os << ')';
				}
				os << ";" << endl;
			}
		}
		else if( command.name == MakeInlineString( "loop" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			os << "for(int i=";
			PrintRegister( os, src, 0 );
			os << ".x,aL=";
			PrintRegister( os, src, 0 );
			os << ".y;i>0;--i,aL+=";
			PrintRegister( os, src, 0 );
			os << ".z){" << endl;
		}
		else if( command.name == MakeInlineString( "lrp" ) )
		{
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "mix(";
			PrintRegister2( os, src2, GetRegisterDestSwizzle( dst ) );
			os << ",";
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			os << ",";
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "m3x2" ) ||
			command.name == MakeInlineString( "m3x3" ) ||
			command.name == MakeInlineString( "m3x4" ) ||
			command.name == MakeInlineString( "m4x3" ) ||
			command.name == MakeInlineString( "m4x4" ) )
		{
			// TODO: not used by HLSL compiler?
			CBR_RETURN_VAL( false, false );
		}
		else if( command.name == MakeInlineString( "mad" ) )
		{
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << "*";
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			os << "+";
			PrintRegister2( os, src2, GetRegisterDestSwizzle( dst ) );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "max" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "max(";
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << ",";
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "min" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "min(";
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << ",";
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "mov" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( dst.name.start[0] == 'a' )
			{
				int dim = GetRegisterSwizzleDimension( dst );
				if( dim == 1 )
				{
					os << "int(";
				}
				else
				{
					os << "ivec" << dim << "(";
				}
				PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
				os << '+';
				if( dim == 1 )
				{
					os << "0.5";
				}
				else
				{
					os << "vec" << dim << "(0.5)";
				}
				os << ')';
			}
			else
			{
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "mova" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim == 1 )
			{
				os << "int(";
				PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
				os << "+0.5)";
			}
			else
			{
				os << "ivec" << dim << '(';
				PrintRegister2( os, src, GetRegisterDestSwizzle( dst ) );
				os << "+vec" << dim << "(0.5))";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "mul" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << '*';
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "nop" ) )
		{
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "nrm" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "normalize(";
			PrintRegister2( os, src, MakeInlineString( "xyz" ) );
			os << ")";
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "pow" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim  << "(";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "pow(";
			PrintRegister2( os, src0, MakeInlineString( "x" ) );
			os << ',';
			PrintRegister2( os, src1, MakeInlineString( "x" ) );
			os << ')';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dim > 1 )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "rcp" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim  << "(";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			os << "1.0/";
			PrintRegister2( os, src, MakeInlineString( "x" ) );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dim > 1 )
			{
				os << ")";
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "rep" ) )
		{
			DX9AsmToken src;
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );

			if( stage.type == PIXEL_STAGE )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( 
						"\\memory(0): warning X0000: loops inside pixel shaders are not permitted in GLES (but will work fine in desktop GL)" );
				}
			}

			os << "for(int i=0;i<";
			PrintRegister( os, src, 1 );
			os << ";++i){" << endl;
		}
		else if( command.name == MakeInlineString( "ret" ) )
		{
			// TODO: procedure call
			CBR_RETURN_VAL( false, false );
		}
		else if( command.name == MakeInlineString( "rsq" ) )
		{
			DX9AsmToken dst, src;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );

			PrintRegister( os, dst, 0 );
			os << "=";
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				os << "vec" << dim  << "(";
			}
			// Common pattern: x = 1/sqrt(y); x = 1/x;
			if( tokenIndex + 4 < tokens.size() &&
				command.modifier != MakeInlineString( "sat" ) &&
				tokens[tokenIndex].type == DX9AsmToken::CommandEnd && 
				tokens[tokenIndex + 1].type == DX9AsmToken::ID && tokens[tokenIndex + 1].name == MakeInlineString( "rcp" ) &&
				tokens[tokenIndex + 2].type == DX9AsmToken::ID && tokens[tokenIndex + 2].name == dst.name && tokens[tokenIndex + 2].swizzle == dst.swizzle &&
				tokens[tokenIndex + 3].type == DX9AsmToken::Coma && 
				tokens[tokenIndex + 4].type == DX9AsmToken::ID && tokens[tokenIndex + 4].name == dst.name && tokens[tokenIndex + 4].swizzle == dst.swizzle )
			{
				if( tokens[tokenIndex + 1].modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				os << "sqrt(abs(";
				PrintRegister2( os, src, MakeInlineString( "x" ) );
				os << "))";
				if( tokens[tokenIndex + 1].modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				tokenIndex += 5;
			}
			else
			{
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister2( os, src, MakeInlineString( "x" ) );
				os << "==0.0?3.402823466e+38:";
				os << "inversesqrt(abs(";
				PrintRegister2( os, src, MakeInlineString( "x" ) );
				os << "))";
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
			}
			if( dim > 1 )
			{
				os << ")";
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "setp" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			PrintCompareOperator( os, command.modifier, src0, src1 );
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "sge" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );

			int length0 = GetRegisterSwizzleDimension( src0 );
			int length1 = GetRegisterSwizzleDimension( src1 );
			if( length0 == 1 && length1 == 1 )
			{
				PrintRegister( os, dst, 0 );
				os << '=';
				PrintRegister( os, src0, 0 );
				os << ">=";
				PrintRegister( os, src1, 0 );
				int outLength = GetRegisterSwizzleDimension( dst );
				if( outLength == 1 )
				{
					os << "?1.0:0.0;";
				}
				else
				{
					os << "?vec" << outLength << "(1.0):vec" << outLength << "(0.0);";
				}
			}
			else
			{
				os << "{bvec4 tmp=greaterThanEqual(";
				PrintRegister( os, src0, 4 );
				os << ",";
				PrintRegister( os, src1, 4 );
				os << ");";
				PrintRegister( os, dst, 0 );
				os << '=';
				if( dst.swizzle )
				{
					os << '(';
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				os << "vec4(tmp.x?1.0:0.0,tmp.y?1.0:0.0,tmp.z?1.0:0.0,tmp.w?1.0:0.0)"; 
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				if( dst.swizzle )
				{
					os << ")." << dst.swizzle;
				}
				os << ";}";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "sgn" ) )
		{
			DX9AsmToken dst, src0, src1, src2;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			int dim = GetRegisterSwizzleDimension( dst );
			if( dim > 1 )
			{
				PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
				os << "<0.0?-1:(";
				PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
				os << "==0.0?0.0:1.0)";
			}
			else
			{
				InlineString swizzle = GetRegisterDestSwizzle( dst );
				os << "vec" << dim << '(';
				for( const char* c = swizzle.start; c != swizzle.end; ++c )
				{
					if( c != swizzle.start )
					{
						os << ',';
					}
					PrintRegister2( os, src0, MakeInlineString( c, c + 1 ) );
					os << "<0.0?-1:(";
					PrintRegister2( os, src0, MakeInlineString( c, c + 1 ) );
					os << "==0.0?0.0:1.0)";
				}
				os << ')';
			}
			os << ";" << endl;
		}
		else if( command.name == MakeInlineString( "sincos" ) )
		{
			DX9AsmToken dst, src0;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			if( dst.swizzle == MakeInlineString( "x" ) )
			{
				os << "=cos(";
				PrintRegister( os, src0, 0 );
			}
			else if( dst.swizzle == MakeInlineString( "y" ) )
			{
				os << "=sin(";
				PrintRegister( os, src0, 0 );
			}
			else
			{
				os << "=vec2(cos(";
				PrintRegister( os, src0, 0 );
				os << ")"; 
				os << ", sin(";
				PrintRegister( os, src0, 0 );
				os << ")";
			}
			os << ");" << endl;
		}
		else if( command.name == MakeInlineString( "slt" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );

			int dim = GetRegisterSwizzleDimension( dst );
			int length0 = GetRegisterSwizzleDimension( src0 );
			int length1 = GetRegisterSwizzleDimension( src1 );
			if( length0 == 1 && length1 == 1 )
			{
				PrintRegister( os, dst, 0 );
				os << '=';
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				PrintRegister( os, src0, 0 );
				os << "<";
				PrintRegister( os, src1, 0 );
				if( dim == 1 )
				{
					os << "?1.0:0.0";
				}
				else
				{
					os << "?vec" << dim << "(1.0):vec" << dim << "(0.0)";
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
			}
			else
			{
				os << "{bvec4 tmp=lessThan(";
				PrintRegister( os, src0, 4 );
				os << ",";
				PrintRegister( os, src1, 4 );
				os << ");";
				PrintRegister( os, dst, 0 );
				os << '=';
				if( dst.swizzle )
				{
					os << '(';
				}
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << "saturate(";
				}
				os << "vec4(tmp.x?1.0:0.0,tmp.y?1.0:0.0,tmp.z?1.0:0.0,tmp.w?1.0:0.0)"; 
				if( command.modifier == MakeInlineString( "sat" ) )
				{
					os << ")";
				}
				if( dst.swizzle )
				{
					os << ")." << dst.swizzle;
				}
				os << ";}";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "sub" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << '=';
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			PrintRegister2( os, src0, GetRegisterDestSwizzle( dst ) );
			os << '-';
			PrintRegister2( os, src1, GetRegisterDestSwizzle( dst ) );
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "texkill" ) )
		{
			DX9AsmToken src;
			EXPECT_TOKEN( stream, src, DX9AsmToken::ID );
			os << "if(any(lessThan(";
			PrintRegister( os, src, 4 );
			os << ",vec4(0.0))))discard;" << endl;
		}
		else if( command.name == MakeInlineString( "texldl" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			bool srgbFunc = false;
			if( g_glesEmulateSampler )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				if( sampler.srgbTexture )
				{
					os << "g2l(";
					srgbFunc = true;
					hasGammaToLinear = true;
				}
			}
			bool borderFunc = false;
			if( g_glesEmulateSampler && samplerTypes[src1.name] != "Cube" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				std::string borders;
				if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "U";
				}
				if( sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "V";
				}
				if( sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER && samplerTypes[src1.name] != "3D" )
				{
					borders += "W";
				}
				if( !borders.empty() )
				{
					borderFunc = true;
					hasBorderFuncs = true;
					os << "ssb" << borders << '(';
					PrintRegister( os, src0, 4 );
					os << ",vec4(" << std::setiosflags( std::ios_base::showpoint ) << sampler.borderColor.x << ',' << sampler.borderColor.y << ',' <<
						sampler.borderColor.z << ',' << sampler.borderColor.w << "),";
				}
			}
			if( stage.type != VERTEX_STAGE )
			{
				switch( g_glesExtensions.Supports( "EXT_shader_texture_lod" ) )
				{
				case GlesExtensionInfo::WARN:
					usedExtensions["EXT_shader_texture_lod"] = GlesExtensionInfo::WARN;
					if( g_printWarnings )
					{
						g_messages.AddMessage( "\\memory(0): warning X0000: tex%slod function in fragment shaders requires EXT_shader_texture_lod GLES extension", samplerTypes[src1.name].c_str() );
					}
					break;
				case GlesExtensionInfo::DISABLE:
					g_messages.AddMessage( "\\memory(0): error X0000: tex%slod function in fragment shaders requires EXT_shader_texture_lod GLES extension", samplerTypes[src1.name].c_str() );
					return false;
				default:
					usedExtensions["EXT_shader_texture_lod"] = GlesExtensionInfo::ENABLE;
				}
				hasTextureLodExtension = true;
			}
			if( samplerTypes[src1.name] == "3D" )
			{
				hasTextureLodExtension = true;
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1) )->second;
				os << "tex3DLod";
				os << '(' << ( stage.type == VERTEX_STAGE ? "v" : "" ) << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ',' << src1.name << "sl,";
				os << (sampler.addressV == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.addressW == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.minFilter == D3DTEXF_LINEAR || sampler.magFilter == D3DTEXF_LINEAR ? "true" : "false" ) << ",";
				PrintRegister( os, src0, 4 );
				os << ".w";
				os << ")";
			}
			else
			{
				os << "texture" << samplerTypes[src1.name] << "Lod";
				os << '(' << ( stage.type == VERTEX_STAGE ? "v" : "" ) << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ',';
				PrintRegister( os, src0, 4 );
				os << ".w";
				os << ")";
			}
			if( borderFunc )
			{
				os << ')';
			}
			if( srgbFunc )
			{
				os << ")";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "texld" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			bool srgbFunc = false;
			if( g_glesEmulateSampler )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				if( sampler.srgbTexture )
				{
					os << "g2l(";
					srgbFunc = true;
					hasGammaToLinear = true;
				}
			}
			bool borderFunc = false;
			if( g_glesEmulateSampler && samplerTypes[src1.name] != "Cube" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1) )->second;
				std::string borders;
				if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "U";
				}
				if( sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "V";
				}
				if( sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER && samplerTypes[src1.name] != "3D" )
				{
					borders += "W";
				}
				if( !borders.empty() )
				{
					borderFunc = true;
					hasBorderFuncs = true;
					os << "ssb" << borders << '(';
					PrintRegister( os, src0, 4 );
					os << ",vec4(" << std::setiosflags( std::ios_base::showpoint ) << sampler.borderColor.x << ',' << sampler.borderColor.y << ',' <<
						sampler.borderColor.z << ',' << sampler.borderColor.w << "),";
				}
			}
			if( samplerTypes[src1.name] == "3D" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1) )->second;
				os << "tex3D";
				os << '(' << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ',' << src1.name << "sl,";
				os << (sampler.addressV == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.addressW == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.minFilter == D3DTEXF_LINEAR || sampler.magFilter == D3DTEXF_LINEAR ? "true" : "false" ) << ",0.0";
				os << ")";
			}
			else
			{
				os << "texture" << samplerTypes[src1.name];
				os << '(' << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ")";
			}
			if( borderFunc )
			{
				os << ')';
			}
			if( srgbFunc )
			{
				os << ")";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "texldd" ) )
		{
			DX9AsmToken dst, src0, src1, src2, src3;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src2, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src3, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			bool srgbFunc = false;
			if( g_glesEmulateSampler )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				if( sampler.srgbTexture )
				{
					os << "g2l(";
					srgbFunc = true;
					hasGammaToLinear = true;
				}
			}
			bool borderFunc = false;
			if( g_glesEmulateSampler && samplerTypes[src1.name] != "Cube" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				std::string borders;
				if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "U";
				}
				if( sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "V";
				}
				if( sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER && samplerTypes[src1.name] != "3D" )
				{
					borders += "W";
				}
				if( !borders.empty() )
				{
					borderFunc = true;
					hasBorderFuncs = true;
					os << "ssb" << borders << '(';
					PrintRegister( os, src0, 4 );
					os << ",vec4(" << std::setiosflags( std::ios_base::showpoint ) << sampler.borderColor.x << ',' << sampler.borderColor.y << ',' <<
						sampler.borderColor.z << ',' << sampler.borderColor.w << "),";
				}
			}
			os << "texture" << samplerTypes[src1.name] << "Grad";
			switch( g_glesExtensions.Supports( "EXT_shader_texture_lod" ) )
			{
			case GlesExtensionInfo::WARN:
				usedExtensions["EXT_shader_texture_lod"] = GlesExtensionInfo::WARN;
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: tex%sgrad function requires EXT_shader_texture_lod GLES extension", samplerTypes[src1.name].c_str() );
				}
				break;
			case GlesExtensionInfo::DISABLE:
				g_messages.AddMessage( "\\memory(0): error X0000: tex%sgrad function requires EXT_shader_texture_lod GLES extension", samplerTypes[src1.name].c_str() );
				return false;
			default:
				usedExtensions["EXT_shader_texture_lod"] = GlesExtensionInfo::ENABLE;
			}
			hasTextureLodExtension = true;
			os << '(' << src1.name << ',';
			PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
			os << ',';
			PrintRegister( os, src2, 2 );
			os << ',';
			PrintRegister( os, src3, 2 );
			os << ")";
			if( borderFunc )
			{
				os << ')';
			}
			if( srgbFunc )
			{
				os << ")";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "texldb" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			bool srgbFunc = false;
			if( g_glesEmulateSampler )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				if( sampler.srgbTexture )
				{
					os << "g2l(";
					srgbFunc = true;
					hasGammaToLinear = true;
				}
			}
			bool borderFunc = false;
			if( g_glesEmulateSampler && samplerTypes[src1.name] != "Cube" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				std::string borders;
				if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "U";
				}
				if( sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "V";
				}
				if( sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER && samplerTypes[src1.name] != "3D" )
				{
					borders += "W";
				}
				if( !borders.empty() )
				{
					borderFunc = true;
					hasBorderFuncs = true;
					os << "ssb" << borders << '(';
					PrintRegister( os, src0, 4 );
					os << ",vec4(" << std::setiosflags( std::ios_base::showpoint ) << sampler.borderColor.x << ',' << sampler.borderColor.y << ',' <<
						sampler.borderColor.z << ',' << sampler.borderColor.w << "),";
				}
			}
			if( samplerTypes[src1.name] == "3D" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1) )->second;
				os << "tex3D";
				os << '(' << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ',' << src1.name << "sl,";
				os << (sampler.addressV == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.addressW == D3D11_TEXTURE_ADDRESS_WRAP ? "true" : "false" ) << ',';
				os << (sampler.minFilter == D3DTEXF_LINEAR || sampler.magFilter == D3DTEXF_LINEAR ? "true" : "false" ) << ",";
				PrintRegister( os, src0, 4 );
				os << ".w";
				os << ")";
			}
			else
			{
				os << "texture" << samplerTypes[src1.name];
				os << '(' << src1.name << ',';
				PrintRegister( os, src0, samplerTypes[src1.name] == "2D" ? 2 : 3 );
				os << ",";
				PrintRegister( os, src0, 4 );
				os << ".w)";
			}
			if( borderFunc )
			{
				os << ')';
			}
			if( srgbFunc )
			{
				os << ")";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "texldp" ) )
		{
			DX9AsmToken dst, src0, src1;
			EXPECT_TOKEN( stream, dst, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src0, DX9AsmToken::ID );
			EXPECT_TOKEN( stream, coma, DX9AsmToken::Coma );
			EXPECT_TOKEN( stream, src1, DX9AsmToken::ID );
			PrintRegister( os, dst, 0 );
			os << "=";
			if( dst.swizzle )
			{
				os << '(';
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << "saturate(";
			}
			bool srgbFunc = false;
			if( g_glesEmulateSampler )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				if( sampler.srgbTexture )
				{
					os << "g2l(";
					srgbFunc = true;
					hasGammaToLinear = true;
				}
			}
			bool borderFunc = false;
			if( g_glesEmulateSampler && samplerTypes[src1.name] != "Cube" )
			{
				const Sampler& sampler = stage.samplers.find( atoi( ToString(src1.name).c_str() + 1 ) )->second;
				std::string borders;
				if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "U";
				}
				if( sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER )
				{
					borders += "V";
				}
				if( sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER && samplerTypes[src1.name] != "3D" )
				{
					borders += "W";
				}
				if( !borders.empty() )
				{
					borderFunc = true;
					hasBorderFuncs = true;
					os << "ssb" << borders << '(';
					PrintRegister( os, src0, 4 );
					os << ",vec4(" << std::setiosflags( std::ios_base::showpoint ) << sampler.borderColor.x << ',' << sampler.borderColor.y << ',' <<
						sampler.borderColor.z << ',' << sampler.borderColor.w << "),";
				}
			}
			os << "texture" << samplerTypes[src1.name] << "Proj";
			os << '(' << src1.name << ',';
			PrintRegister( os, src0, 4 );
			os << ".xyw)";
			if( borderFunc )
			{
				os << ')';
			}
			if( srgbFunc )
			{
				os << ")";
			}
			if( command.modifier == MakeInlineString( "sat" ) )
			{
				os << ")";
			}
			if( dst.swizzle )
			{
				os << ")." << dst.swizzle;
			}
			os << ';' << endl;
		}
		else if( command.name == MakeInlineString( "vs" ) ||
			command.name == MakeInlineString( "ps" ) )
		{
		}
		else
		{
			g_messages.AddMessage( 
				"\\memory(0): error X0000: internal error - unregognized asm command %s", 
				ToString( command.name ).c_str() );
			return false;
		}
		EXPECT_TOKEN( stream, eoc, DX9AsmToken::CommandEnd );
	}

	// Add #extension/precision crap (needs to be at the top of the code)
	std::ostrstream prefixOs;
	if( hasDerivativesExtension )
	{
		if( g_glesExtensions.Supports( "OES_standard_derivatives" ) == GlesExtensionInfo::ENABLE )
		{
			prefixOs << "#extension GL_OES_standard_derivatives: enable\n";
		}
		else
		{
			prefixOs <<
				"#ifdef GL_OES_standard_derivatives\n"
				"#extension GL_OES_standard_derivatives: enable\n"
				"#endif\n";
		}
	}
	if( hasTextureLodExtension )
	{
		if( g_glesExtensions.Supports( "EXT_shader_texture_lod" ) == GlesExtensionInfo::ENABLE )
		{
			prefixOs << "#extension GL_EXT_shader_texture_lod: enable\n";
		}
		else
		{
			prefixOs <<
				"#if defined(GL_EXT_shader_texture_lod)\n"
				"#extension GL_EXT_shader_texture_lod: enable\n"
				"#define texture2DLod texture2DLodEXT\n"
				"#define texture2DProjLod texture2DProjLodEXT\n"
				"#define textureCubeLod textureCubeLodEXT\n"
				"#define texture2DGrad texture2DGradEXT\n"
				"#define texture2DProjGrad texture2DProjGradEXT\n"
				"#define textureCubeGrad textureCubeGradEXT\n"
				"#elif defined(EXT_shader_texture_lod)\n"
				"#extension EXT_shader_texture_lod: enable\n"
				"#define texture2DLod texture2DLodEXT\n"
				"#define texture2DProjLod texture2DProjLodEXT\n"
				"#define textureCubeLod textureCubeLodEXT\n"
				"#define texture2DGrad texture2DGradEXT\n"
				"#define texture2DProjGrad texture2DProjGradEXT\n"
				"#define textureCubeGrad textureCubeGradEXT\n"
				"#elif defined(GL_ARB_shader_texture_lod)\n"
				"#extension GL_ARB_shader_texture_lod: enable\n"
				"#define texture2DGrad texture2DGradARB\n"
				"#endif\n";
		}
	}
	if( hasTexture3DExtension )
	{
		if( g_glesExtensions.Supports( "OES_texture_3D" ) == GlesExtensionInfo::ENABLE )
		{
			prefixOs << "#extension GL_OES_texture_3D: enable\n";
		}
		else
		{
			prefixOs <<
				"#ifdef GL_OES_texture_3D\n"
				"#extension GL_OES_texture_3D: enable\n"
				"#endif\n";
		}
	}
	if( stage.type == PIXEL_STAGE )
	{
		prefixOs << 
			"#ifdef GL_ES\n"
			"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
			"precision highp float;\n"
			"#else\n"
			"precision mediump float;\n"
			"#endif\n"
			"#endif\n";
	}
	if( hasDerivativesExtension && g_glesExtensions.Supports( "OES_standard_derivatives" ) != GlesExtensionInfo::ENABLE )
	{
		prefixOs <<
			"#if defined(GL_ES)&&!defined(GL_OES_standard_derivatives)\n"
			"float dd(float x){return 0.0;}\n"
			"vec2 dd(vec2 x){return vec2(0.0);}\n"
			"vec3 dd(vec3 x){return vec3(0.0);}\n"
			"vec4 dd(vec4 x){return vec4(0.0);}\n"
			"#define dFdx(x) dd(x)\n"
			"#define dFdy(x) dd(x)\n"
			"#endif\n";
	}
	if( hasTextureLodExtension && g_glesExtensions.Supports( "EXT_shader_texture_lod" ) != GlesExtensionInfo::ENABLE )
	{
		prefixOs <<
			"#if defined(GL_ES)&&!defined(GL_EXT_shader_texture_lod)&&!defined(EXT_shader_texture_lod)\n"
			"#define texture2DLod(s,u,l) texture2D(s,u)\n"
			"#define textureCubeLod(s,u,l) textureCube(s,u)\n"
			"#define texture2DGrad(s,u,x,y) texture2D(s,u)\n"
			"#define textureCubeGrad(s,u,x,y) textureCube(s,u)\n"
			"#endif\n";
	}
	if( hasTexture3DExtension )
	{
		if( g_glesExtensions.Supports( "OES_texture_3D" ) == GlesExtensionInfo::ENABLE )
		{
			prefixOs << "#define tex3D(s,uvw,sl,su,sw,lw,l) texture3D(s,uvw,l)\n";
		}
		else
		{
			prefixOs <<
				"#if !defined(GL_ES)||defined(GL_OES_texture_3D)\n"
				"#define tex3D(s,uvw,sl,su,sw,lw,l) texture3D(s,uvw,l)\n";
			if( hasTextureLodExtension )
			{
				prefixOs <<
					"#ifdef GL_EXT_shader_texture_lod\n"
					"#define tex3DLod(s,uvw,l,sl,su,sw,lw) texture3DLod(s,uvw,l)\n"
					"#else\n"
					"#define tex3DLod(s,uvw,l,sl,su,sw,lw) texture3D(s,uvw)\n"
					"#endif\n";
			}
			prefixOs <<
				"#else\n"
				"#define sampler3D sampler2D\n"
				"vec4 tex3D(sampler2D s,vec3 uvw,float sl,bool su,bool sw,bool lw,float l)\n"
				"{\n"
				"float y;\n"
				"if(su) y=fract(uvw.y);\n"
				"else y=clamp(uvw.y,0.0,1.0);\n"
				"y/=sl;\n"
				"float z,s0,s1;\n"
				"z=uvw.z*sl;\n"
				"s0=floor(z);\n"
				"s1=s0+1.0;\n"
				"if(!sw){\n"
				"s0=clamp(s0,0.0,sl-1.0);\n"
				"s1=clamp(s0,0.0,sl-1.0);\n"
				"}\n"
				"s0/=sl;\n"
				"s1/=sl;\n"
				"z=fract(z);\n"
				"vec4 c0=texture2D(s,vec2(uvw.x,y+s0));\n"
				"vec4 c1=texture2D(s,vec2(uvw.x,y+s1));\n"
				"if(lw) return mix(c0,c1,z);\n"
				"return z<0.5?c0:c1;\n"
				"}\n";
			if( hasTextureLodExtension )
			{
				prefixOs <<
					"#ifndef tex3DLod\n"
					"vec4 tex3DLod(sampler2D s,vec3 uvw,float l,float sl,bool su,bool sw,bool lw)\n"
					"{\n"
					"float y;\n"
					"if(su) y=fract(uvw.y);\n"
					"else y=clamp(uvw.y,0.0,1.0);\n"
					"y/=sl;\n"
					"float z,s0,s1;\n"
					"z=uvw.z*sl;\n"
					"s0=floor(z);\n"
					"s1=s0+1.0;\n"
					"if(!sw){\n"
					"s0=clamp(s0,0.0,sl-1.0);\n"
					"s1=clamp(s0,0.0,sl-1.0);\n"
					"}\n"
					"s0/=sl;\n"
					"s1/=sl;\n"
					"z=fract(z);\n"
					"vec4 c0=texture2DLod(s,vec2(uvw.x,y+s0),l);\n"
					"vec4 c1=texture2DLod(s,vec2(uvw.x,y+s1),l);\n"
					"if(lw) return mix(c0,c1,z);\n"
					"return z<0.5?c0:c1;\n"
					"}\n#endif\n";
			}
			prefixOs <<
				"#endif\n";
		}
	}
	if( hasBorderFuncs )
	{
		prefixOs <<
			"vec4 ssbU(vec4 u,vec4 b,vec4 x) {return u.x<0.0||u.x>1.0?b:x;}\n"
			"vec4 ssbV(vec4 u,vec4 b,vec4 x) {return u.y<0.0||u.y>1.0?b:x;}\n"
			"vec4 ssbW(vec4 u,vec4 b,vec4 x) {return u.z<0.0||u.z>1.0?b:x;}\n"
			"vec4 ssbUV(vec4 u,vec4 b,vec4 x) {return u.x<0.0||u.x>1.0||u.y<0.0||u.y>1.0?b:x;}\n"
			"vec4 ssbUVW(vec4 u,vec4 b,vec4 x) {return u.x<0.0||u.x>1.0||u.y<0.0||u.y>1.0||u.z<0.0||u.z>1.0?b:x;}\n";
	}
	if( hasGammaToLinear )
	{
		prefixOs << "vec4 g2l(vec4 x){return vec4(pow(x.xyz,vec3(2.2)),x.w);}\n";
	}

	// Print constant buffer declarations
	for( int cb = 0; cb < 16; ++cb )
	{
		if( cbs[cb].first != -1 )
		{
			declOs << "uniform vec4 cb" << cb << "[" << ( cbs[cb].second - cbs[cb].first ) << "];" << endl;
		}
	}

	// Print other variable declarations
	PrintRegisterBank( registerInfo, declOs, "uniform bool", 'b' );
	PrintRegisterBank( registerInfo, declOs, "uniform ivec4", 'i' );
	PrintRegisterBank( registerInfo, localsOs, "vec4", 'v' );
	PrintRegisterBank( registerInfo, localsOs, "vec4", 'r' );
	if( registerInfo.find( 'a' ) != registerInfo.end() )
	{
		localsOs << "ivec4 a0;" << endl;
	}
	if( registerInfo.find( 'p' ) != registerInfo.end() )
	{
		localsOs << "bvec4 p0;" << endl;
	}
	if( registerInfo.find( 'c' ) != registerInfo.end() && registerInfo['c'].hasIndexed )
	{
		localsOs << "vec4 c[" << ( registerInfo['c'].indexesEnd - registerInfo['c'].indexesBegin ) << "];" << endl;
	}

	if( stage.type == VERTEX_STAGE )
	{
		declOs << "uniform vec3 ssyf;\n";
		if( g_maxClipPlanes )
		{
			declOs << "\n#ifdef PS\n";
			declOs << "uniform vec4 ssf[4];" << endl;
			declOs << "varying ";
			if( g_maxClipPlanes == 1 )
			{
				declOs << "float";
			}
			else
			{
				declOs << "vec";
				declOs << '0' + g_maxClipPlanes;
			}
			declOs << " ssv;" << endl;
			declOs << "#endif\n";

			os << "\n#ifdef PS\n";
			if( g_maxClipPlanes == 1 )
			{
				os << "ssv=dot(ssf[0],gl_Position);" << endl;
			}
			else
			{
				const char* cp[4] = { 
					"ssv.x=dot(ssf[0],gl_Position);",
					"ssv.y=dot(ssf[1],gl_Position);",
					"ssv.z=dot(ssf[2],gl_Position);",
					"ssv.w=dot(ssf[3],gl_Position);",
					};
				for( int i = 0; i < g_maxClipPlanes; ++i )
				{
					os << cp[i] << endl;
				}
			}
			os << "#endif\n";
		}
		os << "gl_Position.xy += ssyf.xy*gl_Position.w;" << endl;
		// Flip Y for vertex shaders and make Z span [-1, 1] instead of [0, 1]
		os << "gl_Position.y*=ssyf.z;" << endl
			<< "gl_Position.z=gl_Position.z*2.0-gl_Position.w;" << endl;
	}
	else
	{
		declOs << "\n#ifdef PS\n";
		declOs << "uniform vec4 ssi;" << endl;
		if( g_maxClipPlanes )
		{
			declOs << "varying ";
			if( g_maxClipPlanes == 1 )
			{
				declOs << "float";
			}
			else
			{
				declOs << "vec";
				declOs << '0' + g_maxClipPlanes;
			}
			declOs << " ssv;" << endl;
		}
		declOs << "#endif\n";

		os << "\n#ifdef PS\n";
		// Alpha test
		os << "float av=floor(clamp(gl_FragData[0].a,0.0,1.0)*255.0+0.5);" << endl
			<< "if(ssi.z==0.0)" << endl 
			<< "{" << endl 
			<< "if(av*ssi.x+ssi.y<0.0)" << endl 
			<< "discard;" << endl 
			<< "}" << endl 
			<< "else" << endl 
			<< "{" << endl 
			<< "if(ssi.x>0.0)" << endl 
			<< "{" << endl 
			<< "if(av==ssi.y)" << endl 
			<< "discard;" << endl 
			<< "}" << endl
			<< "else" << endl
			<< "{" << endl 
			<< "if(av!=ssi.y)" << endl 
			<< "discard;" << endl 
			<< "}" << endl
			<< "}" << endl;
		// Clip planes
		if( g_maxClipPlanes == 1 )
		{
			os << "if(ssv<0.0)discard;" << endl;
		}
		else
		{
			const char* cp[4] = {
				"if(ssv.x<0.0)discard;",
				"if(ssv.y<0.0)discard;",
				"if(ssv.z<0.0)discard;",
				"if(ssv.w<0.0)discard;",
				};
			for( int i = 0; i < g_maxClipPlanes; ++i )
			{
				os << cp[i] << endl;
			}
		}
		os << "#endif\n";
	}


	// Assemble the code
	std::string prefix( prefixOs.str(), prefixOs.str() + prefixOs.pcount() );
	std::string decl( declOs.str(), declOs.str() + declOs.pcount() );
	std::string locals( localsOs.str(), localsOs.str() + localsOs.pcount() );
	std::string main( os.str(), os.str() + os.pcount() );
	glCode = prefix + decl + 
		"void main()\n"
		"{\n"
		+ locals
		+ main + "}";
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a hidden window and GL context (used for validating shaders).
// Return Value:
//   true On success
//   false On error
// --------------------------------------------------------------------------------------
bool EffectCompilerGL2::Create()
{
	InitializeCriticalSection( &s_glCS );

	// We need to create a separate hidden window for OpenGL context.
	// Using console window for it doesn't work well with using the
	// compiler from visual studio.
	WNDCLASS wc; 
    wc.style = CS_OWNDC; 
    wc.lpfnWndProc = (WNDPROC)&DefWindowProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
	wc.hInstance = GetModuleHandle( nullptr ); 
    wc.hIcon = nullptr; 
    wc.hCursor = nullptr; 
    wc.hbrBackground = nullptr; 
    wc.lpszMenuName =  "glsl"; 
    wc.lpszClassName = "glsl"; 
    if (!RegisterClass(&wc)) 
	{
		DWORD error = GetLastError();
		char* lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		if( lpMsgBuf )
		{
			printf( "Error initializing OpenGL: RegisterClass returned FALSE: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);
		}
		else
		{
			printf( "Error initializing OpenGL: RegisterClass returned FALSE\n" );
		}
		return false; 
	}
	s_glWnd = CreateWindow( "glsl", "glsl", WS_OVERLAPPED, 0, 0, 16, 16, nullptr, nullptr, GetModuleHandle( nullptr ), 0 );
	if( s_glWnd == nullptr )
	{
		DWORD error = GetLastError();
		char* lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		if( lpMsgBuf )
		{
			printf( "Error initializing OpenGL: CreateWindow returned NULL: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);
		}
		else
		{
			printf( "Error initializing OpenGL: CreateWindow returned NULL\n" );
		}
		return false; 
	}

	HDC dc = GetDC( s_glWnd );

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat( dc, &pfd );
	if( !SetPixelFormat( dc, iFormat, &pfd ) )
	{
		DWORD error = GetLastError();
		char* lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		if( lpMsgBuf )
		{
			printf( "Error initializing OpenGL: SetPixelFormat returned FALSE: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);
		}
		else
		{
			printf( "Error initializing OpenGL: SetPixelFormat returned FALSE\n" );
		}
		return false;
	}

	auto rc = wglCreateContext( dc );
	if( !rc )
	{
		DWORD error = GetLastError();
		char* lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		printf( "Error initializing OpenGL: %s", lpMsgBuf );
		LocalFree(lpMsgBuf);
		return false;
	}

	if( !wglMakeCurrent( dc, rc ) )
	{
		printf( "Error initializing OpenGL: wglMakeCurrent returned FALSE\n" );
		return false;
	}

	return true;
}

static std::map<DWORD, GLEWContext*> s_contexts;

static GLEWContext* glewGetContext()
{
	DWORD id = GetCurrentThreadId();
	EnterCriticalSection( &s_glCS );
	auto it = s_contexts.find( id );
	if( it == s_contexts.end() )
	{
		HDC dc = GetDC( s_glWnd );
		auto rc = wglCreateContext( dc );
		if( !rc )
		{
			DWORD error = GetLastError();
			char* lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );
			g_messages.AddMessage( "Error initializing OpenGL: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);

			LeaveCriticalSection( &s_glCS );
			return false;
		}

		if( !wglMakeCurrent( dc, rc ) )
		{
			g_messages.AddMessage( "Error initializing OpenGL: wglMakeCurrent returned FALSE\n" );
			LeaveCriticalSection( &s_glCS );
			return false;
		}

		auto ver = glGetString(GL_VERSION);
		GLEWContext* ctx = new GLEWContext;
		auto ret = glewContextInit( ctx );
		if( ret != GLEW_OK )
		{
			g_messages.AddMessage( "Errorr initializing OpenGL: %s\n", glewGetErrorString( ret ) );
			LeaveCriticalSection( &s_glCS );
			return 0;
		}
		s_contexts[id] = ctx;
		LeaveCriticalSection( &s_glCS );
		return ctx;
	}
	GLEWContext* ctx = it->second;
	LeaveCriticalSection( &s_glCS );
	return ctx;
}

extern void PrintPrettyCode( std::ostream& listing, const char* code, const char* indent );
extern void PrintStageInfo( std::ostream& listing, const StageInput& stage, const EffectData& result );

static const std::regex s_summary( "[[:digit:]]+ error\\(s\\), [[:digit:]]+ warning\\(s\\)\\r\\n" );


// --------------------------------------------------------------------------------------
// Description:
//   Executes external process piping stderr of the child process to the g_messages
//   queue.
// Arguments:
//   commandLine - command line for the child process
// Return Value:
//   true On success
//   false On error
// --------------------------------------------------------------------------------------
static bool RunProcess( const char* commandLine )
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE readPipe, writePipe;

	if( !CreatePipe( &readPipe, &writePipe, &sa, 0 ) )
	{
		return false;
	}
	if( !SetHandleInformation( readPipe, HANDLE_FLAG_INHERIT, 0 ) )
	{
		CloseHandle( readPipe );
		CloseHandle( writePipe );
		return false;
	}

	STARTUPINFO startInfo;
	ZeroMemory( &startInfo, sizeof(STARTUPINFO) );
	startInfo.cb = sizeof(STARTUPINFO); 
	startInfo.hStdError = writePipe;
	startInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION procInfo;

	size_t len = strlen( commandLine ) + 1;
	char* newCommandLine = new char[len];
	strcpy_s( newCommandLine, len, commandLine );

	if( !CreateProcess( NULL, newCommandLine, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startInfo, &procInfo ) )
	{
		delete[] newCommandLine;
		CloseHandle( readPipe );
		CloseHandle( writePipe );
		return false;
	}
	delete[] newCommandLine;

	std::string output;
	char buffer[1024];
	while( true )
	{
		DWORD bytesRead;
		if( !ReadFile( readPipe, buffer, sizeof( buffer ) - 1, &bytesRead, nullptr ) || bytesRead == 0 )
		{
			break;
		}
		buffer[bytesRead] = 0;
		output += buffer;
		if( bytesRead < sizeof( buffer ) - 1 )
		{
			break;
		}
	}

	std::string filteredOutput = std::regex_replace( output, s_summary, std::string( "" ) );
	if( !filteredOutput.empty() )
	{
		g_messages.AddMessage( "%s", filteredOutput );
	}

	CloseHandle( procInfo.hProcess );
	CloseHandle( procInfo.hThread );
	CloseHandle( readPipe );
	CloseHandle( writePipe );

	return true;
}

static bool TryOpenGLCall()
{
	__try
	{
		GLuint shader = glCreateShader( GL_VERTEX_SHADER );
		glDeleteShader( shader );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		return false;
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Compiles effect and stores all compiled data in EffectData structure.
// Arguments:
//   source - Effect source code
//   sourceLength - Lenght of source code string
//   defines - Macro defines to be used by preprocessor
//   include - Include handler to be used by preprocessor
//   result - (out) Compiled effect data
// Return Value:
//   true On success
//   false On error
// --------------------------------------------------------------------------------------
bool EffectCompilerGL2::CompileEffect( const char* source, 
										size_t sourceLength, 
										const D3DXMACRO* defines, 
										ID3DXInclude* include, 
										EffectData& result )
{
	auto ret = glewInit();
	if( ret != GLEW_OK )
	{
		g_messages.AddMessage( "Errorr initializing OpenGL: %s\n", glewGetErrorString( ret ) );
		return false;
	}

	// WGL will crash if we exit the program without making a single GL call
	bool useOpenGLValidation = TryOpenGLCall();
	if( !useOpenGLValidation )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: OpenGL calls are failing; skipping OpenGL shader validation" );
	}

	// Fist compile effect as DX9
	if( !g_compilerDX9.CompileEffect( source, sourceLength, defines, include, result, true ) )
	{
		return false;
	}

	// Swap minLod and maxLod sampler states as they are recorded incorrectly in DX9.
	for( auto pass = result.passes.begin(); pass != result.passes.end(); ++pass )
	{
		for( auto stage = pass->stages.begin(); stage != pass->stages.end(); ++stage )
		{
			for( auto sampler = stage->samplers.begin(); sampler != stage->samplers.end(); ++sampler )
			{
				std::swap( sampler->second.minLOD, sampler->second.maxLOD );
			}
		}
	}

	std::stringstream listing;
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


	std::string effectSource;
	if( g_generateListing )
	{
		listing << "permutation:" << std::endl;
		listing << "  platform: GLES2" << std::endl;
		listing << "  id: 000" << std::endl;
		listing << "  defines:" << std::endl;
		for( int i = 0; defines[i].Name; ++i )
		{
			listing << "    " << defines[i].Name << ": " << defines[i].Definition << std::endl;
		}
		CComPtr<ID3DXBuffer> shaderText;
		if( SUCCEEDED( D3DXPreprocessShader( source, sourceLength, defines, include, &shaderText, nullptr ) ) )
		{
			listing << "  source: |" << std::endl;
			PrintPrettyCode( listing, reinterpret_cast<const char*>( shaderText->GetBufferPointer() ), "    " );
		}
		listing << "  passes:" << std::endl;
	}

	GLint vertexShader = 0;
	GLint fragmentShader = 0;

	for( auto pass = result.passes.begin(); pass != result.passes.end(); ++pass )
	{
		if( g_generateListing )
		{
			listing << "  -" << std::endl;
		}
		for( auto stage = pass->stages.begin(); stage != pass->stages.end(); ++stage )
		{
			if( stage->shaderData == nullptr )
			{
				stage->shadowShaderSize = 0;
				stage->shadowShaderData = nullptr;

				continue;
			}
			// Disassemble shader and convert DX9 assembly to GLSL
			CComPtr<ID3DXBuffer> disassembly;
			if( FAILED( D3DXDisassembleShader( (DWORD*)stage->shaderData, FALSE, nullptr, &disassembly ) ) )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: could not disassemble shader" );
				return false;
			}
			const char* src = (const char*)disassembly->GetBufferPointer();
			std::string glesSource;
			GlesExtensionInfo::ExtensionMap usedExtensions;
			if( !AsmToGLES2( src, glesSource, *stage, usedExtensions ) )
			{
				return false;
			}

			if( g_generateListing )
			{
				listing << "    -" << std::endl;
				listing << "        profile: " << ( stage->type == VERTEX_STAGE ? "vs" : "ps" ) << std::endl;
				listing << "        original:" << std::endl;
				listing << "          asm: |" << std::endl;
				PrintPrettyCode( listing, glesSource.c_str(), "            " );
				const char* approximately = "approximately ";
				const char* found = strstr( src, approximately );
				if( found )
				{
					unsigned instructionCount = -1;
					sscanf_s( found + strlen( approximately ), "%u", &instructionCount );
					listing << "          instructionCount: " << instructionCount << std::endl;
				}
				for( auto it = usedExtensions.begin(); it != usedExtensions.end(); ++it )
				{
					listing  << "          " << it->first << ": ";
					if( it->second == GlesExtensionInfo::ENABLE )
					{
						listing << "required" << std::endl;
					}
					else
					{
						listing << "optional" << std::endl;
					}
				}
				PrintStageInfo( listing, *stage, result );
			}

			delete[] stage->shaderData;
			stage->shaderSize = glesSource.length() + 1;
			stage->shaderData = new char[stage->shaderSize];
			strcpy_s( (char*)stage->shaderData, stage->shaderSize, glesSource.c_str() );

			stage->shadowShaderSize = 0;
			stage->shadowShaderData = nullptr;

			// Validate resulting shader
			GLuint shader = 0;
			if( useOpenGLValidation )
			{
				shader = glCreateShader( stage->type == VERTEX_STAGE ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER );
				if( !shader )
				{
					g_messages.AddMessage( "\\memory(0): error X0000: could not create OpenGL shader" );
					return false;
				}
				const char* codeSource = glesSource.c_str();
				glShaderSource( shader, 1, &codeSource, nullptr );
				glCompileShader( shader );

				GLint compiled;
				glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
				if( !compiled )
				{
					GLint infoLen = 0;
					glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
					if( infoLen > 1 )
					{
						char* buffer = new char[infoLen];
						glGetShaderInfoLog( shader, infoLen, nullptr, buffer );
						g_messages.AddMessage( "%s", buffer );
						delete[] buffer;
					}
					else
					{
						g_messages.AddMessage( "\\memory(0): error X0000: undefined error compiling OpenGL shader" );
					}
					glDeleteShader( shader );
					return false;
				}
			}
			// We should not issue warnings from GL compiler since GLSL code is 100% generated,
			// but it might be of interest

			//if( g_printWarnings )
			//{
			//	GLint infoLen = 0;
			//	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
			//	if( infoLen > 1 )
			//	{
			//		char* buffer = new char[infoLen];
			//		glGetShaderInfoLog( shader, infoLen, nullptr, buffer );
			//		g_messages.AddMessage( "%s", buffer );
			//		delete[] buffer;
			//	}
			//}

			if( !g_glExternalCompilerPath.empty() )
			{
				// Compile binary shader using external compiler
				char tempDir[MAX_PATH + 1];
				char inFile[MAX_PATH], outFile[MAX_PATH];
				if( GetTempPath( MAX_PATH + 1, tempDir ) == 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}
				if( GetTempFileName( tempDir, "gli", 0, inFile ) == 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}
				if( GetTempFileName( tempDir, "glo", 0, outFile ) == 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}

				FILE *f;
				if( fopen_s( &f, inFile, "wt" ) != 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}
				fwrite( stage->shaderData, stage->shaderSize, 1, f );
				fclose( f );


				std::string cmdLinePrefix = g_glExternalCompilerPath + " -o " + outFile + " --core=" + g_glExternalCompilerSwitch;
				if( stage->type == VERTEX_STAGE )
				{
					cmdLinePrefix += " --vert ";
				}
				else
				{
					cmdLinePrefix += " --frag ";
				}
				if( !RunProcess( ( cmdLinePrefix + inFile ).c_str() ) )
				{
					g_messages.AddMessage( "\\memory(0): error G0100: external GL compiler returned error" );
					return false;
				}

				if( fopen_s( &f, outFile, "rb" ) != 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}
				fseek( f, 0, SEEK_END );
				size_t sz = size_t( ftell( f ) );
				fseek( f, 0, SEEK_SET );

				delete[] stage->shaderData;
				stage->shaderSize = sz;
				stage->shaderData = new char[stage->shaderSize];
				fread( stage->shaderData, sz, 1, f );
				fclose( f );

				if( !RunProcess( ( cmdLinePrefix + " -DPS=1 " + inFile ).c_str() ) )
				{
					g_messages.AddMessage( "\\memory(0): error G0100: external GL compiler returned error" );
					return false;
				}

				if( fopen_s( &f, outFile, "rb" ) != 0 )
				{
					g_messages.AddMessage( "\\memory(0): error G0101: could not create temporary files for external GL compiler" );
					return false;
				}
				fseek( f, 0, SEEK_END );
				sz = size_t( ftell( f ) );
				fseek( f, 0, SEEK_SET );

				delete[] stage->shadowShaderData;
				stage->shadowShaderSize = sz;
				stage->shadowShaderData = new char[stage->shadowShaderSize];
				fread( stage->shadowShaderData, sz, 1, f );
				fclose( f );

				DeleteFile( inFile );
				DeleteFile( outFile );
			}

			if( useOpenGLValidation )
			{
				if( stage->type == VERTEX_STAGE )
				{
					if( vertexShader )
					{
						glDeleteShader( vertexShader );
					}
					vertexShader = shader;
				}
				else
				{
					if( fragmentShader )
					{
						glDeleteShader( fragmentShader );
					}
					fragmentShader = shader;
				}
			}
		}
	}
	// Validate vertex/fragment shader combination (if the link correctly)
	if( vertexShader && fragmentShader )
	{
		int program = glCreateProgram();
		if( program )
		{
			glAttachShader( program, vertexShader );
			glAttachShader( program, fragmentShader );
			glLinkProgram( program );

			GLint compiled;
			glGetProgramiv( program, GL_LINK_STATUS, &compiled );
			if( !compiled )
			{
				GLint infoLen = 0;
				glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLen );
				if( infoLen > 1 )
				{
					char* buffer = new char[infoLen];
					glGetProgramInfoLog( program, infoLen, nullptr, buffer );
					g_messages.AddMessage( "%s", buffer );
					delete[] buffer;
				}
				else
				{
					g_messages.AddMessage( "\\memory(0): error X0000: undefined error linking OpenGL shader" );
				}
				glDeleteProgram( program );
				return false;
			}
			glDeleteProgram( program );
		}
		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );
	}
	return true;
}
