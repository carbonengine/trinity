////////////////////////////////////////////////////////////
//
//    Created:   May 2016
//    Copyright: CCP 2016
//

#include "stdafx.h"
#include "EffectCompilerGL4.h"
#include "EffectCompilerDx11.h"
#include "EffectData.h"
#include "CompileMessageQueue.h"
#include "InlineString.h"
#include <strstream>
#include <regex>
#include <iomanip>
#include "wglext.h"

extern EffectCompilerDX11 g_compilerDX11;
extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern int g_maxClipPlanes;
extern bool g_glesEmulateSampler;
extern bool g_generateListing;
extern std::string g_listing;
extern CRITICAL_SECTION g_listingCS;
extern bool g_printWarnings;
extern bool g_validateOpenGL;

namespace
{
// Critical section for GL contexts
CRITICAL_SECTION s_glCS;
// Owner window for GL contexts
HWND s_glWnd;


const std::regex s_id( "([-!])?(\\|)?([a-zA-Z_][[:alnum:]]*)(?:_([a-zA-Z0-9_]+))?[ \\t]*(?:\\[([^\\]]*)\\])?(?:\\.([[:alpha:]]+))?(\\|)?" );
const std::regex s_literal( "l\\((\\-?[[:digit:]](?:[[:alnum:]\\.+-])*)(?:,[ \\t]*(\\-?[[:digit:]](?:[[:alnum:]\\.+-])*))?(?:,[ \\t]*(\\-?[[:digit:]](?:[[:alnum:]\\.+-])*))?(?:,[ \\t]*(\\-?[[:digit:]](?:[[:alnum:]\\.+-])*))?\\)" );
const std::regex s_number( "\\-?[[:digit:]]([[:digit:]\\.eE+-])*" );
const std::regex s_coma( "," );
const std::regex s_openp( "\\(" );
const std::regex s_closep( "\\)" );
const std::regex s_assign( "=" );
const std::regex s_space( "[ \\t]+" );
const std::regex s_endl( "([ \\t]*\\n)+" );
const std::regex s_comment( "//.*\\n?" );



// --------------------------------------------------------------------------------------
// Description:
//   Structure to define a token when parsing DX9 assembly code.
// --------------------------------------------------------------------------------------
struct DX11AsmToken
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
		// (
		OpenParen,
		// )
		CloseParen,
		// =
		Assign,
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
	bool literal;
	// Modifier (everything appearing after "_" in command/register name
	InlineString modifier;
	// Register swizzle
	InlineString swizzle;
	// Register indexing expression (we don't parse it since it only contains literals and "a0" register
	InlineString index;
	// TODO: predicate mask? (see setp_comp)
};

