// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "ParserUtils.h"
#include "SymbolTable.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "EffectData.h"


bool ParseRegisterID( const char* start, const char*, char& registerType, int& registerNumber )
{
	char rType;
	int rNumber;
#if _WIN32
	bool result = sscanf_s( start, "%c%i", &rType, 1, &rNumber ) == 2;
#else
	bool result = sscanf( start, "%1c%i", &rType, &rNumber ) == 2;
#endif
	if( result )
	{
		registerType = char( tolower( rType ) );
		registerNumber = rNumber;
	}
	return result;
}

long ParseNumber( const char* start, const char*, unsigned base )
{
	char* stop;
	return strtol( start, &stop, base );
}

long ParseNumber( const char* start, const char* end )
{
	if( end[-1] == 'u' || end[-1] == 'U' || end[-1] == 'l' || end[-1] == 'L' )
	{
		--end;
	}

	bool negate = false;
	if( start[0] == '-' )
	{
		++start;
		while( isspace( *start ) )
		{
			++start;
		}
		negate = true;
	}
	int base = 10;
	if( start[0] == '0' )
	{
		if( start[1] == 'x' || start[1] == 'X' )
		{
			base = 16;
			start += 2;
		}
		else
		{
			base = 8;
			start += 1;
		}
	}
	long result = ParseNumber( start, end, base );
	if( negate )
	{
		result = -result;
	}
	return result;
}

std::string ParseString( const InlineString& string )
{
	if( string.start == nullptr )
	{
		return "";
	}
	std::string result;
	result.reserve( string.end - string.start );
	for( const char* c = string.start + 1; c + 1 < string.end; ++c )
	{
		if( *c == '\\' )
		{
			switch( c[1] )
			{
			case 'a':
				result.append( 1, '\a' );
				break;
			case 'b':
				result.append( 1, '\b' );
				break;
			case 'f':
				result.append( 1, '\f' );
				break;
			case 'n':
				result.append( 1, '\n' );
				break;
			case 'r':
				result.append( 1, '\r' );
				break;
			case 't':
				result.append( 1, '\t' );
				break;
			case 'v':
				result.append( 1, '\v' );
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7': {
				long code = ParseNumber( c, string.end, 8 );
				result.append( 1, static_cast<unsigned char>( code ) );
			}
			break;
			case 'x':
			case 'X':
				if( isxdigit( c[2] ) )
				{
					long code = ParseNumber( c, string.end, 16 );
					result.append( 1, static_cast<unsigned char>( code ) );
				}
			default:
				result.append( 1, c[1] );
			}
			c++;
		}
		else
		{
			result.append( 1, c[0] );
		}
	}
	return result;
}

double ParseFloat( const char* start, const char* end )
{
	if( end[-1] == 'f' || end[-1] == 'F' )
	{
		--end;
	}

	bool negate = false;
	if( start[0] == '-' )
	{
		++start;
		while( isspace( *start ) )
		{
			++start;
		}
		negate = true;
	}

	char* stop;
	double result = strtod( start, &stop );
	if( negate )
	{
		result = -result;
	}
	return result;
}

void MarkUsedSymbols( ASTNode* entryPoint, SymbolTable& symbols )
{
	if( entryPoint == nullptr )
	{
		return;
	}

	const InlineString globalsStructName = MakeInlineString( "GlobalsData" );

	if( entryPoint->GetNodeType() == NT_FUNCTION_ATTRIBUTE &&
		entryPoint->GetChildOrNull( 0 ) &&
		entryPoint->GetToken()->stringValue == MakeInlineString( "patchconstantfunc" ) &&
		entryPoint->GetChildOrNull( 0 )->GetToken()->type == OP_STRING_CONST )
	{
		Symbol* symbol = symbols.LookupGlobal( ParseString( entryPoint->GetChildOrNull( 0 )->GetToken()->stringValue ).c_str() );
		if( symbol && !symbol->used )
		{
			MarkUsedSymbols( symbol->definition, symbols );
		}
	}
	// "Globals" struct is a special case and we don't want to mark all of it's members
	// as "used" automatically.
	if( !( entryPoint->GetNodeType() == NT_STRUCT && entryPoint->GetSymbol()->name == globalsStructName ) && entryPoint->GetNodeType() != NT_SAMPLER_STATE_LIST )
	{
		for( size_t i = 0; i < entryPoint->GetChildrenCount(); ++i )
		{
			MarkUsedSymbols( entryPoint->GetChild( i ), symbols );
		}
	}
	if( entryPoint->GetSymbol() && entryPoint->GetSymbol()->used )
	{
		return;
	}
	if( entryPoint->GetSymbol() )
	{
		entryPoint->GetSymbol()->used = true;
	}
	if( entryPoint->GetSymbol() )
	{
		if( entryPoint->GetSymbol()->definition )
		{
			MarkUsedSymbols( entryPoint->GetSymbol()->definition, symbols );
		}
		if( entryPoint->GetSymbol()->type.symbol && entryPoint->GetSymbol()->type.symbol->definition )
		{
			MarkUsedSymbols( entryPoint->GetSymbol()->type.symbol->definition, symbols );
		}
	}
	if( entryPoint->GetType().symbol )
	{
		MarkUsedSymbols( entryPoint->GetType().symbol->definition, symbols );
	}
}

