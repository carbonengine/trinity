#pragma once

#include <vector>
#include "SymbolTable.h"

struct ScannerToken;
struct Symbol;
struct Type;
class ScopeSymbolTable;

enum ASTNodeType
{
	// used variable: symbol=variable symbol
	NT_VAR_IDENTIFIER,
	// literal constant: token=constant token
	NT_CONSTANT,
	// inline constructor (like {1, 2, 3,}): child[i] = either expression or NT_INLINE_CONSTRUCTOR
	NT_INLINE_CONSTRUCTOR,
	// infix expression:
	// token = operator token
	NT_EXPRESSION,
	// prefix expressions ++, --, -, !, ~: token=operator token
	NT_PREFIX_EXPRESSION,
	// postfix expressions: token=operator token
	NT_POSTFIX_EXPRESSION,
	// C-style cast expression: type=type to cast to
	NT_CAST_EXPRESSION,
	// conditional expression (?:)
	NT_CONDITIONAL_EXPRESSION,
	// function call: symbol=function symbol, token=constructor token, children=arguments
	NT_FUNCTION_CALL,
	// function header declaration: symbol=function name, type=return type, child[i] = NT_FUNCTION_PARAMETER
	NT_FUNCTION_HEADER,
	// function parameter declaration: symbol=parameter name, token=parameter qualifier (in, out, inout), type=parameter type
	// child[0]=array size, child[1]=default value, child[2]=NT_PRIMITIVE_TYPE (optional)
	NT_FUNCTION_PARAMETER,
	// variable name declaration: token=input modifier, symbol=parameter name, child[0]=type, child[1]=array size, child[2]=intial value
	NT_NAME_DECLARATION,
	// var declaration, child[0]=type, child[i]=NT_NAME_DECLARATION
	NT_VAR_DECLARATION_LIST,
	// struct declaration: child[i]=NT_STRUCT_MEMBER
	NT_STRUCT,
	// struct member declaration: child[0]=NT_TYPE_DECLARATION, child[i]=names
	NT_STRUCT_MEMBER,

	// program
	NT_PROGRAM,
	// block scope ({...})
	NT_BLOCK,
	// expression statement: child[0]=NULL or NT_EXPRESSION
	NT_EXPRESSION_STATEMENT,
	// if statement: child[0]=condition, child[1]=branch, child[2]=else branch
	NT_IF,
	// while statement: child[0]=condition, child[1]=loop body
	NT_WHILE,
	// do statement: child[0]=condition, child[1]=loop body
	NT_DO,
	// for statement: child[0]=initializer or NULL, child[1]=condition or NULL, child[2]=increment or NULL, child[3]=body
	NT_FOR,
	// switch statement: children - NT_CASE, last child - condition
	NT_SWITCH,
	// case label with following statements: child[0]=case expression or NULL if default label, child[1] - NT_BLOCK
	NT_CASE,
	// jump statement (break, return, etc.): child[0]=return value
	NT_JUMP,
	// function definition: child[0]=function header, child[1]=function body, chil[2]=NT_FUNCTION_ATTRIBUTE_LIST (optional)
	NT_FUNCTION_DEFINITION,

	// list of sampler states: child[i]=NT_STATE_ASSIGNMENT
	NT_SAMPLER_STATE_LIST,
	// state assignment: token=state name, child[0]=state value(NT_STATE_VALUE, NT_CONSTANT or NT_STATE_PARAMETER)
	NT_STATE_ASSIGNMENT,
	// state assignment constant value: token=state value
	NT_STATE_VALUE,
	// technique declaration: token=technique name, child[i]=NT_PASS_DECLARATION
	NT_TECHNIQUE,
	// pass declaration: token=pass name, child[i]=NT_STATE_ASSIGNMENT or NT_SHADER_ASSIGNMENT
	NT_PASS,
	// shader assignment: token=shader state, child[0]=NT_SHADER_PROFILE, child[1]=NT_FUNCTION_CALL
	NT_SHADER_ASSIGNMENT,
	// shader profile in shader assignment: token=profile name
	NT_SHADER_PROFILE,
	// tbuffer/cbuffer: token=buffer type, symbol=buffer name/register, child[i]=NT_VAR_DECLARATION_LIST
	NT_CBUFFER,

	// Shader/function attribute list (like "[domain(X)]"): child[i]=NT_FUNCTION_ATTRIBUTE
	NT_FUNCTION_ATTRIBUTE_LIST,
	// Single shader/function attribute: token=attribute name, child[i]=NT_FUNCTION_ATTRIBUTE_VALUE
	NT_FUNCTION_ATTRIBUTE,
	// Shader/function attribute value: token=attribute value
	NT_FUNCTION_ATTRIBUTE_VALUE,

	// Geometry shader parameter primitive type ("line", "triangle", etc.): token=primitive type
	NT_PRIMITIVE_TYPE,
};


class ASTNode
{
public:
	ASTNode( ASTNodeType type, const FileLocation& location, ScopeSymbolTable* scope, const ScannerToken* token );

	~ASTNode();

	ASTNode* Copy() const;

	void AddChild( ASTNode* child );
	void InsertChild( unsigned place, ASTNode* child );
	void RemoveChild( unsigned place );

	ASTNodeType GetNodeType() const;
	void SetNodeType( ASTNodeType type );

	size_t GetChildrenCount() const;
	ASTNode* GetChild( size_t index ) const;
	ASTNode* GetChildOrNull( size_t index ) const;

	void MoveChildren( ASTNode* to );

	void SetToken( const ScannerToken* token );
	const ScannerToken* GetToken() const;

	const FileLocation& GetLocation() const;
	void SetLocation( const FileLocation& location );

	void SetSymbol( Symbol* symbol );
	Symbol* GetSymbol() const;
	ScopeSymbolTable* GetScope() const;

	void SetType( Type type );
	Type GetType() const;

	//void PrintHLSL11( std::ostream& os, int level = 0 ) const;


	ASTNode* FindNode( ASTNodeType type );
private:
	ASTNodeType  m_nodeType;
	ScannerToken m_token;
	Symbol* m_symbol;
	Type m_type;
	FileLocation m_location;
	std::vector<ASTNode*> m_children;
	ScopeSymbolTable* m_scope;
};
