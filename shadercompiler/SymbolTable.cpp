// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "SymbolTable.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "ParserUtils.h"
#include "ParserState.h"

bool PackOffset::Create( ParserState& state, const ScannerToken& subComponentToken, const ScannerToken* componentToken )
{
	subComponent = -1;
	component.start = component.end = nullptr;

	char registerType;
	if( !ParseRegisterID( subComponentToken.stringValue.start,
						  subComponentToken.stringValue.end,
						  registerType,
						  subComponent ) ||
		registerType != 'c' )
	{
		state.ShowMessage( subComponentToken.fileLocation, EC_INVALID_PACK_OFFSET );
		return false;
	}
	else if( componentToken )
	{
		component = componentToken->stringValue;
	}
	return true;
}

RegisterSpecifier RegisterSpecifier::Register( char type, int index )
{
	RegisterSpecifier reg;
	reg.shaderProfile.start = nullptr;
	reg.shaderProfile.end = nullptr;
	reg.registerType = type;
	reg.registerNumber = index;
	reg.subComponent = -1;
	reg.space = -1;
	reg.explicitRegister = true;
	reg.explicitSpace = false;
	return reg;
}


bool CompareSymbols::operator()( const Symbol* symbol0, const Symbol* symbol1 ) const
{
	size_t len0 = symbol0->name.end - symbol0->name.start;
	size_t len1 = symbol1->name.end - symbol1->name.start;
	int cmp = strncmp( symbol0->name.start, symbol1->name.start, std::min( len0, len1 ) );
	if( cmp < 0 )
	{
		return true;
	}
	if( cmp == 0 )
	{
		if( len0 < len1 )
		{
			return true;
		}
		if( symbol0->isFunction && symbol1->isFunction )
		{
			return symbol0->definition < symbol1->definition;
		}
		return false;
	}
	return false;
}

Symbol::Symbol() :
	isTypeName( false ),
	isFunction( false ),
	interpolationModifier( 0 ),
	annotations( nullptr ),
	definition( nullptr ),
	intrinsicType( nullptr ),
	addressSpace( AddressSpace::None ),
	resourceRefWrapped( false ),
	used( false )
{
	name.start = name.end = nullptr;
	semantic.start = semantic.end = nullptr;

	packOffset.subComponent = -1;
	packOffset.component.start = nullptr;
	packOffset.component.end = nullptr;

	type.FromTokenType( 0 );
}

ScopeSymbolTable::ScopeSymbolTable( ScopeSymbolTable* parent ) :
	m_parent( parent )
{
}

ScopeSymbolTable::~ScopeSymbolTable()
{
	for( auto it = m_symbols.begin(); it != m_symbols.end(); ++it )
	{
		delete *it;
	}
	for( auto it = m_subBlocks.begin(); it != m_subBlocks.end(); ++it )
	{
		delete *it;
	}
}

static bool compareSymbols( const Symbol* symbol0, const Symbol* symbol1 )
{
	size_t len0 = symbol0->name.end - symbol0->name.start;
	size_t len1 = symbol1->name.end - symbol1->name.start;
	if( len0 != len1 )
	{
		return false;
	}
	int cmp = strncmp( symbol0->name.start, symbol1->name.start, len0 );
	return cmp == 0;
}

Symbol* ScopeSymbolTable::Lookup( const InlineString& name )
{
	auto equals = [&]( Symbol* symbol ) { return symbol->name == name; };
	auto jt = std::find_if( m_symbols.begin(), m_symbols.end(), equals );
	if( jt != m_symbols.end() )
	{
		return *jt;
	}
	if( m_parent == nullptr )
	{
		return nullptr;
	}
	return m_parent->Lookup( name );
}

static bool MatchParameter( ASTNode* parameter, ASTNode* argument, int& casts )
{
	Type argumentType = argument->GetType();
	if( parameter->GetToken() && parameter->GetToken()->type == OP_OUT )
	{
		return parameter->GetType().CanImplicitCast( argumentType, casts );
	}
	else if( parameter->GetToken() && parameter->GetToken()->type == OP_INOUT )
	{
		int tmp;
		return parameter->GetType().CanImplicitCast( argumentType, casts ) &&
			parameter->GetType().CanImplicitCast( parameter->GetType(), tmp );
	}
	else
	{
		return argumentType.CanImplicitCast( parameter->GetType(), casts );
	}
}