static bool HasUsedDeclarations( ASTNode* node )
{
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		ASTNode* child = node->GetChild( i );
		if( child )
		{
			if( child->GetNodeType() == NT_VAR_DECLARATION_LIST ||
				child->GetNodeType() == NT_STRUCT_MEMBER )
			{
				if( HasUsedDeclarations( child ) )
				{
					return true;
				}
			}
			else if( child->GetNodeType() == NT_NAME_DECLARATION )
			{
				if( child->GetSymbol() && child->GetSymbol()->used )
				{
					return true;
				}
			}
		}
	}
	return false;
}

static void MarkCBuffersAndStructsUsed( ASTNode* node, SymbolTable& symbols )
{
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_STRUCT )
	{
		if( HasUsedDeclarations( node ) )
		{
			MarkUsedSymbols( node, symbols );
		}
	}
	auto IsGlobals = [node]() {
		if( !node->GetSymbol() )
		{
			return false;
		}
		auto found = node->GetSymbol()->registerSpecifier.find( MakeInlineString( "" ) );
		if( found == end( node->GetSymbol()->registerSpecifier ) )
		{
			return false;
		}
		return found->second.registerNumber == 0 && found->second.explicitRegister;
	};
	if( node->GetNodeType() == NT_CBUFFER && !IsGlobals() )
	{
		if( HasUsedDeclarations( node ) )
		{
			MarkUsedSymbols( node, symbols );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		MarkCBuffersAndStructsUsed( node->GetChild( i ), symbols );
	}
}

void MarkUsedSymbols( ASTNode* entryPoint, ParserState& state )
{
	ZoneScoped;

	MarkUsedSymbols( entryPoint, state.GetSymbolTable() );
	MarkCBuffersAndStructsUsed( state.GetTree(), state.GetSymbolTable() );
}

bool ComputeMemberType( const Type& leftType, const InlineString& member, Type& type, Symbol*& symbol )
{
	symbol = nullptr;
	if( leftType.symbol )
	{
		ASTNode* structDef = leftType.symbol->definition;
		if( structDef )
		{
			for( unsigned i = 0; i < structDef->GetChildrenCount(); ++i )
			{
				if( structDef->GetChild( i ) == nullptr )
				{
					continue;
				}
				for( unsigned j = 0; j < structDef->GetChild( i )->GetChildrenCount(); ++j )
				{
					if( structDef->GetChild( i )->GetChild( j ) == nullptr ||
						structDef->GetChild( i )->GetChild( j )->GetSymbol() == nullptr )
					{
						continue;
					}
					if( member == structDef->GetChild( i )->GetChild( j )->GetSymbol()->name )
					{
						type = structDef->GetChild( i )->GetChild( j )->GetType();
						symbol = structDef->GetChild( i )->GetChild( j )->GetSymbol();
						return true;
					}
				}
			}
		}
		return false;
	}
	else
	{
		if( leftType.builtInType == OP_RAY_DESC )
		{
			if( member == MakeInlineString( "Origin" ) || member == MakeInlineString( "Direction" ) )
			{
				type.FromTokenType( OP_FLOAT );
				type.width = 3;
				return true;
			}
			if( member == MakeInlineString( "TMin" ) || member == MakeInlineString( "TMax" ) )
			{
				type.FromTokenType( OP_FLOAT );
				return true;
			}
			return false;
		}
		if( ( leftType.builtInType == OP_INPUTPATCH || leftType.builtInType == OP_OUTPUTPATCH ) && member == MakeInlineString( "Length" ) )
		{
			type.FromTokenType( OP_UINT );
			return true;
		}
		if( !leftType.IsScalarOrVector() )
		{
			return false;
		}
		type = leftType;
		if( leftType.height != 1 )
		{
			type.width = 0;
			type.height = 1;
			int swizzleSet = 0;
			for( const char* c = member.start; c != member.end; ++c )
			{
				if( *c != '_' )
				{
					return false;
				}
				++c;
				if( c == member.end )
				{
					return false;
				}
				if( *c == 'm' )
				{
					if( swizzleSet == 1 )
					{
						return false;
					}
					else if( swizzleSet == 0 )
					{
						swizzleSet = 2;
					}
					++c;
					if( c == member.end )
					{
						return false;
					}
				}
				else
				{
					if( swizzleSet == 2 )
					{
						return false;
					}
					else if( swizzleSet == 0 )
					{
						swizzleSet = 1;
					}
				}
				if( c == member.end )
				{
					return false;
				}
				if( swizzleSet == 1 )
				{
					if( *c < '1' || *c > '4' )
					{
						return false;
					}
				}
				else
				{
					if( *c < '0' || *c > '3' )
					{
						return false;
					}
				}
				++c;
				if( c == member.end )
				{
					return false;
				}
				if( swizzleSet == 1 )
				{
					if( *c < '1' || *c > '4' )
					{
						return false;
					}
				}
				else
				{
					if( *c < '0' || *c > '3' )
					{
						return false;
					}
				}
				type.width++;
			}
			return true;
		}
		else
		{
			int swizzleSet = 0;
			for( const char* c = member.start; c != member.end; ++c )
			{
				const char* set1 = "xyzw";
				const char* set2 = "rgba";
				if( const char* pos = strchr( set1, *c ) )
				{
					if( swizzleSet == 2 )
					{
						return false;
					}
					if( pos - set1 >= leftType.width )
					{
						return false;
					}
					swizzleSet = 1;
				}
				else if( const char* pos2 = strchr( set2, *c ) )
				{
					if( swizzleSet == 1 )
					{
						return false;
					}
					if( pos2 - set2 >= leftType.width )
					{
						return false;
					}
					swizzleSet = 2;
				}
				else
				{
					return false;
				}
			}
			type.height = 1;
			type.width = int( member.end - member.start );
			return true;
		}
	}
}


