#include "stdafx.h"
#include "ParserUtils.h"
#include "SymbolTable.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "EffectData.h"


bool ParseRegisterID( const char* start, const char* end, char& registerType, int& registerNumber )
{
	char rType;
	int rNumber;
	bool result = sscanf_s( start, "%c%i", &rType, 1, &rNumber ) == 2;
	if( result )
	{
		registerType = tolower( rType );
		registerNumber = rNumber;
	}
	return result;
}

long ParseNumber( const char* start, const char* end, unsigned base )
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
	size_t index = 0;
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
			case '7':
				{
					long code = ParseNumber( c, string.end, 8 );
					result.append( 1, unsigned char( code ) );
				}
				break;
			case 'x':
			case 'X':
				if( c[2] >= '0' && c[2] <= '9' || c[2] >= 'a' && c[2] <= 'f' || c[2] >= 'A' && c[2] <= 'F' )
				{
					long code = ParseNumber( c, string.end, 16 );
					result.append( 1, unsigned char( code ) );
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
	for( size_t i = 0; i < entryPoint->GetChildrenCount(); ++i )
	{
		MarkUsedSymbols( entryPoint->GetChild( i ), symbols );
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
		MarkUsedSymbols( entryPoint->GetSymbol()->definition, symbols );
		if( entryPoint->GetSymbol()->type.symbol )
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

static void MarkCBuffersUsed( ASTNode* node, SymbolTable& symbols )
{
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_CBUFFER )
	{
		if( HasUsedDeclarations( node ) )
		{
			MarkUsedSymbols( node, symbols );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		MarkCBuffersUsed( node->GetChild( i ), symbols );
	}
}

void MarkUsedSymbols( ASTNode* entryPoint, ParserState& state )
{
	MarkUsedSymbols( entryPoint, state.GetSymbolTable() );
	MarkCBuffersUsed( state.GetTree(), state.GetSymbolTable() );
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
				char *set1 = "xyzw";
				char *set2 = "rgba";
				if( const char *pos = strchr( set1, *c ) )
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
				else if( const char *pos = strchr( set2, *c ) )
				{
					if( swizzleSet == 1 )
					{
						return false;
					}
					if( pos - set2 >= leftType.width )
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


static ASTNode* AddCBuffer( ParserState& state, ASTNode* node, int cbufferIndex, ASTNode*&shadowStruct )
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
	cbuffer->SetSymbol( cbufferName );

	if( cbufferIndex == 10 )
	{
		if( state.GetSymbolTable().LookupGlobal( "DX11ShadowState" ) == nullptr )
		{
			ASTNode* shadowState = new ASTNode( NT_STRUCT, node->GetLocation(), node->GetScope(), nullptr );

			Symbol* symbol = node->GetScope()->AddSymbol( MakeInlineString( "DX11ShadowStateType" ), DISALOW_OVERRIDES ); 
			symbol->isTypeName = true;
			symbol->definition = shadowState;
		
			Type type;
			type.symbol = symbol;

			shadowState->SetSymbol( symbol );
			shadowState->SetType( type );

			ScopeSymbolTable* scope = node->GetScope()->AddScope();

			auto AddMember = [&]( int typeID, int width, int arrayDimension, const char* name )->void
			{
				Type type;
				type.FromTokenType( typeID );
				type.width = width;
				type.arrayDimensions = arrayDimension ? 1 : 0;
				if( arrayDimension )
				{
					type.arraySizes[0] = arrayDimension;
				}

				ASTNode* member = new ASTNode( NT_STRUCT_MEMBER, node->GetLocation(), scope, nullptr );
				ASTNode* nameNode = new ASTNode( NT_NAME_DECLARATION, node->GetLocation(), scope, nullptr );
				if( arrayDimension )
				{
					ScannerToken token;
					token.fileLocation = node->GetLocation();
					token.intValue = 0;
					char buffer[64];
					_itoa_s( arrayDimension, buffer, 10 );
					char* string = state.AllocateString( strlen( buffer ) );
					memcpy( string, buffer, strlen( buffer ) );
					token.stringValue = MakeInlineString( string, string + strlen( buffer ) );
					token.type = OP_INT_CONST;

					ASTNode* index = new ASTNode( NT_CONSTANT, node->GetLocation(), scope, &token );
					nameNode->AddChild( index );
				}
				Symbol* symbol = scope->AddSymbol( MakeInlineString( name ), DISALOW_OVERRIDES );
				symbol->type = type;
				nameNode->SetSymbol( symbol );
				nameNode->SetType( type );
				member->AddChild( nameNode );
				member->SetType( type );
				shadowState->AddChild( member );
			};
			AddMember( OP_INT, 1, 0, "invertedTest" );
			AddMember( OP_INT, 1, 0, "alphaTestRef" );
			AddMember( OP_INT, 1, 0, "alphaTestFunc" );
			AddMember( OP_UINT, 1, 0, "clipPlaneEnabled" );
			AddMember( OP_FLOAT, 4, 4, "clipPlanes" );
			AddMember( OP_FLOAT, 4, 0, "renderTargetSize" );

			shadowStruct = shadowState;

			ASTNode* shadowVar = new ASTNode( NT_NAME_DECLARATION, node->GetLocation(), scope, nullptr );
			type.FromSymbol( symbol );
			shadowVar->SetType( type );

			symbol = node->GetScope()->AddSymbol( MakeInlineString( "DX11ShadowState" ), DISALOW_OVERRIDES ); 
			symbol->type = shadowVar->GetType();
			symbol->definition = shadowState;
			shadowVar->SetSymbol( symbol );
			cbuffer->AddChild( shadowVar );
		}
	}
	return cbuffer;
}

bool HasRegisterBinding( Symbol* symbol, const char* shaderProfile, char registerType, int registerNumber )
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

unsigned GetCBufferIndex( Symbol* symbol )
{
	if( strncmp( symbol->name.start, "PerObjectVS", 11 ) == 0 )
	{					
		if( HasRegisterBinding( symbol, "vs", 'c', 180 ) )
		{
			return 5;
		}
		return 3;
	}
	else if( strncmp( symbol->name.start, "PerFrameVS", 10 ) == 0 )
	{
		return 1;
	}
	else if( strncmp( symbol->name.start, "PerObjectPSInt", 14 ) == 0 )
	{
		return 10;
	}
	else if( strncmp( symbol->name.start, "PerObjectPS", 11 ) == 0 )
	{
		return 4;
	}
	else if( strncmp( symbol->name.start, "PerFramePS", 10 ) == 0 )
	{
		return 2;
	}
	else if( !strncmp( symbol->name.start, "g_uiTransforms", 14 ) )
	{
		return 6;
	}
	return 0;
}

static void PatchCBuffers( ParserState& state, ASTNode* parent, unsigned &index )
{
	ASTNode* node = parent->GetChild( index );
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_NAME_DECLARATION )
	{
		int cbufferIndex = GetCBufferIndex( node->GetSymbol() );
		if( cbufferIndex )
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


	void FindExplicitRegisters( ASTNode* root, InputStageType stage, Registers& registers )
	{
		if( !root )
		{
			return;
		}
		switch( root->GetNodeType() )
		{
		case NT_CBUFFER:
			for( auto& r : root->GetSymbol()->registerSpecifier )
			{
				if( r.second.explicitRegister && DoesProfileMatchStage( r.first, stage ) )
				{
					registers.b.insert( r.second.registerNumber );
					if( !r.second.explicitSpace )
					{
						r.second.space = int( stage );
					}
				}
			}
			break;
		case NT_NAME_DECLARATION:
			for( auto& r : root->GetSymbol()->registerSpecifier )
			{
				if( r.second.explicitRegister && DoesProfileMatchStage( r.first, stage ) )
				{
					auto set = GetRegisterSet( registers, r.second.registerType );
					if( set )
					{
						set->insert( r.second.registerNumber );
						if( !r.second.explicitSpace )
						{
							r.second.space = int( stage );
						}
					}
				}
			}
			break;
		case NT_PROGRAM:
		case NT_VAR_DECLARATION_LIST:
			for( size_t i = 0; i < root->GetChildrenCount(); ++i )
			{
				FindExplicitRegisters( root->GetChild( i ), stage, registers );
			}
			break;
		}
	}

	int AllocateRegister( std::set<int>& set )
	{
		for( int i = 0; ; ++i )
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
			return 't';
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
			return 'u';
		default:
			return 0;
		}
	}

	void AssignRegisters( ASTNode* root, InputStageType stage, Registers& registers )
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
					r.space = int( stage );
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
					r.registerType = registerType;
					r.registerNumber = AllocateRegister( *GetRegisterSet( registers, registerType ) );
					r.subComponent = -1;
					r.space = int( stage );
					r.explicitRegister = false;
					r.explicitSpace = false;
					root->GetSymbol()->registerSpecifier[r.shaderProfile] = r;
				}
			}
			break;
		case NT_PROGRAM:
		case NT_VAR_DECLARATION_LIST:
			for( size_t i = 0; i < root->GetChildrenCount(); ++i )
			{
				AssignRegisters( root->GetChild( i ), stage, registers );
			}
			break;
		}
	}
}

void AssignRegisters( ASTNode* root, int32_t stage )
{
	Registers registers;
	FindExplicitRegisters( root, InputStageType( stage ), registers );
	AssignRegisters( root, InputStageType( stage ), registers );
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
		[]( ASTNode* a, ASTNode* b ) -> bool
	{
		auto pA = GetNodeOrder( a );
		auto pB = GetNodeOrder( b );
		return pA < pB;
	} );
}

void CreateGlobalsCB( ParserState& state, int32_t stage )
{
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
				varStart = min( varStart, i );
				varEnd = max( varEnd, i );
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
		root->InsertChild( unsigned( varStart ), cbuffer );
	}
}