Symbol* ScopeSymbolTable::LookupFunctionDeclaration( const InlineString& name, ASTNode* header ) const
{
	std::vector<std::pair<Symbol*, int>> overrides;
	const ScopeSymbolTable* scope = this;
	while( scope )
	{
		for( auto it = scope->m_symbols.begin(); it != scope->m_symbols.end(); ++it )
		{
			if( ( *it )->name == name && ( *it )->definition && ( *it )->definition->GetNodeType() == NT_FUNCTION_HEADER )
			{
				overrides.push_back( std::make_pair( *it, 100000 ) );
			}
		}
		scope = scope->m_parent;
	}
	if( overrides.empty() )
	{
		return nullptr;
	}

	for( auto it = overrides.begin(); it != overrides.end(); ++it )
	{
		ASTNode* node = it->first->definition;
		auto parameterCount = node->GetChildrenCount();
		auto argumentCount = header->GetChildrenCount();

		if( argumentCount != parameterCount )
		{
			continue;
		}
		if( node->GetType() != header->GetType() )
		{
			continue;
		}
		bool match = true;
		for( size_t i = 0; i < argumentCount; ++i )
		{
			if( node->GetChild( i )->GetType() != header->GetChild( i )->GetType() )
			{
				match = false;
				break;
			}
		}
		if( match )
		{
			return it->first;
		}
	}
	return nullptr;
}

Symbol* ScopeSymbolTable::LookupFunction( const InlineString& name, ASTNode* callNode, std::string& diagnosticMessage ) const
{
	diagnosticMessage = "";

	std::vector<std::pair<Symbol*, int>> overrides;
	const ScopeSymbolTable* scope = this;
	while( scope )
	{
		for( auto it = scope->m_symbols.begin(); it != scope->m_symbols.end(); ++it )
		{
			if( ( *it )->name == name )
			{
				overrides.push_back( std::make_pair( *it, 100000 ) );
			}
		}
		scope = scope->m_parent;
	}
	if( overrides.empty() )
	{
		return nullptr;
	}
	if( overrides.size() == 1 )
	{
		return overrides[0].first;
	}

	// overrides[i] is NT_FUNCTION_HEADER or NT_FUNCTION_DEFINITION
	// callNode is NT_FUNCTION_CALL
	Symbol* bestOverride = nullptr;
	int bestScore = 0;
	for( auto it = overrides.begin(); it != overrides.end(); ++it )
	{
		ASTNode* node = it->first->definition;
		if( node == nullptr )
		{
			continue;
		}
		if( node->GetNodeType() == NT_FUNCTION_DEFINITION )
		{
			node = node->GetChildOrNull( 0 );
		}
		if( node == nullptr )
		{
			continue;
		}
		auto parameterCount = node->GetChildrenCount();
		auto argumentCount = callNode->GetChildrenCount();

		if( argumentCount > parameterCount )
		{
			continue;
		}
		if( argumentCount < parameterCount )
		{
			// Check if next parameter after callNode->GetChildrenCount()
			// has default value.
			ASTNode* nextParameter = node->GetChild( argumentCount );
			if( nextParameter == nullptr )
			{
				continue;
			}
			if( nextParameter->GetChildOrNull( 1 ) == nullptr )
			{
				continue;
			}
		}
		it->second = 0;
		bool match = true;
		for( size_t i = 0; i < argumentCount; ++i )
		{
			if( !MatchParameter( node->GetChild( i ), callNode->GetChild( i ), it->second ) )
			{
				it->second = 100000;
				match = false;
				break;
			}
		}
		if( match && ( bestOverride == nullptr || it->second < bestScore ) )
		{
			bestOverride = it->first;
			bestScore = it->second;
		}
	}
	if( bestOverride )
	{
		for( auto it = overrides.begin(); it != overrides.end(); ++it )
		{
			if( it->first != bestOverride && it->second == bestScore )
			{
				return nullptr;
			}
		}
	}
	else if( !bestOverride )
	{
		std::stringstream s;
		s << "\n  called with " << name << "(";
		for( size_t i = 0; i < callNode->GetChildrenCount(); ++i )
		{
			if( i )
			{
				s << ", ";
			}
			s << callNode->GetChild( i )->GetType().ToString();
		}
		s << ")\n  possible overrides:\n";
		for( auto it = overrides.begin(); it != overrides.end(); ++it )
		{
			ASTNode* node = it->first->definition;
			if( node->GetNodeType() == NT_FUNCTION_DEFINITION )
			{
				node = node->GetChildOrNull( 0 );
			}
			s << "    " << node->GetLocation().fileName << "(" << node->GetLocation().lineNumber << "): " << node->GetType().ToString() << " " << name << "(";
			for( size_t i = 0; i < node->GetChildrenCount(); ++i )
			{
				if( i )
				{
					s << ", ";
				}
				s << node->GetChild( i )->GetType().ToString();
			}
			s << ")\n";
		}
		diagnosticMessage = s.str();
	}
	return bestOverride;
}

Symbol* ScopeSymbolTable::AddSymbol( const InlineString& name, OverrideBehavior overrideBehavior )
{
	if( overrideBehavior == DISALOW_OVERRIDES )
	{
		auto equals = [=]( Symbol* symbol ) { return symbol->name == name; };
		auto it = std::find_if( m_symbols.begin(), m_symbols.end(), equals );
		if( it != m_symbols.end() )
		{
			return nullptr;
		}
	}
	Symbol* newSymbol = new Symbol;
	newSymbol->name = name;
	m_symbols.push_back( newSymbol );
	return newSymbol;
}