static ASTNode* AddCBuffer( ParserState& state, ASTNode* node, int cbufferIndex, ASTNode*& shadowStruct )
{
	shadowStruct = nullptr;

	ScannerToken token;
	token.fileLocation = node->GetLocation();
	token.intValue = 0;
	token.stringValue = MakeInlineString( "cbuffer" );
	token.type = OP_CBUFFER;
	ASTNode* cbuffer = new ASTNode( NT_CBUFFER, node->GetLocation(), node->GetScope(), &token );

	Symbol* cbufferName = state.GetSymbolTable().AddBufferSymbol( node->GetSymbol()->name );
	RegisterSpecifier reg;
	reg.registerNumber = cbufferIndex;
	reg.registerType = 'b';
	reg.shaderProfile.start = nullptr;
	reg.shaderProfile.end = nullptr;
	reg.subComponent = -1;
	reg.space = -1;
	reg.shaderProfile = MakeInlineString( "" );
	reg.explicitRegister = true;
	reg.explicitSpace = false;
	cbufferName->registerSpecifier[reg.shaderProfile] = reg;
	cbufferName->definition = cbuffer;
	cbuffer->SetSymbol( cbufferName );
	return cbuffer;
}

bool HasRegisterBinding( const Symbol* symbol, const char* shaderProfile, char registerType, int registerNumber )
{
	auto found = symbol->registerSpecifier.find( MakeInlineString( shaderProfile ) );
	if( found == symbol->registerSpecifier.end() )
	{
		found = symbol->registerSpecifier.find( MakeInlineString( "" ) );
	}
	if( found != symbol->registerSpecifier.end() )
	{
		return found->second.registerType == registerType && found->second.registerNumber == registerNumber;
	}
	return false;
}

int GetCBufferIndex( const InlineString& name )
{
	if( strncmp( name.start, "PerFrameVS", 10 ) == 0 )
	{
		return 1;
	}
	else if( strncmp( name.start, "PerObjectPS", 11 ) == 0 )
	{
		return 4;
	}
	else if( strncmp( name.start, "PerFramePS", 10 ) == 0 )
	{
		return 2;
	}
	else if( !strncmp( name.start, "g_uiTransforms", 14 ) )
	{
		return 6;
	}
	else if( !strncmp( name.start, "Globals", 7 ) )
	{
		return 0;
	}

	return -1;
}

int GetCBufferIndex( const Symbol* symbol )
{
	if( strncmp( symbol->name.start, "PerObjectVS", 11 ) == 0 )
	{
		if( HasRegisterBinding( symbol, "vs", 'c', 180 ) )
		{
			return 5;
		}
		return 3;
	}

	return GetCBufferIndex( symbol->name );
}

