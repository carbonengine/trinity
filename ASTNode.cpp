#include "StdAfx.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "ParserUtils.h"

ASTNode::ASTNode( ASTNodeType type, const FileLocation& location, ScopeSymbolTable* scope, const ScannerToken* token )
	:m_nodeType( type ),
	m_symbol( nullptr ),
	m_scope( scope ),
	m_location( location )
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
		result->m_children.push_back( ( *it )->Copy() );
	}
	return result;
}

void ASTNode::AddChild( ASTNode* child )
{
	m_children.push_back( child );
}

void ASTNode::InsertChild( unsigned place, ASTNode* child )
{
	m_children.insert( m_children.begin() + place, child );
}

void ASTNode::RemoveChild( unsigned place )
{
	m_children.erase( m_children.begin() + place );
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

void ASTNode::SetType( Type type )
{
	m_type = type;
}

Type ASTNode::GetType() const
{
	return m_type;
}

ASTNode* ASTNode::FindNode( ASTNodeType type )
{
	if( this == nullptr )
	{
		return nullptr;
	}
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