Symbol* ScopeSymbolTable::AddSymbol( Symbol* symbol, OverrideBehavior overrideBehavior )
{
	if( overrideBehavior == DISALOW_OVERRIDES )
	{
		auto equals = [=]( Symbol* symbol0 ) { return compareSymbols( symbol0, symbol ); };
		auto it = std::find_if( m_symbols.begin(), m_symbols.end(), equals );
		if( it != m_symbols.end() )
		{
			return nullptr;
		}
	}
	Symbol* newSymbol = symbol;
	m_symbols.push_back( newSymbol );
	return newSymbol;
}

ScopeSymbolTable* ScopeSymbolTable::AddScope()
{
	ScopeSymbolTable* subBlock = new ScopeSymbolTable( this );
	m_subBlocks.push_back( subBlock );
	return subBlock;
}

ScopeSymbolTable* ScopeSymbolTable::GetParent()
{
	return m_parent;
}

void ScopeSymbolTable::ResetUsedFlag()
{
	for( auto it = m_symbols.begin(); it != m_symbols.end(); ++it )
	{
		( *it )->used = false;
	}
	for( auto it = m_subBlocks.begin(); it != m_subBlocks.end(); ++it )
	{
		( *it )->ResetUsedFlag();
	}
}


SymbolTable::SymbolTable()
{
	m_root = new ScopeSymbolTable( nullptr );
	m_current = m_root;
	m_bufferSymbols = new ScopeSymbolTable( nullptr );
}

SymbolTable::~SymbolTable()
{
	delete m_root;
	delete m_bufferSymbols;
}

Symbol* SymbolTable::Lookup( const InlineString& name ) const
{
	return m_current->Lookup( name );
}

Symbol* SymbolTable::LookupFunction( const InlineString& name, ASTNode* callNode, std::string& diagnosticMessage ) const
{
	return m_current->LookupFunction( name, callNode, diagnosticMessage );
}

Symbol* SymbolTable::LookupFunctionDeclaration( const InlineString& name, ASTNode* header ) const
{
	return m_current->LookupFunctionDeclaration( name, header );
}

Symbol* SymbolTable::LookupType( const InlineString& name ) const
{
	Symbol* symbol = m_current->Lookup( name );
	if( !symbol || !symbol->isTypeName )
	{
		return nullptr;
	}
	return symbol;
}

Symbol* SymbolTable::LookupBuffer( const InlineString& name ) const
{
	return m_bufferSymbols->Lookup( name );
}

Symbol* SymbolTable::AddSymbol( const InlineString& name, OverrideBehavior overrideBehavior )
{
	// TODO: ugly, fix me
	if( overrideBehavior == ALLOW_OVERRIDES )
	{
		if( m_current->GetParent() )
		{
			return m_current->GetParent()->AddSymbol( name, overrideBehavior );
		}
	}
	return m_current->AddSymbol( name, overrideBehavior );
}

Symbol* SymbolTable::AddTypeSymbol( const InlineString& name, OverrideBehavior overrideBehavior )
{
	auto symbol = AddSymbol( name, overrideBehavior );
	if( symbol )
	{
		symbol->isTypeName = true;
	}
	return symbol;
}

Symbol* SymbolTable::AddSymbol( Symbol* symbol, OverrideBehavior overrideBehavior )
{
	Symbol* result = m_current->AddSymbol( symbol, overrideBehavior );
	if( result == nullptr )
	{
		delete symbol;
	}
	return result;
}

Symbol* SymbolTable::AddBufferSymbol( const InlineString& name )
{
	return m_bufferSymbols->AddSymbol( name, ALLOW_OVERRIDES );
}

void SymbolTable::EnterScope()
{
	m_current = m_current->AddScope();
}

void SymbolTable::LeaveScope()
{
	m_current = m_current->GetParent();
}

ScopeSymbolTable* SymbolTable::GetCurrentScope()
{
	return m_current;
}

void SymbolTable::ResetUsedFlag()
{
	m_root->ResetUsedFlag();
}

Symbol* SymbolTable::LookupGlobal( const char* string ) const
{
	if( m_root->m_subBlocks.empty() )
	{
		return nullptr;
	}
	InlineString toFind = MakeInlineString( string );
	auto equals = [=]( Symbol* symbol ) { return symbol->name == toFind; };
	auto symbol = std::find_if( m_root->m_subBlocks[0]->m_symbols.begin(), m_root->m_subBlocks[0]->m_symbols.end(), equals );
	if( symbol != m_root->m_subBlocks[0]->m_symbols.end() )
	{
		return *symbol;
	}
	return nullptr;
}

bool SymbolTable::IsGlobal( Symbol* symbol ) const
{
	if( m_root->m_subBlocks.empty() )
	{
		return false;
	}
	return std::find( m_root->m_subBlocks[0]->m_symbols.begin(), m_root->m_subBlocks[0]->m_symbols.end(), symbol ) != m_root->m_subBlocks[0]->m_symbols.end();
}