static void PatchCBuffers( ParserState& state, ASTNode* parent, unsigned& index )
{
	ASTNode* node = parent->GetChild( index );
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_NAME_DECLARATION )
	{
		int cbufferIndex = GetCBufferIndex( node->GetSymbol() );
		if( cbufferIndex >= 0 )
		{
			ASTNode* shadowStruct;
			ASTNode* cbuffer = AddCBuffer( state, node, cbufferIndex, shadowStruct );

			cbuffer->AddChild( node );
			parent->RemoveChild( index );
			if( shadowStruct )
			{
				parent->InsertChild( index++, shadowStruct );
			}
			parent->InsertChild( index, cbuffer );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		PatchCBuffers( state, node, i );
	}
}

void PatchCBuffers( ParserState& state )
{
	ZoneScoped;

	for( unsigned i = 0; i < state.GetTree()->GetChildrenCount(); ++i )
	{
		PatchCBuffers( state, state.GetTree(), i );
	}
}

bool IsUniformInputArgument( ASTNode* argument )
{
	if( argument->GetChildOrNull( 0 ) )
	{
		// is it correct?
		return false;
	}
	else
	{
		return argument->GetType().storageClass == OP_UNIFORM;
	}
}

namespace
{
struct Registers
{
	std::set<int> b;
	std::set<int> t;
	std::set<int> s;
	std::set<int> u;
	std::set<int> spaces;
};

std::set<int>* GetRegisterSet( Registers& registers, char specifier )
{
	switch( specifier )
	{
	case 'b':
		return &registers.b;
	case 't':
		return &registers.t;
	case 's':
		return &registers.s;
	case 'u':
		return &registers.u;
	default:
		return nullptr;
	}
}

bool DoesProfileMatchStage( const InlineString& profile, InputStageType stage )
{
	if( profile == MakeInlineString( "" ) )
	{
		return true;
	}
	switch( stage )
	{
	case VERTEX_STAGE:
		return profile == MakeInlineString( "vs" );
	case PIXEL_STAGE:
		return profile == MakeInlineString( "ps" );
	case COMPUTE_STAGE:
		return profile == MakeInlineString( "cs" );
	case GEOMETRY_STAGE:
		return profile == MakeInlineString( "gs" );
	case HULL_STAGE:
		return profile == MakeInlineString( "hs" );
	case DOMAIN_STAGE:
		return profile == MakeInlineString( "ds" );
	default:
		return false;
	}
}

int AllocateRegister( std::set<int>& set )
{
	for( int i = 0;; ++i )
	{
		auto inserted = set.insert( i );
		if( inserted.second )
		{
			return i;
		}
	}
}

char GetRegisterType( const Type& type )
{
	if( type.symbol )
	{
		return 0;
	}
	switch( type.builtInType )
	{
	case OP_SAMPLER2D:
	case OP_SAMPLER3D:
	case OP_SAMPLERCUBE:
	case OP_SAMPLER:
	case OP_SAMPLERCOMPARISON:
		return 's';
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
	case OP_STRUCTUREDBUFFER:
	case OP_RAYTRACING_ACCELERATION_STRUCTURE:
	case OP_BYTEADDRESSBUFFER:
		return 't';
	case OP_APPENDSTRUCTUREDBUFFER:
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
		return 'u';
	default:
		return 0;
	}
}

bool SymbolMatchesGlobalInput( const Symbol& symbol, const GlobalInputElement& element )
{
	return symbol.name == element.name && symbol.type == element.type;
}

void AssignRegisters( ASTNode* root, InputStageType stage, Registers& registers, const std::vector<GlobalInputElement>& globalInput, std::vector<RegisterSpecifier>& globalInputRegisters )
{
	if( !root )
	{
		return;
	}
	switch( root->GetNodeType() )
	{
	case NT_CBUFFER:
		if( root->GetSymbol()->used )
		{
			bool assigned = false;
			for( auto& r : root->GetSymbol()->registerSpecifier )
			{
				if( r.second.explicitRegister && DoesProfileMatchStage( r.first, stage ) )
				{
					assigned = true;
					break;
				}
			}
			if( !assigned )
			{
				RegisterSpecifier r;
				r.shaderProfile = MakeInlineString( "" );
				r.registerType = 'b';
				r.registerNumber = AllocateRegister( registers.b );
				r.subComponent = -1;
				r.space = 0;
				r.explicitRegister = false;
				r.explicitSpace = false;
				root->GetSymbol()->registerSpecifier[r.shaderProfile] = r;
			}
		}
		break;
	case NT_NAME_DECLARATION:
		if( !root->GetSymbol()->used )
		{
			return;
		}
		if( auto registerType = GetRegisterType( root->GetSymbol()->type ) )
		{
			auto globalElement = std::find_if( begin( globalInput ), end( globalInput ), [symbol = root->GetSymbol()]( const auto& element ) { return SymbolMatchesGlobalInput( *symbol, element ); } );
			if( globalElement != end( globalInput ) )
			{
				auto& r = globalInputRegisters[globalElement - begin( globalInput )];
				root->GetSymbol()->registerSpecifier[r.shaderProfile] = r;
			}
			else
			{

				bool assigned = false;
				for( auto& r : root->GetSymbol()->registerSpecifier )
				{
					if( r.second.explicitRegister && DoesProfileMatchStage( r.first, stage ) )
					{
						assigned = true;
						break;
					}
				}
				if( !assigned )
				{
					bool isArray = root->GetChildOrNull( 0 ) && root->GetChildOrNull( 0 )->GetNodeType() == NT_BRACKET_LIST;

					RegisterSpecifier r;
					r.shaderProfile = MakeInlineString( "" );
					r.registerType = registerType;
					r.registerNumber = AllocateRegister( *GetRegisterSet( registers, registerType ) );
					if( isArray )
					{
						r.space = AllocateRegister( registers.spaces );
					}
					else
					{
						r.space = 0;
					}
					r.subComponent = -1;
					r.explicitRegister = false;
					r.explicitSpace = false;
					root->GetSymbol()->registerSpecifier[r.shaderProfile] = r;
				}
			}
		}
		break;
	case NT_PROGRAM:
	case NT_VAR_DECLARATION_LIST:
		for( size_t i = 0; i < root->GetChildrenCount(); ++i )
		{
			AssignRegisters( root->GetChild( i ), stage, registers, globalInput, globalInputRegisters );
		}
		break;
	default:
		break;
	}
}
}

