// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "ParserUtils.h"

ASTNode::ASTNode( ASTNodeType type, const FileLocation& location, ScopeSymbolTable* scope, const ScannerToken* token ) :
	m_nodeType( type ),
	m_symbol( nullptr ),
	m_location( location ),
	m_scope( scope )
{
	m_type.FromTokenType( 0 );
	if( token )
	{
		m_token = *token;
	}
	else
	{
		m_token.type = 0;
	}
}

ASTNode::~ASTNode()
{
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		delete *it;
	}
}

ASTNode* ASTNode::Copy() const
{
	ASTNode* result = new ASTNode( m_nodeType, m_location, m_scope, &m_token );
	result->m_symbol = m_symbol;
	result->m_type = m_type;
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		if( *it )
		{
			result->m_children.push_back( ( *it )->Copy() );
		}
		else
		{
			result->m_children.push_back( nullptr );
		}
	}
	return result;
}

void ASTNode::AddChild( ASTNode* child )
{
	m_children.push_back( child );
}

void ASTNode::InsertChild( size_t place, ASTNode* child )
{
	m_children.insert( m_children.begin() + place, child );
}

void ASTNode::ReplaceChild( size_t place, ASTNode* child )
{
	m_children[place] = child;
}

void ASTNode::ReplaceChild( ASTNode* old, ASTNode* child )
{
	auto found = find( begin( m_children ), end( m_children ), old );
	if( found != end( m_children ) )
	{
		*found = child;
	}
}

void ASTNode::RemoveChild( size_t place )
{
	m_children.erase( m_children.begin() + place );
}

void ASTNode::RemoveChild( ASTNode* child )
{
	auto found = find( begin( m_children ), end( m_children ), child );
	if( found != end( m_children ) )
	{
		m_children.erase( found );
	}
}

ASTNodeType ASTNode::GetNodeType() const
{
	return m_nodeType;
}

void ASTNode::SetNodeType( ASTNodeType type )
{
	m_nodeType = type;
}

size_t ASTNode::GetChildrenCount() const
{
	return m_children.size();
}

ASTNode* ASTNode::GetChild( size_t index ) const
{
	return m_children[index];
}

ASTNode* ASTNode::GetChildOrNull( size_t index ) const
{
	return index < m_children.size() ? m_children[index] : nullptr;
}

std::vector<ASTNode*>& ASTNode::GetChildren()
{
	return m_children;
}

void ASTNode::MoveChildren( ASTNode* to )
{
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		to->m_children.push_back( *it );
	}
	m_children.clear();
}

void ASTNode::SetToken( const ScannerToken* token )
{
	if( token )
	{
		m_token = *token;
	}
	else
	{
		m_token.type = 0;
	}
}

const ScannerToken* ASTNode::GetToken() const
{
	if( m_token.type )
	{
		return &m_token;
	}
	else
	{
		return nullptr;
	}
}

void ASTNode::SetToken( const ScannerToken& token )
{
	m_token = token;
}

void ASTNode::ClearToken()
{
	m_token.type = 0;
}

const FileLocation& ASTNode::GetLocation() const
{
	return m_location;
}

void ASTNode::SetLocation( const FileLocation& location )
{
	m_location = location;
}

void ASTNode::SetSymbol( Symbol* symbol )
{
	m_symbol = symbol;
}

Symbol* ASTNode::GetSymbol() const
{
	return m_symbol;
}

ScopeSymbolTable* ASTNode::GetScope() const
{
	return m_scope;
}

void ASTNode::SetType( const Type& type )
{
	m_type = type;
}

const Type& ASTNode::GetType() const
{
	return m_type;
}

ASTNode* ASTNode::FindNode( ASTNodeType type )
{
#if _WIN32
	if( this == nullptr )
	{
		return nullptr;
	}
#endif
	if( m_nodeType == type )
	{
		return this;
	}
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		ASTNode* node = ( *it )->FindNode( type );
		if( node )
		{
			return node;
		}
	}
	return nullptr;
}

void ASTNode::FindNodes( ASTNodeType type, std::vector<ASTNode*>& nodes )
{
#if _WIN32
	if( this == nullptr )
	{
		return;
	}
#endif
	if( m_nodeType == type )
	{
		nodes.push_back( this );
		return;
	}
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		if( *it )
		{
			( *it )->FindNodes( type, nodes );
		}
	}
}