class CompilerError: public std::exception
{
public:
	CompilerError( const char* message )
		:std::exception( message )
	{
	}
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
DX11AsmToken GetDX11AsmToken( InlineString& stream )
{
	DX11AsmToken token;
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
		if( std::regex_search( stream.start, stream.end, match, s_literal, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::ID;
			token.negate = 0;
			token.literal = true;
			token.name = match[1].matched ? MakeInlineString( match[1].first, match[1].second ) : MakeInlineString( nullptr, nullptr );
			token.modifier = match[2].matched ? MakeInlineString( match[2].first, match[2].second ) : MakeInlineString( nullptr, nullptr );
			token.swizzle = match[3].matched ? MakeInlineString( match[3].first, match[3].second ) : MakeInlineString( nullptr, nullptr );
			token.index = match[4].matched ? MakeInlineString( match[4].first, match[4].second ) : MakeInlineString( nullptr, nullptr );
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_id, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::ID;
			token.literal = false;
			token.negate = match[1].matched ? match[1].first[0] : 0;
			token.name = match[3].matched ? MakeInlineString( match[3].first, match[3].second ) : MakeInlineString( nullptr, nullptr );
			token.modifier = match[4].matched ? MakeInlineString( match[4].first, match[4].second ) : MakeInlineString( nullptr, nullptr );
			token.index = match[5].matched ? MakeInlineString( match[5].first, match[5].second ) : MakeInlineString( nullptr, nullptr );
			token.swizzle = match[6].matched ? MakeInlineString( match[6].first, match[6].second ) : MakeInlineString( nullptr, nullptr );
			stream.start = match[0].second;
			if( match[2].matched )
			{
				token.modifier = MakeInlineString( "abs" );
			}
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_number, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::Number;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_coma, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::Coma;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_openp, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::OpenParen;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_closep, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::CloseParen;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_assign, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::Assign;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		if( std::regex_search( stream.start, stream.end, match, s_endl, std::regex_constants::match_continuous ) )
		{
			token.type = DX11AsmToken::CommandEnd;
			token.negate = 0;
			token.name = MakeInlineString( match[0].first, match[0].second );
			token.modifier.start = token.modifier.end = 0;
			token.index.start = token.modifier.end = 0;
			token.swizzle.start = token.modifier.end = 0;
			stream.start = match[0].second;
			return token;
		}
		throw CompilerError( "unexpected asm stream" );
	}
	token.type = DX11AsmToken::EndOfSteam;
	token.negate = 0;
	token.name.start = token.name.end = 0;
	token.modifier.start = token.modifier.end = 0;
	token.index.start = token.modifier.end = 0;
	token.swizzle.start = token.modifier.end = 0;
	return token;
}

using std::endl;

class State
{
public:
	State()
		:m_tokenIndex( 0 ),
		m_tempRegisterCount( 0 )
	{
#define ADD_HANDLER( name ) m_commandHandlers[MakeInlineString( #name )] = &State::name
#define ADD_HANDLER2( name ) m_commandHandlers[MakeInlineString( #name )] = &State::name##_
#define UNSUPPORTED_HANDLER( name ) m_commandHandlers[MakeInlineString( #name )] = &State::unsupported
#define TODO_HANDLER( name ) m_commandHandlers[MakeInlineString( #name )] = &State::unsupported

		ADD_HANDLER( add );
		ADD_HANDLER( and );
		UNSUPPORTED_HANDLER( atomic_and );
		UNSUPPORTED_HANDLER( atomic_cmp_store );
		UNSUPPORTED_HANDLER( atomic_iadd );
		UNSUPPORTED_HANDLER( atomic_imax );
		UNSUPPORTED_HANDLER( atomic_imin );
		UNSUPPORTED_HANDLER( atomic_or );
		UNSUPPORTED_HANDLER( atomic_umax );
		UNSUPPORTED_HANDLER( atomic_umin );
		UNSUPPORTED_HANDLER( atomic_xor );
		ADD_HANDLER( bfi );
		ADD_HANDLER( bfrev );
		ADD_HANDLER2( break );
		ADD_HANDLER( breakc );
		TODO_HANDLER( bufinfo );
		TODO_HANDLER( call );
		TODO_HANDLER( callc );
		ADD_HANDLER2( case );
		ADD_HANDLER2( continue );
		ADD_HANDLER( continuec );
		ADD_HANDLER( countbits );
		ADD_HANDLER( cut );
		ADD_HANDLER( cut_stream );
		ADD_HANDLER( dadd );
		ADD_HANDLER( dcl );
		ADD_HANDLER( ddiv );
		ADD_HANDLER2( default );
		TODO_HANDLER( deq );
		ADD_HANDLER( deriv );
		TODO_HANDLER( dfma );
		TODO_HANDLER( dge );
		ADD_HANDLER( discard );
		ADD_HANDLER( div );
		TODO_HANDLER( dlt );
		TODO_HANDLER( dmax );
		TODO_HANDLER( dmin );
		TODO_HANDLER( dmov );
		TODO_HANDLER( dmovc );
		TODO_HANDLER( dmul );
		TODO_HANDLER( dne );
		ADD_HANDLER( dp2 );
		ADD_HANDLER( dp3 );
		ADD_HANDLER( dp4 );
		TODO_HANDLER( drcp );
		ADD_HANDLER2( else );
		TODO_HANDLER( emit );
		TODO_HANDLER( emitThenCut );
		ADD_HANDLER( endif );
		ADD_HANDLER( endloop );
		ADD_HANDLER( endswitch );
		ADD_HANDLER( eq );
		ADD_HANDLER( exp );
		UNSUPPORTED_HANDLER( f16tof32 );
		UNSUPPORTED_HANDLER( f32tof16 );
		TODO_HANDLER( fcall );
		ADD_HANDLER( firstbit );
		ADD_HANDLER( frc );
		TODO_HANDLER( ftod );
		ADD_HANDLER( ftoi );
		ADD_HANDLER( ftou );
		ADD_HANDLER( gather );
		ADD_HANDLER( ge );

		TODO_HANDLER( hs );
		ADD_HANDLER( iadd );
		TODO_HANDLER( ibfe );
		ADD_HANDLER2( if );

		ADD_HANDLER( ige );
		ADD_HANDLER( ilt );
		ADD_HANDLER( imad );
		ADD_HANDLER( imax );
		ADD_HANDLER( imin );
		UNSUPPORTED_HANDLER( imm );
		ADD_HANDLER( imul );
		ADD_HANDLER( ine );
		ADD_HANDLER( ineg );
		ADD_HANDLER( ishl );
		ADD_HANDLER( ishr );
		ADD_HANDLER( itof );
		TODO_HANDLER( label );

		ADD_HANDLER( ld );
		ADD_HANDLER( ldms );
		TODO_HANDLER( lod );
		ADD_HANDLER( log );
		ADD_HANDLER( loop );

		ADD_HANDLER( lt );
		ADD_HANDLER( mad );
		ADD_HANDLER2( max );
		ADD_HANDLER2( min );
		ADD_HANDLER( mov );
		ADD_HANDLER( movc );
		ADD_HANDLER( mul );

		ADD_HANDLER( ne );
		ADD_HANDLER( nop );
		ADD_HANDLER( not );
		ADD_HANDLER( or );
		ADD_HANDLER( ps );
		ADD_HANDLER( rcp );
		ADD_HANDLER( resinfo );

		ADD_HANDLER( ret );
		ADD_HANDLER( retc );
		ADD_HANDLER( round );

		ADD_HANDLER( rsq );
		ADD_HANDLER( sample );
		UNSUPPORTED_HANDLER( sampleinfo );
		UNSUPPORTED_HANDLER( samplepos );

		ADD_HANDLER( sincos );
		ADD_HANDLER( sqrt );
		UNSUPPORTED_HANDLER( store );
		TODO_HANDLER( swapc );
		ADD_HANDLER2( switch );
		UNSUPPORTED_HANDLER( sync );
		ADD_HANDLER( uaddc );
		TODO_HANDLER( ubfe );
		ADD_HANDLER( udiv );
		ADD_HANDLER( uge );
		ADD_HANDLER( ult );
		ADD_HANDLER( umad );
		ADD_HANDLER( umax );
		ADD_HANDLER( umin );
		ADD_HANDLER( ushr );
		TODO_HANDLER( usubb );
		ADD_HANDLER( utof );
		ADD_HANDLER( xor );

		ADD_HANDLER( vs );
	}

	std::string parse( const char* source, InputStageType stage, const std::vector<InputDescription>& inputs )
	{
		m_stage = stage;
		m_inputs = inputs;

		InlineString stream = MakeInlineString( source );
		while( true )
		{
			DX11AsmToken command = GetDX11AsmToken( stream );
			if( command.type == DX11AsmToken::EndOfSteam )
			{
				break;
			}
			m_tokens.push_back( command );
		}

		for( m_tokenIndex = 0; m_tokenIndex < m_tokens.size(); )
		{
			auto& command = m_tokens[m_tokenIndex++];
			auto found = m_commandHandlers.find( command.name );
			if( found == m_commandHandlers.end() )
			{
				throw CompilerError( "unsupported asm command" );
			}
			auto handler = found->second;
			( this->*handler )( command );
			ExpectToken( DX11AsmToken::CommandEnd );
		}

		switch( stage )
		{
		case VERTEX_STAGE:
			m_declOs << "uniform vec4 ss[5];" << endl;
			m_codeOs << "#ifdef PS" << endl <<
				"gl_ClipDistance[0]=dot(ss[1],gl_Position);" << endl <<
				"#endif" << endl <<
				"gl_Position.xy += ss[0].xy*gl_Position.w;" << endl <<
				"gl_Position.y*=ss[0].z;" << endl <<
				"gl_Position.z=gl_Position.z*2.0-gl_Position.w;" << endl;
			break;
		case PIXEL_STAGE:
			m_declOs << "#ifdef PS" << endl <<
				"uniform vec4 ss[2];" << endl <<
				"#endif" << endl;
			m_codeOs << "#ifdef PS" << endl <<
				"if(ss[0].w!=0.0){" << endl <<
				"float av=floor(clamp(o0.a,0.0,1.0)*255.0+0.5);" << endl <<
				"if(ss[0].z==0.0){if(av*ss[0].x+ss[0].y<0.0) discard;}" << endl <<
				"else if(ss[0].x>0.0){if(av==ss[0].y) discard;}" << endl <<
				"else{if(av!=ss[0].y) discard;}" << endl <<
				"}" << endl <<
				"#endif" << endl;
			break;

		}
		m_declOs << "void main()" << endl << "{" << endl;
		m_codeOs << "}" << endl;

		std::string decl( m_declOs.str(), m_declOs.str() + m_declOs.pcount() );
		std::string code( m_codeOs.str(), m_codeOs.str() + m_codeOs.pcount() );

		return decl + code;

	}

	void unsupported( const DX11AsmToken& )
	{
		throw CompilerError( "unsupported asm command" );
	}

	void add( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle ) << '+' << Src( src1, dst.swizzle );
		});
		m_codeOs << ';' << endl;
	}

	void and( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << "=intBitsToFloat(";
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle, Int ) << '&' << Src( src1, dst.swizzle, Int );
		});
		m_codeOs << ");" << endl;
	}

	void bfi( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src3 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;


		m_codeOs << "{uint";
		if( length > 1 )
		{
			m_codeOs << length;
		}
		m_codeOs << " width=floatBitsToUint(" << Src( src0, dst.swizzle, Float ) << ");";
		m_codeOs << "uint";
		if( length > 1 )
		{
			m_codeOs << length;
		}
		m_codeOs << " offset=floatBitsToUint(" << Src( src1, dst.swizzle, Float ) << ");";
		m_codeOs << "uint";
		if( length > 1 )
		{
			m_codeOs << length;
		}
		m_codeOs << " bitmask=(((1<<width)-1)<<offset)&0xffffffff;";
		m_codeOs << Dest( dst ) << "=intBitsToFloat(((" << Src( src2, dst.swizzle, Int ) << "<<offset)&bitmask)|(" << 
			Src( src3, dst.swizzle, Int ) << "&~bitmask));}" << endl;
	}

	void bfrev( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(bitfieldReverse(" << Src( src0, dst.swizzle, Int ) << "));" << endl;
	}

	void break_( const DX11AsmToken& command )
	{
		m_codeOs << "break;" << endl;
	}

	void breakc( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		if( command.modifier == MakeInlineString( "z" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "==0.0)";
		}
		else if( command.modifier == MakeInlineString( "nz" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "!=0.0)";
		}
		else
		{
			unsupported( command );
		}
		m_codeOs << "break;" << endl;
	}

	void case_( const DX11AsmToken& command )
	{
		auto& lbl = ExpectToken( DX11AsmToken::Number );
		m_codeOs << "case " << lbl.name << ':' << endl;
	}

	void continue_( const DX11AsmToken& command )
	{
		m_codeOs << "continue;" << endl;
	}

	void continuec( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		if( command.modifier == MakeInlineString( "z" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "==0.0)";
		}
		else if( command.modifier == MakeInlineString( "nz" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "!=0.0)";
		}
		else
		{
			unsupported( command );
		}
		m_codeOs << "continue;" << endl;
	}

	void countbits( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(bitCount(" << Src( src0, dst.swizzle, Int ) << "));" << endl;
	}

	void cut( const DX11AsmToken& command )
	{
		m_codeOs << "EndPrimitive();" << endl;
	}

	void cut_stream( const DX11AsmToken& command )
	{
		auto& lbl = ExpectToken( DX11AsmToken::Number );
		m_codeOs << "EndStreamPrimitive(" << lbl.name << ");" << endl;
	}

	void dadd( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;
		bool sat = command.modifier == MakeInlineString( "sat" );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		if( length == 4 )
		{
			m_codeOs << "uvec4(packDouble2x32(";
			if( sat )
			{
				m_codeOs << "clamp(";
			}
			m_codeOs << Src( src0, dst.swizzle, Double ) << '+' << Src( src1, dst.swizzle, Double );
			if( sat )
			{
				m_codeOs << "dvec2(0.0),dvec2(1.0))";
			}
			m_codeOs << "))";
		}
		else
		{
			m_codeOs << "packDouble2x32(";
			if( sat )
			{
				m_codeOs << "clamp(";
			}
			m_codeOs << Src( src0, dst.swizzle, Double ) << '+' << Src( src1, dst.swizzle, Double );
			if( sat )
			{
				m_codeOs << "0.0,1.0)";
			}
			m_codeOs << ')';
		}
		m_codeOs << ");" << endl;
	}

	void dcl( const DX11AsmToken& command )
	{
		if( command.modifier == MakeInlineString( "constantbuffer" ) )
		{
			auto& reg = ExpectToken( DX11AsmToken::ID );
			m_declOs << "uniform vec4 " << reg.name << '[' << reg.index << "];" << endl;
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
		}
		else if( command.modifier == MakeInlineString( "function_body" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "function_table" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "globalFlags" ) )
		{
			ExpectToken( DX11AsmToken::ID );
		}
		else if( command.modifier == MakeInlineString( "hs_fork_phase_instance_count" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "hs_join_phase_instance_count" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "hs_max_tessfactor" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "immediateConstantBuffer" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "indexableTemp" ) )
		{
			auto& reg = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::Number );
			m_codeOs << "vec4 " << reg.name << '[' << reg.index << "];" << endl;
		}
		else if( command.modifier == MakeInlineString( "indexRange" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "indexRange" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "input" ) )
		{
			auto& reg = ExpectToken( DX11AsmToken::ID );
			if( m_usedInputs.find( reg.name ) == m_usedInputs.end() )
			{
				m_usedInputs.insert( reg.name );

				int regIndex = atoi( reg.name.start + 1 );
				for( auto each = m_inputs.begin(); each != m_inputs.end(); ++each )
				{
					if( each->registerIndex == regIndex )
					{
						switch( each->componentType )
						{
						case 1:
							m_declOs << "layout(location=" << MakeInlineString( reg.name.start + 1, reg.name.end ) << ") in vec4 " << reg.name << ';' << endl;
							break;
						case 2:
							m_declOs << "layout(location=" << MakeInlineString( reg.name.start + 1, reg.name.end ) << ") in vec4 " << reg.name << ';' << endl;
							break;
						default:
							m_declOs << "layout(location=" << MakeInlineString( reg.name.start + 1, reg.name.end ) << ") in vec4 " << reg.name << ';' << endl;
							break;
						}
						break;
					}
				}
			}
		}
		else if( command.modifier == MakeInlineString( "input_ps" ) )
		{
			auto& interp = ExpectToken( DX11AsmToken::ID );
			auto& reg = ExpectToken( DX11AsmToken::ID );

			if( m_usedInputs.find( reg.name ) == m_usedInputs.end() )
			{
				m_usedInputs.insert( reg.name );
				m_declOs << "layout(location=" << MakeInlineString( reg.name.start + 1, reg.name.end ) << ") in ";
				if( interp.name == MakeInlineString( "constant" ) )
				{
					m_declOs << "constant ";
				}
				else if( interp.name == MakeInlineString( "linearNoperspective" ) )
				{
					m_declOs << "noperspective ";
				}
				m_declOs << "vec4 " << reg.name << ';' << endl;
			}
		}
		else if( command.modifier == MakeInlineString( "input_ps_siv" ) )
		{
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::ID );
			auto& from = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& to = ExpectToken( DX11AsmToken::ID );

			if( to.name == MakeInlineString( "position" ) )
			{
				m_specialRegisters[from.name] = MakeInlineString( "gl_FragCoord" );
			}
			else
			{
				throw CompilerError( "unsupported SIV" );
			}
		}
		else if( command.modifier == MakeInlineString( "inputPrimitive" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "interface" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "interface_dynamicindexed" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "maxOutputVertexCount" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "output" ) )
		{
			auto& reg = ExpectToken( DX11AsmToken::ID );

			if( m_usedOutputs.find( reg.name ) == m_usedOutputs.end() )
			{
				m_usedOutputs.insert( reg.name );
				m_declOs << "layout(location=" << MakeInlineString( reg.name.start + 1, reg.name.end ) << ") out vec4 " << reg.name << ';' << endl;
			}
		}
		else if( command.modifier == MakeInlineString( "output_control_point_count" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "output_sgv" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "output_siv" ) )
		{
			auto& from = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& to = ExpectToken( DX11AsmToken::ID );
			if( to.name == MakeInlineString( "position" ) )
			{
				m_specialRegisters[from.name] = MakeInlineString( "gl_Position" );
			}
			else
			{
				throw CompilerError( "unsupported SIV" );
			}
		}
		else if( command.modifier == MakeInlineString( "outputTopology" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "resource" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "resource_structured" ) )
		{
			m_declOs << "uniform sampler1D " << ExpectToken( DX11AsmToken::ID ).name << ';' << endl;
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::Number );
		}
		else if( command.modifier == MakeInlineString( "resource_raw" ) )
		{
			m_declOs << "uniform sampler1D " << ExpectToken( DX11AsmToken::ID ).name << ';' << endl;
		}
		else if( command.modifier == MakeInlineString( "resource_texture2d" ) )
		{
			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::CloseParen );
			m_declOs << "uniform sampler2D " << ExpectToken( DX11AsmToken::ID ).name << ';' << endl;
		}
		else if( command.modifier == MakeInlineString( "resource_texture2dms" ) )
		{
			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::Number );
			ExpectToken( DX11AsmToken::CloseParen );

			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::CloseParen );
			m_declOs << "uniform sampler2DMS " << ExpectToken( DX11AsmToken::ID ).name << ';' << endl;
		}
		else if( command.modifier == MakeInlineString( "resource_texturecube" ) )
		{
			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::CloseParen );
			m_declOs << "uniform samplerCube " << ExpectToken( DX11AsmToken::ID ).name << ';' << endl;
		}
		else if( command.modifier == MakeInlineString( "sampler" ) )
		{
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
		}
		else if( command.modifier == MakeInlineString( "stream" ) )
		{
			unsupported( command );
		}
		else if( command.modifier == MakeInlineString( "temps" ) )
		{
			auto& number = ExpectToken( DX11AsmToken::Number );
			int count = atoi( number.name.start );
			for( int i = 0; i < count; ++i )
			{
				m_codeOs << "vec4 r" << i << ';' << endl;
			}
		}
		else
		{
			throw CompilerError( "unexpected declaration" );
		}
	}

	void ddiv( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;
		bool sat = command.modifier == MakeInlineString( "sat" );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		if( length == 4 )
		{
			m_codeOs << "uvec4(packDouble2x32(";
			if( sat )
			{
				m_codeOs << "clamp(";
			}
			m_codeOs << Src( src0, dst.swizzle, Double ) << '/' << Src( src1, dst.swizzle, Double );
			if( sat )
			{
				m_codeOs << "dvec2(0.0),dvec2(1.0))";
			}
			m_codeOs << "))";
		}
		else
		{
			m_codeOs << "packDouble2x32(";
			if( sat )
			{
				m_codeOs << "clamp(";
			}
			m_codeOs << Src( src0, dst.swizzle, Double ) << '/' << Src( src1, dst.swizzle, Double );
			if( sat )
			{
				m_codeOs << "0.0,1.0)";
			}
			m_codeOs << ')';
		}
		m_codeOs << ");" << endl;
	}

	void default_( const DX11AsmToken& command )
	{
		m_codeOs << "default:" << endl;
	}

	void deriv( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		if( command.modifier == MakeInlineString( "rtx_coarse" ) || command.modifier == MakeInlineString( "rtx_fine" ) )
		{
			m_codeOs << Dest( dst ) << "=dFdx(" << Src( src0, dst.swizzle ) << ");" << endl;
		}
		else if( command.modifier == MakeInlineString( "rtx_coarse_sat" ) || command.modifier == MakeInlineString( "rtx_fine_sat" ) )
		{
			m_codeOs << Dest( dst ) << "=clamp(dFdx(" << Src( src0, dst.swizzle ) << "),";
			int length = dst.swizzle.end - dst.swizzle.start;
			if( length == 1 )
			{
				m_codeOs << "0.0,1.0);" << endl;
			}
			else
			{
				m_codeOs << "vec" << length << "(0.0),vec" << length << "(1.0));" << endl;
			}
		}
		else if( command.modifier == MakeInlineString( "rty_coarse" ) || command.modifier == MakeInlineString( "rty_fine" ) )
		{
			m_codeOs << Dest( dst ) << "=dFdy(" << Src( src0, dst.swizzle ) << ");" << endl;
		}
		else if( command.modifier == MakeInlineString( "rty_coarse_sat" ) || command.modifier == MakeInlineString( "rty_fine_sat" ) )
		{
			m_codeOs << Dest( dst ) << "=clamp(dFdy(" << Src( src0, dst.swizzle ) << "),";
			int length = dst.swizzle.end - dst.swizzle.start;
			if( length == 1 )
			{
				m_codeOs << "0.0,1.0);" << endl;
			}
			else
			{
				m_codeOs << "vec" << length << "(0.0),vec" << length << "(1.0));" << endl;
			}
		}
		else
		{
			unsupported( command );
		}
	}

	void discard( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		if( command.modifier == MakeInlineString( "z" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "==0.0)";
		}
		else 
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "!=0.0)";
		}
		m_codeOs << "discard;" << endl;
	}

	void div( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle ) << '/' << Src( src1, dst.swizzle );
		} );
		m_codeOs << ';' << endl;
	}

	void dp2( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "dot(" << Src( src0, "xy" ) << ',' << Src( src1, "xy" ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void dp3( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "dot(" << Src( src0, "xyz" ) << ',' << Src( src1, "xyz" ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void dp4( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "dot(" << Src( src0, "xyzw" ) << ',' << Src( src1, "xyzw" ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void else_( const DX11AsmToken& command )
	{
		m_codeOs << "}else{" << endl;
	}

	void endif( const DX11AsmToken& command )
	{
		m_codeOs << "}" << endl;
	}

	void endloop( const DX11AsmToken& command )
	{
		m_codeOs << "}" << endl;
	}

	void endswitch( const DX11AsmToken& command )
	{
		m_codeOs << "}" << endl;
	}

	void eq( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );
		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst, precise ) << '=';
		if( length == 1 )
		{
			m_codeOs << "float";
		}
		else
		{
			m_codeOs << "vec" << length;
		}
		m_codeOs << '(';
		Saturate( m_codeOs, command, dst, [&]() {
			if( length == 1 )
			{
				m_codeOs << Src( src0, dst.swizzle ) << "==" << Src( src1, dst.swizzle );
			}
			else
			{
				m_codeOs << "equal(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
			}
		});
		m_codeOs << ");" << endl;
	}

	void exp( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "exp(" << Src( src0, dst.swizzle ) << ')';
		});
		m_codeOs << ';' << endl;
	}

	void firstbit( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		if( command.modifier == MakeInlineString( "hi" ) )
		{
			m_codeOs << Dest( dst ) << "=intBitsToFloat(firstbit_hi(" << Src( src0, dst.swizzle, Int ) << "));" << endl;
		}
		else if( command.modifier == MakeInlineString( "lo" ) )
		{
			m_codeOs << Dest( dst ) << "=intBitsToFloat(firstbit_lo(" << Src( src0, dst.swizzle, Int ) << "));" << endl;
		}
		else
		{
			m_codeOs << Dest( dst ) << "=intBitsToFloat(firstbit_shi(" << Src( src0, dst.swizzle, Int ) << "));" << endl;
		}
	}

	void frc( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "fract(" << Src( src0, dst.swizzle ) << ')';
		});
		m_codeOs << ';' << endl;
	}

	void ftoi( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << "=intBitsToFloat(" << GlslType( Int, dst ) << "(clamp(";
		m_codeOs << Src( src0, dst.swizzle );
		m_codeOs << "," << GlslType( Float, dst ) << "(-2147483648.999)," << GlslType( Float, dst ) << "(2147483647.999))";
		m_codeOs << "));" << endl;
	}

	void ftou( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << "=(" << GlslType( Uint, dst ) << "(clamp(";
		m_codeOs << Src( src0, dst.swizzle );
		m_codeOs << "," << GlslType( Float, dst ) << "(0.0)," << GlslType( Float, dst ) << "(4294967295.999))";
		m_codeOs << "));" << endl;
	}

	void gather( const DX11AsmToken& command )
	{
		bool po = command.modifier == MakeInlineString( "po" ) || command.modifier == MakeInlineString( "po_c" );
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		DX11AsmToken offset;
		if( po )
		{
			offset = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
		}
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		int component = 0;
		if( src2.swizzle.start )
		{
			switch( *src2.swizzle.start )
			{
			case 'y':
				component = 1;
				break;
			case 'z':
				component = 2;
				break;
			case 'w':
				component = 3;
				break;
			}
		}
		m_codeOs << Dest( dst ) << "=textureGather(" << src1.name << ',' << Src( src0, "xyzw" );
		if( po )
		{
			m_codeOs << ',' << Src( offset, "xyzw", Int );
		}
		m_codeOs << ',' << component << ')';
		if( src1.swizzle.start )
		{
			m_codeOs << "." << src1.swizzle;
		}
		m_codeOs << endl;
	}

	void ge( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );
		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst, precise ) << '=' << GlslType( Float, dst );
		m_codeOs << '(';
		Saturate( m_codeOs, command, dst, [&]() {
			if( length == 1 )
			{
				m_codeOs << Src( src0, dst.swizzle ) << ">=" << Src( src1, dst.swizzle );
			}
			else
			{
				m_codeOs << "greaterThanEqual(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
			}
		});
		m_codeOs << ");" << endl;
	}

	void iadd( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << '+' << Src( src1, dst.swizzle, Int );
		m_codeOs << ");" << endl;
	}

	void ieq( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=intBitsToFloat(" << GlslType( Int, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Int ) << "==" << Src( src1, dst.swizzle, Int );
		}
		else
		{
			m_codeOs << "equal(" << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void if_( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		m_codeOs << "if(" << Src( src0, "x" );
		if( command.modifier == MakeInlineString( "z" ) )
		{
			m_codeOs << "==0.0){" << endl;
		}
		else
		{
			m_codeOs << "!=0.0){" << endl;
		}
	}

	void ige( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=intBitsToFloat(" << GlslType( Int, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Int ) << ">=" << Src( src1, dst.swizzle, Int );
		}
		else
		{
			m_codeOs << "greaterThanEqual(" << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void ilt( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=intBitsToFloat(" << GlslType( Int, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Int ) << "<" << Src( src1, dst.swizzle, Int );
		}
		else
		{
			m_codeOs << "lessThan(" << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void imad( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << '*' << Src( src1, dst.swizzle, Int ) << '+' << Src( src2, dst.swizzle, Int );
		m_codeOs << ");" << endl;
	}

	void imax( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(max(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int );
		m_codeOs << "));" << endl;
	}

	void imin( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(min(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int );
		m_codeOs << "));" << endl;
	}

	void imul( const DX11AsmToken& command )
	{
		auto& dst0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& dst1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		if( dst0.name == MakeInlineString( "null" ) )
		{
			m_codeOs << Dest( dst1 ) << "=intBitsToFloat(" << Src( src0, dst1.swizzle, Int ) << "*" << Src( src0, dst1.swizzle, Int ) << ");" << endl;
		}
		else
		{
			m_codeOs << "{" << GlslType( Int, dst0 ) << " _x,_y;imulExtended(" << Src( src0, dst0.swizzle, Int ) << ',' << Src( src1, dst0.swizzle, Int ) << ",_x,_y);";
			m_codeOs << Dest( dst0 ) << "=intBitsToFloat(_x);";
			m_codeOs << Dest( dst1 ) << "=intBitsToFloat(_y);";
			m_codeOs << "}" << endl;
		}
	}

	void ine( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=intBitsToFloat(" << GlslType( Int, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Int ) << "!=" << Src( src1, dst.swizzle, Int );
		}
		else
		{
			m_codeOs << "notEqual(" << Src( src0, dst.swizzle, Int ) << ',' << Src( src1, dst.swizzle, Int ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void ineg( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat((abs(" << Src( src0, dst.swizzle, Int ) << ")^" << GlslType( Int, dst ) << "(0xffffffff))+" << GlslType( Int, dst ) << "(1));" << endl;
	}

	void ishl( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << "<<" << Src( src1, dst.swizzle, Int );
		m_codeOs << ");" << endl;
	}

	void ishr( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Int ) << ">>" << Src( src1, dst.swizzle, Int );
		m_codeOs << ");" << endl;
	}

	void itof( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(" << Src( src0, dst.swizzle, Int ) << ");" << endl;
	}

	void ld( const DX11AsmToken& command )
	{
		if( command.modifier == MakeInlineString( "structured" ) )
		{
			unexpected();
		}
		else if( command.modifier == MakeInlineString( "structured_indexable" ) )
		{
			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Assign );
			auto& stride = ExpectToken( DX11AsmToken::Number );
			ExpectToken( DX11AsmToken::CloseParen );
			ExpectToken( DX11AsmToken::OpenParen );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::CloseParen );


			auto& dst = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& src0 = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& offset = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& src1 = ExpectToken( DX11AsmToken::ID );

			m_codeOs << Dest( dst ) << "=texelFetch(" << src1.name<< ',' << Src( src0, dst.swizzle, Int ) << '*' << stride.name << ",0)";
			if( src1.swizzle.start )
			{
				m_codeOs << "." << src1.swizzle;
			}
			m_codeOs << ';';
		}
		else
		{
			auto& dst = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& src0 = ExpectToken( DX11AsmToken::ID );
			ExpectToken( DX11AsmToken::Coma );
			auto& src1 = ExpectToken( DX11AsmToken::ID );

			m_codeOs << Dest( dst ) << "=texelFetch(" << src1.name<< ',' << Src( src0, dst.swizzle, Int ) << ')';
			if( src1.swizzle.start )
			{
				m_codeOs << "." << src1.swizzle;
			}
			m_codeOs << ';';
		}
		m_codeOs << endl;
	}

	void ldms( const DX11AsmToken& command )
	{
		ExpectToken( DX11AsmToken::OpenParen );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );
		ExpectToken( DX11AsmToken::OpenParen );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );

		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=texelFetch(" << src1.name << ',' << Src( src0, "xy", Int ) << ',' << Src( src2, "x", Int ) << ')';
		if( src1.swizzle.start )
		{
			m_codeOs << '.';
			int offset = 0;
			for( auto it = dst.swizzle.start; it != dst.swizzle.end; ++it )
			{
				m_codeOs << src1.swizzle.start[offset++];
			}
		}
		m_codeOs << ';' << endl;
	}

	void log( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "log(" << Src( src0, dst.swizzle ) << ')';
		});
		m_codeOs << ';' << endl;
	}

	void loop( const DX11AsmToken& command )
	{
		m_codeOs << "while(true){" << endl;
	}

	void lt( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );
		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst, precise ) << '=' << GlslType( Float, dst );
		m_codeOs << '(';
		Saturate( m_codeOs, command, dst, [&]() {
			if( length == 1 )
			{
				m_codeOs << Src( src0, dst.swizzle ) << "<" << Src( src1, dst.swizzle );
			}
			else
			{
				m_codeOs << "lessThan(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
			}
		});
		m_codeOs << ");" << endl;
	}

	void mad( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle ) << '*' << Src( src1, dst.swizzle ) << '+' << Src( src2, dst.swizzle );
		});
		m_codeOs << ';' << endl;
	}

	void max_( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "max(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void min_( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "min(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void mov( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle );
		} );
		m_codeOs << ';' << endl;
	}

	void movc( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "mix(" << Src( src2, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ',';
			auto length = dst.swizzle.end - dst.swizzle.start;
			if( length == 1 )
			{
				m_codeOs << "bool(";
			}
			else
			{
				m_codeOs << "bvec" << length << "(";
			}
			m_codeOs << Src( src0, dst.swizzle ) << "))";
		} );
		m_codeOs << ';' << endl;
	}

	void mul( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle ) << '*' << Src( src1, dst.swizzle );
		} );
		m_codeOs << ';' << endl;
	}

	void ne( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );
		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst, precise ) << '=' << GlslType( Float, dst );
		m_codeOs << '(';
		Saturate( m_codeOs, command, dst, [&]() {
			if( length == 1 )
			{
				m_codeOs << Src( src0, dst.swizzle ) << "!=" << Src( src1, dst.swizzle );
			}
			else
			{
				m_codeOs << "notEqual(" << Src( src0, dst.swizzle ) << ',' << Src( src1, dst.swizzle ) << ')';
			}
		});
		m_codeOs << ");" << endl;
	}

	void nop( const DX11AsmToken& )
	{
	}

	void not( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(~" << Src( src0, dst.swizzle, Int ) << ");" << endl;
	}

	void or( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << "=intBitsToFloat(";
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle, Int ) << '|' << Src( src1, dst.swizzle, Int );
		});
		m_codeOs << ");" << endl;
	}

	void ps( const DX11AsmToken& )
	{
	}

	void rcp( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=" << GlslType( Float, dst ) << "(1.0)/" << Src( src0, dst.swizzle ) << ");" << endl;
	}

	void resinfo( const DX11AsmToken& command )
	{
		ExpectToken( DX11AsmToken::OpenParen );
		auto& resType = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );
		ExpectToken( DX11AsmToken::OpenParen );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );
		ExpectToken( DX11AsmToken::ID );

		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=intBitsToFloat(textureSize(" << src1.name;
		if( resType.name.end[-1] == 's' )
		{
			m_codeOs << ").xy);" << endl;
		}
		else
		{
			m_codeOs << ',' << Src( src0, "x" ) << "));" << endl;
		}
	}

	void ret( const DX11AsmToken& )
	{
	}

	void retc( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		if( command.modifier == MakeInlineString( "z" ) )
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "==0.0)";
		}
		else
		{
			m_codeOs << "if(" << Src( src0, "x" ) << "!=0.0)";
		}
		m_codeOs << "return;" << endl;
	}

	void round( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			if( command.modifier == MakeInlineString( "ne" ) )
			{
				m_codeOs << "round" << "(" << Src( src0, dst.swizzle ) << ')';
			}
			else if( command.modifier == MakeInlineString( "ni" ) )
			{
				m_codeOs << "floor" << "(" << Src( src0, dst.swizzle ) << ')';
			}
			else if( command.modifier == MakeInlineString( "pi" ) )
			{
				m_codeOs << "ceil" << "(" << Src( src0, dst.swizzle ) << ')';
			}
			else
			{
				m_codeOs << GlslType( Float, dst ) << '(' << GlslType( Int, dst ) << "(" << Src( src0, dst.swizzle ) << "))";
			}
		});
		m_codeOs << ';' << endl;
	}

	void rsq( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "inversesqrt(" << Src( src0, dst.swizzle ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void sample( const DX11AsmToken& command )
	{
		ExpectToken( DX11AsmToken::OpenParen );
		auto& type = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );
		ExpectToken( DX11AsmToken::OpenParen );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::CloseParen );

		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=";
		if( command.modifier == MakeInlineString( "indexable" ) )
		{
			m_codeOs << "texture("; 
		}
		else if( command.modifier == MakeInlineString( "l_indexable" ) )
		{
			m_codeOs << "textureLod("; 
		}
		else
		{
			throw CompilerError( "sample: unknown mode" );
		}
		m_codeOs << src1.name << ',';
		if( type.name == MakeInlineString( "texture1d" ) )
		{
			m_codeOs << Src( src0, "x" );
		}
		else if( type.name == MakeInlineString( "texture2d" ) )
		{
			m_codeOs << Src( src0, "xy" );
		}
		else if( type.name == MakeInlineString( "texture3d" ) )
		{
			m_codeOs << Src( src0, "xyz" );
		}
		else if( type.name == MakeInlineString( "texturecube" ) )
		{
			m_codeOs << Src( src0, "xyz" );
		}
		else
		{
			throw CompilerError( "unsupported texture fetch" );
		}
		if( command.modifier == MakeInlineString( "l_indexable" ) )
		{
			ExpectToken( DX11AsmToken::Coma );
			auto& lod = ExpectToken( DX11AsmToken::ID );
			m_codeOs << ',' << Src( lod, "x" );
		}
		m_codeOs << ").";
		for( int i = 0; i < dst.swizzle.end - dst.swizzle.start; ++i )
		{
			switch( dst.swizzle.start[i] )
			{
			case 'x':
				m_codeOs << src1.swizzle.start[0];
				break;
			case 'y':
				m_codeOs << src1.swizzle.start[1];
				break;
			case 'z':
				m_codeOs << src1.swizzle.start[2];
				break;
			case 'w':
				m_codeOs << src1.swizzle.start[3];
				break;
			}
		}
		m_codeOs << ';' << endl;
	}

	void sincos( const DX11AsmToken& command )
	{
		auto& dst0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& dst1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		if( dst0.name != MakeInlineString( "null" ) )
		{
			m_codeOs << Dest( dst0, precise ) << '=';
			Saturate( m_codeOs, command, dst0, [&]() {
				m_codeOs << "sin(" << Src( src0, dst0.swizzle ) << ')';
			} );
			m_codeOs << ';' << endl;
		}
		if( dst1.name != MakeInlineString( "null" ) )
		{
			m_codeOs << Dest( dst1, precise ) << '=';
			Saturate( m_codeOs, command, dst1, [&]() {
				m_codeOs << "cos(" << Src( src0, dst1.swizzle ) << ')';
			} );
			m_codeOs << ';' << endl;
		}
	}

	void sqrt( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << '=';
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << "sqrt(" << Src( src0, dst.swizzle ) << ')';
		} );
		m_codeOs << ';' << endl;
	}

	void switch_( const DX11AsmToken& command )
	{
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << "switch(" << Src( src0, "x", Int ) << "){" << endl;
	}

	void uaddc( const DX11AsmToken& command )
	{
		auto& dst0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& dst1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		m_codeOs << "{" << GlslType( Uint, dst0 ) << " _x,_y;uaddCarry(" << Src( src0, dst0.swizzle, Uint ) << ',' << Src( src1, dst0.swizzle, Uint ) 
			<< ",_x,_y);" << Dest( dst0 ) << "=uintBitsToFloat(_x);" << Dest( dst1 ) << "=uintBitsToFloat(_x);}" << endl;
	}

	void udiv( const DX11AsmToken& command )
	{
		auto& dst0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& dst1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		m_codeOs << Dest( dst0 ) << "=uintBitsToFloat(" << Src( src0, dst0.swizzle, Uint ) << '/' << Src( src1, dst0.swizzle, Uint ) << ");" << endl;
		m_codeOs << Dest( dst1 ) << "=uintBitsToFloat(" << Src( src0, dst1.swizzle, Uint ) << '%' << Src( src1, dst1.swizzle, Uint ) << ");" << endl;
	}

	void uge( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(" << GlslType( Uint, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Uint ) << ">=" << Src( src1, dst.swizzle, Uint );
		}
		else
		{
			m_codeOs << "greaterThanEqual(" << Src( src0, dst.swizzle, Uint ) << ',' << Src( src1, dst.swizzle, Uint ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void ult( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		auto length = dst.swizzle.end - dst.swizzle.start;

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(" << GlslType( Uint, dst );
		m_codeOs << '(';
		if( length == 1 )
		{
			m_codeOs << Src( src0, dst.swizzle, Uint ) << "<" << Src( src1, dst.swizzle, Uint );
		}
		else
		{
			m_codeOs << "lessThan(" << Src( src0, dst.swizzle, Uint ) << ',' << Src( src1, dst.swizzle, Uint ) << ')';
		}
		m_codeOs << ");" << endl;
	}

	void umad( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src2 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Uint ) << '*' << Src( src1, dst.swizzle, Uint ) << '+' << Src( src1, dst.swizzle, Uint );
		m_codeOs << ");" << endl;
	}

	void umax( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(max(";
		m_codeOs << Src( src0, dst.swizzle, Uint ) << ',' << Src( src1, dst.swizzle, Uint );
		m_codeOs << "));" << endl;
	}

	void umin( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(min(";
		m_codeOs << Src( src0, dst.swizzle, Uint ) << ',' << Src( src1, dst.swizzle, Uint );
		m_codeOs << "));" << endl;
	}

	void umul( const DX11AsmToken& command )
	{
		auto& dst0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& dst1 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );
		m_codeOs << "{" << GlslType( Int, dst0 ) << " _x,_y;umulExtended(" << Src( src0, dst0.swizzle, Uint ) << ',' << Src( src1, dst0.swizzle, Uint ) 
			<< ",_x,_y);" << Dest( dst0 ) << "=uintBitsToFloat(_x);" << Dest( dst1 ) << "=uintBitsToFloat(_x);}" << endl;
	}

	void ushr( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=uintBitsToFloat(";
		m_codeOs << Src( src0, dst.swizzle, Uint ) << ">>" << Src( src1, dst.swizzle, Uint );
		m_codeOs << ");" << endl;
	}

	void utof( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );

		m_codeOs << Dest( dst ) << "=" << GlslType( Float, dst ) << '(' << Src( src0, dst.swizzle, Uint ) << ");" << endl;
	}

	void vs( const DX11AsmToken& )
	{
		m_declOs << "out gl_PerVertex {vec4 gl_Position;" << endl << 
			"#ifdef PS" << endl <<
			"float gl_ClipDistance[1];" << endl <<
			"#endif" << endl <<
			"};" << endl;
	}

	void xor( const DX11AsmToken& command )
	{
		auto& dst = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src0 = ExpectToken( DX11AsmToken::ID );
		ExpectToken( DX11AsmToken::Coma );
		auto& src1 = ExpectToken( DX11AsmToken::ID );

		bool precise = command.index == MakeInlineString( "precise" );

		m_codeOs << Dest( dst, precise ) << "=intBitsToFloat(";
		Saturate( m_codeOs, command, dst, [&]() {
			m_codeOs << Src( src0, dst.swizzle, Int ) << '^' << Src( src1, dst.swizzle, Int );
		});
		m_codeOs << ");" << endl;
	}
private:

	enum Type
	{
		Float,
		Int,
		Uint,
		Bool,
		Double,
	};

	class _Dest
	{
	public:
		_Dest( State& state, const DX11AsmToken& reg, bool precise = false )
			:m_state( state ),
			m_reg( reg ),
			m_precise( precise )
		{
		}
		void Print( std::ostream& os ) const
		{
			if( m_precise )
			{
				os << "precise vec4 tmp" << ++m_state.m_tempRegisterCount << ";" << endl;
			}
			auto found = m_state.m_specialRegisters.find( m_reg.name );
			if( found != m_state.m_specialRegisters.end() )
			{
				os << found->second;
			}
			else
			{
				os << m_reg.name; 
			}
			if( m_reg.index )
			{
				os << "[int(" << m_reg.index << ")]";
			}
			if( m_reg.swizzle != MakeInlineString( "xyzw" ) )
			{
				os << '.' << m_reg.swizzle;
			}
			if( m_precise )
			{
				os << "=tmp" << m_state.m_tempRegisterCount;
			}
		}

	private:
		State& m_state;
		const DX11AsmToken& m_reg;
		bool m_precise;
	};

	class _Src
	{
	public:
		_Src( State& state, const DX11AsmToken& reg, const InlineString& destSwizzle, Type type )
			:m_state( state ),
			m_reg( reg ),
			m_destSwizzle( destSwizzle ),
			m_type( type )
		{
		}
		void Print( std::ostream& os ) const
		{
			if( m_reg.literal )
			{
				Literal( os, m_reg, m_destSwizzle );
				return;
			}

			switch( m_type )
			{
			case Int:
				os << "floatBitsToInt(";
				break;
			case Uint:
				os << "floatBitsToUint(";
				break;
			}
			if( m_reg.negate )
			{
				os << '-';
			}
			bool abs = m_reg.modifier == MakeInlineString( "abs" );
			if( abs )
			{
				os << "abs(";
			}
			if( m_type == Double )
			{
				if( m_destSwizzle.end - m_destSwizzle.start == 4 )
				{
					os << "dvec2(unpackDouble2x32(" << m_reg.name << ".xy),unpackDouble2x32(" << m_reg.name << ".zw))";
				}
				else
				{
					os << "unpackDouble2x32(" << m_reg.name << ".xy)";
				}
			}
			else
			{
				auto found = m_state.m_specialRegisters.find( m_reg.name );
				if( found != m_state.m_specialRegisters.end() )
				{
					os << found->second;
				}
				else
				{
					os << m_reg.name; 
					if( m_reg.index )
					{
						os << "[int(" << m_reg.index << ")]";
					}
				}
				if( !m_destSwizzle.start || m_destSwizzle == MakeInlineString( "xyzw" ) )
				{
					if( m_reg.swizzle != MakeInlineString( "xyzw" ) )
					{
						os << '.' << m_reg.swizzle;
					}
				}
				else
				{
					os << '.';
					int offset = 0;
					int srcMax = m_reg.swizzle.end - m_reg.swizzle.start - 1;
					for( auto it = m_destSwizzle.start; it != m_destSwizzle.end; ++it )
					{
						switch( *it )
						{
						case 'x':
							os << m_reg.swizzle.start[0];
							break;
						case 'y':
							os << m_reg.swizzle.start[min( srcMax, 1 )];
							break;
						case 'z':
							os << m_reg.swizzle.start[min( srcMax, 2 )];
							break;
						case 'w':
							os << m_reg.swizzle.start[min( srcMax, 3 )];
							break;
						}
					}
				}
			}
			if( abs )
			{
				os << ')';
			}
			switch( m_type )
			{
			case Int:
			case Uint:
				os << ")";
				break;
			}
		}

	private:
		std::ostream& Literal( std::ostream& os, const DX11AsmToken& reg, const InlineString& destSwizzle ) const
		{
			int length = 4;
			int srcMax = 3;
			InlineString components[] = { reg.name, reg.modifier, reg.swizzle, reg.index };
			for( auto it = 0; it < 4; ++it )
			{
				if( !components[it].start )
				{
					srcMax = it - 1;
					break;
				}
			}
			if( destSwizzle.start )
			{
				length = destSwizzle.end - destSwizzle.start;
			}
			else
			{
				length = srcMax + 1;
			}
			if( length > 1 )
			{
				switch( m_type )
				{
				case Uint:
					os << "uvec" << length << '(';
					break;
				case Int:
					os << "ivec" << length << '(';
					break;
				default:
					os << "vec" << length << "(";
				}
			}

			for( int i = 0; i < length; ++i )
			{
				if( i )
				{
					os << ',';
				}
				if( destSwizzle.start )
				{
					switch( destSwizzle.start[i] )
					{
					case 'x':
						os << components[0];
						break;
					case 'y':
						os << components[min(1, srcMax)];
						break;
					case 'z':
						os << components[min(2, srcMax)];
						break;
					case 'w':
						os << components[min(3, srcMax)];
						break;
					}
				}
				else
				{
					os << components[i];
				}
			}
			if( length > 1 )
			{
				os << ')';
			}
			return os;
		}

		State& m_state;
		const DX11AsmToken& m_reg;
		InlineString m_destSwizzle;
		Type m_type;
	};

	class _GlslType
	{
	public:
		_GlslType( State& state, State::Type type, size_t length )
			:m_state( state ),
			m_type( type ),
			m_length( length )
		{
		}
		void Print( std::ostream& os ) const
		{
			switch( m_type )
			{
			case Float:
				if( m_length == 1 )
				{
					os << "float";
				}
				else
				{
					os << "vec" << m_length;
				}
				break;
			case Int:
				if( m_length == 1 )
				{
					os << "int";
				}
				else
				{
					os << "ivec" << m_length;
				}
				break;
			case Uint:
				if( m_length == 1 )
				{
					os << "uint";
				}
				else
				{
					os << "uvec" << m_length;
				}
				break;
			case Bool:
				if( m_length == 1 )
				{
					os << "bool";
				}
				else
				{
					os << "bvec" << m_length;
				}
				break;
			case Double:
				if( m_length == 1 )
				{
					os << "double";
				}
				else
				{
					os << "dvec" << m_length;
				}
				break;
			}
		}

	private:
		State& m_state;
		State::Type m_type;
		size_t m_length;
	};

	friend std::ostream& operator<<( std::ostream&, const _Dest& );
	friend std::ostream& operator<<( std::ostream&, const _Src& );
	friend std::ostream& operator<<( std::ostream&, const _GlslType& );

	_Dest Dest( const DX11AsmToken& reg, bool precise = false )
	{
		return _Dest( *this, reg, precise );
	}

	_Src Src( const DX11AsmToken& reg, const char* destSwizzle, Type type = Float )
	{
		return _Src( *this, reg, MakeInlineString( destSwizzle ), type );
	}

	_Src Src( const DX11AsmToken& reg, const InlineString& destSwizzle, Type type = Float )
	{
		return _Src( *this, reg, destSwizzle, type );
	}

	_GlslType GlslType( Type type, size_t length )
	{
		return _GlslType( *this, type, length );
	}

	_GlslType GlslType( Type type, const DX11AsmToken& reg )
	{
		return _GlslType( *this, type, reg.swizzle.end - reg.swizzle.start );
	}

	const DX11AsmToken& ExpectToken( DX11AsmToken::Type type )
	{
		if( m_tokenIndex >= m_tokens.size() )
		{
			throw CompilerError( "unexpected end of asm stream" );
		}
		auto& token = m_tokens[m_tokenIndex++];
		if( token.type != type )
		{
			throw CompilerError( "unexpected asm token" );
		}
		return token;
	}

	template <typename T>
	void Saturate( std::ostream& os, const DX11AsmToken& command, const DX11AsmToken& dest, T t )
	{
		bool sat = command.modifier == MakeInlineString( "sat" );
		if( sat )
		{
			os << "clamp(";
		}
		t();
		if( sat )
		{
			auto length = dest.swizzle.end - dest.swizzle.start;
			if( length > 1 )
			{
				os << ",vec" << length << "(0.0),vec" << length << "(1.0))";
			}
			else
			{
				os << ",0.0,1.0)";
			}
		}
	}

	InlineString m_stream;

	typedef void (State::*CommandHandler)( const DX11AsmToken& command );
	std::map<InlineString, CommandHandler> m_commandHandlers;

	// Code stream (code inside main())
	std::ostrstream m_codeOs;
	// Declaration stream (ends up outside main())
	std::ostrstream m_declOs;
	//std::ostrstream m_localsOs;

	std::vector<DX11AsmToken> m_tokens;
	size_t m_tokenIndex;

	std::map<InlineString, InlineString> m_specialRegisters;

	size_t m_tempRegisterCount;
	InputStageType m_stage;

	std::set<InlineString> m_usedInputs;
	std::set<InlineString> m_usedOutputs;

	std::vector<InputDescription> m_inputs;
};

std::ostream& operator<<( std::ostream& os, const State::_Dest& dest )
{
	dest.Print( os );
	return os;
}

std::ostream& operator<<( std::ostream& os, const State::_Src& src )
{
	src.Print( os );
	return os;
}

std::ostream& operator<<( std::ostream& os, const State::_GlslType& src )
{
	src.Print( os );
	return os;
}

}

#define CBR_RETURN_VAL( c, val ) {													\
	if( !( c ) ) {																	\
	g_messages.AddMessage( "\\memory(0): error X0000: internal compiler error" );	\
	assert( false );																\
	return val; } }




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

// --------------------------------------------------------------------------------------
// Description:
//   Creates a hidden window and GL context (used for validating shaders).
// Return Value:
//   true On success
//   false On error
// --------------------------------------------------------------------------------------
bool EffectCompilerGL4::Create()
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
    wc.lpszMenuName =  nullptr; 
    wc.lpszClassName = "glsl4"; 
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
	s_glWnd = CreateWindow( "glsl4", "glsl4", WS_OVERLAPPED, 0, 0, 16, 16, nullptr, nullptr, GetModuleHandle( nullptr ), 0 );
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

	auto ret = glewInit();
	if( ret != GLEW_OK )
	{
		g_messages.AddMessage( "Error initializing OpenGL: %s\n", glewGetErrorString( ret ) );
		return false;
	}

	auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress( "wglCreateContextAttribsARB" );
	if( !wglCreateContextAttribsARB )
	{
		printf( "Error initializing OpenGL: wglCreateContextAttribsARB not found\n" );
		return false;
	}

	wglDeleteContext( rc );

	int attriblist[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB, 1, WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB };
	wglDeleteContext( rc );
	
	rc = wglCreateContextAttribsARB( dc, nullptr, attriblist );
	if( !wglMakeCurrent( dc, rc ) )
	{
		printf( "Error initializing OpenGL: wglMakeCurrent returned FALSE\n" );
		return false;
	}



	return true;
}

extern void PrintPrettyCode( std::ostream& listing, const char* code, const char* indent );
extern void PrintStageInfo( std::ostream& listing, const StageInput& stage, const EffectData& result );

static const std::regex s_summary( "[[:digit:]]+ error\\(s\\), [[:digit:]]+ warning\\(s\\)\\r\\n" );


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

static bool ValidateShader( const char* source, InputStageType stage, const char* define )
{
	GLuint shader = 0;
	const char* prefix = "#version 410 core\n";
	const char* codes[] = { prefix, define, source };
	shader = glCreateShaderProgramv( stage == VERTEX_STAGE ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER, 3, codes );
	if( !shader )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: could not create OpenGL shader" );
		return false;
	}

	GLint compiled;
	glGetProgramiv( shader, GL_LINK_STATUS, &compiled );
	if( !compiled )
	{
		GLint infoLen = 0;
		glGetProgramiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
		if( infoLen > 1 )
		{
			char* buffer = new char[infoLen];
			glGetProgramInfoLog( shader, infoLen, nullptr, buffer );
			g_messages.AddMessage( "%s", buffer );
			delete[] buffer;
		}
		else
		{
			g_messages.AddMessage( "\\memory(0): error X0000: undefined error compiling OpenGL shader" );
		}
		glDeleteProgram( shader );
		return false;
	}
	if( g_printWarnings )
	{
		GLint infoLen = 0;
		glGetProgramiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
		if( infoLen > 1 )
		{
			char* buffer = new char[infoLen];
			glGetProgramInfoLog( shader, infoLen, nullptr, buffer );
			g_messages.AddMessage( "%s", buffer );
			delete[] buffer;
		}
	}
	glDeleteProgram( shader );
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
bool EffectCompilerGL4::CompileEffect( const char* source, 
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
	bool useOpenGLValidation = false;
	if( g_validateOpenGL )
	{
		useOpenGLValidation = TryOpenGLCall();
		if( !useOpenGLValidation )
		{
			g_messages.AddMessage( "\\memory(0): warning X0000: OpenGL calls are failing; skipping OpenGL shader validation" );
		}
	}

	D3DXMACRO newDefines[256] = { 0 };
	for( int i = 0; defines[i].Name; ++i )
	{
		newDefines[i].Name = defines[i].Name;
		if( strcmp( newDefines[i].Name, "PLATFORM" ) == 0 )
		{
			newDefines[i].Definition = "2";
		}
		else
		{
			newDefines[i].Definition = defines[i].Definition;
		}
	}

	// Fist compile effect as DX11
	if( !g_compilerDX11.CompileEffect( source, sourceLength, newDefines, include, result, false ) )
	{
		return false;
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
		listing << "  platform: GL4" << std::endl;
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
			// Disassemble shader and convert DX assembly to GLSL
			CComPtr<ID3DBlob> disassembly;
			if( FAILED( D3DDisassemble( stage->shaderData, stage->shaderSize, 0, nullptr, &disassembly ) ) )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: could not disassemble shader" );
				return false;
			}
			const char* src = (const char*)disassembly->GetBufferPointer();
			std::string glesSource;

			State state;
			try
			{
				glesSource = state.parse( src, stage->type, stage->inputs );
			}
			catch( CompilerError e )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: %s", e.what() );
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
				PrintStageInfo( listing, *stage, result );
			}

			delete[] stage->shaderData;
			stage->shaderSize = glesSource.length() + 1;
			stage->shaderData = new char[stage->shaderSize];
			strcpy_s( (char*)stage->shaderData, stage->shaderSize, glesSource.c_str() );

			stage->shadowShaderSize = 0;
			stage->shadowShaderData = nullptr;

			if( useOpenGLValidation )
			{
				if( !ValidateShader( glesSource.c_str(), stage->type, "" ) || !ValidateShader( glesSource.c_str(), stage->type, "#define PS\n" ) )
				{
					return false;
				}
			}
		}
	}
	return true;
}