void AssignGlobalInputRegisters( const std::vector<GlobalInputElement>& globalInput, Registers& registers, std::vector<RegisterSpecifier>& globalInputRegisters )
{
	for( auto& input : globalInput )
	{
		if( auto registerType = GetRegisterType( input.type ) )
		{
			bool isArray = input.type.arrayDimensions > 0;

			RegisterSpecifier r;
			r.shaderProfile = MakeInlineString( "" );
			r.registerType = registerType;
			r.registerNumber = AllocateRegister( *GetRegisterSet( registers, registerType ) );
			if( isArray )
			{
				r.space = AllocateRegister( registers.spaces );
			}
			else
			{
				r.space = 0;
			}
			r.subComponent = -1;
			r.explicitRegister = false;
			r.explicitSpace = false;
			globalInputRegisters.push_back( r );
		}
		else if( input.declaration && input.declaration->GetNodeType() == NT_CBUFFER )
		{
			bool assigned = false;
			for( auto& r : input.declaration->GetSymbol()->registerSpecifier )
			{
				if( r.second.explicitRegister && r.first == "" )
				{
					assigned = true;
					globalInputRegisters.push_back( r.second );
					break;
				}
			}
			if( !assigned )
			{
				RegisterSpecifier r;
				r.shaderProfile = MakeInlineString( "" );
				r.registerType = 'b';
				r.registerNumber = AllocateRegister( registers.b );
				r.subComponent = -1;
				r.space = 0;
				r.explicitRegister = false;
				r.explicitSpace = false;
				globalInputRegisters.push_back( r );
			}
		}
		else
		{
			globalInputRegisters.push_back( RegisterSpecifier() );
		}
	}
}

void AssignRegisters( ASTNode* root, int32_t stage, const std::vector<GlobalInputElement>& globalInput )
{
	ZoneScoped;

	Registers registers;
	registers.spaces.insert( 0 );

	std::vector<RegisterSpecifier> globalInputRegisters;
	AssignGlobalInputRegisters( globalInput, registers, globalInputRegisters );

	AssignRegisters( root, InputStageType( stage ), registers, globalInput, globalInputRegisters );
}

int GetNodeOrder( ASTNode* t )
{
	switch( t->GetNodeType() )
	{
	case NT_VAR_DECLARATION_LIST:
		if( auto child = t->GetChildOrNull( 0 ) )
		{
			if( child->GetNodeType() == NT_CBUFFER )
			{
				return 2;
			}
		}
		if( auto child = t->GetChildOrNull( 1 ) )
		{
			if( child->GetNodeType() == NT_CBUFFER )
			{
				return 2;
			}
		}
		if( GetRegisterType( t->GetType() ) )
		{
			return 2;
		}
		else
		{
			return 1;
		}
	case NT_TECHNIQUE:
		return 12;
	case NT_FUNCTION_DEFINITION:
		return 11;
	case NT_STRUCT:
		return 0;
	default:
		return 0;
	}
}

