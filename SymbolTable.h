#pragma once

#include <set>
#include <vector>
#include "Type.h"
#include "InlineString.h"
#include "ParserState.h"

class ParserState;
class ASTNode;

typedef Type ( *IntrinsicType )( ASTNode* call, int argIndex );

struct PackOffset
{
	bool Create( ParserState& state, const ScannerToken& subComponent, const ScannerToken* component );

	int subComponent;
	InlineString component;
};

struct RegisterSpecifier
{
	static RegisterSpecifier Register( char type, int index = -1 );

	InlineString shaderProfile;
	char registerType;
	int registerNumber;
	int subComponent;
	int space;
	bool explicitRegister;
	bool explicitSpace;
};

struct SymbolAnnotation
{
	int type;
	InlineString name;
	ScannerToken value;
};

enum class AddressSpace
{
	None = 0,
	Device,
	Constant,
	Thread,
	Threadgroup,
	Threadgroup_imageblock
};

typedef std::vector<SymbolAnnotation> SymbolAnnotations;

struct Symbol
{
	Symbol();

	InlineString name;

	InlineString semantic;

	bool isTypeName;
	bool isFunction;

	int interpolationModifier;

	PackOffset packOffset;
	std::map<InlineString, RegisterSpecifier> registerSpecifier;

	SymbolAnnotations* annotations;

	Type type;
	ASTNode* definition;

	IntrinsicType intrinsicType;

	// Specific to Metal shaders.
	AddressSpace addressSpace;

	bool used;
};

enum OverrideBehavior
{
	DISALOW_OVERRIDES,
	ALLOW_OVERRIDES,
};

struct CompareSymbols
{
	bool operator()( const Symbol*, const Symbol* ) const;
};

class ScopeSymbolTable
{
public:
	ScopeSymbolTable( ScopeSymbolTable* parent );
	~ScopeSymbolTable();

	Symbol* Lookup( const InlineString& name );
	Symbol* LookupFunction( const InlineString& name, ASTNode* callNode, std::string& diagnosticMessage ) const;
	Symbol* LookupFunctionDeclaration( const InlineString& name, ASTNode* header ) const;
	Symbol* AddSymbol( const InlineString& name, OverrideBehavior overrideBehavior );
	Symbol* AddSymbol( Symbol* symbol, OverrideBehavior overrideBehavior );
	ScopeSymbolTable* AddScope();
	ScopeSymbolTable* GetParent();

	void ResetUsedFlag();
private:
	ScopeSymbolTable* m_parent;
	std::vector<Symbol*> m_symbols;
	std::vector<ScopeSymbolTable*> m_subBlocks;
	friend class SymbolTable;
};

class SymbolTable
{
public:
	SymbolTable();
	~SymbolTable();

	Symbol* Lookup( const InlineString& name ) const;
	Symbol* LookupFunction( const InlineString& name, ASTNode* callNode, std::string& diagnosticMessage ) const;
	Symbol* LookupFunctionDeclaration( const InlineString& name, ASTNode* header ) const;
	Symbol* LookupType( const InlineString& name ) const;
	Symbol* LookupBuffer( const InlineString& name ) const;
	bool IsGlobal( Symbol* symbol ) const;
	Symbol* AddSymbol( const InlineString& name, OverrideBehavior overrideBehavior = DISALOW_OVERRIDES );
	Symbol* AddSymbol( Symbol* symbol, OverrideBehavior overrideBehavior = DISALOW_OVERRIDES );
	Symbol* AddBufferSymbol( const InlineString& name );
	void EnterScope();
	void LeaveScope();

	ScopeSymbolTable* GetCurrentScope();

	Symbol* LookupGlobal( const char* string ) const;

	void ResetUsedFlag();
private:
	ScopeSymbolTable* m_root;
	ScopeSymbolTable* m_current;
	ScopeSymbolTable* m_bufferSymbols;
};