void SortProgramNodes( ASTNode* root )
{
	if( !root )
	{
		return;
	}
	std::stable_sort(
		root->GetChildren().begin(),
		root->GetChildren().end(),
		[]( ASTNode* a, ASTNode* b ) -> bool {
			auto pA = GetNodeOrder( a );
			auto pB = GetNodeOrder( b );
			return pA < pB;
		} );
}

void CreateGlobalsCB( ParserState& state )
{
	ZoneScoped;

	auto root = state.GetTree();
	if( !root )
	{
		return;
	}
	SortProgramNodes( root );
	size_t varStart = root->GetChildrenCount();
	size_t varEnd = 0;
	for( size_t i = 0; i < root->GetChildrenCount(); ++i )
	{
		auto child = root->GetChild( i );
		if( !child )
		{
			continue;
		}
		if( child->GetNodeType() == NT_VAR_DECLARATION_LIST )
		{
			if( auto name = child->GetChildOrNull( 0 ) )
			{
				if( name->GetNodeType() == NT_CBUFFER )
				{
					continue;
				}
			}
			if( auto name = child->GetChildOrNull( 1 ) )
			{
				if( name->GetNodeType() == NT_CBUFFER )
				{
					continue;
				}
			}
			if( !GetRegisterType( child->GetType() ) )
			{
				varStart = std::min( varStart, i );
				varEnd = std::max( varEnd, i );
			}
		}
	}
	if( varStart <= varEnd )
	{
		ScannerToken token;
		token.fileLocation = root->GetLocation();
		token.intValue = 0;
		token.stringValue = MakeInlineString( "cbuffer" );
		token.type = OP_CBUFFER;
		ASTNode* cbuffer = new ASTNode( NT_CBUFFER, root->GetLocation(), root->GetScope(), &token );

		Symbol* cbufferName = state.GetSymbolTable().AddBufferSymbol( MakeInlineString( "Globals" ) );
		RegisterSpecifier reg;
		reg.registerNumber = 0;
		reg.registerType = 'b';
		reg.shaderProfile.start = nullptr;
		reg.shaderProfile.end = nullptr;
		reg.subComponent = -1;
		reg.space = -1;
		reg.shaderProfile = MakeInlineString( "" );
		reg.explicitRegister = true;
		reg.explicitSpace = false;
		cbufferName->registerSpecifier[reg.shaderProfile] = reg;
		cbuffer->SetSymbol( cbufferName );

		for( size_t i = 0; i <= varEnd - varStart; ++i )
		{
			cbuffer->AddChild( root->GetChild( varStart ) );
			root->RemoveChild( unsigned( varStart ) );
		}

		if( cbuffer->GetChildrenCount() > 1 )
		{
			size_t back = cbuffer->GetChildrenCount() - 1;
			for( size_t i = 0; i < back; ++i )
			{
				auto nameDecl = cbuffer->GetChild( i )->GetChild( 0 );
				auto type = nameDecl->GetType();
				if( type.IsScalar() )
				{
					std::swap( cbuffer->GetChildren()[i], cbuffer->GetChildren()[back] );
					--back;
					--i;
				}
			}
		}
		root->InsertChild( unsigned( varStart ), cbuffer );
	}
}

ASTNode* NewStruct( ParserState& state, const InlineString& name )
{
	auto structDecl = state.NewNode( NT_STRUCT );
	auto symbol = state.GetSymbolTable().AddSymbol( name ? name : state.AllocateName() );
	symbol->isTypeName = true;
	symbol->definition = structDecl;
	structDecl->SetSymbol( symbol );
	structDecl->SetType( TypeFromSymbol( symbol ) );
	return structDecl;
}

ASTNode* NewVarIdentifier( ParserState& state, Symbol* var )
{
	auto access = state.NewNode( NT_VAR_IDENTIFIER, ScannerToken::ID( var->name ) );
	access->SetSymbol( var );
	access->SetType( var->type );
	return access;
}

ASTNode* NewLiteralConst( ParserState& state, float value )
{
	ScannerToken token = { OP_FLOAT, 0, state.AllocateName( std::to_string( value ).c_str() ), {} };
	auto result = state.NewNode( NT_CONSTANT, token );
	result->SetType( TypeFromTokenType( OP_FLOAT ) );
	return result;
}

ASTNode* NewLiteralConst( ParserState& state, uint32_t value )
{
	ScannerToken token = { OP_UINT, 0, state.AllocateName( std::to_string( value ).c_str() ), {} };
	auto result = state.NewNode( NT_CONSTANT, token );
	result->SetType( TypeFromTokenType( OP_UINT ) );
	return result;
}

ASTNode* NewLiteralConst( ParserState& state, bool value )
{
	auto result = state.NewNode( NT_CONSTANT, { OP_BOOL_CONST, 1, MakeInlineString( value ? "true" : "false" ), {} } );
	result->SetType( hlsl::bool_t );
	return result;
}

ASTNode* NewDot( ParserState& state, ASTNode* expr, Symbol* field )
{
	auto access = state.NewNode( NT_POSTFIX_EXPRESSION, ScannerToken::ID( field->name ) );
	access->AddChild( expr );
	access->SetType( field->type );
	access->SetSymbol( field );
	return access;
}

ASTNode* NewDot( ParserState& state, ASTNode* expr, const InlineString& field )
{
	auto access = state.NewNode( NT_POSTFIX_EXPRESSION, ScannerToken::ID( field ) );
	access->AddChild( expr );
	Type type;
	Symbol* symbol;
	if( !ComputeMemberType( expr->GetType(), field, type, symbol ) )
	{
		state.ShowMessage( EC_INVALID_SUBSCRIPT, ToString( field ).c_str() );
		type.FromTokenType( OP_VOID );
	}
	access->SetType( type );
	access->SetSymbol( symbol );
	return access;
}

ASTNode* NewBinaryExpression( ParserState& state, int op, ASTNode* left, ASTNode* right )
{
	auto expr = state.NewNode( NT_EXPRESSION, ScannerToken::FromTokenType( op ) );
	expr->AddChild( left );
	expr->AddChild( right );
	Type type;
	if( !GetCommonType( left->GetType(), right->GetType(), type ) )
	{
		state.ShowMessage( EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	expr->SetType( type );
	return expr;
}

ASTNode* NewConditionalExpression( ParserState& state, ASTNode* condition, ASTNode* trueExpr, ASTNode* falseExpr )
{
	auto expr = state.NewNode( NT_CONDITIONAL_EXPRESSION, ScannerToken::FromTokenType( OP_QUESTION ) );
	expr->AddChild( condition );
	expr->AddChild( trueExpr );
	expr->AddChild( falseExpr );
	Type type;
	if( !GetCommonType( trueExpr->GetType(), falseExpr->GetType(), type ) )
	{
		state.ShowMessage( EC_TYPE_MISMATCH );
		type.FromTokenType( OP_VOID );
	}
	expr->SetType( type );
	return expr;
}

ASTNode* NewVarDeclaration( ParserState& state, const Type& type, const InlineString& name )
{
	auto nameDecl = state.NewNode( NT_NAME_DECLARATION );
	auto nameSymbol = state.GetSymbolTable().AddSymbol( name ? name : state.AllocateName() );
	nameDecl->SetSymbol( nameSymbol );

	nameSymbol->type = type;
	nameSymbol->definition = nameDecl;
	nameDecl->SetType( nameSymbol->type );

	auto declList = state.NewNode( NT_VAR_DECLARATION_LIST );
	declList->AddChild( nameDecl );
	declList->SetType( nameSymbol->type );
	return declList;
}

ASTNode* NewVarDeclaration( ParserState& state, Symbol* symbol, ASTNode* initializer )
{
	auto nameDecl = state.NewNode( NT_NAME_DECLARATION );
	nameDecl->SetSymbol( symbol );
	nameDecl->SetType( symbol->type );

	auto declList = state.NewNode( NT_VAR_DECLARATION_LIST );
	declList->AddChild( nameDecl );
	declList->SetType( symbol->type );

	if( initializer )
	{
		nameDecl->AddChild( nullptr );
		nameDecl->AddChild( initializer );
	}
	return declList;
}

ASTNode* NewFunctionCall( ParserState& state, const Type& type, const char* name, const std::vector<ASTNode*>& args )
{
	ScannerToken token = ScannerToken::ID( MakeInlineString( name ) );
	auto ctr = state.NewNode( NT_FUNCTION_CALL );
	ctr->SetToken( &token );
	ctr->SetType( type );
	for( auto arg : args )
	{
		ctr->AddChild( arg );
	}
	return ctr;
}

ASTNode* NewCastExpression( ParserState& state, const Type& type, ASTNode* child )
{
	auto cast = state.NewNode( NT_CAST_EXPRESSION );
	cast->SetType( type );
	cast->AddChild( child );
	return cast;
}

ASTNode* NewExpressionStatement( ParserState& state, ASTNode* expr )
{
	auto statement = state.NewNode( NT_EXPRESSION_STATEMENT );
	statement->AddChild( expr );
	return statement;
}

ASTNode* NewReturn( ParserState& state, ASTNode* expr )
{
	auto returnNode = state.NewNode( NT_JUMP, ScannerToken::FromTokenType( OP_RETURN ) );
	if( expr )
	{
		returnNode->AddChild( expr );
	}
	return returnNode;
}

ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const InlineString& name )
{
	auto param = state.NewNode( NT_FUNCTION_PARAMETER );
	auto symbol = state.GetSymbolTable().AddSymbol( name ? name : state.AllocateName() );
	symbol->type = type;
	symbol->definition = param;
	param->SetType( symbol->type );
	param->SetSymbol( symbol );
	if( type.arrayDimensions )
	{
		auto brackets = state.NewNode( NT_BRACKET_LIST );
		brackets->AddChild( nullptr );
		param->AddChild( brackets );
	}
	else
	{
		param->AddChild( nullptr );
	}
	return param;
}


ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const char* name )
{
	return NewFunctionParameter( state, type, state.AllocateName( name ) );
}

ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const char* name, const RegisterSpecifier& reg )
{
	auto arg = NewFunctionParameter( state, type, state.AllocateName( name ) );
	arg->GetSymbol()->registerSpecifier[MakeInlineString( "" )] = reg;
	return arg;
}

std::optional<std::string> PreprocessString( const InlineString& input );
std::optional<std::vector<InlineString>> TokenizeGlobalInput( const InlineString& input );


std::optional<std::vector<GlobalInputElement>> ParseGlobalInput( ParserState& state, const ScannerToken& input )
{
	auto processed = PreprocessString( input.stringValue );
	if( !processed )
	{
		return {};
	}

	auto tokens = TokenizeGlobalInput( { processed->c_str(), processed->c_str() + processed->size() } );
	if( !tokens )
	{
		state.ShowMessage( input.fileLocation, EC_CUSTOM_ERROR, "Malformed globalinput string" );
		return {};
	}

	std::vector<GlobalInputElement> result;
	for( auto& name : *tokens )
	{
		auto symbol = state.GetSymbolTable().LookupGlobal( ToString( name ).c_str() );
		bool isCBuffer = false;
		bool useCBuffers = false;
		if( useCBuffers )
		{
			if( symbol && symbol->type.IsStruct() )
			{
				symbol = nullptr;
			}
			if( !symbol )
			{
				symbol = state.GetSymbolTable().LookupBuffer( name );
				isCBuffer = true;
			}
		}
		else
		{
			if( !symbol )
			{
				symbol = state.GetSymbolTable().LookupBuffer( name );
				isCBuffer = true;
			}
			if( symbol && symbol->type.IsStruct() )
			{
				isCBuffer = true;
			}
		}
		if( !symbol )
		{
			state.ShowMessage( input.fileLocation, EC_UNDECLARED_IDENTIFIER, ToString( name ).c_str() );
			return {};
		}
		auto& type = symbol->type;
		if( !isCBuffer && !type.IsTexture() && !type.IsSampler() && !type.IsBuffer() && !( type.symbol == nullptr && type.builtInType == OP_RAYTRACING_ACCELERATION_STRUCTURE ) )
		{
			state.ShowMessage( input.fileLocation, EC_CUSTOM_ERROR, "Unsupported type for \"%s\" in the globalinput", ToString( name ).c_str() );
			return {};
		}
		result.push_back( { type, symbol->name, symbol->definition, symbol } );
	}
	return result;
}

bool ProcessGlobalInputAttribute( ParserState& state, ASTNode* func, std::vector<GlobalInputElement>& globalInput )
{
	if( auto attribs = func->GetChildOrNull( 2 ) )
	{
		for( auto& attrib : attribs->GetChildren() )
		{
			if( attrib->GetToken()->stringValue == "globalinput" )
			{
				if( !attrib->GetChildOrNull( 0 ) )
				{
					state.ShowMessage( attrib->GetToken()->fileLocation, EC_CUSTOM_ERROR, "Function annotation globalinput must have a string parameter with global input declarations" );
					return false;
				}
				auto input = ParseGlobalInput( state, *attrib->GetChildOrNull( 0 )->GetToken() );
				if( !input )
				{
					return false;
				}
				if( !globalInput.empty() )
				{
					if( globalInput != *input )
					{
						state.ShowMessage( attrib->GetToken()->fileLocation, EC_CUSTOM_ERROR, "Function annotations globalinput must match across all functions in the library" );
						return false;
					}
				}
				else
				{
					globalInput = *input;
				}
			}
		}
	}
	return true;
}